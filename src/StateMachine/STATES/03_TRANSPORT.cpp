#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"
#include "ConfigApi/MachineSettings.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern ServoControl swivelArmServo;
extern unsigned long stateTimer;
extern float STEPS_PER_INCH;

// External Z dropoff step target from dropoff state (maintained by applyTASettings()).
// Z_DROPOFF_SPEED and Z_DROPOFF_LOWER_INCHES are curated mutable settings (see
// MachineSettings.h, included above).
extern int Z_DROPOFF_POS;

// TRANSPORT STATE CONFIG
// Curated settings (X_DROPOFF_INCHES, SERVO_TRAVEL_POS, SERVO_DROPOFF_POS) are
// now mutable globals owned by MachineSettings.cpp.

// Non-curated derived-position inputs (extern const so applyTASettings() can use them)
extern const float X_OVERSHOOT_OFFSET_INCHES = 2.3;   // 2.3" past dropoff for servo rotation
extern const float X_SERVO_ROTATE_LEAD_INCHES = 2.0;  // Start servo rotation this far before dropoff

// Calculated positions (steps) - maintained by applyTASettings()
int X_DROPOFF_POS = 0;
int X_OVERSHOOT_POS = 0;
int X_SERVO_ROTATE_POS = 0;
static bool servoRotatedEnRoute = false;

// TRANSPORT STATE
// This state handles transporting the object from pickup to dropoff:
// Rotate servo, move to overshoot, rotate servo again, move to dropoff

bool handleTransport() {
  switch(transportState) {
    case TRANSPORT_ROTATE_SERVO:
      swivelArmServo.write(SERVO_TRAVEL_POS);
      if (xStepper) {
        xStepper->moveTo(X_OVERSHOOT_POS);
      }
      servoRotatedEnRoute = false;
      transportState = TRANSPORT_MOVE_TO_OVERSHOOT;
      break;
      
    case TRANSPORT_MOVE_TO_OVERSHOOT:
      // Begin rotating servo to dropoff orientation a bit before reaching dropoff
      if (xStepper && !servoRotatedEnRoute &&
          xStepper->getCurrentPosition() >= X_SERVO_ROTATE_POS) {
        swivelArmServo.write(SERVO_DROPOFF_POS);
        servoRotatedEnRoute = true;
      }
      if (isMotorAtTarget(xStepper)) {
        // Immediately start X moving back to dropoff AND start lowering Z concurrently
        if (xStepper) {
          xStepper->moveTo(X_DROPOFF_POS);
        }
        if (zStepper) {
          zStepper->setSpeedInHz(Z_DROPOFF_SPEED);
          zStepper->moveTo(Z_DROPOFF_POS);
        }
        transportState = TRANSPORT_MOVE_TO_DROPOFF;
      }
      break;
      
    case TRANSPORT_MOVE_TO_DROPOFF:
      if (isMotorAtTarget(xStepper)) {
        transportState = TRANSPORT_DONE;
      }
      break;
      
    case TRANSPORT_DONE:
      transportState = TRANSPORT_ROTATE_SERVO;  // Reset for next cycle
      return true;  // Transport complete
  }
  
  return false;  // Transport not complete
} 