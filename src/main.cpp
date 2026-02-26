#include <Arduino.h>
#include <FastAccelStepper.h>
#include "ServoControl.h"
#include <Bounce2.h>
#include "globals.h"
#include "OTA/OTA_Upload.h"
#include "WebServer.h"

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
// void initOTA();
// void handleOTA();

//* ************************************************************************
//* ************************ HARDWARE OBJECTS *****************************
//* ************************************************************************
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *xStepper = NULL;
FastAccelStepper *zStepper = NULL;
ServoControl swivelArmServo;

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
unsigned long lastCycleCompleteTime = 0;
bool autoHomePending = false;

// Timing variables
unsigned long stateTimer = 0;
bool vacuumActive = false;

//* ************************************************************************
//* ************************ SETUP FUNCTION ********************************
//* ************************************************************************
void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
  delay(100);
  
  // Initialize OTA functionality
  setupOTA();
  
  // Configure pins
  setupPins();
  
  // Configure debouncers
  setupDebouncers();
  
  // Configure steppers
  setupSteppers();
  
  // Configure servo
  setupServo();

  // Start Web Server
  setupWebServer();
  
  systemState = STATE_HOMING;
}

//* ************************************************************************
//* ************************ SETUP FUNCTIONS *******************************
//* ************************************************************************
void setupPins() {
  // Input pins
  pinMode(START_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(STAGE1_SIGNAL_PIN, INPUT_PULLDOWN);
  pinMode(X_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode(Z_HOME_SWITCH_PIN, INPUT);  // Z-axis: direct high reading, no pull-down
  pinMode(STOP_SIGNAL_STAGE_2, INPUT_PULLDOWN);
  
  // Output pins
  pinMode(X_ENABLE_PIN, OUTPUT);
  pinMode(SOLENOID_RELAY_PIN, OUTPUT);
  pinMode(STAGE2_SIGNAL_PIN, OUTPUT);
  
  // Initial states
  digitalWrite(X_ENABLE_PIN, LOW);   // Enable X motor for smooth operation
  digitalWrite(SOLENOID_RELAY_PIN, LOW);  // Vacuum off
  digitalWrite(STAGE2_SIGNAL_PIN, LOW);   // Stage 2 signal off
}

void setupDebouncers() {
  // X-axis: no debounce - direct read
  
  // Z-axis: 3ms debounce for reliable homing
  zHomeSwitch.attach(Z_HOME_SWITCH_PIN);
  zHomeSwitch.interval(3);  // 3ms debounce
  
  // Configure input signals with 10ms debounce
  startButton.attach(START_BUTTON_PIN);
  startButton.interval(10);  // 10ms debounce
  
  stage1Signal.attach(STAGE1_SIGNAL_PIN);
  stage1Signal.interval(10);  // 10ms debounce
  
  stopSignalStage2.attach(STOP_SIGNAL_STAGE_2);
  stopSignalStage2.interval(10);  // 10ms debounce
}

void setupSteppers() {
  engine.init();
  
  xStepper = engine.stepperConnectToPin(X_STEP_PIN);
  if (xStepper) {
    xStepper->setDirectionPin(X_DIR_PIN);
    xStepper->setEnablePin(X_ENABLE_PIN);
    xStepper->setAutoEnable(true);
    xStepper->setSpeedInHz(X_MAX_SPEED);
    xStepper->setAcceleration(X_ACCELERATION);
  }
  
  zStepper = engine.stepperConnectToPin(Z_STEP_PIN);
  if (zStepper) {
    zStepper->setDirectionPin(Z_DIR_PIN);
    zStepper->setSpeedInHz(Z_MAX_SPEED);
    zStepper->setAcceleration(Z_ACCELERATION);
  }
  
  vacuumActive = false;
}

//* ************************************************************************
//* ************************ MAIN LOOP - STATE MACHINE ********************
//* ************************************************************************
void loop() {
  // Handle OTA updates only in IDLE state
  if (systemState == STATE_IDLE) {
    handleOTA();
  }
  
  // Update all debouncers first
  zHomeSwitch.update();  // Z-axis: 3ms debounce
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
        lastCycleCompleteTime = millis();
        autoHomePending = true;
        systemState = STATE_IDLE;  // Go idle and wait for next start command
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
  
  // Update dashboard status
  updateDashboardStatus();
  
  // Yield to other tasks to reduce CPU load
  delay(1);
} 

//* ************************************************************************
//* ************************ UTILITY FUNCTIONS ****************************
//* ************************************************************************
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

//* ************************************************************************
//* ************************ SERIAL HANDLER *******************************
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