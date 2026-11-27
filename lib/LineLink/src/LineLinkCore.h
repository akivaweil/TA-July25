#pragma once

// LineLink core: transport-independent messaging logic (heartbeats, reliable
// EVENTs with ACK + resend, duplicate suppression, peer tracking, link test).
// The radio, clock and lock are injected so the same code runs on the ESP32
// (LineLink.cpp) and in host-side unit tests (test/test_linelink.cpp).
//
// Threading: onReceive() runs in the WiFi task; service() runs in the LineLink
// task (or the test driver); everything else is called from machine loops / web
// handlers. Shared tables are touched only inside short ILock sections and the
// radio is never called while the lock is held.

#include "LineLinkProtocol.h"

namespace linelink {

class IRadio {
public:
    virtual ~IRadio() {}
    virtual bool addPeer(const uint8_t mac[6]) = 0;
    virtual bool sendBroadcast(const uint8_t* data, size_t len) = 0;
    virtual bool sendUnicast(const uint8_t mac[6], const uint8_t* data, size_t len) = 0;
};

class IClock {
public:
    virtual ~IClock() {}
    virtual uint32_t millis() = 0;
    virtual uint32_t micros() = 0;
};

class ILock {
public:
    virtual ~ILock() {}
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

class NullLock : public ILock {
public:
    void lock() {}
    void unlock() {}
};

// A received EVENT, as handed to the machine loop.
struct Event {
    uint8_t  src;
    uint8_t  signal;
    int32_t  value;
    uint16_t seq;
    uint32_t rxMs;
};

enum EventStatus : uint8_t {
    EVENT_PENDING = 0,   // still being (re)sent
    EVENT_ACKED   = 1,   // peer acknowledged it
    EVENT_FAILED  = 2,   // gave up after EVENT_GIVE_UP_MS
    EVENT_UNKNOWN = 3    // not in flight and not in the recent-result ring
};

// Snapshot of one peer, for status pages and tests.
struct PeerStats {
    bool     alive;          // heartbeat heard within PEER_STALE_MS
    bool     heardEver;
    bool     macKnown;
    uint32_t ageMs;          // since last heartbeat (valid when heardEver)
    uint8_t  state;          // peer's machine state byte (last heartbeat)
    uint8_t  flags;          // peer's heartbeat flags (last heartbeat)
    uint32_t heartbeats;     // heartbeats received
    uint32_t heartbeatsLost; // estimated from heartbeat sequence gaps
    uint32_t restarts;       // peer reboots seen (frame epoch changes)
    uint32_t switchControlGapCount;  // expired ID 5 heartbeats since the first one
    uint32_t eventsTx;       // events accepted for sending to this peer
    uint32_t eventsAcked;
    uint32_t retries;        // extra transmissions
    uint32_t failed;         // gave up
    uint32_t rx;             // events received from this peer (delivered or consumed)
    uint32_t duplicates;     // duplicate events suppressed
    uint32_t rxDropped;      // events refused because the receive queue was full
    uint32_t ackCount;       // ACK latency samples
    uint32_t ackMinUs;
    uint32_t ackMaxUs;
    uint32_t ackAvgUs;
};

struct PingTestResult {
    bool     active;     // test running
    bool     done;       // a test has finished (results below are final)
    bool     completed;  // false if abandoned (timeout)
    uint8_t  to;
    uint16_t requested;
    uint16_t sent;
    uint16_t delivered;
    uint16_t failed;
    uint32_t retries;
    uint32_t minUs;
    uint32_t avgUs;
    uint32_t maxUs;
};

class Core {
public:
    Core(IRadio& radio, IClock& clock, ILock& lock);

    // selfId: this machine (ID_*). seqSeed: starting event sequence number.
    // bootNonce: random per boot (low 16 bits = the frame epoch); lets peers notice a reboot.
    void begin(uint8_t selfId, uint16_t seqSeed, int32_t bootNonce);

    // Drive timers: heartbeat, ACK flush, event (re)sends and give-ups, link test.
    // Call every ~1 ms from the LineLink task.
    void service();

    // Feed one received frame (any thread; copies and returns quickly).
    void onReceive(const uint8_t* mac, const uint8_t* data, int len);

    // Reliable event to a peer. Returns false if dst is invalid or the in-flight
    // table is full. giveUpMs 0 = EVENT_GIVE_UP_MS. outSeq receives the sequence
    // number to poll with eventStatus().
    bool sendEvent(uint8_t dst, uint8_t signal, int32_t value = 0,
                   uint32_t giveUpMs = 0, uint16_t* outSeq = nullptr);
    EventStatus eventStatus(uint8_t dst, uint16_t seq);

