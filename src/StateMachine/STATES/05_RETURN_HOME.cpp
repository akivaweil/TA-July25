#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "globals.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern ServoControl swivelArmServo;
extern float STEPS_PER_INCH;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🔄 RETURN HOME STATE CONFIG                                            ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Position settings (inches)
const float X_RETURN_HOME_INCHES_RH = 0.25;   // Move X away from home at end of cycle
const float Z_RETURN_HOME_INCHES = 0.25;      // Move Z away from home at end of cycle

// Servo settings (degrees)
int SERVO_HOME_POS = 52;    // Neutral/home position

// Calculated positions (steps) - initialized at runtime
int X_RETURN_HOME_POS_RH = 0;
int Z_RETURN_HOME_POS = 0;
static bool returnHomeConfigInitialized = false;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🔄 RETURN HOME STATE                                                   ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state returns the system to home position:
// Turn off Stage 2 signal, reset servo, move X home, then back to pickup position

bool handleReturnHome() {
  static int returnStep = 0;
  
  // Initialize calculated positions on first call
  if (!returnHomeConfigInitialized) {
    X_RETURN_HOME_POS_RH = (int)(X_RETURN_HOME_INCHES_RH * STEPS_PER_INCH);
    Z_RETURN_HOME_POS = (int)(Z_RETURN_HOME_INCHES * STEPS_PER_INCH);
    returnHomeConfigInitialized = true;
  }
  
  switch(returnStep) {
    case 0:  // Signal Stage 2 and move both axes to return home positions
      digitalWrite(STAGE2_SIGNAL_PIN, LOW);  // Turn off Stage 2 signal
      swivelArmServo.write(SERVO_HOME_POS);    // Reset servo
      if (xStepper) {
        xStepper->moveTo(X_RETURN_HOME_POS_RH);   // Move X to 0.25" away from home
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