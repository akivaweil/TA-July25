#include <Arduino.h>
#include <FastAccelStepper.h>
#include "globals.h"
#include "ConfigApi/MachineSettings.h"

// Stage 1 reports a missing suction confirmation with a SUCTION_FAILURE LineLink
// event (no retry). It is an explicit recovery command and never a pickup trigger:
// the TA eases X and Z to a stop at once (normal deceleration, no abrupt halt),
// releases the vacuum, turns the servo home and then re-homes (Z up first, then X).
const unsigned long SUCTION_FAILURE_VACUUM_RELEASE_SETTLE_MS = 50;

extern unsigned long stateTimer;
extern int X_MAX_SPEED;
extern int Z_MAX_SPEED;

namespace {
enum class SuctionFailureReturnPhase {
  Idle,
  GentleStop,
  VacuumReleaseSettle,
};

SuctionFailureReturnPhase returnPhase = SuctionFailureReturnPhase::Idle;
unsigned long vacuumReleaseTime = 0;

void beginSuctionFailureReturn() {
  // Decelerated stops (configured acceleration), wherever the axes are.
  if (xStepper && xStepper->isRunning()) {
    xStepper->stopMove();
  }
  if (zStepper && zStepper->isRunning()) {
    zStepper->stopMove();
  }

  stateTimer = 0;
  returnPhase = SuctionFailureReturnPhase::GentleStop;
  systemState = STATE_SUCTION_FAILURE_RETURN;
  Serial.println("[TA] Stage 1 suction failure: easing to a stop and returning home");
}
}

// Called from the LineLink event handler when Stage 1 sends SUCTION_FAILURE.
void onStage1SuctionFailure() {
  const bool cycleCanRecover =
      systemState == STATE_PICKUP ||
      systemState == STATE_TRANSPORT ||
      systemState == STATE_DROPOFF;
  if (cycleCanRecover) {
    beginSuctionFailureReturn();   // right away, also mid-pickup (homing lifts Z before X moves)
  }
}

bool handleSuctionFailureReturnState() {
  switch (returnPhase) {
    case SuctionFailureReturnPhase::Idle:
      returnPhase = SuctionFailureReturnPhase::GentleStop;
      break;

    case SuctionFailureReturnPhase::GentleStop: {
      const bool xStopped = !xStepper || !xStepper->isRunning();
      const bool zStopped = !zStepper || !zStepper->isRunning();
      if (xStopped && zStopped) {
        deactivateVacuum();
        swivelArmServo.write(SERVO_HOME_POS);
        vacuumReleaseTime = millis();
        returnPhase = SuctionFailureReturnPhase::VacuumReleaseSettle;
      }
      break;
    }

    case SuctionFailureReturnPhase::VacuumReleaseSettle:
      if (millis() - vacuumReleaseTime >= SUCTION_FAILURE_VACUUM_RELEASE_SETTLE_MS) {
        if (xStepper) xStepper->setSpeedInHz(X_MAX_SPEED);
        if (zStepper) zStepper->setSpeedInHz(Z_MAX_SPEED);
        returnPhase = SuctionFailureReturnPhase::Idle;
        return true;
      }
      break;
  }

  return false;
}
