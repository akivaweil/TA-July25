#include "LineLinkCore.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace linelink {

Core::Core(IRadio& radio, IClock& clock, ILock& lock)
    : radio_(radio), clock_(clock), lock_(lock) {
    self_ = ID_NONE;
    epoch_ = 0;
    localFlags_ = 0;
    localState_ = 0;
    hbSeq_ = 0;
    hbSentOnce_ = false;
    flagsChanged_ = false;
    lastHbSendMs_ = 0;
    memset(nextSeq_, 0, sizeof(nextSeq_));
    memset(peers_, 0, sizeof(peers_));
    memset(tx_, 0, sizeof(tx_));
    memset(rxQueue_, 0, sizeof(rxQueue_));
    memset(ackQueue_, 0, sizeof(ackQueue_));
    memset(results_, 0, sizeof(results_));
    memset(&ping_, 0, sizeof(ping_));
    rxHead_ = rxCount_ = 0;
    ackHead_ = ackCount_ = 0;
    resultNext_ = 0;
    for (uint8_t i = 0; i < PEER_SLOTS; ++i) peers_[i].ackMinUs = HOP_MIN_US_UNSET;
}

void Core::begin(uint8_t selfId, uint16_t seqSeed, int32_t bootNonce) {
    lock_.lock();
    self_ = selfId;
    epoch_ = (uint16_t)bootNonce;
    for (uint8_t i = 0; i < PEER_SLOTS; ++i) nextSeq_[i] = seqSeed;
    lock_.unlock();
}

// Duplicate window per source: highest sequence number seen plus a bitmap of the
// DEDUP_WINDOW numbers below it. Handles reordering, duplicates and uint16
// wraparound. A sequence number further behind than the window can only be a
// late copy of something already delivered: sendEvent() never lets a still-unfinished
// event fall that far behind the newest one, and a sender reboot is detected by the
// frame epoch (noteEpoch), which restarts the window.

bool Core::dedupIsNew(const Dedup& d, uint16_t seq) {
    if (!d.valid) return true;
    const int16_t delta = (int16_t)(uint16_t)(seq - d.highest);
    if (delta > 0) return true;
    const uint16_t back = (uint16_t)(d.highest - seq);
    if (back >= DEDUP_WINDOW) return false;
    return (d.bitmap & (1u << back)) == 0;
}

void Core::dedupCommit(Dedup& d, uint16_t seq) {
    if (!d.valid) {
        d.valid = true;
        d.highest = seq;
        d.bitmap = 1u;
        return;
    }
    const int16_t delta = (int16_t)(uint16_t)(seq - d.highest);
    if (delta > 0) {
        d.bitmap = (delta >= (int16_t)DEDUP_WINDOW) ? 1u : ((d.bitmap << delta) | 1u);
        d.highest = seq;
        return;
    }
    const uint16_t back = (uint16_t)(d.highest - seq);
    if (back < DEDUP_WINDOW) d.bitmap |= (1u << back);
}

// Frame building (lock held)

void Core::buildFrame(Frame& f, uint8_t type, uint8_t dst, uint16_t seq,
                      uint8_t signal, int32_t value) {
    f.magic = FRAME_MAGIC;
    f.version = PROTOCOL_VERSION;
    f.srcId = self_;
    f.dstId = dst;
    f.type = type;
    f.seq = seq;
    f.signal = signal;
    f.value = value;
    f.flags = localFlags_;
    f.state = localState_;
    f.epoch = epoch_;
}

// A new epoch from a known peer means it rebooted: its sequence numbers restart (lock held)

void Core::noteEpoch(Peer& p, uint16_t epoch) {
    if (!p.epochValid) {
        p.epochValid = true;
        p.epoch = epoch;
    } else if (p.epoch != epoch) {
        p.epoch = epoch;
        p.restarts++;
        p.dedup.valid = false;
        p.hbSeqValid = false;
    }
}

// Final outcome of an in-flight event (lock held)

