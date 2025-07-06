#include <Arduino.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>
#include "globals.h"

// Function declarations
void setupPins();
void setupSteppers(); 
void setupServo();
void setupDebouncers();
bool handleHoming();
bool handleIdle();
bool handlePickup();
bool handleTransport();
bool handleDropoff();
bool handleReturnHome();
void handleSerial();

// OTA function declarations (implemented in OTA_Manager.cpp)
void initOTA();
void handleOTA();

//* ************************************************************************
//* ************************ HARDWARE OBJECTS *****************************
//* ************************************************************************
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *xStepper = NULL;
FastAccelStepper *zStepper = NULL;
Servo swivelArmServo;

//* ************************************************************************
//* ************************ BOUNCE2 OBJECTS *******************************
//* ************************************************************************
Bounce xHomeSwitch = Bounce();
Bounce zHomeSwitch = Bounce();
Bounce startButton = Bounce();
Bounce stage1Signal = Bounce();
Bounce stopSignalStage2 = Bounce();

//* ************************************************************************
//* ************************ STATE VARIABLES *******************************
//* ************************************************************************
SystemState systemState = STATE_HOMING;
PickupState pickupState = PICKUP_MOVE_X;
TransportState transportState = TRANSPORT_ROTATE_SERVO;
DropoffState dropoffState = DROPOFF_LOWER_Z;

// Timing variables
unsigned long stateTimer = 0;
bool vacuumActive = false;

//* ************************************************************************
//* ************************ SETUP FUNCTION ********************************
//* ************************************************************************
void setup() {
  // Initialize OTA functionality
  initOTA();
  
  // Configure pins
  setupPins();
  
  // Configure debouncers
  setupDebouncers();
  
  // Configure steppers
  setupSteppers();
  
  // Configure servo
  setupServo();
  
  systemState = STATE_HOMING;
}

//* ************************************************************************
//* ************************ SETUP FUNCTIONS *******************************
//* ************************************************************************
void setupPins() {
  // Input pins
  pinMode((int)START_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode((int)STAGE1_SIGNAL_PIN, INPUT_PULLDOWN);
  pinMode((int)X_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode((int)Z_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode((int)STOP_SIGNAL_STAGE_2, INPUT_PULLDOWN);
  
  // Output pins
  pinMode((int)X_ENABLE_PIN, OUTPUT);
  pinMode((int)SOLENOID_RELAY_PIN, OUTPUT);
  pinMode((int)STAGE2_SIGNAL_PIN, OUTPUT);
  
  // Initial states
  digitalWrite((int)X_ENABLE_PIN, LOW);   // Enable X motor for smooth operation
  digitalWrite((int)SOLENOID_RELAY_PIN, LOW);  // Vacuum off
  digitalWrite((int)STAGE2_SIGNAL_PIN, LOW);   // Stage 2 signal off
}

void setupDebouncers() {
  // Configure limit switches with 2ms debounce (same as Transfer-Arm_TA-June25)
  xHomeSwitch.attach((int)X_HOME_SWITCH_PIN);
  xHomeSwitch.interval(2);  // 2ms debounce
  
  zHomeSwitch.attach((int)Z_HOME_SWITCH_PIN);
  zHomeSwitch.interval(2);  // 2ms debounce
  
  // Configure input signals with 10ms debounce
  startButton.attach((int)START_BUTTON_PIN);
  startButton.interval(10);  // 10ms debounce
  
  stage1Signal.attach((int)STAGE1_SIGNAL_PIN);
  stage1Signal.interval(10);  // 10ms debounce
  
  stopSignalStage2.attach((int)STOP_SIGNAL_STAGE_2);
  stopSignalStage2.interval(10);  // 10ms debounce
}

void setupSteppers() {
  engine.init();
  
  xStepper = engine.stepperConnectToPin((int)X_STEP_PIN);
  if (xStepper) {
    xStepper->setDirectionPin((int)X_DIR_PIN);
    xStepper->setEnablePin((int)X_ENABLE_PIN);
    xStepper->setAutoEnable(true);
    xStepper->setSpeedInHz((uint32_t)X_MAX_SPEED);
    xStepper->setAcceleration((uint32_t)X_ACCELERATION);
  }
  
  zStepper = engine.stepperConnectToPin((int)Z_STEP_PIN);
  if (zStepper) {
    zStepper->setDirectionPin((int)Z_DIR_PIN);
    zStepper->setSpeedInHz((uint32_t)Z_MAX_SPEED);
    zStepper->setAcceleration((uint32_t)Z_ACCELERATION);
  }
  
  vacuumActive = false;
}

void setupServo() {
  swivelArmServo.attach((int)SERVO_PIN);
  swivelArmServo.write((int)SERVO_HOME_POS);
  // Note: Removed blocking delay for smooth stepper operation
}

//* ************************************************************************
//* ************************ MAIN LOOP - STATE MACHINE ********************
//* ************************************************************************
void loop() {
  // Handle OTA updates
  handleOTA();
  
  // Update all debouncers first
  xHomeSwitch.update();
  zHomeSwitch.update();
  startButton.update();
  stage1Signal.update();
  stopSignalStage2.update();
  
  // FastAccelStepper runs automatically via interrupts - no need to call run()
  
  // Main state machine
  switch(systemState) {
    case STATE_HOMING:
      if (handleHoming()) {
        systemState = STATE_IDLE;
      }
      break;
      
    case STATE_IDLE:
      if (handleIdle()) {
        systemState = STATE_PICKUP;
        pickupState = PICKUP_MOVE_X;
      }
      break;
      
    case STATE_PICKUP:
      if (handlePickup()) {
        systemState = STATE_TRANSPORT;
        transportState = TRANSPORT_ROTATE_SERVO;
      }
      break;
      
    case STATE_TRANSPORT:
      if (handleTransport()) {
        systemState = STATE_DROPOFF;
        dropoffState = DROPOFF_LOWER_Z;
      }
      break;
      
    case STATE_DROPOFF:
      if (handleDropoff()) {
        systemState = STATE_RETURN_HOME;
      }
      break;
      
    case STATE_RETURN_HOME:
      if (handleReturnHome()) {
        systemState = STATE_IDLE;
      }
      break;
  }
  
  // Handle serial commands
  handleSerial();
} 

//* ************************************************************************
//* ************************ UTILITY FUNCTIONS ****************************
//* ************************************************************************
bool isMotorAtTarget(FastAccelStepper* motor) {
  if (!motor) return true;
  return !motor->isRunning();
}

bool waitForTime(unsigned long duration) {
  if (stateTimer == 0) {
    stateTimer = millis();
    return false;
  }
  
  if (millis() - stateTimer >= duration) {
    stateTimer = 0;
    return true;
  }
  
  return false;
}

void activateVacuum() {
  digitalWrite((int)SOLENOID_RELAY_PIN, HIGH);
  vacuumActive = true;
}

void deactivateVacuum() {
  digitalWrite((int)SOLENOID_RELAY_PIN, LOW);
  vacuumActive = false;
}

void enableXMotor() {
  if (xStepper) {
    xStepper->enableOutputs();
  }
}

void disableXMotor() {
  if (xStepper) {
    xStepper->disableOutputs();
  }
}

//* ************************************************************************
//* ************************ SERIAL COMMUNICATION **************************
//* ************************************************************************
void handleSerial() {
  // This function can be used to handle serial commands for debugging
  // For example, trigger states, print status, etc.
  // Currently empty to prevent interference with motor operation.
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    // Example: send 'H' to re-trigger homing
    if (command == "H") {
      systemState = STATE_HOMING;
    }
  }
} 