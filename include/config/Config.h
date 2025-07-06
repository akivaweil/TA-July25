#ifndef CONFIG_H
#define CONFIG_H

//* ************************************************************************
//* ************************* CONFIGURATION *******************************
//* ************************************************************************
// Updated to match original Transfer-Arm_TA-June25 project values

//* ************************************************************************
//* ************************ MECHANICAL SETTINGS **************************
//* ************************************************************************
extern float STEPS_PER_REV;   // Steps per revolution (1.8° with 1/2 microstepping)
extern float PULLEY_TEETH;     // Number of teeth on pulleys
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

extern float Z_PICKUP_LOWER_INCHES;     // Lower Z for pickup
extern float Z_DROPOFF_LOWER_INCHES;    // Lower Z for dropoff
extern float Z_SUCTION_START_INCHES;    // Start suction when this far down

//* ************************************************************************
//* ************************ CONVERTED POSITIONS ***************************
//* ************************************************************************
// Converted positions (in steps)
extern float X_HOME_POS;
extern float Z_HOME_POS;
extern float X_PICKUP_POS;
extern float X_DROPOFF_POS;
extern float X_OVERSHOOT_POS;

// Z-axis positions (original coordinate system: Z_UP = 0, positive = down)
extern float Z_UP_POS;     // Z-axis fully up position
extern float Z_PICKUP_POS;     // Z down for pickup
extern float Z_DROPOFF_POS;   // Z down for dropoff
extern float Z_SUCTION_START_POS; // Z position to start suction

//* ************************************************************************
//* ************************ SERVO SETTINGS ********************************
//* ************************************************************************
// Servo settings (in degrees)
extern float SERVO_HOME_POS;      // Neutral position
extern float SERVO_PICKUP_POS;    // Pickup orientation
extern float SERVO_TRAVEL_POS;     // Travel position
extern float SERVO_DROPOFF_POS;   // Dropoff orientation

//* ************************************************************************
//* ************************ TIMING SETTINGS *******************************
//* ************************************************************************
// Timing settings (in milliseconds)
extern float PICKUP_HOLD_TIME;     // Hold time at pickup position
extern float DROPOFF_HOLD_TIME;    // Hold time at dropoff position
extern float SERVO_ROTATION_TIME;  // Wait time for servo rotation

//* ************************************************************************
//* ************************ STEPPER MOTOR SETTINGS ***********************
//* ************************************************************************
extern float X_MAX_SPEED;  // Steps per second
extern float X_ACCELERATION; // Steps per second^2
extern float X_HOME_SPEED;    // Homing speed

extern float Z_MAX_SPEED; // Steps per second
extern float Z_ACCELERATION; // Steps per second^2
extern float Z_HOME_SPEED;    // Homing speed
extern float Z_DROPOFF_SPEED; // Dropoff speed

#endif 