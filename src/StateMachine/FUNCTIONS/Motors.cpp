// This file will contain functions for controlling the stepper motors.
#include "globals.h"

bool isMotorAtTarget(FastAccelStepper* motor) {
  if (!motor) return true;
  return !motor->isRunning();
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