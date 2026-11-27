#pragma once

// Flash interlock. The machines work in sequence and hand the wood on:
// Stage 1 -> TA -> Stage 2 -> Router (LINE_ORDER). Uploading to one machine:
//   1. request(): the target publishes FLAG_FLASH_REQUEST. Stage 1 sees it and
//      starts no new cycle after its current one (flashPending()).
//   2. The pieces already in the line keep moving. The upload may start once the
//      target and every machine BEFORE it in the line are idle, without a break,
//      for FLASH_IDLE_SETTLE_MS. Machines after the target finish their cycles.
//   3. During the upload the target publishes FLAG_FLASHING; Stage 1 keeps
//      holding until it is back from its reboot.
// Without a request an upload is still allowed when the whole line has been idle
// for FLASH_IDLE_SETTLE_MS. A request expires after FLASH_REQUEST_TIMEOUT_MS.
// Peers that are off / silent don't block (they can't be mid-cycle); peers
// without FLAG_FLASH_GATE run older firmware and are ignored.
//
// update() is cheap to call every loop: it re-evaluates at most once per
// FLASH_GATE_EVAL_PERIOD_MS, so hot loops don't hammer the Core's spinlock.
// request() / cancel() may be called from another task (web server).

#include <stdio.h>

#include "LineLinkCore.h"

namespace linelink {

const uint32_t FLASH_IDLE_SETTLE_MS      = 2000;     // idle this long (no break) before an upload may start
const uint32_t FLASH_GATE_EVAL_PERIOD_MS = 20;       // re-evaluate the gate at most this often
const uint32_t FLASH_REQUEST_TIMEOUT_MS  = 900000;   // 15 min: a request with no upload expires (Stage 1 resumes)
const uint32_t FLASH_REBOOT_HOLD_MS      = 60000;    // Stage 1 keeps holding at most this long while a flashed machine reboots

// Machines in the order the wood passes through them.
const uint8_t LINE_ORDER[MAX_ID] = {ID_STAGE1, ID_TA, ID_STAGE2, ID_ROUTER};

inline uint8_t linePosition(uint8_t id) {
    for (uint8_t i = 0; i < MAX_ID; ++i) if (LINE_ORDER[i] == id) return i;
    return MAX_ID;
}

class FlashGate {
public:
    explicit FlashGate(Core& core)
        : core_(core), evaluatedOnce_(false), lastEvalMs_(0), publishedIdle_(false),
          publishedRequest_(false), requestAsked_(false), cancelAsked_(false), requestAskedMs_(0),
          requestPending_(false), requestSinceMs_(0), lineIdle_(false), lineIdleSinceMs_(0),
          targetReady_(false), targetReadySinceMs_(0), allowed_(false), flashing_(false),
          flashPending_(false), waitingFor_(ID_NONE), rebootPeer_(ID_NONE), rebootSeenMs_(0) {}

    // Call every machine loop. selfIdle: this machine is idle (or in a state the
    // line treats as idle, e.g. homing / reload) with no start pending.
    void update(bool selfIdle, uint32_t nowMs) {
        if (evaluatedOnce_ && (nowMs - lastEvalMs_) < FLASH_GATE_EVAL_PERIOD_MS) return;
        if (!evaluatedOnce_) core_.setLocalFlag(FLAG_FLASH_GATE, true);
        evaluatedOnce_ = true;
        lastEvalMs_ = nowMs;

        // Request bookkeeping (request()/cancel() only raise flags).
        if (cancelAsked_) { cancelAsked_ = false; requestAsked_ = false; requestPending_ = false; }
        if (requestAsked_) { requestAsked_ = false; requestPending_ = true; requestSinceMs_ = requestAskedMs_; }
        if (requestPending_ && !flashing_ && (nowMs - requestSinceMs_) >= FLASH_REQUEST_TIMEOUT_MS) {
            requestPending_ = false;
        }
        publish(FLAG_FLASH_REQUEST, requestPending_, publishedRequest_);

        if (flashing_) selfIdle = false;
        publish(FLAG_IDLE, selfIdle, publishedIdle_);

        // Scan the line.
        const uint8_t selfPos = linePosition(core_.selfId());
        bool lineIdle = selfIdle;
        bool upstreamIdle = true;
        uint8_t firstBusyUpstream = ID_NONE;
        bool peerFlashing = false;
        bool peerRequest = false;
        for (uint8_t i = 0; i < MAX_ID; ++i) {
            const uint8_t id = LINE_ORDER[i];
            if (id == core_.selfId() || !core_.isPeerAlive(id)) continue;
            if (core_.peerFlag(id, FLAG_FLASHING)) {
                peerFlashing = true;
                rebootPeer_ = id;
                rebootSeenMs_ = nowMs;
            }
            if (core_.peerFlag(id, FLAG_FLASH_REQUEST)) peerRequest = true;
            const bool busy = core_.peerFlag(id, FLAG_FLASH_GATE) && !core_.peerFlag(id, FLAG_IDLE);
            if (busy) {
                lineIdle = false;
                if (i < selfPos) {
                    upstreamIdle = false;
                    if (firstBusyUpstream == ID_NONE) firstBusyUpstream = id;
                }
            }
        }

        // A machine that was flashing keeps Stage 1 holding while it reboots
        // (silent), until it is heard again or FLASH_REBOOT_HOLD_MS passes.
        bool rebootHold = false;
        if (rebootPeer_ != ID_NONE) {
            const bool back = core_.isPeerAlive(rebootPeer_) && !core_.peerFlag(rebootPeer_, FLAG_FLASHING);
            if (back || (nowMs - rebootSeenMs_) >= FLASH_REBOOT_HOLD_MS) rebootPeer_ = ID_NONE;
            else rebootHold = true;
        }

        flashPending_ = requestPending_ || flashing_ || peerFlashing || peerRequest || rebootHold;

        // Who a pending request is waiting for (status / upload tool).
        if (!requestPending_)  waitingFor_ = ID_NONE;
        else if (!selfIdle)    waitingFor_ = core_.selfId();
        else                   waitingFor_ = firstBusyUpstream;

        const bool lineOk   = lineIdle && !peerFlashing;
        const bool targetOk = requestPending_ && selfIdle && upstreamIdle && !peerFlashing;
        settle(lineOk, lineIdle_, lineIdleSinceMs_, nowMs);
        settle(targetOk, targetReady_, targetReadySinceMs_, nowMs);
        allowed_ = (lineIdle_ && (nowMs - lineIdleSinceMs_) >= FLASH_IDLE_SETTLE_MS) ||
                   (targetReady_ && (nowMs - targetReadySinceMs_) >= FLASH_IDLE_SETTLE_MS);
    }

