// This file will contain functions for controlling the servo motor.
#include "globals.h"
#include <ESP32Servo.h>

// Servo swivelArmServo; // Definition is in main.cpp

void setupServo() {
  swivelArmServo.attach(SERVO_PIN);
  swivelArmServo.write(SERVO_HOME_POS);
  // Note: Removed blocking delay for smooth stepper operation
} 