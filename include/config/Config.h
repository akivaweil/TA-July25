#ifndef CONFIG_H
#define CONFIG_H

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🌐 SHARED CONFIGURATION                                              ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Only values used by multiple states or by main setup live here.
// State-specific settings live at the top of each state file.

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⚙️  MECHANICAL SETTINGS                                               ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
constexpr float STEPS_PER_REV  = 400.0f;  // Steps per rev (1.8° @ 1/2 microstepping)
constexpr float PULLEY_TEETH   = 20.0f;   // Teeth on drive pulleys
constexpr float BELT_PITCH     = 2.0f;    // GT2 belt pitch (mm)
constexpr float STEPS_PER_MM   = STEPS_PER_REV / (PULLEY_TEETH * BELT_PITCH);
constexpr float STEPS_PER_INCH = STEPS_PER_MM * 25.4f;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 📍 SHARED POSITIONS                                                   ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Home positions used by homing + return-home
constexpr float X_HOME_POS = 0.0f;
constexpr float Z_HOME_POS = 0.0f;
constexpr float Z_UP_POS   = 0.0f;  // Z fully up (homing / pickup / dropoff)

// Pickup X position (used by homing, pickup, return-home)
constexpr float X_PICKUP_INCHES = 1.0f;
constexpr float X_PICKUP_POS    = X_PICKUP_INCHES * STEPS_PER_INCH;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🔧 SERVO - SHARED                                                     ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Neutral position used by setup + return-home
constexpr float SERVO_HOME_POS = 90.0f;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🏃 STEPPER SPEEDS - SHARED                                            ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Used by main setup + multiple states
constexpr float X_MAX_SPEED    = 7000.0f;   // steps/s
constexpr float X_ACCELERATION = 10000.0f;  // steps/s^2

constexpr float Z_MAX_SPEED    = 10000.0f;  // steps/s
constexpr float Z_ACCELERATION = 10000.0f;  // steps/s^2

#endif
