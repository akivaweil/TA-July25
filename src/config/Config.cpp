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
float X_PICKUP_INCHES = 0.2;     // X pickup position
float X_DROPOFF_INCHES = 20.5;  // X dropoff position (was 20.6)
float X_OVERSHOOT_INCHES = (X_DROPOFF_INCHES + 2.3);  // 2.0" past dropoff for servo rotation

float Z_HOME_OFFSET_INCHES = 0.3;      // Move Z away from home after homing
float X_RETURN_HOME_INCHES = 0.25;     // Move X away from home at end of cycle
float Z_RETURN_HOME_INCHES = 0.25;     // Move Z away from home at end of cycle
float Z_PICKUP_LOWER_INCHES = 6.3;     // Lower Z for pickup (updated from original)
float Z_DROPOFF_LOWER_INCHES = 6.5;    // Lower Z for dropoff
float Z_SUCTION_START_INCHES = 4.0;    // Start suction when down

//* ************************************************************************
//* ************************ CONVERTED POSITIONS ***************************
//* ************************************************************************
// Converted positions (in steps)
int X_HOME_POS = 0;
int Z_HOME_POS = (int)(Z_HOME_OFFSET_INCHES * STEPS_PER_INCH); // Z position away from physical home (new zero)
int X_RETURN_HOME_POS = (int)(X_RETURN_HOME_INCHES * STEPS_PER_INCH); // X position away from home
int Z_RETURN_HOME_POS = (int)(Z_RETURN_HOME_INCHES * STEPS_PER_INCH); // Z position away from home
int X_PICKUP_POS = (int)(X_PICKUP_INCHES * STEPS_PER_INCH);
int X_DROPOFF_POS = (int)(X_DROPOFF_INCHES * STEPS_PER_INCH); 
int X_OVERSHOOT_POS = (int)(X_OVERSHOOT_INCHES * STEPS_PER_INCH);

// Z-axis positions (coordinate system: 0.2" from physical home = 0, positive = down)
int Z_UP_POS = 0;     // Z-axis at offset position from physical home
int Z_PICKUP_POS = (int)(Z_PICKUP_LOWER_INCHES * STEPS_PER_INCH);     // Z down for pickup
int Z_DROPOFF_POS = (int)(Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH);   // Z down for dropoff
int Z_SUCTION_START_POS = (int)(Z_SUCTION_START_INCHES * STEPS_PER_INCH); // Z position to start suction
int Z_EARLY_RETURN_POS = (int)(Z_EARLY_RETURN_INCHES * STEPS_PER_INCH); // Z position to start X return home

//* ************************************************************************
//* ************************ SERVO SETTINGS ********************************
//* ************************************************************************
// Servo settings (in degrees)
int SERVO_HOME_POS = 52;      // Neutral position (reduced by 40° total)
int SERVO_PICKUP_POS = 52;    // Pickup orientation (reduced by 40° total)
int SERVO_TRAVEL_POS = 32;     // Travel position (reduced by 40° total)
int SERVO_DROPOFF_POS = 116;   // Dropoff orientation (reduced by 40° total)

//* ************************************************************************
//* ************************ TIMING SETTINGS *******************************
//* ************************************************************************
// Timing settings (in milliseconds)  
int START_SIGNAL_DELAY = 325;   // Delay after start signal before beginning pick cycle
int PICKUP_HOLD_TIME = 300;     // Hold time at pickup position
int DROPOFF_HOLD_TIME = 100;    // Hold time at dropoff position
int SERVO_ROTATION_TIME = 500;  // Wait time for servo rotation

//* ************************************************************************
//* ************************ DROPOFF SETTINGS ******************************
//* ************************************************************************
// Dropoff behavior settings
float Z_EARLY_RETURN_INCHES = 2.0;  // Z distance to travel up before starting X return home

//* ************************************************************************
//* ************************ STEPPER MOTOR SETTINGS ***********************
//* ************************************************************************
// Stepper motor settings (updated from original)
int X_MAX_SPEED = 8000;  // Steps per second
int X_ACCELERATION = 10000; // Steps per second^2
int X_HOME_SPEED = 1000;   // Homing speed

int Z_MAX_SPEED = 15000; // Steps per second
int Z_ACCELERATION = 15000; // Steps per second^2
int Z_HOME_SPEED = 600;     // Homing speed (doubled)
int Z_DROPOFF_SPEED = 15000; // Same speed as normal for dropoff 