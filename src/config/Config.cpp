#include "config/Config.h"

//* ************************************************************************
//* ************************* CONFIGURATION *******************************
//* ************************************************************************
// Updated to match original Transfer-Arm_TA-June25 project values

//* ************************************************************************
//* ************************ MECHANICAL SETTINGS **************************
//* ************************************************************************
int STEPS_PER_REV = 400;   // Steps per revolution (1.8° with 1/2 microstepping)
int PULLEY_TEETH = 20;     // Number of teeth on pulleys
float BELT_PITCH = 2.0;        // GT2 belt pitch in mm
float STEPS_PER_MM = ((float)STEPS_PER_REV / (PULLEY_TEETH * BELT_PITCH));
float STEPS_PER_INCH = (STEPS_PER_MM * 25.4);

//* ************************************************************************
//* ************************ POSITION SETTINGS *****************************
//* ************************************************************************
// Position settings (in inches from home)
float X_PICKUP_INCHES = 1.0;     // X pickup position
float X_DROPOFF_INCHES = 20.95;  // X dropoff position (updated from original)
float X_OVERSHOOT_INCHES = (X_DROPOFF_INCHES + 1.75);  // 1.75" past dropoff for servo rotation

float Z_PICKUP_LOWER_INCHES = 7.0;     // Lower Z by 7" for pickup (updated from original)
float Z_DROPOFF_LOWER_INCHES = 5.5;    // Lower Z by 5.5" for dropoff
float Z_SUCTION_START_INCHES = 4.0;    // Start suction when 4" down

//* ************************************************************************
//* ************************ CONVERTED POSITIONS ***************************
//* ************************************************************************
// Converted positions (in steps)
int X_HOME_POS = 0;
int Z_HOME_POS = 0;
int X_PICKUP_POS = (int)(X_PICKUP_INCHES * STEPS_PER_INCH);
int X_DROPOFF_POS = (int)(X_DROPOFF_INCHES * STEPS_PER_INCH); 
int X_OVERSHOOT_POS = (int)(X_OVERSHOOT_INCHES * STEPS_PER_INCH);

// Z-axis positions (original coordinate system: Z_UP = 0, positive = down)
int Z_UP_POS = 0;     // Z-axis fully up position
int Z_PICKUP_POS = (int)(Z_PICKUP_LOWER_INCHES * STEPS_PER_INCH);     // Z down for pickup
int Z_DROPOFF_POS = (int)(Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH);   // Z down for dropoff
int Z_SUCTION_START_POS = (int)(Z_SUCTION_START_INCHES * STEPS_PER_INCH); // Z position to start suction

//* ************************************************************************
//* ************************ SERVO SETTINGS ********************************
//* ************************************************************************
// Servo settings (in degrees)
int SERVO_HOME_POS = 90;      // Neutral position
int SERVO_PICKUP_POS = 10;    // Pickup orientation (updated from original)
int SERVO_TRAVEL_POS = 0;     // Travel position (updated from original)
int SERVO_DROPOFF_POS = 80;   // Dropoff orientation (updated from original)

//* ************************************************************************
//* ************************ TIMING SETTINGS *******************************
//* ************************************************************************
// Timing settings (in milliseconds)  
int PICKUP_HOLD_TIME = 300;     // Hold time at pickup position
int DROPOFF_HOLD_TIME = 100;    // Hold time at dropoff position
int SERVO_ROTATION_TIME = 500;  // Wait time for servo rotation

//* ************************************************************************
//* ************************ STEPPER MOTOR SETTINGS ***********************
//* ************************************************************************
// Stepper motor settings (updated from original)
int X_MAX_SPEED = 7000;  // Steps per second
int X_ACCELERATION = 10000; // Steps per second^2
int X_HOME_SPEED = 1000;    // Homing speed

int Z_MAX_SPEED = 10000; // Steps per second
int Z_ACCELERATION = 10000; // Steps per second^2
int Z_HOME_SPEED = 1000;    // Homing speed
int Z_DROPOFF_SPEED = 10000; // Same speed as normal for dropoff 