void Core::finalizeTx(TxEvent& t, bool ok, uint32_t rxUs) {
    Peer& p = peers_[t.dst];
    uint32_t latencyUs = 0;
    if (ok) {
        p.eventsAcked++;
        if (t.sentOnce) {
            latencyUs = rxUs - t.firstSendUs;
            p.ackCount++;
            p.ackSumUs += latencyUs;
            if (latencyUs < p.ackMinUs) p.ackMinUs = latencyUs;
            if (latencyUs > p.ackMaxUs) p.ackMaxUs = latencyUs;
        }
    } else {
        p.failed++;
    }

    results_[resultNext_].dst = t.dst;
    results_[resultNext_].seq = t.seq;
    results_[resultNext_].ok = ok;
    results_[resultNext_].valid = true;
    resultNext_ = (uint8_t)((resultNext_ + 1) % RESULT_RING);

    if (t.signal == SIG_PING && ping_.active && ping_.curActive &&
        ping_.to == t.dst && ping_.curSeq == t.seq) {
        ping_.curActive = false;
        ping_.retries += t.retries;
        if (ok) {
            ping_.delivered++;
            ping_.sumUs += latencyUs;
            if (latencyUs < ping_.minUs) ping_.minUs = latencyUs;
            if (latencyUs > ping_.maxUs) ping_.maxUs = latencyUs;
        } else {
            ping_.failed++;
        }
    }
    t.used = false;
}

// Receive path (WiFi task)

void Core::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (!mac || !data || len != (int)sizeof(Frame)) return;
    Frame f;
    memcpy(&f, data, sizeof(f));
    if (f.magic != FRAME_MAGIC || f.version != PROTOCOL_VERSION) return;
    if (f.srcId < 1 || f.srcId > MAX_ID) return;

    const uint32_t nowMs = clock_.millis();
    const uint32_t nowUs = clock_.micros();

    lock_.lock();
    if (f.srcId == self_ || (f.dstId != self_ && f.dstId != ID_BROADCAST)) {
        lock_.unlock();
        return;
    }
    Peer& p = peers_[f.srcId];
    if (f.srcId == ID_SWITCH_CONTROLLER && p.retiredSwitchEpochValid &&
        f.epoch == p.retiredSwitchEpoch) {
        lock_.unlock();
        return;
    }

    // Learn the sender's MAC from any valid frame (heartbeats are the normal source).
    if (!p.macKnown || memcmp(p.mac, mac, 6) != 0) {
        memcpy(p.mac, mac, 6);
        p.macKnown = true;
        p.addPending = true;
    }

    // Keep a delayed switch heartbeat from replaying a previous button level.
    if (f.srcId == ID_SWITCH_CONTROLLER && f.type == FRAME_HEARTBEAT &&
        f.dstId == ID_BROADCAST && p.epochValid && p.epoch == f.epoch && p.hbSeqValid) {
        const uint16_t gap = (uint16_t)(f.seq - p.lastHbSeq);
        if (gap == 0 || (gap >= 0x8000 && nowMs - p.lastHbMs < PEER_STALE_MS)) {
            lock_.unlock();
            return;
        }
    }

    if (f.srcId == ID_SWITCH_CONTROLLER && p.epochValid && p.epoch != f.epoch) {
        p.retiredSwitchEpoch = p.epoch;
        p.retiredSwitchEpochValid = true;
        p.state &= (uint8_t)~SWITCH_STATE_VALID;
    }

    noteEpoch(p, f.epoch);

    if (f.type == FRAME_HEARTBEAT && f.dstId == ID_BROADCAST) {
        // Remember an expired link even if it recovered before the motion loop read it.
        if (f.srcId == ID_SWITCH_CONTROLLER && p.heardEver &&
            nowMs - p.lastHbMs >= PEER_STALE_MS) {
            p.switchControlGapCount++;
        }
        p.heardEver = true;
        p.lastHbMs = nowMs;
        p.flags = f.flags;
        p.state = f.state;
        p.heartbeats++;
        if (p.hbSeqValid) {
            const uint16_t gap = (uint16_t)(f.seq - p.lastHbSeq);
            if (gap > 1 && gap < HEARTBEAT_GAP_MAX) p.heartbeatsLost += (uint32_t)(gap - 1);
        }
        p.lastHbSeq = f.seq;
        p.hbSeqValid = true;

    } else if (f.type == FRAME_EVENT && f.dstId == self_) {
        const bool isPing = (f.signal == SIG_PING);
        bool ack = false;
        if (!dedupIsNew(p.dedup, f.seq)) {
            p.duplicates++;
            ack = true;                      // our earlier ACK may have been lost
        } else if (!isPing && rxCount_ >= RX_QUEUE_SIZE) {
            p.rxDropped++;                   // no ACK: the sender will retry later
        } else {
            dedupCommit(p.dedup, f.seq);
            p.rx++;
            if (!isPing) {
                Event& e = rxQueue_[(rxHead_ + rxCount_) % RX_QUEUE_SIZE];
                e.src = f.srcId;
                e.signal = f.signal;
                e.value = f.value;
                e.seq = f.seq;
                e.rxMs = nowMs;
                rxCount_++;
            }
            ack = true;
        }
        if (ack && ackCount_ < ACK_QUEUE_SIZE) {
            AckItem& a = ackQueue_[(ackHead_ + ackCount_) % ACK_QUEUE_SIZE];
            a.dst = f.srcId;
            a.signal = f.signal;
            a.seq = f.seq;
            ackCount_++;
        }

    } else if (f.type == FRAME_ACK && f.dstId == self_) {
        for (uint8_t i = 0; i < TX_SLOTS; ++i) {
            TxEvent& t = tx_[i];
            if (t.used && t.dst == f.srcId && t.seq == f.seq && t.signal == f.signal) {
                finalizeTx(t, true, nowUs);
                break;
            }
        }
    }
    lock_.unlock();
}

