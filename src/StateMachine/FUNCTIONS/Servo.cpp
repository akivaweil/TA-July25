//* ************************************************************************
//* ************************ SERVO FUNCTIONS *******************************
//* ************************************************************************
// This file contains functions for controlling the servo motor using custom ServoControl class

#include "ServoControl.h"
#include "globals.h"

// ServoControl swivelArmServo; // Definition is in main.cpp

//* ************************************************************************
//* ************************ SERVO SETUP ***********************************
//* ************************************************************************
void setupServo() {
  // Initialize servo using standard Arduino Servo library (MCPWM on ESP32-S3)
  swivelArmServo.init(SERVO_PIN);
  // Set servo to home position
  swivelArmServo.write(SERVO_HOME_POS);
  // Note: Removed blocking delay for smooth stepper operation
} 