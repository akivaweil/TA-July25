// This file will contain functions for controlling the stepper motors.
#include "globals.h"
#include <stdlib.h>

// "At target" tolerance (steps). A normally completed moveTo() ends exactly at
// targetPos(), so this is satisfied with margin to spare; it only rejects the
// case where the motor stopped short (e.g. force-stopped) of its commanded
// target, which must NOT be reported as "reached".
const int32_t MOTOR_AT_TARGET_TOLERANCE_STEPS = 5;

bool isMotorAtTarget(FastAccelStepper* motor) {
  if (!motor) return true;

  // Still moving: not there yet.
  if (motor->isRunning()) return false;

  // Stopped — only "at target" if the current position is within tolerance of
  // the commanded target. This distinguishes a completed move from a
  // force-stopped-short condition.
  return abs(motor->getCurrentPosition() - motor->targetPos())
             <= MOTOR_AT_TARGET_TOLERANCE_STEPS;
} 