// Host-side unit tests for the LineLink core (no hardware).
// Build/run: test/run_tests.sh  (clang++/g++, no dependencies).
//
// A deterministic mock radio network connects several Core instances with
// injectable loss, duplication, reordering/jitter, a blackhole, node power-off
// and a settable millis()/micros() origin (for wraparound tests).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "LineLinkCore.h"
#include "LineLinkFlashGate.h"

using namespace linelink;

static int g_checks = 0;
static int g_failures = 0;
static const char* g_test = "";

#define CHECK(cond)                                                                  \
    do {                                                                             \
        ++g_checks;                                                                  \
        if (!(cond)) {                                                               \
            ++g_failures;                                                            \
            printf("  FAIL [%s] %s:%d: %s\n", g_test, __FILE__, __LINE__, #cond);    \
        }                                                                            \
    } while (0)

#define CHECK_EQ(a, b)                                                               \
    do {                                                                             \
        ++g_checks;                                                                  \
        long long va_ = (long long)(a), vb_ = (long long)(b);                        \
        if (va_ != vb_) {                                                            \
            ++g_failures;                                                            \
            printf("  FAIL [%s] %s:%d: %s == %s  (%lld vs %lld)\n", g_test, __FILE__, \
                   __LINE__, #a, #b, va_, vb_);                                      \
        }                                                                            \
    } while (0)

// Mock network

class Net;

class Node : public IRadio, public IClock, public ILock {
public:
    Node(Net& net, uint8_t id, uint16_t seqSeed, int32_t nonce);

    // IRadio
    bool addPeer(const uint8_t mac[6]) {
        peers.insert(macKey(mac));
        ++addPeerCalls;
        return true;
    }
    bool sendBroadcast(const uint8_t* data, size_t len);
    bool sendUnicast(const uint8_t mac[6], const uint8_t* data, size_t len);

    // IClock
    uint32_t millis();
    uint32_t micros();

    // ILock (single-threaded tests)
    void lock() {}
    void unlock() {}

    static uint64_t macKey(const uint8_t mac[6]) {
        uint64_t k = 0;
        for (int i = 0; i < 6; ++i) k = (k << 8) | mac[i];
        return k;
    }

    void reboot(uint16_t seqSeed, int32_t nonce) {
        core.reset(new Core(*this, *this, *this));
        core->begin(id, seqSeed, nonce);
        peers.clear();
    }

    Net& net;
    uint8_t id;
    uint8_t mac[6];
    bool powered;
    std::set<uint64_t> peers;
    int addPeerCalls;
    int unicastToUnregistered;
    int unicastSent;
    std::unique_ptr<Core> core;
    std::vector<Event> received;   // drained events
    bool drainEnabled;
};

struct Packet {
    uint8_t dstNode;
    uint8_t srcMac[6];
    std::vector<uint8_t> data;
};

class Net {
public:
    Net() : tUs(0), startMs(0), startUs(0), loss(0), dup(0), baseDelayUs(500), jitterUs(0),
            blackhole(false), dropEventSeq(-1), rng(0x12345678u), delivered(0), dropped(0) {}

    Node* add(uint8_t id, uint16_t seqSeed = 1, int32_t nonce = 0) {
        nodes.push_back(std::unique_ptr<Node>(new Node(*this, id, seqSeed, nonce ? nonce : (int32_t)(id * 7919))));
        return nodes.back().get();
    }

    Node* byId(uint8_t id) {
        for (auto& n : nodes) if (n->id == id) return n.get();
        return nullptr;
    }
    Node* byMac(const uint8_t mac[6]) {
        for (auto& n : nodes) if (memcmp(n->mac, mac, 6) == 0) return n.get();
        return nullptr;
    }

    uint32_t rnd() {   // xorshift32
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }
    double rnd01() { return (rnd() & 0xFFFFFF) / 16777216.0; }

    void enqueue(Node* from, Node* to, const uint8_t* data, size_t len) {
        if (!to->powered || !from->powered) { ++dropped; return; }
        if (blackhole) { ++dropped; return; }
        if (dropEventSeq >= 0 && len == sizeof(Frame)) {   // selective loss: one specific event never arrives
            Frame f;
            memcpy(&f, data, sizeof(f));
            if (f.type == FRAME_EVENT && f.seq == (uint16_t)dropEventSeq) { ++dropped; return; }
        }
        int copies = 1;
        if (dup > 0 && rnd01() < dup) copies = 2;
        for (int c = 0; c < copies; ++c) {
            if (loss > 0 && rnd01() < loss) { ++dropped; continue; }
            Packet p;
            p.dstNode = to->id;
            memcpy(p.srcMac, from->mac, 6);
            p.data.assign(data, data + len);
            uint64_t due = tUs + baseDelayUs + (jitterUs ? (rnd() % (jitterUs + 1)) : 0);
            if (c == 1) due += 700 + (rnd() % 3000);   // duplicate arrives a little later
            inflight.insert(std::make_pair(due, p));
        }
    }

    void deliverDue() {
        while (!inflight.empty() && inflight.begin()->first <= tUs) {
            Packet p = inflight.begin()->second;
            inflight.erase(inflight.begin());
            Node* n = byId(p.dstNode);
            if (n && n->powered) {
                ++delivered;
                n->core->onReceive(p.srcMac, p.data.data(), (int)p.data.size());
            }
        }
    }

    // Advance simulated time by `ms`: 100 us sub-steps for delivery, service() every ms.
    void run(uint32_t ms) {
        for (uint32_t m = 0; m < ms; ++m) {
            for (int k = 0; k < 10; ++k) {
                tUs += 100;
                deliverDue();
            }
            for (auto& n : nodes) {
                if (!n->powered) continue;
                n->core->service();
                if (n->drainEnabled) {
                    Event e;
                    while (n->core->popEvent(e)) n->received.push_back(e);
                }
            }
        }
    }

    // Run until pred() or timeout (ms); returns true if pred became true.
    template <class F>
    bool runUntil(F pred, uint32_t timeoutMs) {
        for (uint32_t m = 0; m < timeoutMs; ++m) {
            if (pred()) return true;
            run(1);
        }
        return pred();
    }

    uint64_t tUs;
    uint32_t startMs, startUs;
    double loss, dup;
    uint32_t baseDelayUs, jitterUs;
    bool blackhole;
    int dropEventSeq;
    uint32_t rng;
    unsigned delivered, dropped;
    std::vector<std::unique_ptr<Node>> nodes;
    std::multimap<uint64_t, Packet> inflight;
};

Node::Node(Net& n, uint8_t nid, uint16_t seqSeed, int32_t nonce)
    : net(n), id(nid), powered(true), addPeerCalls(0), unicastToUnregistered(0),
      unicastSent(0), drainEnabled(true) {
    mac[0] = 0xE4; mac[1] = 0xB0; mac[2] = 0x63; mac[3] = 0x00; mac[4] = 0x00; mac[5] = nid;
    core.reset(new Core(*this, *this, *this));
    core->begin(id, seqSeed, nonce);
}

bool Node::sendBroadcast(const uint8_t* data, size_t len) {
    for (auto& other : net.nodes) {
        if (other.get() == this) continue;
        net.enqueue(this, other.get(), data, len);
    }
    return true;
}

bool Node::sendUnicast(const uint8_t m[6], const uint8_t* data, size_t len) {
    // Like ESP-NOW: unicast to a MAC that was never registered as a peer fails.
    if (peers.find(macKey(m)) == peers.end()) {
        ++unicastToUnregistered;
        return false;
    }
    ++unicastSent;
    Node* to = net.byMac(m);
    if (to) net.enqueue(this, to, data, len);
    return true;
}

uint32_t Node::millis() { return (uint32_t)(net.startMs + net.tUs / 1000); }
uint32_t Node::micros() { return (uint32_t)(net.startUs + net.tUs); }

static void begin_test(const char* name) {
    g_test = name;
    printf("[ RUN  ] %s\n", name);
}

static unsigned countValue(const std::vector<Event>& v, int32_t value) {
    unsigned c = 0;
    for (size_t i = 0; i < v.size(); ++i) if (v[i].value == value) ++c;
    return c;
}

// Tests

static void test_mac_learning_from_heartbeats() {
    begin_test("mac learning from heartbeats");
    Net net;
    Node* ta = net.add(ID_TA);
    Node* s1 = net.add(ID_STAGE1);

    uint8_t m[6];
    CHECK(!ta->core->peerMac(ID_STAGE1, m));
    CHECK(!s1->core->peerMac(ID_TA, m));
    CHECK(!ta->core->isPeerAlive(ID_STAGE1));

    // Queue an event before either side has heard a heartbeat.
    uint16_t seq = 0;
    CHECK(ta->core->sendEvent(ID_STAGE1, SIG_START_PICKUP, 42, 0, &seq));
    CHECK(ta->core->eventStatus(ID_STAGE1, seq) == EVENT_PENDING);

    net.run(1);
    CHECK_EQ(ta->unicastSent, 0);   // MAC unknown: nothing unicast yet

    net.run(20);
    CHECK(ta->core->peerMac(ID_STAGE1, m));
    CHECK(memcmp(m, s1->mac, 6) == 0);
    CHECK(s1->core->peerMac(ID_TA, m));
    CHECK(memcmp(m, ta->mac, 6) == 0);
    CHECK(ta->peers.count(Node::macKey(s1->mac)) == 1);   // registered with the radio
    CHECK_EQ(ta->unicastToUnregistered, 0);                // never unicast before registering
    CHECK(ta->core->isPeerAlive(ID_STAGE1));
    CHECK(ta->core->eventStatus(ID_STAGE1, seq) == EVENT_ACKED);
    CHECK_EQ(s1->received.size(), 1u);
    if (!s1->received.empty()) {
        CHECK_EQ(s1->received[0].src, ID_TA);
        CHECK_EQ(s1->received[0].signal, SIG_START_PICKUP);
        CHECK_EQ(s1->received[0].value, 42);
    }

    // A changed MAC for the same id (replaced board) is re-learned.
    net.byId(ID_STAGE1)->mac[5] = 0x99;
    net.run(150);
    CHECK(ta->core->peerMac(ID_STAGE1, m));
    CHECK_EQ(m[5], 0x99);
}

static void test_reliable_under_loss() {
    begin_test("reliable EVENT delivery, 30% loss both ways");
    Net net;
    net.loss = 0.30;
    net.jitterUs = 4000;
    Node* a = net.add(ID_STAGE1);
    Node* b = net.add(ID_TA);
    net.run(300);   // let heartbeats establish the link despite loss

    const int N = 120;
    std::vector<uint16_t> seqs;
    for (int i = 0; i < N; ++i) {
        uint16_t seq = 0;
        CHECK(a->core->sendEvent(ID_TA, SIG_START_PICKUP, i, 0, &seq));
        seqs.push_back(seq);
        net.run(25);
    }
    net.run(2000);

    for (int i = 0; i < N; ++i) CHECK_EQ(countValue(b->received, i), 1u);   // exactly once
    // eventStatus() only remembers the last RESULT_RING outcomes (completion order, not send order).
    int remembered = 0;
    for (int i = 0; i < N; ++i) {
        const EventStatus st = a->core->eventStatus(ID_TA, seqs[i]);
        CHECK(st == EVENT_ACKED || st == EVENT_UNKNOWN);   // never PENDING/FAILED
        if (st == EVENT_ACKED) ++remembered;
    }
    CHECK_EQ(remembered, RESULT_RING);
    PeerStats s;
    CHECK(a->core->getPeerStats(ID_TA, s));
    CHECK_EQ(s.eventsTx, N);
    CHECK_EQ(s.eventsAcked, N);
    CHECK_EQ(s.failed, 0);
    CHECK(s.retries > 0);
    PeerStats r;
    CHECK(b->core->getPeerStats(ID_STAGE1, r));
    CHECK_EQ(r.rx, N);
    printf("         tx=%u acked=%u retries=%u failed=%u | rx=%u dups=%u | pkts delivered=%u dropped=%u\n",
           s.eventsTx, s.eventsAcked, s.retries, s.failed, r.rx, r.duplicates, net.delivered, net.dropped);
}

static void test_duplicate_suppression() {
    begin_test("duplicate suppression");
    Net net;
    net.dup = 1.0;       // every packet is delivered twice
    net.loss = 0.2;
    net.jitterUs = 3000;
    Node* a = net.add(ID_STAGE2);
    Node* b = net.add(ID_ROUTER);
    net.run(300);

    for (int i = 0; i < 40; ++i) {
        CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, i));
        net.run(40);
    }
    net.run(1500);
    for (int i = 0; i < 40; ++i) CHECK_EQ(countValue(b->received, i), 1u);
    PeerStats r;
    CHECK(b->core->getPeerStats(ID_STAGE2, r));
    CHECK_EQ(r.rx, 40);
    CHECK(r.duplicates > 0);

    // Raw replay of an identical frame: one delivery, still ACKed each time.
    Net net2;
    Node* c = net2.add(ID_TA);
    Node* d = net2.add(ID_STAGE1);
    net2.run(300);
    Frame f;
    memset(&f, 0, sizeof(f));
    f.magic = FRAME_MAGIC; f.version = PROTOCOL_VERSION; f.srcId = ID_TA; f.dstId = ID_STAGE1;
    f.type = FRAME_EVENT; f.seq = 777; f.signal = SIG_STAGE2_START; f.value = 5;
    for (int i = 0; i < 3; ++i) d->core->onReceive(c->mac, (const uint8_t*)&f, sizeof(f));
    net2.run(10);
    CHECK_EQ(d->received.size(), 1u);
    PeerStats r2;
    CHECK(d->core->getPeerStats(ID_TA, r2));
    CHECK_EQ(r2.duplicates, 2);
}

