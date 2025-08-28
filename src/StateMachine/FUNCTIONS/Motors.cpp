// This file will contain functions for controlling the stepper motors.
#include "globals.h"

bool isMotorAtTarget(FastAccelStepper* motor) {
  if (!motor) return true;
  
  // Check if motor is running
  if (motor->isRunning()) return false;
  
  // Since FastAccelStepper doesn't have getTargetPosition(),
  // we'll use a simpler approach: just check if motor is not running
  // The motor will stop when it reaches the target position
  return true;
}

void enableXMotor() {
  if (xStepper) {
    xStepper->enableOutputs();
  }
}

void disableXMotor() {
  if (xStepper) {
    xStepper->disableOutputs();
  }
} 