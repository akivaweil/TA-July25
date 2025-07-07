#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern ServoControl swivelArmServo;
extern unsigned long stateTimer;

//* ************************************************************************
//* ************************ TRANSPORT STATE *******************************
//* ************************************************************************
// This state handles transporting the object from pickup to dropoff:
// Rotate servo, move to overshoot, rotate servo again, move to dropoff

bool handleTransport() {
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