static void test_reordering() {
    begin_test("reordering (random delay 0-120 ms)");
    Net net;
    net.jitterUs = 120000;
    Node* a = net.add(ID_TA);
    Node* b = net.add(ID_STAGE2);
    net.run(500);
    const int N = 60;
    int sent = 0;
    for (int ms = 0; ms < 4000 && sent < N; ++ms) {
        if (a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, sent)) ++sent;
        net.run(1);
    }
    CHECK_EQ(sent, N);
    net.run(2500);
    for (int i = 0; i < N; ++i) CHECK_EQ(countValue(b->received, i), 1u);
    PeerStats s;
    CHECK(a->core->getPeerStats(ID_STAGE2, s));
    CHECK_EQ(s.failed, 0);
    CHECK_EQ(s.eventsAcked, N);
}

static void test_give_up_and_failure_count() {
    begin_test("give up after the bounded window, failure counted");
    Net net;
    Node* a = net.add(ID_STAGE2);
    Node* b = net.add(ID_ROUTER);
    net.run(400);
    CHECK(a->core->isPeerAlive(ID_ROUTER));

    net.blackhole = true;
    uint16_t seq = 0;
    CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, 1, 0, &seq));
    net.run(EVENT_GIVE_UP_MS - 100);
    CHECK(a->core->eventStatus(ID_ROUTER, seq) == EVENT_PENDING);
    net.run(150);
    CHECK(a->core->eventStatus(ID_ROUTER, seq) == EVENT_FAILED);
    PeerStats s;
    CHECK(a->core->getPeerStats(ID_ROUTER, s));
    CHECK_EQ(s.failed, 1);
    CHECK_EQ(s.eventsAcked, 0);
    CHECK(s.retries >= 40 && s.retries <= 50);
    CHECK_EQ(b->received.size(), 0u);

    // Custom (short) window.
    uint16_t seq2 = 0;
    CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, 2, 200, &seq2));
    net.run(180);
    CHECK(a->core->eventStatus(ID_ROUTER, seq2) == EVENT_PENDING);
    net.run(40);
    CHECK(a->core->eventStatus(ID_ROUTER, seq2) == EVENT_FAILED);
    CHECK(a->core->getPeerStats(ID_ROUTER, s));
    CHECK_EQ(s.failed, 2);

    // The link recovers: a later event goes through.
    net.blackhole = false;
    net.run(200);
    uint16_t seq3 = 0;
    CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, 3, 0, &seq3));
    net.run(100);
    CHECK(a->core->eventStatus(ID_ROUTER, seq3) == EVENT_ACKED);
    CHECK_EQ(countValue(b->received, 3), 1u);
    CHECK_EQ(countValue(b->received, 1), 0u);
}

