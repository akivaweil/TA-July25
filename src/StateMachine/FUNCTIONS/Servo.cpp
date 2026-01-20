//* ************************************************************************
//* ************************ SERVO FUNCTIONS *******************************
//* ************************************************************************
// This file contains functions for controlling the servo motor using custom ServoControl class

#include "ServoControl.h"
#include "globals.h"

// ServoControl swivelArmServo; // Definition is in main.cpp

// External reference to SERVO_HOME_POS from return home state
extern int SERVO_HOME_POS;

//* ************************************************************************
//* ************************ SERVO SETUP ***********************************
//* ************************************************************************
void setupServo() {
  // Initialize servo with PWM channel 0
  swivelArmServo.init(SERVO_PIN, 0);
  
  // Set servo to home position
  swivelArmServo.write(SERVO_HOME_POS);
} 