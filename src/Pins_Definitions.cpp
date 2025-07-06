#include "config/Pins_Definitions.h"

//* ************************************************************************
//* ************************ PIN DEFINITIONS ******************************
//* ************************************************************************
// Pin assignments for Freenove ESP32 board
// Updated to match original Transfer-Arm_TA-June25 project

// INPUT PINS (Active HIGH)
float START_BUTTON_PIN = 2.0;     // Start button input (active high)
float STAGE1_SIGNAL_PIN = 23.0;   // Stage 1 machine signal input (active high)
float X_HOME_SWITCH_PIN = 15.0;   // X-axis home limit switch (active high)
float Z_HOME_SWITCH_PIN = 21.0;   // Z-axis home limit switch (active high)
float STOP_SIGNAL_STAGE_2 = 22.0; // Stage 2 safety signal (active high, wait for low)

// OUTPUT PINS - STEPPER MOTORS
float X_STEP_PIN = 27.0;          // X-axis stepper motor step pin
float X_DIR_PIN = 14.0;           // X-axis stepper motor direction pin
float X_ENABLE_PIN = 4.0;         // X-axis stepper motor enable pin (active low)
float Z_STEP_PIN = 19.0;          // Z-axis stepper motor step pin
float Z_DIR_PIN = 18.0;           // Z-axis stepper motor direction pin

// OUTPUT PINS - ACTUATORS
float SERVO_PIN = 26.0;           // Servo control pin
float SOLENOID_RELAY_PIN = 33.0;  // Solenoid relay control pin
float STAGE2_SIGNAL_PIN = 25.0;   // Signal output to Stage 2 machine (active high) 