static void test_stale_peer_detection() {
    begin_test("stale-peer detection and fail-safe flags");
    Net net;
    Node* ta = net.add(ID_TA);
    Node* r = net.add(ID_ROUTER);
    CHECK(!ta->core->isPeerAlive(ID_ROUTER));          // never heard = dead
    CHECK(!ta->core->peerFlag(ID_ROUTER, FLAG_READY));
    CHECK_EQ(ta->core->peerHeartbeatAgeMs(ID_ROUTER), 0xFFFFFFFFu);

    r->core->setLocalFlag(FLAG_READY, true);
    r->core->setLocalState(7);
    net.run(250);
    CHECK(ta->core->isPeerAlive(ID_ROUTER));
    CHECK(ta->core->peerFlag(ID_ROUTER, FLAG_READY));
    CHECK(!ta->core->peerFlag(ID_ROUTER, FLAG_CUT_INHIBIT));
    CHECK_EQ(ta->core->peerState(ID_ROUTER), 7);

    r->core->setLocalFlag(FLAG_READY, false);
    net.run(150);
    CHECK(!ta->core->peerFlag(ID_ROUTER, FLAG_READY));
    r->core->setLocalFlag(FLAG_READY, true);
    net.run(150);
    CHECK(ta->core->peerFlag(ID_ROUTER, FLAG_READY));

    // Router goes silent: flag must read false once the stale window passes.
    r->powered = false;
    net.run(PEER_STALE_MS - 150);
    CHECK(ta->core->isPeerAlive(ID_ROUTER));
    net.run(250);
    CHECK(!ta->core->isPeerAlive(ID_ROUTER));
    CHECK(!ta->core->peerFlag(ID_ROUTER, FLAG_READY));       // stale reads as not ready
    CHECK(ta->core->peerHeartbeatAgeMs(ID_ROUTER) >= PEER_STALE_MS);

    // Recovery.
    r->powered = true;
    net.run(250);
    CHECK(ta->core->isPeerAlive(ID_ROUTER));
    CHECK(ta->core->peerFlag(ID_ROUTER, FLAG_READY));
}

