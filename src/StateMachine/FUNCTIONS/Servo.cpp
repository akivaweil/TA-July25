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
  // Initialize servo with PWM channel 0
  swivelArmServo.init(SERVO_PIN, 0);
  
  // Set servo to home position
  swivelArmServo.write(SERVO_HOME_POS);

  //! ************************************************************************
  //! STEP 0: SIMPLE STARTUP TEST MOVE
  //! ************************************************************************
  // Move through travel and dropoff positions, then back to home
  delay(SERVO_ROTATION_TIME);
  swivelArmServo.write(SERVO_TRAVEL_POS);
  delay(SERVO_ROTATION_TIME);
  swivelArmServo.write(SERVO_DROPOFF_POS);
  delay(SERVO_ROTATION_TIME);
  swivelArmServo.write(SERVO_HOME_POS);
} 