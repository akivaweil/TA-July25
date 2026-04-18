#include <Arduino.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
#include "globals.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🚚 TRANSPORT STATE CONFIG                                             ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// X positions (inches from home) and derived step positions
const float X_DROPOFF_INCHES   = 20.95f;                          // X dropoff position
const float X_OVERSHOOT_INCHES = X_DROPOFF_INCHES + 1.75f;        // 1.75" past dropoff for servo rotation
const float X_DROPOFF_POS      = X_DROPOFF_INCHES   * STEPS_PER_INCH;
const float X_OVERSHOOT_POS    = X_OVERSHOOT_INCHES * STEPS_PER_INCH;

// Servo orientations (degrees)
const float SERVO_TRAVEL_POS  = 0.0f;    // Travel position
const float SERVO_DROPOFF_POS = 80.0f;   // Dropoff orientation

// Wait time for servo to rotate (ms)
const float SERVO_ROTATION_TIME = 500.0f;

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern Servo swivelArmServo;
extern unsigned long stateTimer;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🚚 TRANSPORT STATE                                                    ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state handles transporting the object from pickup to dropoff:
// Rotate servo, move to overshoot, rotate servo again, move to dropoff

bool handleTransport() {
  switch(transportState) {
    case TRANSPORT_ROTATE_SERVO:
      swivelArmServo.write((int)SERVO_TRAVEL_POS);
      if (xStepper) {
        xStepper->moveTo((int32_t)X_OVERSHOOT_POS);
      }
      transportState = TRANSPORT_MOVE_TO_OVERSHOOT;
      break;
      
    case TRANSPORT_MOVE_TO_OVERSHOOT:
      if (isMotorAtTarget(xStepper)) {
        swivelArmServo.write((int)SERVO_DROPOFF_POS);
        transportState = TRANSPORT_WAIT_SERVO;
      }
      break;
      
    case TRANSPORT_WAIT_SERVO:
      if (waitForTime((unsigned long)SERVO_ROTATION_TIME)) {
        if (xStepper) {
          xStepper->moveTo((int32_t)X_DROPOFF_POS);
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