static void test_sequence_wraparound() {
    begin_test("uint16 sequence wraparound");
    Net net;
    net.dup = 0.5;
    net.jitterUs = 2000;
    Node* a = net.add(ID_STAGE1, 65525);
    Node* b = net.add(ID_TA);
    net.run(300);
    std::vector<uint16_t> seqs;
    for (int i = 0; i < 40; ++i) {
        uint16_t seq = 0;
        CHECK(a->core->sendEvent(ID_TA, SIG_START_PICKUP, i, 0, &seq));
        seqs.push_back(seq);
        net.run(30);
    }
    net.run(500);
    bool wrapped = false;
    for (size_t i = 1; i < seqs.size(); ++i) if (seqs[i] < seqs[i - 1]) wrapped = true;
    CHECK(wrapped);
    for (int i = 0; i < 40; ++i) CHECK_EQ(countValue(b->received, i), 1u);
    PeerStats s;
    CHECK(a->core->getPeerStats(ID_TA, s));
    CHECK_EQ(s.eventsAcked, 40);
    CHECK_EQ(s.failed, 0);
}

static void test_millis_wraparound() {
    begin_test("millis() wraparound");
    Net net;
    net.startMs = 0xFFFFFF00u;       // wraps 256 ms into the run
    net.startUs = 0xFFFFFC00u;       // micros() wraps too
    net.loss = 0.2;
    Node* a = net.add(ID_STAGE2);
    Node* b = net.add(ID_ROUTER);
    b->core->setLocalFlag(FLAG_READY, true);
    net.run(150);
    CHECK(a->core->peerFlag(ID_ROUTER, FLAG_READY));

    // Events straddling the wrap.
    for (int i = 0; i < 10; ++i) {
        CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, i));
        net.run(60);
    }
    net.run(600);
    for (int i = 0; i < 10; ++i) CHECK_EQ(countValue(b->received, i), 1u);
    CHECK(a->core->isPeerAlive(ID_ROUTER));
    CHECK(a->core->peerFlag(ID_ROUTER, FLAG_READY));

    // Give-up and staleness timing across the wrap: the link goes dead ~1 ms before
    // millis() wraps, so the whole give-up window lies on the far side of the wrap.
    Net n3;
    n3.startMs = 0xFFFFFFFFu - 400;
    Node* e = n3.add(ID_STAGE2);
    Node* f = n3.add(ID_ROUTER);
    (void)f;
    n3.run(400);                     // link up; millis() is now ~1 ms from wrapping
    n3.blackhole = true;
    uint16_t seq = 0;
    CHECK(e->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, 9, 0, &seq));
    n3.run(EVENT_GIVE_UP_MS - 100);
    CHECK(e->core->eventStatus(ID_ROUTER, seq) == EVENT_PENDING);   // not expired early by the wrap
    n3.run(200);
    CHECK(e->core->eventStatus(ID_ROUTER, seq) == EVENT_FAILED);
    CHECK(!e->core->isPeerAlive(ID_ROUTER));       // heartbeats stopped across the wrap
}

static void test_peer_restart_resets_dedup() {
    begin_test("peer reboot (new frame epoch) restarts the duplicate window");
    Net net;
    Node* a = net.add(ID_TA, 1000);
    Node* b = net.add(ID_STAGE2);
    net.run(300);
    for (int i = 0; i < 10; ++i) {
        CHECK(a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, i));
        net.run(30);
    }
    net.run(200);
    CHECK_EQ(b->received.size(), 10u);

    // TA reboots and restarts its sequence numbers inside the old window.
    a->reboot(1005, 0x5151);
    net.run(300);   // heartbeat with the new epoch reaches Stage 2
    PeerStats s;
    CHECK(b->core->getPeerStats(ID_TA, s));
    CHECK_EQ(s.restarts, 1);
    CHECK(a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, 100));
    net.run(200);
    CHECK_EQ(countValue(b->received, 100), 1u);

    // Reboot far behind the old window: also delivered.
    a->reboot(100, 0x6262);
    net.run(300);
    CHECK(a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, 101));
    net.run(200);
    CHECK_EQ(countValue(b->received, 101), 1u);

    // First frame after a reboot is an EVENT (its heartbeat was lost): the epoch in the
    // event frame alone restarts the window.
    Net net2;
    Node* c = net2.add(ID_TA, 1);
    Node* d = net2.add(ID_STAGE2);
    net2.run(300);
    for (int i = 0; i < 5; ++i) { CHECK(c->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, i)); net2.run(30); }
    net2.run(100);
    CHECK_EQ(d->received.size(), 5u);
    Frame f;
    memset(&f, 0, sizeof(f));
    f.magic = FRAME_MAGIC; f.version = PROTOCOL_VERSION; f.srcId = ID_TA; f.dstId = ID_STAGE2;
    f.type = FRAME_EVENT; f.seq = 3; f.signal = SIG_STAGE2_START; f.value = 77; f.epoch = 0xBEEF;
    d->core->onReceive(c->mac, (const uint8_t*)&f, sizeof(f));
    net2.run(20);
    CHECK_EQ(countValue(d->received, 77), 1u);
}

