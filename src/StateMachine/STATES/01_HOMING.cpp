#include <Arduino.h>
#include <FastAccelStepper.h>
#include "globals.h"
#include "config/Pins_Definitions.h"

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;

//* ************************************************************************
//* ************************ HOMING STATE **********************************
//* ************************************************************************
// This state homes both Z and X axes sequentially
// Z axis homes first, then moves up, then X axis homes and moves to pickup

bool handleHoming() {
  static int homingStep = 0;
  static unsigned long zHomingStartTime = 0;
  
  switch(homingStep) {
    case 0:  // Start Z homing
      if (zStepper) {
        // Z-axis always uses same speed for initial and subsequent homing
        zStepper->setSpeedInHz(Z_HOME_SPEED);
        zStepper->move(-50000);  // Move negative direction
        zHomingStartTime = millis();  // Record start time for timeout
      }
      homingStep = 1;
      break;
      
    case 1:  // Wait for Z home switch or timeout
      if (zHomeSwitch.read() == HIGH || (millis() - zHomingStartTime) >= 5000) {
        if (zStepper) {
          zStepper->forceStop();
          zStepper->setCurrentPosition(0);  // Set physical home as position 0
          zStepper->setSpeedInHz(Z_MAX_SPEED);
          zStepper->moveTo(Z_HOME_POS);  // Move to offset position (1.0" from physical home)
        }
        homingStep = 2;
      }
      break;
      
    case 2:  // Wait for Z to reach up position
      if (isMotorAtTarget(zStepper)) {
        if (xStepper) {
          // Use initial homing speed if this is the first homing, otherwise use normal speed
          int xSpeed = isInitialHoming ? X_INITIAL_HOME_SPEED : X_HOME_SPEED;
          xStepper->setSpeedInHz(xSpeed);
          xStepper->move(-50000);  // Move negative direction
        }
        homingStep = 3;
      }
      break;
      
    case 3:  // Wait for X home switch
      if (xHomeSwitch.read() == HIGH) {
        if (xStepper) {
          xStepper->forceStop();
          xStepper->setCurrentPosition(X_HOME_POS);
          xStepper->setSpeedInHz(X_MAX_SPEED);
          xStepper->moveTo(X_PICKUP_POS);  // Move to pickup
        }
        homingStep = 4;
      }
      break;
      
    case 4:  // Wait for X to reach pickup
      if (isMotorAtTarget(xStepper)) {
        isInitialHoming = false;  // Clear initial homing flag after first homing
        homingStep = 0;   // Reset for next homing
        return true;      // Homing complete
      }
      break;
  }
  
  return false;  // Homing not complete
} 