// Timers and transmit path (LineLink task)

void Core::service() {
    const uint32_t now = clock_.millis();

    // 1. Register newly learned MACs as unicast peers with the radio.
    for (uint8_t id = 1; id <= MAX_ID; ++id) {
        uint8_t mac[6];
        bool add = false;
        lock_.lock();
        if (peers_[id].addPending && peers_[id].macKnown) {
            memcpy(mac, peers_[id].mac, 6);
            peers_[id].addPending = false;
            add = true;
        }
        lock_.unlock();
        if (add && !radio_.addPeer(mac)) {
            lock_.lock();
            if (peers_[id].macKnown && memcmp(peers_[id].mac, mac, 6) == 0) peers_[id].addPending = true;
            lock_.unlock();
        }
    }

    // 2. Heartbeat broadcast.
    {
        Frame hb;
        bool due;
        lock_.lock();
        const uint32_t sinceHb = (uint32_t)(now - lastHbSendMs_);
        // Periodic, or at once when a local flag changed (interlocks react in ms).
        due = !hbSentOnce_ || sinceHb >= HEARTBEAT_INTERVAL_MS ||
              (flagsChanged_ && sinceHb >= HEARTBEAT_MIN_GAP_MS);
        if (due) {
            buildFrame(hb, FRAME_HEARTBEAT, ID_BROADCAST, hbSeq_++, SIG_NONE, 0);
            hbSentOnce_ = true;
            flagsChanged_ = false;
            lastHbSendMs_ = now;
        }
        lock_.unlock();
        if (due) radio_.sendBroadcast((const uint8_t*)&hb, sizeof(hb));
    }

    // 3. Queued ACKs.
    for (;;) {
        Frame f;
        uint8_t mac[6];
        bool have = false;
        lock_.lock();
        if (ackCount_ > 0) {
            const AckItem a = ackQueue_[ackHead_];
            const Peer& p = peers_[a.dst];
            if (p.macKnown && !p.addPending) {
                memcpy(mac, p.mac, 6);
                buildFrame(f, FRAME_ACK, a.dst, a.seq, a.signal, 0);
                ackHead_ = (uint8_t)((ackHead_ + 1) % ACK_QUEUE_SIZE);
                ackCount_--;
                have = true;
            }
        }
        lock_.unlock();
        if (!have) break;
        radio_.sendUnicast(mac, (const uint8_t*)&f, sizeof(f));
    }

    // 4. In-flight events: give up, send, resend.
    for (uint8_t i = 0; i < TX_SLOTS; ++i) {
        Frame f;
        uint8_t mac[6];
        bool send = false;
        const uint32_t nowUs = clock_.micros();
        lock_.lock();
        TxEvent& t = tx_[i];
        if (t.used) {
            if ((uint32_t)(now - t.startMs) >= t.giveUpMs) {
                finalizeTx(t, false, 0);
            } else {
                const Peer& p = peers_[t.dst];
                const bool due = !t.sentOnce ||
                                 (uint32_t)(now - t.lastSendMs) >= EVENT_RESEND_INTERVAL_MS;
                if (due && p.macKnown && !p.addPending) {
                    memcpy(mac, p.mac, 6);
                    buildFrame(f, FRAME_EVENT, t.dst, t.seq, t.signal, t.value);
                    if (!t.sentOnce) {
                        t.sentOnce = true;
                        t.firstSendUs = nowUs;
                    } else {
                        t.retries++;
                        peers_[t.dst].retries++;
                    }
                    t.lastSendMs = now;
                    send = true;
                }
            }
        }
        lock_.unlock();
        if (send) radio_.sendUnicast(mac, (const uint8_t*)&f, sizeof(f));
    }

    // 5. Link test: one PING in flight at a time.
    {
        bool sendPing = false;
        uint8_t to = 0;
        int32_t index = 0;
        lock_.lock();
        if (ping_.active) {
            if ((uint32_t)(now - ping_.startMs) >= PING_TEST_MAX_MS) {
                ping_.active = false;
                ping_.done = true;
                ping_.completed = false;
            } else if (!ping_.curActive) {
                if (ping_.sent >= ping_.requested) {
                    ping_.active = false;
                    ping_.done = true;
                    ping_.completed = true;
                } else {
                    sendPing = true;
                    to = ping_.to;
                    index = (int32_t)ping_.sent;
                }
            }
        }
        lock_.unlock();
        if (sendPing) {
            uint16_t seq = 0;
            if (sendEvent(to, SIG_PING, index, 0, &seq)) {
                lock_.lock();
                ping_.curActive = true;
                ping_.curSeq = seq;
                ping_.sent++;
                lock_.unlock();
            }
        }
    }
}

