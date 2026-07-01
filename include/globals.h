#pragma once

#include <Bounce2.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include "Config/Config.h"
#include "Config/Pins_Definitions.h"

// Bounce2 objects
// Bounce2 objects for debounced inputs
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;
extern Bounce startButton;
extern Bounce stage1Signal;
extern Bounce stopSignalStage2;

extern FastAccelStepperEngine engine;
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;

extern ServoControl swivelArmServo;

// State definitions
enum SystemState {
  STATE_IDLE,
  STATE_PICKUP,
  STATE_TRANSPORT,
  STATE_DROPOFF,
  STATE_HOMING
};

enum PickupState {
  PICKUP_MOVE_X,
  PICKUP_LOWER_Z,
  PICKUP_WAIT,
  PICKUP_RAISE_Z,
  PICKUP_DONE
};

enum TransportState {
  TRANSPORT_ROTATE_SERVO,
  TRANSPORT_MOVE_TO_OVERSHOOT,
  TRANSPORT_MOVE_TO_DROPOFF,
  TRANSPORT_DONE
};

enum DropoffState {
  DROPOFF_LOWER_Z,
  DROPOFF_VACUUM_DELAY,
  DROPOFF_RELEASE,
  DROPOFF_WAIT,
  DROPOFF_SETTLE,
  DROPOFF_RAISE_Z,
  DROPOFF_EARLY_RETURN,
  DROPOFF_X_AT_PICKUP,
  DROPOFF_X_HOME,
  DROPOFF_X_RETURN_PICKUP,
  DROPOFF_DONE
};

// State variables
extern SystemState systemState;
extern PickupState pickupState;
extern TransportState transportState;
extern DropoffState dropoffState;
extern bool vacuumActive;

// Function declarations
// Hardware setup functions
void setupPins();
void setupSteppers();
void setupServo();
void setupDebouncers();

// State handler functions
bool handleHomingState();
bool handleIdleState();
bool handlePickupState();
bool handleTransportState();
bool handleDropoffState();
void handleSerial();

// Utility functions
bool isMotorAtTarget(FastAccelStepper* motor);
bool waitForTime(unsigned long duration);
void activateVacuum();
void deactivateVacuum();
