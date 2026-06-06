#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"
#include "ConfigApi/MachineSettings.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern unsigned long stateTimer;
extern Bounce stopSignalStage2;
extern Bounce zHomeSwitch;
extern ServoControl swivelArmServo;
extern float STEPS_PER_INCH;

// DROPOFF STATE CONFIG
// Curated settings (Z_DROPOFF_LOWER_INCHES, DROPOFF_SETTLE_TIME, SERVO_HOME_POS,
// Z_DROPOFF_SPEED) are now mutable globals owned by MachineSettings.cpp.

// Non-curated derived-position input (extern const so applyTASettings() can use it)
extern const float Z_EARLY_RETURN_INCHES = 2.0;      // Z distance to travel up before starting X return home

// Timing settings (ms)
const int DROPOFF_HOLD_TIME = 10;     // Hold time at dropoff position

// Speed settings
const int Z_HOME_SPEED = 450;         // Z homing speed during X return

// Calculated positions (steps) - maintained by applyTASettings()
int Z_DROPOFF_POS = 0;
int Z_EARLY_RETURN_POS = 0;

// External positions from other states
extern int Z_HOME_POS;
extern int Z_MAX_SPEED;
extern int X_HOME_POS;
extern int X_MAX_SPEED;
extern const int X_HOME_SPEED;

// X pickup position from pickup state (steps)
extern int X_PICKUP_POS;

// DROPOFF STATE
// This state handles dropping off the object:
// Check safety signal, lower Z, release vacuum, wait, raise Z with early X return

bool handleDropoffState() {
  static bool zHomingOffsetPhase = false;
  static unsigned long zHomingStartTime = 0;

  switch(dropoffState) {
    case DROPOFF_LOWER_Z:
      // Check safety signal before lowering (using debounced input)
      if (stopSignalStage2.read() == HIGH) {
        return false;  // Wait for safety signal to go low
      }
      
      if (zStepper) {
        // Set target position for dropoff
        zStepper->setSpeedInHz(Z_DROPOFF_SPEED);  // Slower for dropoff
        zStepper->moveTo(Z_DROPOFF_POS);
      }
      
      // Wait for Z to fully reach dropoff position before proceeding
      if (isMotorAtTarget(zStepper)) {
        // Additional verification: ensure we're actually at the dropoff position
        if (zStepper && abs(zStepper->getCurrentPosition() - Z_DROPOFF_POS) <= 10) {
          deactivateVacuum();
          dropoffState = DROPOFF_VACUUM_DELAY;
        }
      }
      break;
      
    case DROPOFF_VACUUM_DELAY:
      if (waitForTime(50)) {
        dropoffState = DROPOFF_RELEASE;
      }
      break;
      
    case DROPOFF_RELEASE:
      dropoffState = DROPOFF_WAIT;
      break;
      
    case DROPOFF_WAIT:
      if (waitForTime(DROPOFF_HOLD_TIME)) {
        dropoffState = DROPOFF_SETTLE;
      }
      break;
      
    case DROPOFF_SETTLE:
      if (waitForTime(DROPOFF_SETTLE_TIME)) {
        if (zStepper) {
          zStepper->setSpeedInHz(Z_MAX_SPEED);  // Back to normal speed
          zStepper->moveTo(Z_HOME_POS);  // Move up toward home area
        }
        dropoffState = DROPOFF_RAISE_Z;
      }
      break;
      
    case DROPOFF_RAISE_Z:
      // Z moved up enough - start X return AND Z homing in parallel (X does NOT home)
      if (zStepper && zStepper->getCurrentPosition() <= Z_EARLY_RETURN_POS) {
        swivelArmServo.write(SERVO_HOME_POS);
        digitalWrite(STAGE2_SIGNAL_PIN, HIGH);
        // Start X moving back to the same pickup position (no X homing)
        if (xStepper) {
          xStepper->moveTo(X_PICKUP_POS);
        }
        // Start Z homing (run to switch) during X move
        if (zStepper) {
          zStepper->forceStop();
          zStepper->setSpeedInHz(Z_HOME_SPEED);
          zStepper->move(-50000);
        }
        zHomingStartTime = millis();
        zHomingOffsetPhase = false;
        dropoffState = DROPOFF_EARLY_RETURN;
      }
      break;
      
    case DROPOFF_EARLY_RETURN:
      // Z homing: when switch hits, set pos 0 and move to offset; wait for both X and Z
      if (!zHomingOffsetPhase) {
        if (zHomeSwitch.read() == HIGH || (millis() - zHomingStartTime) >= 5000) {
          if (zStepper) {
            zStepper->forceStop();
            zStepper->setCurrentPosition(0);
            zStepper->setSpeedInHz(Z_MAX_SPEED);
            zStepper->moveTo(Z_HOME_POS);
          }
          zHomingOffsetPhase = true;
        }
      } else if (isMotorAtTarget(zStepper) && isMotorAtTarget(xStepper)) {
        zHomingOffsetPhase = false;
        dropoffState = DROPOFF_X_AT_PICKUP;
      }
      break;

    case DROPOFF_X_AT_PICKUP:
      // X reached pickup return position — now re-home X to correct any drift
      if (isMotorAtTarget(xStepper)) {
        digitalWrite(STAGE2_SIGNAL_PIN, LOW);  // Turn off Stage 2 signal
        if (xStepper) {
          xStepper->setSpeedInHz(X_HOME_SPEED);
          xStepper->move(-50000);  // Move negative toward home switch
        }
        dropoffState = DROPOFF_X_HOME;
      }
      break;

    case DROPOFF_X_HOME:
      // Wait for X home switch, then zero position and move back to pickup
      if (digitalRead(X_HOME_SWITCH_PIN) == HIGH) {
        if (xStepper) {
          xStepper->forceStop();
          xStepper->setCurrentPosition(X_HOME_POS);
          xStepper->setSpeedInHz(X_MAX_SPEED);
          xStepper->moveTo(X_PICKUP_POS);
        }
        dropoffState = DROPOFF_X_RETURN_PICKUP;
      }
      break;

    case DROPOFF_X_RETURN_PICKUP:
      if (isMotorAtTarget(xStepper)) {
        dropoffState = DROPOFF_DONE;
      }
      break;

    case DROPOFF_DONE:
      dropoffState = DROPOFF_LOWER_Z;  // Reset for next cycle
      return true;  // Dropoff complete
  }
  
  return false;  // Dropoff not complete
} 