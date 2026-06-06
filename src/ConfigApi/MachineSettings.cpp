#include "MachineSettings.h"
#include <Arduino.h>
#include <Preferences.h>
#include <FastAccelStepper.h>
#include "globals.h"

// MACHINE SETTINGS IMPLEMENTATION

//* ************************************************************************
//* ******************** CURATED MUTABLE SETTINGS *************************
//* ************************************************************************
// Single definitions (compile-time defaults match the original hardcoded values).
float X_PICKUP_INCHES        = 0.2f;
float Z_PICKUP_LOWER_INCHES  = 6.35f;
int   SERVO_PICKUP_POS       = 52;
int   PICKUP_HOLD_TIME       = 100;
float X_DROPOFF_INCHES       = 20.75f;
int   SERVO_TRAVEL_POS       = 32;
int   SERVO_DROPOFF_POS      = 113;
float Z_DROPOFF_LOWER_INCHES = 6.65f;
int   SERVO_HOME_POS         = 52;
int   DROPOFF_SETTLE_TIME    = 25;
int   Z_DROPOFF_SPEED        = 15000;
// X_MAX_SPEED is defined in Config.cpp (kept at its existing home).

//* ************************************************************************
//* ******************** EXTERNS FROM THE REST OF THE FIRMWARE ************
//* ************************************************************************
// Curated setting whose definition stays in Config.cpp.
extern int X_MAX_SPEED;

// Mechanical scaling.
extern float STEPS_PER_INCH;

// Derived step positions (defined in the state files; recomputed here).
extern int Z_HOME_POS;             // 01_HOMING.cpp
extern int X_PICKUP_POS;           // 02_PICKUP.cpp
extern int Z_PICKUP_POS;           // 02_PICKUP.cpp
extern int Z_SUCTION_START_POS;    // 02_PICKUP.cpp
extern int Z_TRANSPORT_RAISE_POS;  // 02_PICKUP.cpp
extern int Z_SERVO_ROTATE_POS;     // 02_PICKUP.cpp
extern int X_DROPOFF_POS;          // 03_TRANSPORT.cpp
extern int X_OVERSHOOT_POS;        // 03_TRANSPORT.cpp
extern int X_SERVO_ROTATE_POS;     // 03_TRANSPORT.cpp
extern int Z_DROPOFF_POS;          // 04_DROPOFF.cpp
extern int Z_EARLY_RETURN_POS;     // 04_DROPOFF.cpp

// Non-curated derived-position inputs (kept as const in their state files).
extern const float Z_HOME_OFFSET_INCHES;            // 01_HOMING.cpp
extern const float Z_SUCTION_START_INCHES;          // 02_PICKUP.cpp
extern const float Z_TRANSPORT_RAISE_OFFSET_INCHES; // 02_PICKUP.cpp
extern const float X_OVERSHOOT_OFFSET_INCHES;       // 03_TRANSPORT.cpp
extern const float X_SERVO_ROTATE_LEAD_INCHES;      // 03_TRANSPORT.cpp
extern const float Z_EARLY_RETURN_INCHES;           // 04_DROPOFF.cpp

//* ************************************************************************
//* ******************** NVS PERSISTENCE *********************************
//* ************************************************************************
static const char* NVS_NAMESPACE = "ta_config";
static const uint32_t SETTINGS_MAGIC = 0x5441C0FE;  // "TA cofe" sentinel

static Preferences prefs;

void saveSettings() {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putFloat("xPickIn", X_PICKUP_INCHES);
  prefs.putFloat("zPickLowIn", Z_PICKUP_LOWER_INCHES);
  prefs.putInt("servoPick", SERVO_PICKUP_POS);
  prefs.putInt("pickHold", PICKUP_HOLD_TIME);
  prefs.putFloat("xDropIn", X_DROPOFF_INCHES);
  prefs.putInt("servoTravel", SERVO_TRAVEL_POS);
  prefs.putInt("servoDrop", SERVO_DROPOFF_POS);
  prefs.putFloat("zDropLowIn", Z_DROPOFF_LOWER_INCHES);
  prefs.putInt("servoHome", SERVO_HOME_POS);
  prefs.putInt("dropSettle", DROPOFF_SETTLE_TIME);
  prefs.putInt("xMaxSpeed", X_MAX_SPEED);
  prefs.putInt("zDropSpeed", Z_DROPOFF_SPEED);
  prefs.putUInt("magic", SETTINGS_MAGIC);
  prefs.end();
}