static void test_unfinished_event_window_span() {
    begin_test("an unfinished event cannot fall outside the receiver's duplicate window");
    Net net;
    Node* a = net.add(ID_STAGE1, 500);
    Node* b = net.add(ID_TA);
    net.run(300);

    net.dropEventSeq = 500;      // the first event's frames never arrive
    uint16_t first = 0;
    CHECK(a->core->sendEvent(ID_TA, SIG_START_PICKUP, 1000, 0, &first));
    CHECK_EQ(first, 500);
    int accepted = 0;
    bool refused = false;
    for (int i = 0; i < 60; ++i) {
        if (a->core->sendEvent(ID_TA, SIG_START_PICKUP, i)) ++accepted; else refused = true;
        net.run(20);
    }
    CHECK(refused);                                   // blocked behind the stuck event
    CHECK(accepted >= 25 && accepted < 40);
    CHECK(a->core->eventStatus(ID_TA, first) == EVENT_PENDING || a->core->eventStatus(ID_TA, first) == EVENT_FAILED);
    net.run(EVENT_GIVE_UP_MS);                        // stuck event gives up, window reopens
    CHECK(a->core->eventStatus(ID_TA, first) == EVENT_FAILED);
    CHECK(a->core->sendEvent(ID_TA, SIG_START_PICKUP, 999));
    net.run(100);
    CHECK_EQ(countValue(b->received, 999), 1u);
    CHECK_EQ(countValue(b->received, 1000), 0u);      // never delivered
    for (int i = 0; i < accepted; ++i) CHECK_EQ(countValue(b->received, i), 1u);   // each exactly once
}

static void test_ping_link_test() {
    begin_test("link test (PING events)");
    Net net;
    net.baseDelayUs = 3000;     // 3 ms each way -> ~6 ms ack latency
    Node* s1 = net.add(ID_STAGE1);
    Node* ta = net.add(ID_TA);
    net.run(300);

    CHECK(!s1->core->startPingTest(ID_STAGE1, 5));     // to self
    CHECK(!s1->core->startPingTest(ID_TA, 0));
    CHECK(!s1->core->startPingTest(ID_TA, PING_TEST_MAX_COUNT + 1));
    CHECK(s1->core->startPingTest(ID_TA, 20));
    CHECK(!s1->core->startPingTest(ID_TA, 5));         // already running
    CHECK(net.runUntil([&] { return s1->core->pingTestResult().done; }, 3000));
    PingTestResult r = s1->core->pingTestResult();
    CHECK(r.completed);
    CHECK_EQ(r.sent, 20);
    CHECK_EQ(r.delivered, 20);
    CHECK_EQ(r.failed, 0);
    CHECK_EQ(r.retries, 0);
    CHECK(r.minUs >= 6000 && r.minUs <= 9000);
    CHECK(r.maxUs >= r.avgUs && r.avgUs >= r.minUs && r.maxUs <= 9500);
    CHECK_EQ(ta->received.size(), 0u);                 // PING never reaches machine logic
    PeerStats rx;
    CHECK(ta->core->getPeerStats(ID_STAGE1, rx));
    CHECK_EQ(rx.rx, 20);
    printf("         20 pings: min=%.2f avg=%.2f max=%.2f ms\n", r.minUs / 1000.0, r.avgUs / 1000.0, r.maxUs / 1000.0);

    char json[512];
    s1->core->formatPingResultJson(json, sizeof(json));
    CHECK(strstr(json, "\"delivered\":20") != nullptr);
    CHECK(strstr(json, "\"to\":\"ta\"") != nullptr);

    // Under 30% loss: all still delivered, with retries and a larger worst case.
    net.loss = 0.3;
    CHECK(s1->core->startPingTest(ID_TA, 40));
    CHECK(net.runUntil([&] { return s1->core->pingTestResult().done; }, 10000));
    r = s1->core->pingTestResult();
    CHECK_EQ(r.delivered, 40);
    CHECK_EQ(r.failed, 0);
    CHECK(r.retries > 0);
    CHECK(r.maxUs > 30000);

    // Dead peer: pings fail after the give-up window and are counted.
    net.loss = 0;
    net.blackhole = true;
    CHECK(s1->core->startPingTest(ID_TA, 2));
    CHECK(net.runUntil([&] { return s1->core->pingTestResult().done; }, 2 * EVENT_GIVE_UP_MS + 500));
    r = s1->core->pingTestResult();
    CHECK_EQ(r.delivered, 0);
    CHECK_EQ(r.failed, 2);
}

static void test_rx_queue_backpressure() {
    begin_test("receive queue full -> no ACK, sender retries");
    Net net;
    Node* a = net.add(ID_STAGE2);
    Node* b = net.add(ID_ROUTER);
    b->drainEnabled = false;
    net.run(300);
    const int N = RX_QUEUE_SIZE + 4;
    for (int i = 0; i < RX_QUEUE_SIZE; ++i) CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, i));
    net.run(200);
    PeerStats sa;
    CHECK(a->core->getPeerStats(ID_ROUTER, sa));
    CHECK_EQ(sa.eventsAcked, RX_QUEUE_SIZE);      // queue exactly full
    for (int i = RX_QUEUE_SIZE; i < N; ++i) CHECK(a->core->sendEvent(ID_ROUTER, SIG_ROUTER_START, i));
    net.run(300);
    PeerStats rb;
    CHECK(b->core->getPeerStats(ID_STAGE2, rb));
    CHECK_EQ(rb.rx, RX_QUEUE_SIZE);
    CHECK(rb.rxDropped > 0);                       // refused, not acked
    CHECK(a->core->getPeerStats(ID_ROUTER, sa));
    CHECK_EQ(sa.eventsAcked, RX_QUEUE_SIZE);       // the rest are unacked, still in flight

    // Machine loop starts draining before the give-up window: everything arrives once.
    b->drainEnabled = true;
    net.run(800);
    for (int i = 0; i < N; ++i) CHECK_EQ(countValue(b->received, i), 1u);
    CHECK(a->core->getPeerStats(ID_ROUTER, sa));
    CHECK_EQ(sa.eventsAcked, N);
    CHECK_EQ(sa.failed, 0);
}

