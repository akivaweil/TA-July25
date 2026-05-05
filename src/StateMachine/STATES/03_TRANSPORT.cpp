#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern ServoControl swivelArmServo;
extern unsigned long stateTimer;
extern float STEPS_PER_INCH;

// External Z dropoff settings from dropoff state
extern int Z_DROPOFF_POS;
extern const int Z_DROPOFF_SPEED;
extern const float Z_DROPOFF_LOWER_INCHES;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🚚 TRANSPORT STATE CONFIG                                              ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Position settings (inches)
const float X_DROPOFF_INCHES = 20.75;                             // X dropoff position
const float X_OVERSHOOT_INCHES = (X_DROPOFF_INCHES + 2.3);       // 2.3" past dropoff for servo rotation
const float X_SERVO_ROTATE_LEAD_INCHES = 2.0;                    // Start servo rotation this far before dropoff
const float X_SERVO_ROTATE_INCHES = (X_DROPOFF_INCHES - X_SERVO_ROTATE_LEAD_INCHES);

// Servo settings (degrees)
extern const int SERVO_TRAVEL_POS = 32;      // Travel position (shared with pickup)
const int SERVO_DROPOFF_POS = 110;    // Dropoff orientation (higher is more clockwise)

// Calculated positions (steps) - initialized at runtime
int X_DROPOFF_POS = 0;
int X_OVERSHOOT_POS = 0;
int X_SERVO_ROTATE_POS = 0;
static bool transportConfigInitialized = false;
static bool servoRotatedEnRoute = false;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🚚 TRANSPORT STATE                                                     ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state handles transporting the object from pickup to dropoff:
// Rotate servo, move to overshoot, rotate servo again, move to dropoff

bool handleTransport() {
  // Initialize calculated positions on first call
  if (!transportConfigInitialized) {
    X_DROPOFF_POS = (int)(X_DROPOFF_INCHES * STEPS_PER_INCH);
    X_OVERSHOOT_POS = (int)(X_OVERSHOOT_INCHES * STEPS_PER_INCH);
    X_SERVO_ROTATE_POS = (int)(X_SERVO_ROTATE_INCHES * STEPS_PER_INCH);
    // Ensure Z dropoff position is computed even if dropoff hasn't run yet
    if (Z_DROPOFF_POS == 0) {
      Z_DROPOFF_POS = (int)(Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH);
    }
    transportConfigInitialized = true;
  }
  
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