#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern ServoControl swivelArmServo;

//* ************************************************************************
//* ************************ RETURN HOME STATE *****************************
//* ************************************************************************
// This state returns the system to home position:
// Turn off Stage 2 signal, reset servo, move X home, then back to pickup position

bool handleReturnHome() {
  static int returnStep = 0;
  
  switch(returnStep) {
    case 0:  // Signal Stage 2 and move both axes to return home positions
      digitalWrite(STAGE2_SIGNAL_PIN, LOW);  // Turn off Stage 2 signal
      swivelArmServo.write(SERVO_HOME_POS);    // Reset servo
      if (xStepper) {
        xStepper->moveTo(X_RETURN_HOME_POS);   // Move X to 0.25" away from home
      }
      if (zStepper) {
        zStepper->moveTo(Z_RETURN_HOME_POS);   // Move Z to 0.25" away from home
      }
      returnStep = 1;
      break;
      
    case 1:  // Wait for both axes to reach return home positions
      if (isMotorAtTarget(xStepper) && isMotorAtTarget(zStepper)) {
        returnStep = 0;   // Reset for next cycle
        return true;      // Return complete
      }
      break;
  }
  
  return false;  // Return not complete
} 