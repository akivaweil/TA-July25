#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern ServoControl swivelArmServo;

//* ************************************************************************
//* ************************ RETURN HOME STATE *****************************
//* ************************************************************************
// This state returns the system to home position:
// Turn off Stage 2 signal, reset servo, move X home, then back to pickup position

bool handleReturnHome() {
  static int returnStep = 0;
  
  switch(returnStep) {
    case 0:  // Signal Stage 2 and move X home
      digitalWrite((int)STAGE2_SIGNAL_PIN, LOW);  // Turn off Stage 2 signal
      swivelArmServo.write((int)SERVO_HOME_POS);    // Reset servo
      if (xStepper) {
        xStepper->moveTo((int32_t)X_HOME_POS);           // Move X home
      }
      returnStep = 1;
      break;
      
    case 1:  // Wait for X to reach home
      if (isMotorAtTarget(xStepper)) {
        if (xStepper) {
          xStepper->moveTo((int32_t)X_PICKUP_POS);
        }
        returnStep = 2;
      }
      break;
      
    case 2:  // Wait for X to reach pickup
      if (isMotorAtTarget(xStepper)) {
        returnStep = 0;   // Reset for next cycle
        return true;      // Return complete
      }
      break;
  }
  
  return false;  // Return not complete
} 