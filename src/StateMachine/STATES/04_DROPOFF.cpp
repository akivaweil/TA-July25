#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern unsigned long stateTimer;
extern Bounce stopSignalStage2;
extern ServoControl swivelArmServo;

//* ************************************************************************
//* ************************ DROPOFF STATE *********************************
//* ************************************************************************
// This state handles dropping off the object:
// Check safety signal, lower Z, release vacuum, wait, raise Z with early X return

bool handleDropoff() {
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
      if (waitForTime(50)) {
        if (zStepper) {
          zStepper->setSpeedInHz(Z_MAX_SPEED);  // Back to normal speed
          zStepper->moveTo(Z_HOME_POS);  // Move to offset position (0.2" from physical home)
        }
        dropoffState = DROPOFF_RAISE_Z;
      }
      break;
      
    case DROPOFF_RAISE_Z:
      //! ************************************************************************
      //! Check if Z has moved up enough to start X return home
      //! ************************************************************************
      if (zStepper && zStepper->getCurrentPosition() <= Z_EARLY_RETURN_POS) {
        // Z has moved up 2 inches, start X return home to 0.25" from home
        digitalWrite(STAGE2_SIGNAL_PIN, HIGH);  // Signal Stage 2
        if (xStepper) {
          xStepper->moveTo(X_RETURN_HOME_POS);  // Move X to 0.25" away from home
        }
        dropoffState = DROPOFF_EARLY_RETURN;
      }
      break;
      
    case DROPOFF_EARLY_RETURN:
      //! ************************************************************************
      //! Wait for both Z to reach full up position and X to reach return home position
      //! ************************************************************************
      if (isMotorAtTarget(zStepper) && isMotorAtTarget(xStepper)) {
        // Both motors at target, reset servo
        swivelArmServo.write(SERVO_HOME_POS);    // Reset servo to home position
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