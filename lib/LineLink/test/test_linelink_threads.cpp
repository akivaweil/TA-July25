// Concurrency stress test for the LineLink core: the receive callback, service()
// and the application API run on different threads, exactly like the WiFi task,
// the LineLink task and the machine loop on the ESP32. Build with
// -fsanitize=thread (see run_tests.sh) so any unlocked shared access is reported.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "LineLinkCore.h"

using namespace linelink;

namespace {

const int TEST_DURATION_MS = 2500;
const int EVENTS_PER_SENDER = 150;
const double LOSS = 0.2;

class MutexLock : public ILock {
public:
    void lock() { m_.lock(); }
    void unlock() { m_.unlock(); }
private:
    std::mutex m_;
};

class RealClock : public IClock {
public:
    RealClock() : start_(std::chrono::steady_clock::now()) {}
    uint32_t millis() { return (uint32_t)us() / 1000u; }
    uint32_t micros() { return (uint32_t)us(); }
private:
    uint64_t us() {
        return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now() - start_).count();
    }
    std::chrono::steady_clock::time_point start_;
};

struct Packet { int dst; uint8_t mac[6]; std::vector<uint8_t> data; };

class Air {
public:
    Air() : stop(false), rng(12345) {}
    void push(int dst, const uint8_t mac[6], const uint8_t* d, size_t n) {
        std::lock_guard<std::mutex> g(m);
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;   // xorshift32 (an LCG's low bits are far too regular)
        if ((rng >> 8) % 1000 < LOSS * 1000) return;
        Packet p; p.dst = dst; memcpy(p.mac, mac, 6); p.data.assign(d, d + n);
        q.push_back(p);
        cv.notify_one();
    }
    bool pop(Packet& p) {
        std::unique_lock<std::mutex> g(m);
        cv.wait_for(g, std::chrono::milliseconds(2), [&] { return !q.empty() || stop; });
        if (q.empty()) return false;
        p = q.front(); q.pop_front();
        return true;
    }
    std::mutex m; std::condition_variable cv; std::deque<Packet> q; std::atomic<bool> stop; uint32_t rng;
};

struct Machine : public IRadio {
    Machine(Air& a, int idx, uint8_t id_) : air(a), index(idx), id(id_), core(*this, clock, lock) {
        memset(mac, 0, sizeof(mac)); mac[5] = id_;
    }
    bool addPeer(const uint8_t*) { return true; }
    bool sendBroadcast(const uint8_t* d, size_t n) { air.push(1 - index, mac, d, n); return true; }
    bool sendUnicast(const uint8_t*, const uint8_t* d, size_t n) { air.push(1 - index, mac, d, n); return true; }
    Air& air; int index; uint8_t id; uint8_t mac[6];
    RealClock clock; MutexLock lock; Core core;
    std::mutex gotM; std::vector<int32_t> got;
};

}  // namespace

int main() {
    Air air;
    Machine a(air, 0, ID_STAGE2), b(air, 1, ID_ROUTER);
    Machine* ms[2] = {&a, &b};
    a.core.begin(a.id, 40000, 1);
    b.core.begin(b.id, 65500, 2);
    std::atomic<bool> stop(false);
    std::vector<std::thread> threads;

    // "WiFi task": delivers packets to the destination's onReceive.
    threads.push_back(std::thread([&] {
        Packet p;
        while (!stop) if (air.pop(p)) ms[p.dst]->core.onReceive(p.mac, p.data.data(), (int)p.data.size());
    }));
    // "LineLink task" per machine.
    for (int i = 0; i < 2; ++i) threads.push_back(std::thread([&, i] {
        while (!stop) { ms[i]->core.service(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    }));
    // "Machine loop" per machine: sends events, drains events, flips flags, reads peers.
    std::atomic<int> sendFailures(0);
    for (int i = 0; i < 2; ++i) threads.push_back(std::thread([&, i] {
        Machine& me = *ms[i];
        const uint8_t peerId = ms[1 - i]->id;
        int sent = 0;
        int tick = 0;
        while (!stop) {
            if (sent < EVENTS_PER_SENDER && me.core.sendEvent(peerId, SIG_START_PICKUP, sent)) ++sent;
            Event e;
            while (me.core.popEvent(e)) { std::lock_guard<std::mutex> g(me.gotM); me.got.push_back(e.value); }
            me.core.setLocalFlag(FLAG_READY, (tick++ / 50) % 2 == 0);
            me.core.setLocalState((uint8_t)(tick & 0x7F));
            (void)me.core.peerFlag(peerId, FLAG_READY);
            (void)me.core.isPeerAlive(peerId);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        (void)sendFailures;
    }));
    // "HTTP handler": status JSON + link test while traffic flows.
    threads.push_back(std::thread([&] {
        char buf[LINK_STATUS_JSON_CAP];
        bool started = false;
        while (!stop) {
            a.core.formatStatusJson(buf, sizeof(buf));
            b.core.formatStatusJson(buf, sizeof(buf));
            PeerStats s; a.core.getPeerStats(ID_ROUTER, s);
            if (!started) started = a.core.startPingTest(ID_ROUTER, 30);
            a.core.pingTestResult();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }));

    std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DURATION_MS));
    stop = true; air.stop = true;
    for (auto& t : threads) t.join();

    int bad = 0;
    for (int i = 0; i < 2; ++i) {
        // Events sent by the OTHER machine arrive in ms[i].got: each value exactly once.
        std::multiset<int32_t> seen(ms[i]->got.begin(), ms[i]->got.end());
        for (int v = 0; v < EVENTS_PER_SENDER; ++v) {
            if (seen.count(v) != 1) { ++bad; printf("  machine %d: value %d seen %zu times\n", i, v, seen.count(v)); if (bad > 10) break; }
        }
        PeerStats s; ms[1 - i]->core.getPeerStats(ms[i]->id, s);
        printf("machine %d -> %d: tx=%u acked=%u retries=%u failed=%u | rx at peer=%zu\n", 1 - i, i,
               s.eventsTx, s.eventsAcked, s.retries, s.failed, ms[i]->got.size());
    }
    PingTestResult r = a.core.pingTestResult();
    printf("ping: sent=%u delivered=%u failed=%u done=%d\n", r.sent, r.delivered, r.failed, (int)r.done);
    if (bad == 0 && r.delivered + r.failed == r.sent) { printf("THREAD TEST PASSED\n"); return 0; }
    printf("THREAD TEST FAILED\n");
    return 1;
}