// Application API

bool Core::sendEvent(uint8_t dst, uint8_t signal, int32_t value,
                     uint32_t giveUpMs, uint16_t* outSeq) {
    const uint32_t now = clock_.millis();
    bool ok = false;
    lock_.lock();
    if (dst >= 1 && dst <= MAX_ID && dst != self_) {
        // Keep the unfinished events to one destination inside the receiver's duplicate
        // window: refuse a new event while an older one is DEDUP_WINDOW-1 numbers behind.
        bool windowFull = false;
        for (uint8_t i = 0; i < TX_SLOTS; ++i) {
            if (tx_[i].used && tx_[i].dst == dst &&
                (uint16_t)(nextSeq_[dst] - tx_[i].seq) >= (uint16_t)(DEDUP_WINDOW - 1)) {
                windowFull = true;
            }
        }
        for (uint8_t i = 0; i < TX_SLOTS && !windowFull; ++i) {
            TxEvent& t = tx_[i];
            if (t.used) continue;
            memset(&t, 0, sizeof(t));
            t.used = true;
            t.dst = dst;
            t.signal = signal;
            t.value = value;
            t.seq = nextSeq_[dst]++;
            t.startMs = now;
            t.giveUpMs = giveUpMs ? giveUpMs : EVENT_GIVE_UP_MS;
            peers_[dst].eventsTx++;
            if (outSeq) *outSeq = t.seq;
            ok = true;
            break;
        }
    }
    lock_.unlock();
    return ok;
}

EventStatus Core::eventStatus(uint8_t dst, uint16_t seq) {
    EventStatus st = EVENT_UNKNOWN;
    lock_.lock();
    for (uint8_t i = 0; i < TX_SLOTS; ++i) {
        if (tx_[i].used && tx_[i].dst == dst && tx_[i].seq == seq) {
            st = EVENT_PENDING;
            break;
        }
    }
    if (st == EVENT_UNKNOWN) {
        for (uint8_t i = 0; i < RESULT_RING; ++i) {
            if (results_[i].valid && results_[i].dst == dst && results_[i].seq == seq) {
                st = results_[i].ok ? EVENT_ACKED : EVENT_FAILED;
                break;
            }
        }
    }
    lock_.unlock();
    return st;
}

bool Core::popEvent(Event& out) {
    bool have = false;
    lock_.lock();
    if (rxCount_ > 0) {
        out = rxQueue_[rxHead_];
        rxHead_ = (uint8_t)((rxHead_ + 1) % RX_QUEUE_SIZE);
        rxCount_--;
        have = true;
    }
    lock_.unlock();
    return have;
}

void Core::setLocalFlag(uint8_t mask, bool on) {
    lock_.lock();
    const uint8_t flags = on ? (uint8_t)(localFlags_ | mask) : (uint8_t)(localFlags_ & ~mask);
    if (flags != localFlags_) flagsChanged_ = true;   // service() broadcasts it at once
    localFlags_ = flags;
    lock_.unlock();
}