// Persist a SINGLE curated value straight to NVS without touching any live
// global. Used by the deferred POST path: the new value is durable across a
// power cut, but the live runtime keeps its old value until the next IDLE entry
// copies persisted -> live via loadSettings()/applyTASettings(). The magic
// sentinel is already present (written at boot), so loadSettings() will read
// these new keys back.
void persistSettingInt(const char* nvsKey, int value) {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putInt(nvsKey, value);
  prefs.end();
}

void persistSettingFloat(const char* nvsKey, float value) {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putFloat(nvsKey, value);
  prefs.end();
}

void loadSettings() {
  prefs.begin(NVS_NAMESPACE, true);  // read-only
  uint32_t magic = prefs.getUInt("magic", 0);
  if (magic != SETTINGS_MAGIC) {
    // First boot: keep compile-time defaults and persist them.
    prefs.end();
    saveSettings();
    return;
  }
  X_PICKUP_INCHES        = prefs.getFloat("xPickIn", X_PICKUP_INCHES);
  Z_PICKUP_LOWER_INCHES  = prefs.getFloat("zPickLowIn", Z_PICKUP_LOWER_INCHES);
  SERVO_PICKUP_POS       = prefs.getInt("servoPick", SERVO_PICKUP_POS);
  PICKUP_HOLD_TIME       = prefs.getInt("pickHold", PICKUP_HOLD_TIME);
  X_DROPOFF_INCHES       = prefs.getFloat("xDropIn", X_DROPOFF_INCHES);
  SERVO_TRAVEL_POS       = prefs.getInt("servoTravel", SERVO_TRAVEL_POS);
  SERVO_DROPOFF_POS      = prefs.getInt("servoDrop", SERVO_DROPOFF_POS);
  Z_DROPOFF_LOWER_INCHES = prefs.getFloat("zDropLowIn", Z_DROPOFF_LOWER_INCHES);
  SERVO_HOME_POS         = prefs.getInt("servoHome", SERVO_HOME_POS);
  DROPOFF_SETTLE_TIME    = prefs.getInt("dropSettle", DROPOFF_SETTLE_TIME);
  X_MAX_SPEED            = prefs.getInt("xMaxSpeed", X_MAX_SPEED);
  Z_DROPOFF_SPEED        = prefs.getInt("zDropSpeed", Z_DROPOFF_SPEED);
  prefs.end();
}

//* ************************************************************************
//* ******************** APPLY (LIVE RUNTIME) ****************************
//* ************************************************************************
// Recomputes EVERY derived step position that the per-state init latches used
// to compute once. Formulas are preserved EXACTLY from the original state code.
void applyTASettings() {
  // --- Homing (was 01_HOMING.cpp homingConfigInitialized) ---
  Z_HOME_POS = (int)(Z_HOME_OFFSET_INCHES * STEPS_PER_INCH);
  // 01_HOMING now moves X to the live X_PICKUP_POS computed below (the old
  // per-state xPickupPosHoming copy was removed).

  // --- Pickup (was 02_PICKUP.cpp pickupConfigInitialized) ---
  X_PICKUP_POS         = (int)(X_PICKUP_INCHES * STEPS_PER_INCH);
  Z_PICKUP_POS         = (int)(Z_PICKUP_LOWER_INCHES * STEPS_PER_INCH);
  Z_SUCTION_START_POS  = (int)(Z_SUCTION_START_INCHES * STEPS_PER_INCH);
  Z_TRANSPORT_RAISE_POS = Z_HOME_POS + (int)(Z_TRANSPORT_RAISE_OFFSET_INCHES * STEPS_PER_INCH);
  Z_SERVO_ROTATE_POS   = (2 * Z_PICKUP_POS + Z_TRANSPORT_RAISE_POS) / 3;

  // --- Transport (was 03_TRANSPORT.cpp transportConfigInitialized) ---
  X_DROPOFF_POS       = (int)(X_DROPOFF_INCHES * STEPS_PER_INCH);
  X_OVERSHOOT_POS     = (int)((X_DROPOFF_INCHES + X_OVERSHOOT_OFFSET_INCHES) * STEPS_PER_INCH);
  X_SERVO_ROTATE_POS  = (int)((X_DROPOFF_INCHES - X_SERVO_ROTATE_LEAD_INCHES) * STEPS_PER_INCH);

  // --- Dropoff (was 04_DROPOFF.cpp dropoffConfigInitialized) ---
  Z_DROPOFF_POS       = (int)(Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH);
  Z_EARLY_RETURN_POS  = (int)(Z_EARLY_RETURN_INCHES * STEPS_PER_INCH);

  // --- Re-apply motor speeds ---
  if (xStepper) {
    xStepper->setSpeedInHz(X_MAX_SPEED);
  }
  // Z_DROPOFF_SPEED is applied on demand at the moment of the dropoff move; the
  // state code reads the live global, so no persistent re-apply is required here.
}
