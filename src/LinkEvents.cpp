#include "LinkEvents.h"

#include <Arduino.h>

#include "LineLink.h"
#include "globals.h"

namespace {
// A START_PICKUP request stays valid this long. Stage 1's start wire stayed HIGH
// for 5 s (TA_SIGNAL_DURATION default), so a request that arrives while the TA is
// still finishing the previous cycle is honoured when it gets back to IDLE.
const unsigned long PICKUP_REQUEST_HOLD_MS = 5000;

bool pickupPending = false;
unsigned long pickupRequestTime = 0;
}  // namespace

void onStage1SuctionFailure();   // 05_SUCTION_FAILURE_RETURN.cpp

void setupLineLink() {
    linelink::begin(linelink::ID_TA);  // full TX power (lowering it didn't fix the servo stutter)
}

void updateLineLink() {
    linelink::Core& link = linelink::link();

    linelink::Event ev;
    while (link.popEvent(ev)) {
        if (ev.src != linelink::ID_STAGE1) continue;
        if (ev.signal == linelink::SIG_START_PICKUP) {
            pickupPending = true;
            pickupRequestTime = millis();
        } else if (ev.signal == linelink::SIG_SUCTION_FAILURE) {
            // Stage 1 has stopped (latched error): drop any queued pickup so the
            // TA doesn't start another run, then recover a cycle in progress.
            pickupPending = false;
            onStage1SuctionFailure();
        }
    }
    if (pickupPending && millis() - pickupRequestTime > PICKUP_REQUEST_HOLD_MS) {
        pickupPending = false;
    }

    link.setLocalState((uint8_t)systemState);

    // Flash interlock: idle = IDLE with no Stage 1 pickup queued, or HOMING.
    linelink::flashGate().update(
        (systemState == STATE_IDLE && !pickupPending) || systemState == STATE_HOMING, millis());
}

bool pickupRequestActive() {
    return pickupPending;
}

void clearPickupRequest() {
    pickupPending = false;
}

void sendStage2Start() {
    linelink::link().sendEvent(linelink::ID_STAGE2, linelink::SIG_STAGE2_START);
}
