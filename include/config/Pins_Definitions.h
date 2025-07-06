#ifndef PINS_DEFINITIONS_H
#define PINS_DEFINITIONS_H

//* ************************************************************************
//* ************************ PIN DEFINITIONS ******************************
//* ************************************************************************
// Pin assignments for Freenove ESP32 board
// Updated to match original Transfer-Arm_TA-June25 project

// INPUT PINS (Active HIGH)
extern float START_BUTTON_PIN;     // Start button input (active high)
extern float STAGE1_SIGNAL_PIN;   // Stage 1 machine signal input (active high)
extern float X_HOME_SWITCH_PIN;   // X-axis home limit switch (active high)
extern float Z_HOME_SWITCH_PIN;   // Z-axis home limit switch (active high)
extern float STOP_SIGNAL_STAGE_2; // Stage 2 safety signal (active high, wait for low)

// OUTPUT PINS - STEPPER MOTORS
extern float X_STEP_PIN;          // X-axis stepper motor step pin
extern float X_DIR_PIN;           // X-axis stepper motor direction pin
extern float X_ENABLE_PIN;         // X-axis stepper motor enable pin (active low)
extern float Z_STEP_PIN;          // Z-axis stepper motor step pin
extern float Z_DIR_PIN;           // Z-axis stepper motor direction pin

// OUTPUT PINS - ACTUATORS
extern float SERVO_PIN;           // Servo control pin
extern float SOLENOID_RELAY_PIN;  // Solenoid relay control pin
extern float STAGE2_SIGNAL_PIN;   // Signal output to Stage 2 machine (active high)

#endif 