    // Next received event for the machine logic (PING never appears here).
    bool popEvent(Event& out);

    // Local heartbeat contents.
    void setLocalFlag(uint8_t mask, bool on);
    void setLocalFlags(uint8_t flags);
    uint8_t localFlags();
    void setLocalState(uint8_t state);

    // Peer queries. A stale or never-heard peer is dead: peerFlag() is false.
    bool     isPeerAlive(uint8_t id);
    bool     peerFlag(uint8_t id, uint8_t mask);   // alive && all bits of mask set
    uint8_t  peerState(uint8_t id);
    uint32_t peerHeartbeatAgeMs(uint8_t id);       // 0xFFFFFFFF if never heard
    uint32_t peerLastHeartbeatMs(uint8_t id);      // clock millis() of last heartbeat
    bool     peerMac(uint8_t id, uint8_t mac[6]);
    bool     getPeerStats(uint8_t id, PeerStats& out);

    // Link test: sends `count` PING events one at a time and measures ACK latency.
    bool startPingTest(uint8_t to, uint16_t count);
    PingTestResult pingTestResult();
    void cancelPingTest();

    // JSON helpers (no heap): {"self":1,"peers":[...]} and the link-test result.
    // Return the length written (always NUL-terminated, truncated at cap-1).
    size_t formatStatusJson(char* buf, size_t cap);
    size_t formatPingResultJson(char* buf, size_t cap);

    uint8_t selfId() const { return self_; }

private:
    struct Dedup {
        bool     valid;
        uint16_t highest;
        uint32_t bitmap;   // bit i = (highest - i) already seen
    };

    struct Peer {
        bool     macKnown;
        bool     addPending;
        uint8_t  mac[6];
        bool     heardEver;
        uint32_t lastHbMs;
        uint8_t  flags;
        uint8_t  state;
        bool     epochValid;
        uint16_t epoch;
        bool     retiredSwitchEpochValid;
        uint16_t retiredSwitchEpoch;
        uint32_t switchControlGapCount;
        bool     hbSeqValid;
        uint16_t lastHbSeq;
        Dedup    dedup;
        uint32_t heartbeats, heartbeatsLost, restarts;
        uint32_t eventsTx, eventsAcked, retries, failed, rx, duplicates, rxDropped;
        uint32_t ackCount, ackMinUs, ackMaxUs;
        uint64_t ackSumUs;
    };

    struct TxEvent {
        bool     used;
        bool     sentOnce;
        uint8_t  dst;
        uint8_t  signal;
        uint16_t seq;
        int32_t  value;
        uint32_t startMs;
        uint32_t lastSendMs;
        uint32_t giveUpMs;
        uint32_t firstSendUs;
        uint32_t retries;
    };

    struct AckItem { uint8_t dst; uint8_t signal; uint16_t seq; };
    struct Result  { uint8_t dst; uint16_t seq; bool ok; bool valid; };

    struct PingState {
        bool     active, done, completed, curActive;
        uint8_t  to;
        uint16_t requested, sent, delivered, failed, curSeq;
        uint32_t retries, minUs, maxUs, startMs;
        uint64_t sumUs;
    };

    static bool dedupIsNew(const Dedup& d, uint16_t seq);
    static void dedupCommit(Dedup& d, uint16_t seq);
    void finalizeTx(TxEvent& t, bool ok, uint32_t rxUs);   // lock held
    void noteEpoch(Peer& p, uint16_t epoch);                // lock held
    void buildFrame(Frame& f, uint8_t type, uint8_t dst, uint16_t seq,
                    uint8_t signal, int32_t value);        // lock held

    IRadio& radio_;
    IClock& clock_;
    ILock&  lock_;

    uint8_t  self_;
    uint16_t epoch_;
    uint8_t  localFlags_;
    uint8_t  localState_;
    uint16_t hbSeq_;
    bool     hbSentOnce_;
    bool     flagsChanged_;   // a local flag changed since the last heartbeat
    uint32_t lastHbSendMs_;
    uint16_t nextSeq_[PEER_SLOTS];

    Peer     peers_[PEER_SLOTS];
    TxEvent  tx_[TX_SLOTS];
    Event    rxQueue_[RX_QUEUE_SIZE];
    uint8_t  rxHead_, rxCount_;
    AckItem  ackQueue_[ACK_QUEUE_SIZE];
    uint8_t  ackHead_, ackCount_;
    Result   results_[RESULT_RING];
    uint8_t  resultNext_;
    PingState ping_;
};

}  // namespace linelink
