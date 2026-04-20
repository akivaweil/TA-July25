#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern ServoControl swivelArmServo;
extern unsigned long stateTimer;
extern float STEPS_PER_INCH;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🚚 TRANSPORT STATE CONFIG                                              ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Position settings (inches)
const float X_DROPOFF_INCHES = 20.5;                             // X dropoff position
const float X_OVERSHOOT_INCHES = (X_DROPOFF_INCHES + 2.3);       // 2.3" past dropoff for servo rotation

// Servo settings (degrees)
extern const int SERVO_TRAVEL_POS = 32;      // Travel position (shared with pickup)
const int SERVO_DROPOFF_POS = 112;    // Dropoff orientation

// Timing settings (ms)
const int SERVO_ROTATION_TIME = 500;  // Wait time for servo rotation

// Calculated positions (steps) - initialized at runtime
int X_DROPOFF_POS = 0;
int X_OVERSHOOT_POS = 0;
static bool transportConfigInitialized = false;

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
    transportConfigInitialized = true;
  }
  
  switch(transportState) {
    case TRANSPORT_ROTATE_SERVO:
      swivelArmServo.write(SERVO_TRAVEL_POS);
      if (xStepper) {
        xStepper->moveTo(X_OVERSHOOT_POS);
      }
      transportState = TRANSPORT_MOVE_TO_OVERSHOOT;
      break;
      
    case TRANSPORT_MOVE_TO_OVERSHOOT:
      if (isMotorAtTarget(xStepper)) {
        swivelArmServo.write(SERVO_DROPOFF_POS);
        transportState = TRANSPORT_WAIT_SERVO;
      }
      break;
      
    case TRANSPORT_WAIT_SERVO:
      if (waitForTime(SERVO_ROTATION_TIME)) {
        if (xStepper) {
          xStepper->moveTo(X_DROPOFF_POS);
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