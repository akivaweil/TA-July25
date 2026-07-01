#include <Arduino.h>
#include <FastAccelStepper.h>
#include "globals.h"
#include "Config/Pins_Definitions.h"
#include "ConfigApi/MachineSettings.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;

// HOMING STATE CONFIG
// Position settings (inches) - non-curated derived-position input (extern so
// applyTASettings() can recompute the derived steps).
extern const float Z_HOME_OFFSET_INCHES = 0.3;      // Move Z away from home after homing

// Homing speeds (steps/sec)
extern const int X_HOME_SPEED = 800;    // X homing speed (shared with end-of-cycle X homing)
extern const int Z_HOME_SPEED = 450;    // Z homing speed (single definition; 04_DROPOFF references via extern)

// Distance commanded for an open-ended home-switch seek (steps). Large enough to
// always reach the switch; the move is force-stopped the moment the switch trips.
const long HOMING_SEEK_DISTANCE_STEPS = -50000;

// Homing fault timeouts (ms). If a home switch is not found within these windows
// the axis is NOT zeroed (a false origin could drive an axis into a hard stop);
// homing instead holds in a fault until the board is reset.
const unsigned long Z_HOMING_TIMEOUT_MS = 5000;   // normal Z seek reaches home well within this
const unsigned long X_HOMING_TIMEOUT_MS = 75000;  // generous: a full slow X seek can take ~60s

// External references from Config.cpp
extern float STEPS_PER_INCH;
extern int X_MAX_SPEED;
extern int Z_MAX_SPEED;
extern int X_HOME_POS;

// Calculated positions (steps) - maintained by applyTASettings()
int Z_HOME_POS = 0;
// X pickup step target, maintained by applyTASettings().
extern int X_PICKUP_POS;

// HOMING STATE
// This state homes both Z and X axes sequentially
// Z axis homes first, then moves up, then X axis homes and moves to pickup

bool handleHomingState() {
  static int homingStep = 0;
  static unsigned long zHomingStartTime = 0;
  static unsigned long xHomingStartTime = 0;
  static bool homingFault = false;

  // A homing fault means a home switch was never found within its timeout. Hold
  // here with motors stopped and DO NOT establish an origin: running the cycle
  // off a false datum could drive an axis into a hard stop. Recovery = reset.
  if (homingFault) {
    return false;
  }

  switch(homingStep) {
    case 0:  // Start Z homing
      // Fail-safe: force vacuum and the Stage 2 signal to their OFF/safe state at
      // the start of homing (not just at boot), so a re-home from any state can
      // never leave the gripper holding or Stage 2 asserted.
      digitalWrite(SOLENOID_RELAY_PIN, LOW);  // Vacuum off
      digitalWrite(STAGE2_SIGNAL_PIN, LOW);   // Stage 2 signal off
      vacuumActive = false;

      if (zStepper) {
        zStepper->setSpeedInHz(Z_HOME_SPEED);
        zStepper->move(HOMING_SEEK_DISTANCE_STEPS);  // Move negative direction
        zHomingStartTime = millis();  // Record start time for timeout
      }
      homingStep = 1;
      break;

    case 1:  // Wait for Z home switch (fault on timeout instead of false-zeroing)
      if (zHomeSwitch.read() == HIGH) {
        if (zStepper) {
          zStepper->forceStop();
          zStepper->setCurrentPosition(0);  // Set physical home as position 0
          zStepper->setSpeedInHz(Z_MAX_SPEED);
          zStepper->moveTo(Z_HOME_POS);  // Move to offset position (1.0" from physical home)
        }
        homingStep = 2;
      } else if ((millis() - zHomingStartTime) >= Z_HOMING_TIMEOUT_MS) {
        // Switch never seen — stop and fault rather than zeroing a false origin.
        if (zStepper) zStepper->forceStop();
        homingFault = true;
        Serial.println("[TA] FAULT: Z home switch not found");
      }
      break;

    case 2:  // Wait for Z to reach up position
      if (isMotorAtTarget(zStepper)) {
        if (xStepper) {
          xStepper->setSpeedInHz(X_HOME_SPEED);
          xStepper->move(HOMING_SEEK_DISTANCE_STEPS);  // Move negative direction
          xHomingStartTime = millis();  // Record start time for timeout
        }
        homingStep = 3;
      }
      break;

    case 3:  // Wait for X home switch (fault on timeout instead of hanging forever)
      if (xHomeSwitch.read() == HIGH) {
        if (xStepper) {
          xStepper->forceStop();
          xStepper->setCurrentPosition(X_HOME_POS);
          xStepper->setSpeedInHz(X_MAX_SPEED);
          xStepper->moveTo(X_PICKUP_POS);  // Move to pickup
        }
        homingStep = 4;
      } else if ((millis() - xHomingStartTime) >= X_HOMING_TIMEOUT_MS) {
        if (xStepper) xStepper->forceStop();
        homingFault = true;
        Serial.println("[TA] FAULT: X home switch not found");
      }
      break;

    case 4:  // Wait for X to reach pickup
      if (isMotorAtTarget(xStepper)) {
        homingStep = 0;   // Reset for next homing
        return true;      // Homing complete
      }
      break;
  }

  return false;  // Homing not complete
} 