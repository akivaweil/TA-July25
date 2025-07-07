#include "config/Pins_Definitions.h"

//* ************************************************************************
//* ************************ PIN DEFINITIONS ******************************
//* ************************************************************************
// Pin assignments for Freenove ESP32 board
// Updated to match original Transfer-Arm_TA-June25 project

// INPUT PINS (Active HIGH)
int START_BUTTON_PIN = 2;     // Start button input (active high)
int STAGE1_SIGNAL_PIN = 23;   // Stage 1 machine signal input (active high)
int X_HOME_SWITCH_PIN = 15;   // X-axis home limit switch (active high)
int Z_HOME_SWITCH_PIN = 21;   // Z-axis home limit switch (active high)
int STOP_SIGNAL_STAGE_2 = 22; // Stage 2 safety signal (active high, wait for low)

// OUTPUT PINS - STEPPER MOTORS
int X_STEP_PIN = 27;          // X-axis stepper motor step pin
int X_DIR_PIN = 14;           // X-axis stepper motor direction pin
int X_ENABLE_PIN = 4;         // X-axis stepper motor enable pin (active low)
int Z_STEP_PIN = 19;          // Z-axis stepper motor step pin
int Z_DIR_PIN = 18;           // Z-axis stepper motor direction pin

// OUTPUT PINS - ACTUATORS
int SERVO_PIN = 12;           // Servo control pin
int SOLENOID_RELAY_PIN = 33;  // Solenoid relay control pin
int STAGE2_SIGNAL_PIN = 25;   // Signal output to Stage 2 machine (active high) 