#pragma once

// Pin definitions
// Pin assignments for Freenove ESP32 board
// Updated to match original Transfer-Arm_TA-June25 project

// INPUT PINS (Active HIGH)
extern int START_BUTTON_PIN;     // Start button input (active high)
extern int STAGE1_SIGNAL_PIN;   // Stage 1 machine signal input (active high)
extern int X_HOME_SWITCH_PIN;   // X-axis home limit switch (active high)
extern int Z_HOME_SWITCH_PIN;   // Z-axis home limit switch (active high)
extern int STOP_SIGNAL_STAGE_2; // Stage 2 safety signal (active high, wait for low)

// OUTPUT PINS - STEPPER MOTORS
extern int X_STEP_PIN;          // X-axis stepper motor step pin
extern int X_DIR_PIN;           // X-axis stepper motor direction pin
extern int X_ENABLE_PIN;         // X-axis stepper motor enable pin (active low)
extern int Z_STEP_PIN;          // Z-axis stepper motor step pin
extern int Z_DIR_PIN;           // Z-axis stepper motor direction pin

// OUTPUT PINS - ACTUATORS
extern int SERVO_PIN;           // Servo control pin
extern int SOLENOID_RELAY_PIN;  // Solenoid relay control pin
extern int STAGE2_SIGNAL_PIN;   // Signal output to Stage 2 machine (active high) 