    // Ask for an upload slot (any task). Takes effect at the next update().
    void request(uint32_t nowMs) {
        requestAskedMs_ = nowMs;
        requestAsked_ = true;
    }
    void cancel() { cancelAsked_ = true; }

    // An upload may start now (result of the last evaluation).
    bool allowed() const { return allowed_; }
    bool requested() const { return requestPending_ || requestAsked_; }
    bool flashing() const { return flashing_; }

    // Machine a pending request is waiting on (this one or one before it in the
    // line); ID_NONE when nothing is busy (ready, or settling).
    uint8_t waitingFor() const { return waitingFor_; }

    // OTA start (true) / error (false). A successful upload reboots, which clears
    // everything; an error also drops the request so Stage 1 resumes.
    void setFlashing(bool on) {
        flashing_ = on;
        core_.setLocalFlag(FLAG_FLASHING, on);
        if (on) {
            core_.setLocalFlag(FLAG_IDLE, false);
            publishedIdle_ = false;
            lineIdle_ = targetReady_ = allowed_ = false;
            flashPending_ = true;
        } else {
            cancelAsked_ = true;
        }
    }

    // A flash is requested, running, or a flashed machine is rebooting, here or
    // anywhere on the line: Stage 1 must not start a new cycle.
    bool flashPending() const { return flashPending_ || flashing_ || requestAsked_; }

private:
    void publish(uint8_t flag, bool on, bool& published) {
        if (on != published) {
            core_.setLocalFlag(flag, on);
            published = on;
        }
    }

    static void settle(bool ok, bool& active, uint32_t& sinceMs, uint32_t nowMs) {
        if (!ok) { active = false; return; }
        if (!active) { active = true; sinceMs = nowMs; }
    }

    Core&             core_;
    bool              evaluatedOnce_;
    uint32_t          lastEvalMs_;
    bool              publishedIdle_;
    bool              publishedRequest_;
    volatile bool     requestAsked_;
    volatile bool     cancelAsked_;
    volatile uint32_t requestAskedMs_;
    bool              requestPending_;
    uint32_t          requestSinceMs_;
    bool              lineIdle_;
    uint32_t          lineIdleSinceMs_;
    bool              targetReady_;
    uint32_t          targetReadySinceMs_;
    bool              allowed_;
    bool              flashing_;
    bool              flashPending_;
    uint8_t           waitingFor_;
    uint8_t           rebootPeer_;
    uint32_t          rebootSeenMs_;
};

// /api/flash body: {"machine":"ta","requested":true,"ready":false,"flashing":false,
// "waitingFor":"stage1"} (waitingFor null when nothing is busy).
const size_t FLASH_STATUS_BODY_CAP = 160;

inline size_t formatFlashStatusJson(const FlashGate& gate, uint8_t selfId, char* buf, size_t cap) {
    const uint8_t w = gate.waitingFor();
    char waiting[24];
    if (w == ID_NONE) snprintf(waiting, sizeof(waiting), "null");
    else snprintf(waiting, sizeof(waiting), "\"%s\"", machineName(w));
    const int n = snprintf(buf, cap,
        "{\"machine\":\"%s\",\"requested\":%s,\"ready\":%s,\"flashing\":%s,\"waitingFor\":%s}",
        machineName(selfId), gate.requested() ? "true" : "false", gate.allowed() ? "true" : "false",
        gate.flashing() ? "true" : "false", waiting);
    return n < 0 ? 0 : ((size_t)n < cap ? (size_t)n : cap - 1);
}

}  // namespace linelink
