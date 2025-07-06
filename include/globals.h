#ifndef GLOBALS_H
#define GLOBALS_H

#include <Bounce2.h>
#include <FastAccelStepper.h>
#include "config/Config.h"
#include "config/Pins_Definitions.h"

//* ************************************************************************
//* ************************ BOUNCE2 OBJECTS ******************************
//* ************************************************************************
// Bounce2 objects for debounced inputs
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;
extern Bounce startButton;
extern Bounce stage1Signal;
extern Bounce stopSignalStage2;

//* ************************************************************************
//* ************************ STATE DEFINITIONS *****************************
//* ************************************************************************
enum SystemState {
  STATE_IDLE,
  STATE_PICKUP,
  STATE_TRANSPORT, 
  STATE_DROPOFF,
  STATE_RETURN_HOME,
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
  TRANSPORT_WAIT_SERVO,
  TRANSPORT_MOVE_TO_DROPOFF,
  TRANSPORT_DONE
};

enum DropoffState {
  DROPOFF_LOWER_Z,
  DROPOFF_RELEASE,
  DROPOFF_WAIT,
  DROPOFF_RAISE_Z,
  DROPOFF_DONE
};

//* ************************************************************************
//* ************************ STATE VARIABLES *******************************
//* ************************************************************************
extern SystemState systemState;
extern PickupState pickupState;
extern TransportState transportState;
extern DropoffState dropoffState;

//* ************************************************************************
//* ************************ FUNCTION DECLARATIONS *************************
//* ************************************************************************
// Hardware setup functions
void setupPins();
void setupSteppers(); 
void setupServo();
void setupDebouncers();

// State handler functions
bool handleHoming();
bool handleIdle();
bool handlePickup();
bool handleTransport();
bool handleDropoff();
bool handleReturnHome();
void handleSerial();

// Utility functions
bool isMotorAtTarget(FastAccelStepper* motor);
bool waitForTime(unsigned long duration);
void activateVacuum();
void deactivateVacuum();
void enableXMotor();
void disableXMotor();

#endif 