static void test_frame_validation() {
    begin_test("malformed / foreign frames are ignored");
    Net net;
    Node* a = net.add(ID_TA);
    Frame f;
    memset(&f, 0, sizeof(f));
    f.magic = FRAME_MAGIC; f.version = PROTOCOL_VERSION; f.srcId = ID_STAGE1; f.dstId = ID_BROADCAST;
    f.type = FRAME_HEARTBEAT;
    const uint8_t mac[6] = {1, 2, 3, 4, 5, 6};

    Frame bad = f; bad.magic = 0;
    a->core->onReceive(mac, (const uint8_t*)&bad, sizeof(bad));
    bad = f; bad.version = 99;
    a->core->onReceive(mac, (const uint8_t*)&bad, sizeof(bad));
    bad = f; bad.srcId = ID_TA;                       // from ourselves
    a->core->onReceive(mac, (const uint8_t*)&bad, sizeof(bad));
    bad = f; bad.srcId = 9;                           // unknown machine
    a->core->onReceive(mac, (const uint8_t*)&bad, sizeof(bad));
    bad = f; bad.type = FRAME_EVENT; bad.dstId = ID_ROUTER;   // addressed to someone else
    a->core->onReceive(mac, (const uint8_t*)&bad, sizeof(bad));
    a->core->onReceive(mac, (const uint8_t*)&f, sizeof(f) - 1);   // truncated
    a->core->onReceive(nullptr, (const uint8_t*)&f, sizeof(f));
    CHECK(!a->core->isPeerAlive(ID_STAGE1));
    uint8_t m[6];
    CHECK(!a->core->peerMac(ID_STAGE1, m));

    a->core->onReceive(mac, (const uint8_t*)&f, sizeof(f));       // the valid one
    CHECK(a->core->isPeerAlive(ID_STAGE1));
    CHECK(a->core->peerMac(ID_STAGE1, m));
    CHECK_EQ(sizeof(Frame), 16u);   // fits any ESP-NOW payload (max 250)

    // Machine name / id parsing used by the HTTP link-test endpoint.
    CHECK_EQ(machineIdFromText("ta"), ID_TA);
    CHECK_EQ(machineIdFromText("Stage1"), ID_STAGE1);
    CHECK_EQ(machineIdFromText("STAGE2"), ID_STAGE2);
    CHECK_EQ(machineIdFromText("router"), ID_ROUTER);
    CHECK_EQ(machineIdFromText("3"), ID_STAGE2);
    CHECK_EQ(machineIdFromText("5"), ID_SWITCH_CONTROLLER);
    CHECK_EQ(machineIdFromText("stage"), ID_NONE);
    CHECK_EQ(machineIdFromText("routers"), ID_NONE);
    CHECK_EQ(machineIdFromText(""), ID_NONE);
    CHECK_EQ(machineIdFromText(nullptr), ID_NONE);
}

static void test_full_mesh() {
    begin_test("full mesh: every machine reaches every other");
    Net net;
    net.loss = 0.1;
    net.jitterUs = 3000;
    Node* n[5];
    for (uint8_t id = 1; id <= 4; ++id) n[id] = net.add(id);
    net.run(400);
    for (uint8_t from = 1; from <= 4; ++from)
        for (uint8_t to = 1; to <= 4; ++to) {
            if (from == to) continue;
            for (int k = 0; k < 5; ++k) CHECK(n[from]->core->sendEvent(to, SIG_START_PICKUP, from * 100 + k));
            net.run(40);
        }
    net.run(1500);
    for (uint8_t to = 1; to <= 4; ++to)
        for (uint8_t from = 1; from <= 4; ++from) {
            if (from == to) continue;
            for (int k = 0; k < 5; ++k) CHECK_EQ(countValue(n[to]->received, from * 100 + k), 1u);
            CHECK(n[to]->core->isPeerAlive(from));
        }
    char json[1500];
    size_t len = n[1]->core->formatStatusJson(json, sizeof(json));
    CHECK(len > 100 && len < sizeof(json));
    CHECK(strstr(json, "\"name\":\"router\"") != nullptr);
    CHECK(!n[1]->core->sendEvent(1, SIG_PING));      // not to self
    CHECK(!n[1]->core->sendEvent(0, SIG_PING));
    CHECK(!n[1]->core->sendEvent(ID_SWITCH_CONTROLLER + 1, SIG_PING));
}

static void test_tx_table_full_and_status_lifecycle() {
    begin_test("in-flight table limit and event status lifecycle");
    Net net;
    Node* a = net.add(ID_TA);
    net.add(ID_STAGE2)->powered = false;     // never answers
    net.run(50);
    uint16_t seqs[TX_SLOTS];
    for (int i = 0; i < TX_SLOTS; ++i) CHECK(a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, i, 0, &seqs[i]));
    CHECK(!a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, 99));   // table full
    CHECK(a->core->eventStatus(ID_STAGE2, seqs[0]) == EVENT_PENDING);
    CHECK(a->core->eventStatus(ID_STAGE2, 12345) == EVENT_UNKNOWN);
    net.run(EVENT_GIVE_UP_MS + 100);
    CHECK(a->core->eventStatus(ID_STAGE2, seqs[0]) == EVENT_FAILED);
    CHECK(a->core->sendEvent(ID_STAGE2, SIG_STAGE2_START, 100));   // slots free again
}

