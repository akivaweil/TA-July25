#ifndef CONFIG_H
#define CONFIG_H

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⚙️  GLOBAL CONFIGURATION                                               ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Mechanical settings used for step calculations across all states
// State-specific settings have been moved to their respective state files:
//   - 00_IDLE.cpp: START_SIGNAL_DELAY
//   - 01_HOMING.cpp: Z_HOME_OFFSET, homing speeds
//   - 02_PICKUP.cpp: X_PICKUP, Z_PICKUP, SUCTION, SERVO_PICKUP, PICKUP_HOLD_TIME
//   - 03_TRANSPORT.cpp: X_DROPOFF, X_OVERSHOOT, SERVO_TRAVEL/DROPOFF, SERVO_ROTATION_TIME
//   - 04_DROPOFF.cpp: Z_DROPOFF, Z_EARLY_RETURN, DROPOFF_HOLD_TIME, Z_DROPOFF_SPEED
//   - 05_RETURN_HOME.cpp: X/Z_RETURN_HOME, SERVO_HOME_POS

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⚙️  MECHANICAL SETTINGS                                                ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
extern int STEPS_PER_REV;      // Steps per revolution
extern int PULLEY_TEETH;       // Number of teeth on pulleys
extern float BELT_PITCH;       // GT2 belt pitch in mm
extern float STEPS_PER_MM;     // Calculated steps per mm
extern float STEPS_PER_INCH;   // Calculated steps per inch

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⚙️  GLOBAL POSITIONS                                                   ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
extern int X_HOME_POS;  // X home position (physical home)

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⚙️  STEPPER MOTOR SETTINGS (used in main.cpp setup)                    ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
extern int X_MAX_SPEED;        // X max speed (steps/sec)
extern int X_ACCELERATION;     // X acceleration (steps/sec²)
extern int Z_MAX_SPEED;        // Z max speed (steps/sec)
extern int Z_ACCELERATION;     // Z acceleration (steps/sec²)

#endif