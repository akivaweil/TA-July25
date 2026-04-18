#include <Arduino.h>
#include <FastAccelStepper.h>
#include "globals.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🏠 HOMING STATE CONFIG                                                ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
const float X_HOME_SPEED = 1000.0f;  // X homing speed (steps/s)
const float Z_HOME_SPEED = 1000.0f;  // Z homing speed (steps/s)

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern Bounce zHomeSwitch;
extern Bounce xHomeSwitch;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🏠 HOMING STATE                                                       ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state homes both Z and X axes sequentially
// Z axis homes first, then moves up, then X axis homes and moves to pickup

bool handleHoming() {
  static int homingStep = 0;
  
  switch(homingStep) {
    case 0:  // Start Z homing
      if (zStepper) {
        zStepper->setSpeedInHz((uint32_t)Z_HOME_SPEED);
        zStepper->move(-50000);  // Move negative direction
      }
      homingStep = 1;
      break;
      
    case 1:  // Wait for Z home switch
      if (zHomeSwitch.read() == HIGH) {
        if (zStepper) {
          zStepper->forceStop();
          zStepper->setCurrentPosition((int32_t)Z_HOME_POS);
          zStepper->setSpeedInHz((uint32_t)Z_MAX_SPEED);
          zStepper->moveTo((int32_t)Z_UP_POS);  // Move up 5 inches
        }
        homingStep = 2;
      }
      break;
      
    case 2:  // Wait for Z to reach up position
      if (isMotorAtTarget(zStepper)) {
        if (xStepper) {
          xStepper->setSpeedInHz((uint32_t)X_HOME_SPEED);
          xStepper->move(-50000);  // Move negative direction
        }
        homingStep = 3;
      }
      break;
      
    case 3:  // Wait for X home switch
      if (xHomeSwitch.read() == HIGH) {
        if (xStepper) {
          xStepper->forceStop();
          xStepper->setCurrentPosition((int32_t)X_HOME_POS);
          xStepper->setSpeedInHz((uint32_t)X_MAX_SPEED);
          xStepper->moveTo((int32_t)X_PICKUP_POS);  // Move to pickup
        }
        homingStep = 4;
      }
      break;
      
    case 4:  // Wait for X to reach pickup
      if (isMotorAtTarget(xStepper)) {
        homingStep = 0;   // Reset for next homing
        return true;      // Homing complete
      }
      break;
  }
  
  return false;  // Homing not complete
}
