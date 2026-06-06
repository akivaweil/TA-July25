#pragma once

// MACHINE SETTINGS (NVS-backed curated config for the dashboard)
// Owns the 12 curated, runtime-editable settings for the Transfer Arm (TA).
// All curated values are de-const'd mutable globals (single definition here).
// applyTASettings() copies NVS values into the live globals, recomputes every
// derived step position previously owned by the four per-state init latches,
// and re-applies motor speeds. Persistence is NVS via Preferences ("ta_config").

// Curated mutable settings
// These are the ONLY definitions of these symbols (state files now extern them).
extern float X_PICKUP_INCHES;        // X pickup position (in)         [shared w/ homing]
extern float Z_PICKUP_LOWER_INCHES;  // Lower Z for pickup (in)
extern int   SERVO_PICKUP_POS;       // Pickup servo orientation (deg)
extern int   PICKUP_HOLD_TIME;       // Hold time at pickup (ms)
extern float X_DROPOFF_INCHES;       // X dropoff position (in)
extern int   SERVO_TRAVEL_POS;       // Travel servo orientation (deg) [shared w/ pickup]
extern int   SERVO_DROPOFF_POS;      // Dropoff servo orientation (deg)
extern float Z_DROPOFF_LOWER_INCHES; // Lower Z for dropoff (in)       [shared w/ transport]
extern int   SERVO_HOME_POS;         // Servo neutral/home (deg)
extern int   DROPOFF_SETTLE_TIME;    // Settle time before raising Z (ms)
// X_MAX_SPEED and Z_DROPOFF_SPEED are also curated; X_MAX_SPEED keeps its home in
// Config.cpp, Z_DROPOFF_SPEED is de-const'd and defined here.
extern int   Z_DROPOFF_SPEED;        // Z speed for dropoff (steps/sec)[shared w/ transport]

// Persistence / apply API
// Load persisted values from NVS into the curated globals (seeds defaults on
// first boot via a magic sentinel and persists them).
void loadSettings();

// Persist the current curated globals to NVS.
void saveSettings();

// Persist a SINGLE curated value straight to NVS by its NVS key, WITHOUT writing
// any live global. Used by the deferred POST path so a new value survives a power
// cut while the running cycle/homing keeps reading the old live value; the next
// IDLE entry copies persisted -> live via loadSettings()/applyTASettings().
void persistSettingInt(const char* nvsKey, int value);
void persistSettingFloat(const char* nvsKey, float value);

// Copy curated globals into live runtime, recompute ALL derived step positions
// (previously done by the per-state init latches), and re-apply motor speeds.
// Call once at boot (after steppers exist) and again whenever configDirty is
// applied on entry to the motionless IDLE state.
void applyTASettings();
