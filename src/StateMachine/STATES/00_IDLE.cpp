#include <Arduino.h>
#include "globals.h"
#include "LinkEvents.h"
#include "ConfigApi/MachineSettings.h"

// External reference to state timer
extern unsigned long stateTimer;
extern unsigned long lastCycleEndTime;

// IDLE STATE CONFIG
const int START_SIGNAL_DELAY = 325;              // Delay after start signal before beginning pick cycle (ms)
const unsigned long MIN_CYCLE_INTERVAL_MS = 2000; // Minimum time between consecutive cycles to avoid slamming Stage 2

// Servo signal off while idle: with no pulses the servo stops correcting, so it
// can't buzz/stutter at rest (the gearbox holds the arm). Set false to keep it
// actively holding home.
const bool SERVO_RELAX_AT_IDLE = true;
const unsigned long SERVO_RELAX_DELAY_MS = 1500;  // Time in IDLE before the signal goes off, so the servo finishes its move home (ms)

// X driver: enabled (holding) at all times, off only after this long in IDLE.
const unsigned long X_IDLE_DISABLE_DELAY_MS = 120000;  // 2 minutes

static bool xIdleDisabled = false;

// IDLE STATE
// This state waits for a manual Start hold or a Stage 1 pickup request over LineLink.
// When triggered, waits for START_SIGNAL_DELAY before transitioning to pickup sequence
// Also enforces a minimum interval between consecutive cycles

bool handleIdleState() {
  static bool timingManualHold = false;
  static unsigned long manualHoldStarted = 0;
  const unsigned long now = millis();

  // Start no new cycle until the existing cooldown has elapsed.
  if (lastCycleEndTime != 0 && now - lastCycleEndTime < MIN_CYCLE_INTERVAL_MS) {
    timingManualHold = false;
    stateTimer = 0;
    return false;
  }

  if (!startButton.unusedPress()) {
    timingManualHold = false;
  } else if (!timingManualHold) {
    timingManualHold = true;
    manualHoldStarted = now;
  }

  const bool automaticRequest = pickupRequestActive();
  const bool automaticReady = automaticRequest && waitForTime(START_SIGNAL_DELAY);
  if (!automaticRequest) stateTimer = 0;
  const bool manualReady = timingManualHold && now - manualHoldStarted >= START_SIGNAL_DELAY;

  if (!automaticReady && !manualReady) return false;

  timingManualHold = false;
  stateTimer = 0;
  startButton.consumePress();
  clearPickupRequest();
  return true;
}

// Called at the end of every loop: turns the X driver off once IDLE has lasted
// X_IDLE_DISABLE_DELAY_MS (and X has stopped), and back on in the same loop pass
// that leaves IDLE, before any other state handler can command an X move.
void updateXIdleDisable() {
  static bool idleTiming = false;
  static unsigned long idleSinceMs = 0;
  if (!xStepper) return;

  if (systemState != STATE_IDLE) {
    if (xIdleDisabled) {
      xStepper->enableOutputs();
      xIdleDisabled = false;
    }
    idleTiming = false;
    return;
  }

  if (xIdleDisabled) return;
  if (!idleTiming) {
    idleTiming = true;
    idleSinceMs = millis();
    return;
  }
  if (millis() - idleSinceMs >= X_IDLE_DISABLE_DELAY_MS && !xStepper->isRunning()) {
    xStepper->disableOutputs();
    xIdleDisabled = true;
  }
}

bool isXMotorEnabled() {
  return xStepper && !xIdleDisabled;
}

// Called at the end of every loop: turns the servo signal off after
// SERVO_RELAX_DELAY_MS in IDLE, and back on (home) in the same loop pass that
// leaves IDLE, before any other state handler runs.
void updateServoIdleRelax() {
  static bool relaxed = false;
  static unsigned long idleSince = 0;

  if (systemState != STATE_IDLE) {
    if (relaxed) {
      swivelArmServo.write(SERVO_HOME_POS);  // IDLE always rests at home
      relaxed = false;
    }
    idleSince = 0;
    return;
  }

  if (!SERVO_RELAX_AT_IDLE || relaxed) return;
  if (idleSince == 0) {
    idleSince = millis();
    return;
  }
  if (millis() - idleSince >= SERVO_RELAX_DELAY_MS) {
    swivelArmServo.writeMicroseconds(0);  // duty 0: line held LOW, no pulses
    relaxed = true;
  }
}