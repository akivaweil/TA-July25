#ifndef CONFIG_H
#define CONFIG_H

//* ************************************************************************
//* ************************* CONFIGURATION *******************************
//* ************************************************************************
// Updated to match original Transfer-Arm_TA-June25 project values

//* ************************************************************************
//* ************************ MECHANICAL SETTINGS **************************
//* ************************************************************************
extern int STEPS_PER_REV;   // Steps per revolution (1.8° with 1/2 microstepping)
extern int PULLEY_TEETH;     // Number of teeth on pulleys
extern float BELT_PITCH;        // GT2 belt pitch in mm
extern float STEPS_PER_MM;
extern float STEPS_PER_INCH;

//* ************************************************************************
//* ************************ POSITION SETTINGS *****************************
//* ************************************************************************
// Position settings (in inches from home)
extern float X_PICKUP_INCHES;     // X pickup position
extern float X_DROPOFF_INCHES;    // X dropoff position
extern float X_OVERSHOOT_INCHES;  // X overshoot position for servo rotation

extern float Z_HOME_OFFSET_INCHES;      // Move Z away from home after homing
extern float Z_PICKUP_LOWER_INCHES;     // Lower Z for pickup
extern float Z_DROPOFF_LOWER_INCHES;    // Lower Z for dropoff
extern float Z_SUCTION_START_INCHES;    // Start suction when this far down

//* ************************************************************************
//* ************************ CONVERTED POSITIONS ***************************
//* ************************************************************************
// Converted positions (in steps)
extern int X_HOME_POS;
extern int Z_HOME_POS;
extern int X_PICKUP_POS;
extern int X_DROPOFF_POS;
extern int X_OVERSHOOT_POS;

// Z-axis positions (original coordinate system: Z_UP = 0, positive = down)
extern int Z_UP_POS;     // Z-axis fully up position
extern int Z_HOME_OFFSET_POS; // Z position away from home after homing
extern int Z_PICKUP_POS;     // Z down for pickup
extern int Z_DROPOFF_POS;   // Z down for dropoff
extern int Z_SUCTION_START_POS; // Z position to start suction
extern int Z_EARLY_RETURN_POS; // Z position to start X return home

//* ************************************************************************
//* ************************ SERVO SETTINGS ********************************
//* ************************************************************************
// Servo settings (in degrees)
extern int SERVO_HOME_POS;      // Neutral position
extern int SERVO_PICKUP_POS;    // Pickup orientation
extern int SERVO_TRAVEL_POS;     // Travel position
extern int SERVO_DROPOFF_POS;   // Dropoff orientation

//* ************************************************************************
//* ************************ TIMING SETTINGS *******************************
//* ************************************************************************
// Timing settings (in milliseconds)
extern int PICKUP_HOLD_TIME;     // Hold time at pickup position
extern int DROPOFF_HOLD_TIME;    // Hold time at dropoff position
extern int SERVO_ROTATION_TIME;  // Wait time for servo rotation

//* ************************************************************************
//* ************************ DROPOFF SETTINGS ******************************
//* ************************************************************************
// Dropoff behavior settings
extern float Z_EARLY_RETURN_INCHES;  // Z distance to travel up before starting X return home

//* ************************************************************************
//* ************************ STEPPER MOTOR SETTINGS ***********************
//* ************************************************************************
extern int X_MAX_SPEED;  // Steps per second
extern int X_ACCELERATION; // Steps per second^2
extern int X_HOME_SPEED;    // Homing speed

extern int Z_MAX_SPEED; // Steps per second
extern int Z_ACCELERATION; // Steps per second^2
extern int Z_HOME_SPEED;    // Homing speed
extern int Z_DROPOFF_SPEED; // Dropoff speed

#endif 