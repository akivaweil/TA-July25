#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern unsigned long stateTimer;
extern Bounce stopSignalStage2;
extern Bounce zHomeSwitch;
extern ServoControl swivelArmServo;
extern float STEPS_PER_INCH;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 📤 DROPOFF STATE CONFIG                                                ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Position settings (inches)
extern const float Z_DROPOFF_LOWER_INCHES = 6.35;    // Lower Z for dropoff (shared with transport)
const float Z_EARLY_RETURN_INCHES = 2.0;             // Z distance to travel up before starting X return home

// Timing settings (ms)
const int DROPOFF_HOLD_TIME = 10;     // Hold time at dropoff position
const int DROPOFF_SETTLE_TIME = 25;   // Settle time before raising Z

// Speed settings
extern const int Z_DROPOFF_SPEED = 15000;    // Z speed for dropoff (steps/sec, shared with transport)
const int Z_HOME_SPEED = 450;         // Z homing speed during X return

// Calculated positions (steps) - initialized at runtime
int Z_DROPOFF_POS = 0;
int Z_EARLY_RETURN_POS = 0;
static bool dropoffConfigInitialized = false;

// External positions from other states
extern int Z_HOME_POS;
extern int Z_MAX_SPEED;

// X pickup position from pickup state (steps)
extern int X_PICKUP_POS;

// Servo neutral/home position (degrees)
int SERVO_HOME_POS = 52;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 📤 DROPOFF STATE                                                       ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state handles dropping off the object:
// Check safety signal, lower Z, release vacuum, wait, raise Z with early X return

bool handleDropoff() {
  static bool zHomingOffsetPhase = false;
  static unsigned long zHomingStartTime = 0;

  // Initialize calculated positions on first call
  if (!dropoffConfigInitialized) {
    Z_DROPOFF_POS = (int)(Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH);
    Z_EARLY_RETURN_POS = (int)(Z_EARLY_RETURN_INCHES * STEPS_PER_INCH);
    dropoffConfigInitialized = true;
  }
  
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
      //! ************************************************************************
      //! Z moved up enough - start X return AND Z homing in parallel (X does NOT home)
      //! ************************************************************************
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
      //! ************************************************************************
      //! Z homing: when switch hits, set pos 0 and move to offset; wait for both X and Z
      //! ************************************************************************
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
        dropoffState = DROPOFF_DONE;
      }
      break;
      
    case DROPOFF_DONE:
      // Wait for X to reach return home position
      if (isMotorAtTarget(xStepper)) {
        digitalWrite(STAGE2_SIGNAL_PIN, LOW);  // Turn off Stage 2 signal
        dropoffState = DROPOFF_LOWER_Z;  // Reset for next cycle
        return true;  // Dropoff complete
      }
      break;
  }
  
  return false;  // Dropoff not complete
} 