void Core::setLocalFlags(uint8_t flags) {
    lock_.lock();
    if (flags != localFlags_) flagsChanged_ = true;
    localFlags_ = flags;
    lock_.unlock();
}

uint8_t Core::localFlags() {
    lock_.lock();
    const uint8_t v = localFlags_;
    lock_.unlock();
    return v;
}

void Core::setLocalState(uint8_t state) {
    lock_.lock();
    localState_ = state;
    lock_.unlock();
}

// Peer queries

bool Core::isPeerAlive(uint8_t id) {
    if (id < 1 || id > MAX_ID) return false;
    const uint32_t now = clock_.millis();
    lock_.lock();
    const Peer& p = peers_[id];
    const bool alive = p.heardEver && (uint32_t)(now - p.lastHbMs) < PEER_STALE_MS;
    lock_.unlock();
    return alive;
}

bool Core::peerFlag(uint8_t id, uint8_t mask) {
    if (id < 1 || id > MAX_ID) return false;
    const uint32_t now = clock_.millis();
    lock_.lock();
    const Peer& p = peers_[id];
    const bool alive = p.heardEver && (uint32_t)(now - p.lastHbMs) < PEER_STALE_MS;
    const bool set = alive && (p.flags & mask) == mask;
    lock_.unlock();
    return set;
}

uint8_t Core::peerState(uint8_t id) {
    if (id < 1 || id > MAX_ID) return 0;
    lock_.lock();
    const uint8_t s = peers_[id].state;
    lock_.unlock();
    return s;
}

uint32_t Core::peerHeartbeatAgeMs(uint8_t id) {
    if (id < 1 || id > MAX_ID) return 0xFFFFFFFFu;
    const uint32_t now = clock_.millis();
    lock_.lock();
    const Peer& p = peers_[id];
    const uint32_t age = p.heardEver ? (uint32_t)(now - p.lastHbMs) : 0xFFFFFFFFu;
    lock_.unlock();
    return age;
}

uint32_t Core::peerLastHeartbeatMs(uint8_t id) {
    if (id < 1 || id > MAX_ID) return 0;
    lock_.lock();
    const uint32_t t = peers_[id].lastHbMs;
    lock_.unlock();
    return t;
}

bool Core::peerMac(uint8_t id, uint8_t mac[6]) {
    if (id < 1 || id > MAX_ID) return false;
    lock_.lock();
    const bool known = peers_[id].macKnown;
    if (known) memcpy(mac, peers_[id].mac, 6);
    lock_.unlock();
    return known;
}

bool Core::getPeerStats(uint8_t id, PeerStats& out) {
    if (id < 1 || id > MAX_ID) return false;
    const uint32_t now = clock_.millis();
    lock_.lock();
    const Peer& p = peers_[id];
    out.heardEver = p.heardEver;
    out.macKnown = p.macKnown;
    out.ageMs = p.heardEver ? (uint32_t)(now - p.lastHbMs) : 0xFFFFFFFFu;
    out.alive = p.heardEver && out.ageMs < PEER_STALE_MS;
    out.state = p.state;
    out.flags = p.flags;
    out.heartbeats = p.heartbeats;
    out.heartbeatsLost = p.heartbeatsLost;
    out.restarts = p.restarts;
    out.switchControlGapCount = p.switchControlGapCount;
    out.eventsTx = p.eventsTx;
    out.eventsAcked = p.eventsAcked;
    out.retries = p.retries;
    out.failed = p.failed;
    out.rx = p.rx;
    out.duplicates = p.duplicates;
    out.rxDropped = p.rxDropped;
    out.ackCount = p.ackCount;
    out.ackMinUs = p.ackCount ? p.ackMinUs : 0;
    out.ackMaxUs = p.ackCount ? p.ackMaxUs : 0;
    out.ackAvgUs = p.ackCount ? (uint32_t)(p.ackSumUs / p.ackCount) : 0;
    lock_.unlock();
    return true;
}

// Link test

bool Core::startPingTest(uint8_t to, uint16_t count) {
    if (to < 1 || to > MAX_ID || count < 1 || count > PING_TEST_MAX_COUNT) return false;
    const uint32_t now = clock_.millis();
    bool ok = false;
    lock_.lock();
    if (to != self_ && !ping_.active) {
        memset(&ping_, 0, sizeof(ping_));
        ping_.active = true;
        ping_.to = to;
        ping_.requested = count;
        ping_.minUs = HOP_MIN_US_UNSET;
        ping_.startMs = now;
        ok = true;
    }
    lock_.unlock();
    return ok;
}

