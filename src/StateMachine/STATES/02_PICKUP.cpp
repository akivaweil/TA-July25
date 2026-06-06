#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern ServoControl swivelArmServo;
extern unsigned long stateTimer;
extern bool vacuumActive;
extern float STEPS_PER_INCH;

// PICKUP STATE CONFIG
// Position settings (inches)
extern const float X_PICKUP_INCHES = 0.2;    // X pickup position (shared with homing)
const float Z_PICKUP_LOWER_INCHES = 6.35;    // Lower Z for pickup
const float Z_SUCTION_START_INCHES = 4.0;    // Start suction when this far down
const float Z_TRANSPORT_RAISE_OFFSET_INCHES = 0.5; // Stop 0.5" farther from home after pickup

// Servo settings (degrees)
const int SERVO_PICKUP_POS = 52;    // Pickup orientation

// Timing settings (ms)
const int PICKUP_HOLD_TIME = 100;   // Hold time at pickup position

// Calculated positions (steps) - initialized at runtime 
int X_PICKUP_POS = 0;
int Z_PICKUP_POS = 0;
int Z_SUCTION_START_POS = 0;
int Z_TRANSPORT_RAISE_POS = 0;
int Z_SERVO_ROTATE_POS = 0;  // One-third up on Z raise, where servo starts rotating to travel position
static bool pickupConfigInitialized = false;
static bool servoRotatedDuringRaise = false;

// External Z home position from homing state
extern int Z_HOME_POS;

// External travel servo position from transport state
extern const int SERVO_TRAVEL_POS;

// PICKUP STATE
// This state handles the pickup sequence:
// Move X to pickup position, set servo, lower Z, activate vacuum, wait, raise Z

bool handlePickup() {
  // Initialize calculated positions on first call
  if (!pickupConfigInitialized) {
    X_PICKUP_POS = (int)(X_PICKUP_INCHES * STEPS_PER_INCH);
    Z_PICKUP_POS = (int)(Z_PICKUP_LOWER_INCHES * STEPS_PER_INCH);
    Z_SUCTION_START_POS = (int)(Z_SUCTION_START_INCHES * STEPS_PER_INCH);
    Z_TRANSPORT_RAISE_POS = Z_HOME_POS + (int)(Z_TRANSPORT_RAISE_OFFSET_INCHES * STEPS_PER_INCH);
    Z_SERVO_ROTATE_POS = (2 * Z_PICKUP_POS + Z_TRANSPORT_RAISE_POS) / 3;
    pickupConfigInitialized = true;
  }
  
  switch(pickupState) {
    case PICKUP_MOVE_X:
      if (xStepper) {
        xStepper->moveTo(X_PICKUP_POS);
        if (isMotorAtTarget(xStepper)) {
          swivelArmServo.write(SERVO_PICKUP_POS);
          pickupState = PICKUP_LOWER_Z;
        }
      }
      break;
      
    case PICKUP_LOWER_Z:
      if (zStepper) {
        zStepper->moveTo(Z_PICKUP_POS);
        // Activate vacuum when halfway down
        if (zStepper->getCurrentPosition() <= Z_SUCTION_START_POS && !vacuumActive) {
          activateVacuum();
        }
      }
      if (isMotorAtTarget(zStepper)) {
        pickupState = PICKUP_WAIT;
      }
      break;
      
    case PICKUP_WAIT:
      if (waitForTime(PICKUP_HOLD_TIME)) {
        if (zStepper) {
          zStepper->moveTo(Z_TRANSPORT_RAISE_POS);  // Stop 0.5" lower for transport
        }
        servoRotatedDuringRaise = false;
        pickupState = PICKUP_RAISE_Z;
      }
      break;
      
    case PICKUP_RAISE_Z:
      // Start rotating servo to travel position after Z has moved a third of the way up
      if (zStepper && !servoRotatedDuringRaise &&
          zStepper->getCurrentPosition() <= Z_SERVO_ROTATE_POS) {
        swivelArmServo.write(SERVO_TRAVEL_POS);
        servoRotatedDuringRaise = true;
      }
      if (isMotorAtTarget(zStepper)) {
        pickupState = PICKUP_DONE;
      }
      break;
      
    case PICKUP_DONE:
      pickupState = PICKUP_MOVE_X;  // Reset for next cycle
      return true;  // Pickup complete
  }
  
  return false;  // Pickup not complete
} 