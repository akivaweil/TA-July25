#include "Config/Config.h"

// GLOBAL CONFIGURATION
// Mechanical settings used for step calculations across all states
// State-specific settings have been moved to their respective state files

// MECHANICAL SETTINGS
int STEPS_PER_REV = 400;       // Steps per revolution (1.8° with 1/2 microstepping)
int PULLEY_TEETH = 20;         // Number of teeth on pulleys
float BELT_PITCH = 2.0;        // GT2 belt pitch in mm
float STEPS_PER_MM = ((float)STEPS_PER_REV / (PULLEY_TEETH * BELT_PITCH));
float STEPS_PER_INCH = (STEPS_PER_MM * 25.4);

// GLOBAL POSITIONS
int X_HOME_POS = 0;  // X home position (physical home)

// STEPPER MOTOR SETTINGS (used in main.cpp setup)
int X_MAX_SPEED = 6000;        // X max speed (steps/sec)
int X_ACCELERATION = 10000;    // X acceleration (steps/sec²)
int Z_MAX_SPEED = 10000;       // Z max speed (steps/sec)
int Z_ACCELERATION = 10000;    // Z acceleration (steps/sec²)

// INPUT DEBOUNCE SETTINGS
float Z_HOME_SWITCH_DEBOUNCE_MS = 2.0;  // Z home switch debounce (ms)
const uint16_t X_HOME_SWITCH_DEBOUNCE_MS = 10;  // X home switch debounce (ms)
const uint16_t INPUT_DEBOUNCE_MS = 10;          // start/stage signal debounce (ms)

// WIFI LINK SCORE (dashboard 1–10 connection score, mapped from RSSI)
const int8_t WIFI_RSSI_BEST_DBM = -50;   // RSSI at/above this scores 10/10
const int8_t WIFI_RSSI_WORST_DBM = -90;  // RSSI at/below this scores 1/10