static void test_heartbeat_loss_estimate() {
    begin_test("heartbeat loss estimate");
    Net net;
    net.loss = 0.3;
    Node* a = net.add(ID_TA);
    net.add(ID_ROUTER);
    net.run(20000);
    PeerStats s;
    CHECK(a->core->getPeerStats(ID_ROUTER, s));
    const double total = s.heartbeats + s.heartbeatsLost;
    const double lossRate = s.heartbeatsLost / total;
    CHECK(lossRate > 0.2 && lossRate < 0.4);
    printf("         heartbeats rx=%u est.lost=%u (%.0f%%)\n", s.heartbeats, s.heartbeatsLost, lossRate * 100.0);
}

static void test_flash_gate() {
    begin_test("flash gate: whole line idle before an upload");
    Net net;
    Node* n[5];
    for (uint8_t id = 1; id <= 4; ++id) n[id] = net.add(id);
    std::unique_ptr<FlashGate> g[5];
    for (uint8_t id = 1; id <= 4; ++id) g[id].reset(new FlashGate(*n[id]->core));
    bool idle[5] = {false, true, true, true, true};
    // Drive every machine loop for `ms`, 1 ms apart.
    auto run = [&](uint32_t ms) {
        for (uint32_t i = 0; i < ms; ++i) {
            for (uint8_t id = 1; id <= 4; ++id) g[id]->update(idle[id], n[id]->millis());
            net.run(1);
        }
    };

    run(FLASH_IDLE_SETTLE_MS / 2);
    CHECK(!g[1]->allowed());                       // idle, but not for long enough yet
    CHECK(n[1]->core->peerFlag(ID_ROUTER, FLAG_IDLE));
    run(FLASH_IDLE_SETTLE_MS);
    for (uint8_t id = 1; id <= 4; ++id) CHECK(g[id]->allowed());

    // One machine busy: nobody may flash; settle restarts once it is idle again.
    idle[ID_STAGE1] = false;
    run(250);
    for (uint8_t id = 1; id <= 4; ++id) CHECK(!g[id]->allowed());
    idle[ID_STAGE1] = true;
    run(FLASH_IDLE_SETTLE_MS / 2);
    CHECK(!g[ID_TA]->allowed());
    run(FLASH_IDLE_SETTLE_MS);
    CHECK(g[ID_TA]->allowed());

    // TA flashing: others see it, stop allowing uploads; TA idle flag drops.
    CHECK(!g[ID_STAGE1]->flashPending());
    g[ID_TA]->setFlashing(true);
    CHECK(!g[ID_TA]->allowed());
    CHECK(g[ID_TA]->flashPending());
    run(250);
    CHECK(g[ID_STAGE1]->flashPending());
    CHECK(!n[ID_STAGE1]->core->peerFlag(ID_TA, FLAG_IDLE));
    for (uint8_t id = 2; id <= 4; ++id) CHECK(!g[id]->allowed());
    g[ID_TA]->setFlashing(false);                  // upload error: back to normal
    run(250);
    CHECK(!g[ID_STAGE1]->flashPending());
    run(FLASH_IDLE_SETTLE_MS);
    CHECK(g[ID_STAGE1]->allowed());

    // A powered-off machine doesn't block (it can't be mid-cycle) ...
    n[ID_ROUTER]->powered = false;
    idle[ID_ROUTER] = false;
    run(PEER_STALE_MS + 250);
    run(FLASH_IDLE_SETTLE_MS);
    CHECK(g[ID_TA]->allowed());
    // ... and blocks again as soon as it is back and busy.
    n[ID_ROUTER]->powered = true;
    run(250);
    CHECK(!g[ID_TA]->allowed());
    // A flashing machine that goes silent (rebooting) keeps the hold until it is
    // heard again, or FLASH_REBOOT_HOLD_MS at most.
    g[ID_STAGE2]->setFlashing(true);
    run(250);
    CHECK(g[ID_TA]->flashPending());
    n[ID_STAGE2]->powered = false;
    run(PEER_STALE_MS + 250);
    CHECK(g[ID_TA]->flashPending());
    run(FLASH_REBOOT_HOLD_MS);
    CHECK(!g[ID_TA]->flashPending());
}

