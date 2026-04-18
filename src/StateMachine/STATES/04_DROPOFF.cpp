#include <Arduino.h>
#include <FastAccelStepper.h>
#include "globals.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 📦 DROPOFF STATE CONFIG                                               ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Z distance (inches from home) and derived step position
const float Z_DROPOFF_LOWER_INCHES = 5.5f;  // Lower Z this many inches for dropoff
const float Z_DROPOFF_POS          = Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH;

// Dropoff-specific Z speed (steps/s)
const float Z_DROPOFF_SPEED = 10000.0f;

// Hold time after releasing (ms)
const float DROPOFF_HOLD_TIME = 100.0f;

// External references to objects defined in main file
extern FastAccelStepper *zStepper;
extern unsigned long stateTimer;
extern Bounce stopSignalStage2;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 📦 DROPOFF STATE                                                      ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state handles dropping off the object:
// Check safety signal, lower Z, release vacuum, wait, raise Z, signal Stage 2

bool handleDropoff() {
  switch(dropoffState) {
    case DROPOFF_LOWER_Z:
      // Check safety signal before lowering (using debounced input)
      if (stopSignalStage2.read() == HIGH) {
        return false;  // Wait for safety signal to go low
      }
      
      if (zStepper) {
        zStepper->setSpeedInHz((uint32_t)Z_DROPOFF_SPEED);  // Slower for dropoff
        zStepper->moveTo((int32_t)Z_DROPOFF_POS);
      }
      if (isMotorAtTarget(zStepper)) {
        deactivateVacuum();
        dropoffState = DROPOFF_RELEASE;
      }
      break;
      
    case DROPOFF_RELEASE:
      dropoffState = DROPOFF_WAIT;
      break;
      
    case DROPOFF_WAIT:
      if (waitForTime((unsigned long)DROPOFF_HOLD_TIME)) {
        if (zStepper) {
          zStepper->setSpeedInHz((uint32_t)Z_MAX_SPEED);  // Back to normal speed
          zStepper->moveTo((int32_t)Z_UP_POS);
        }
        dropoffState = DROPOFF_RAISE_Z;
      }
      break;
      
    case DROPOFF_RAISE_Z:
      if (isMotorAtTarget(zStepper)) {
        digitalWrite((int)STAGE2_SIGNAL_PIN, HIGH);  // Signal Stage 2
        dropoffState = DROPOFF_DONE;
      }
      break;
      
    case DROPOFF_DONE:
      dropoffState = DROPOFF_LOWER_Z;  // Reset for next cycle
      return true;  // Dropoff complete
  }
  
  return false;  // Dropoff not complete
}