PingTestResult Core::pingTestResult() {
    PingTestResult r;
    lock_.lock();
    r.active = ping_.active;
    r.done = ping_.done;
    r.completed = ping_.completed;
    r.to = ping_.to;
    r.requested = ping_.requested;
    r.sent = ping_.sent;
    r.delivered = ping_.delivered;
    r.failed = ping_.failed;
    r.retries = ping_.retries;
    r.minUs = ping_.delivered ? ping_.minUs : 0;
    r.maxUs = ping_.delivered ? ping_.maxUs : 0;
    r.avgUs = ping_.delivered ? (uint32_t)(ping_.sumUs / ping_.delivered) : 0;
    lock_.unlock();
    return r;
}

void Core::cancelPingTest() {
    lock_.lock();
    if (ping_.active) {
        ping_.active = false;
        ping_.done = true;
        ping_.completed = false;
    }
    lock_.unlock();
}

// JSON (no heap)

namespace {
struct Out {
    char*  buf;
    size_t cap;
    size_t len;

    void add(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
};

void Out::add(const char* fmt, ...) {
    if (len + 1 >= cap) return;
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(buf + len, cap - len, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    len += ((size_t)n < cap - len) ? (size_t)n : (cap - len - 1);
}
}  // namespace

size_t Core::formatStatusJson(char* buf, size_t cap) {
    if (!buf || cap == 0) return 0;
    buf[0] = '\0';
    Out o = { buf, cap, 0 };
    o.add("{\"self\":%u,\"name\":\"%s\",\"peers\":[", (unsigned)self_, machineName(self_));
    bool first = true;
    for (uint8_t id = 1; id <= MAX_ID; ++id) {
        if (id == self_) continue;
        PeerStats s;
        if (!getPeerStats(id, s)) continue;
        o.add("%s{\"id\":%u,\"name\":\"%s\",\"alive\":%s,\"ageMs\":%ld,\"state\":%u,\"flags\":%u,"
              "\"hb\":%lu,\"hbLost\":%lu,\"restarts\":%lu,\"tx\":%lu,\"acked\":%lu,\"retries\":%lu,"
              "\"failed\":%lu,\"rx\":%lu,\"dups\":%lu,\"dropped\":%lu,"
              "\"ackMs\":{\"n\":%lu,\"min\":%.1f,\"avg\":%.1f,\"max\":%.1f}}",
              first ? "" : ",", (unsigned)id, machineName(id), s.alive ? "true" : "false",
              s.heardEver ? (long)s.ageMs : -1L, (unsigned)s.state, (unsigned)s.flags,
              (unsigned long)s.heartbeats, (unsigned long)s.heartbeatsLost,
              (unsigned long)s.restarts, (unsigned long)s.eventsTx, (unsigned long)s.eventsAcked,
              (unsigned long)s.retries, (unsigned long)s.failed, (unsigned long)s.rx,
              (unsigned long)s.duplicates, (unsigned long)s.rxDropped, (unsigned long)s.ackCount,
              s.ackMinUs / 1000.0, s.ackAvgUs / 1000.0, s.ackMaxUs / 1000.0);
        first = false;
    }
    o.add("]}");
    return o.len;
}

size_t Core::formatPingResultJson(char* buf, size_t cap) {
    if (!buf || cap == 0) return 0;
    buf[0] = '\0';
    const PingTestResult r = pingTestResult();
    Out o = { buf, cap, 0 };
    o.add("{\"from\":\"%s\",\"to\":\"%s\",\"running\":%s,\"done\":%s,\"completed\":%s,"
          "\"requested\":%u,\"sent\":%u,\"delivered\":%u,\"failed\":%u,\"retries\":%lu,"
          "\"ackMs\":{\"min\":%.2f,\"avg\":%.2f,\"max\":%.2f}}",
          machineName(self_), machineName(r.to), r.active ? "true" : "false",
          r.done ? "true" : "false", r.completed ? "true" : "false",
          (unsigned)r.requested, (unsigned)r.sent, (unsigned)r.delivered, (unsigned)r.failed,
          (unsigned long)r.retries, r.minUs / 1000.0, r.avgUs / 1000.0, r.maxUs / 1000.0);
    return o.len;
}

}  // namespace linelink