// Line order: Stage 1 -> TA -> Stage 2 -> Router.
static void test_flash_request_drains_upstream() {
    begin_test("flash request: target waits for itself + upstream, Stage 1 holds");
    Net net;
    Node* n[5];
    for (uint8_t id = 1; id <= 4; ++id) n[id] = net.add(id);
    std::unique_ptr<FlashGate> g[5];
    for (uint8_t id = 1; id <= 4; ++id) g[id].reset(new FlashGate(*n[id]->core));
    bool idle[5] = {false, true, true, true, true};
    auto run = [&](uint32_t ms) {
        for (uint32_t i = 0; i < ms; ++i) {
            for (uint8_t id = 1; id <= 4; ++id) g[id]->update(idle[id], n[id]->millis());
            net.run(1);
        }
    };
    run(500);

    // Upload to the TA while Stage 1 is mid-cycle and Stage 2 / router are busy.
    idle[ID_STAGE1] = idle[ID_STAGE2] = idle[ID_ROUTER] = false;
    run(250);
    g[ID_TA]->request(n[ID_TA]->millis());
    CHECK(g[ID_TA]->requested());
    run(250);
    CHECK(g[ID_STAGE1]->flashPending());           // Stage 1: no new cycle after this one
    CHECK(!g[ID_TA]->allowed());
    CHECK_EQ(g[ID_TA]->waitingFor(), ID_STAGE1);

    // Stage 1 finishes and hands to the TA (TA busy).
    idle[ID_STAGE1] = true;
    idle[ID_TA] = false;
    run(250);
    CHECK_EQ(g[ID_TA]->waitingFor(), ID_TA);
    CHECK(!g[ID_TA]->allowed());

    // TA finishes; Stage 2 and router still busy downstream -> upload may start
    // after the settle time.
    idle[ID_TA] = true;
    run(250);
    CHECK_EQ(g[ID_TA]->waitingFor(), ID_NONE);
    CHECK(!g[ID_TA]->allowed());
    run(FLASH_IDLE_SETTLE_MS);
    CHECK(g[ID_TA]->allowed());
    CHECK(!g[ID_STAGE2]->allowed());               // nobody else got a slot
    CHECK(!g[ID_ROUTER]->allowed());

    // Upload starts, then fails: request dropped, Stage 1 released.
    g[ID_TA]->setFlashing(true);
    run(250);
    CHECK(g[ID_STAGE1]->flashPending());
    g[ID_TA]->setFlashing(false);
    run(250);
    CHECK(!g[ID_TA]->requested());
    CHECK(!g[ID_STAGE1]->flashPending());

    // Router as target: every machine before it must be idle.
    idle[ID_STAGE2] = false;
    g[ID_ROUTER]->request(n[ID_ROUTER]->millis());
    idle[ID_ROUTER] = true;
    run(FLASH_IDLE_SETTLE_MS + 500);
    CHECK(!g[ID_ROUTER]->allowed());
    CHECK_EQ(g[ID_ROUTER]->waitingFor(), ID_STAGE2);
    idle[ID_STAGE2] = true;
    run(FLASH_IDLE_SETTLE_MS + 500);
    CHECK(g[ID_ROUTER]->allowed());
    g[ID_ROUTER]->cancel();
    run(250);
    CHECK(!g[ID_ROUTER]->requested());

    // Stage 1 as target: nothing upstream; downstream may stay busy.
    idle[ID_TA] = idle[ID_STAGE2] = idle[ID_ROUTER] = false;
    g[ID_STAGE1]->request(n[ID_STAGE1]->millis());
    run(FLASH_IDLE_SETTLE_MS + 500);
    CHECK(g[ID_STAGE1]->allowed());
    CHECK(g[ID_STAGE1]->flashPending());
    g[ID_STAGE1]->cancel();
    run(250);

    // A request with no upload expires and releases Stage 1.
    idle[ID_TA] = false;
    g[ID_TA]->request(n[ID_TA]->millis());
    run(250);
    CHECK(g[ID_STAGE1]->flashPending());
    for (uint32_t t = 0; t < FLASH_REQUEST_TIMEOUT_MS; t += 1000) {   // coarse steps: heartbeats still flow
        for (uint8_t id = 1; id <= 4; ++id) g[id]->update(idle[id], n[id]->millis());
        net.run(1000);
    }
    run(250);
    CHECK(!g[ID_TA]->requested());
    CHECK(!g[ID_STAGE1]->flashPending());
}

static void test_flag_change_sent_promptly() {
    begin_test("a local flag change is broadcast at once");
    Net net;
    Node* s2 = net.add(ID_STAGE2);
    Node* r = net.add(ID_ROUTER);
    net.run(250);
    net.run(HEARTBEAT_INTERVAL_MS / 3);            // mid-interval: the periodic beat is far off
    CHECK(!r->core->peerFlag(ID_STAGE2, FLAG_DROPPING));
    s2->core->setLocalFlag(FLAG_DROPPING, true);
    net.run(HEARTBEAT_MIN_GAP_MS + 3);
    CHECK(r->core->peerFlag(ID_STAGE2, FLAG_DROPPING));
    s2->core->setLocalFlag(FLAG_DROPPING, false);
    net.run(HEARTBEAT_MIN_GAP_MS + 3);
    CHECK(!r->core->peerFlag(ID_STAGE2, FLAG_DROPPING));

    // Re-setting an unchanged flag every loop must not add heartbeats.
    PeerStats before, after;
    r->core->getPeerStats(ID_STAGE2, before);
    for (uint32_t i = 0; i < HEARTBEAT_INTERVAL_MS * 3; ++i) {
        s2->core->setLocalFlag(FLAG_DROPPING, false);
        net.run(1);
    }
    r->core->getPeerStats(ID_STAGE2, after);
    CHECK(after.heartbeats - before.heartbeats <= 4);
}

static void test_flash_gate_legacy_peer() {
    begin_test("flash gate: peers without the interlock are ignored");
    Net net;
    Node* ta = net.add(ID_TA);
    Node* legacy = net.add(ID_ROUTER);    // older firmware: no FlashGate, never sets FLAG_IDLE
    FlashGate g(*ta->core);
    auto run = [&](uint32_t ms) {
        for (uint32_t i = 0; i < ms; ++i) { g.update(true, ta->millis()); net.run(1); }
    };
    run(FLASH_IDLE_SETTLE_MS + 500);
    CHECK(ta->core->isPeerAlive(ID_ROUTER));
    CHECK(g.allowed());
    CHECK(legacy->core->peerFlag(ID_TA, FLAG_FLASH_GATE));
    CHECK(legacy->core->peerFlag(ID_TA, FLAG_IDLE));
    // An older machine that is itself flashing still blocks and is reported.
    legacy->core->setLocalFlag(FLAG_FLASHING, true);
    run(250);
    CHECK(!g.allowed());
    CHECK(g.flashPending());
}

int main() {
    test_mac_learning_from_heartbeats();
    test_reliable_under_loss();
    test_duplicate_suppression();
    test_reordering();
    test_give_up_and_failure_count();
    test_stale_peer_detection();
    test_sequence_wraparound();
    test_millis_wraparound();
    test_peer_restart_resets_dedup();
    test_unfinished_event_window_span();
    test_ping_link_test();
    test_rx_queue_backpressure();
    test_frame_validation();
    test_full_mesh();
    test_tx_table_full_and_status_lifecycle();
    test_heartbeat_loss_estimate();
    test_flash_gate();
    test_flash_gate_legacy_peer();
    test_flag_change_sent_promptly();
    test_flash_request_drains_upstream();

    printf("\n%d checks, %d failures\n", g_checks, g_failures);
    printf(g_failures == 0 ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    return g_failures == 0 ? 0 : 1;
}
