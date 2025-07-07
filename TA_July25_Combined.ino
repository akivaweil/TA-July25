#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Bounce2.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>

// --- START OF FILE: include/config/Config.h ---
#ifndef CONFIG_H
#define CONFIG_H

//* ************************************************************************
//* ************************* CONFIGURATION *******************************
//* ************************************************************************
// Updated to match original Transfer-Arm_TA-June25 project values

//* ************************************************************************
//* ************************ MECHANICAL SETTINGS **************************
//* ************************************************************************
extern int STEPS_PER_REV;   // Steps per revolution (1.8° with 1/2 microstepping)
extern int PULLEY_TEETH;     // Number of teeth on pulleys
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
extern int X_HOME_POS;
extern int Z_HOME_POS;
extern int X_PICKUP_POS;
extern int X_DROPOFF_POS;
extern int X_OVERSHOOT_POS;

// Z-axis positions (original coordinate system: Z_UP = 0, positive = down)
extern int Z_UP_POS;     // Z-axis fully up position
extern int Z_PICKUP_POS;     // Z down for pickup
extern int Z_DROPOFF_POS;   // Z down for dropoff
extern int Z_SUCTION_START_POS; // Z position to start suction

//* ************************************************************************
//* ************************ SERVO SETTINGS ********************************
//* ************************************************************************
// Servo settings (in degrees)
extern int SERVO_HOME_POS;      // Neutral position
extern int SERVO_PICKUP_POS;    // Pickup orientation
extern int SERVO_TRAVEL_POS;     // Travel position
extern int SERVO_DROPOFF_POS;   // Dropoff orientation

//* ************************************************************************
//* ************************ TIMING SETTINGS *******************************
//* ************************************************************************
// Timing settings (in milliseconds)
extern int PICKUP_HOLD_TIME;     // Hold time at pickup position
extern int DROPOFF_HOLD_TIME;    // Hold time at dropoff position
extern int SERVO_ROTATION_TIME;  // Wait time for servo rotation

//* ************************************************************************
//* ************************ STEPPER MOTOR SETTINGS ***********************
//* ************************************************************************
extern int X_MAX_SPEED;  // Steps per second
extern int X_ACCELERATION; // Steps per second^2
extern int X_HOME_SPEED;    // Homing speed

extern int Z_MAX_SPEED; // Steps per second
extern int Z_ACCELERATION; // Steps per second^2
extern int Z_HOME_SPEED;    // Homing speed
extern int Z_DROPOFF_SPEED; // Dropoff speed

#endif 
// --- END OF FILE: include/config/Config.h ---

// --- START OF FILE: include/config/Pins_Definitions.h ---
#ifndef PINS_DEFINITIONS_H
#define PINS_DEFINITIONS_H

//* ************************************************************************
//* ************************ PIN DEFINITIONS ******************************
//* ************************************************************************
// Pin assignments for Freenove ESP32 board
// Updated to match original Transfer-Arm_TA-June25 project

// INPUT PINS (Active HIGH)
extern int START_BUTTON_PIN;     // Start button input (active high)
extern int STAGE1_SIGNAL_PIN;   // Stage 1 machine signal input (active high)
extern int X_HOME_SWITCH_PIN;   // X-axis home limit switch (active high)
extern int Z_HOME_SWITCH_PIN;   // Z-axis home limit switch (active high)
extern int STOP_SIGNAL_STAGE_2; // Stage 2 safety signal (active high, wait for low)

// OUTPUT PINS - STEPPER MOTORS
extern int X_STEP_PIN;          // X-axis stepper motor step pin
extern int X_DIR_PIN;           // X-axis stepper motor direction pin
extern int X_ENABLE_PIN;         // X-axis stepper motor enable pin (active low)
extern int Z_STEP_PIN;          // Z-axis stepper motor step pin
extern int Z_DIR_PIN;           // Z-axis stepper motor direction pin

// OUTPUT PINS - ACTUATORS
extern int SERVO_PIN;           // Servo control pin
extern int SOLENOID_RELAY_PIN;  // Solenoid relay control pin
extern int STAGE2_SIGNAL_PIN;   // Signal output to Stage 2 machine (active high)

#endif 
// --- END OF FILE: include/config/Pins_Definitions.h ---

// --- START OF FILE: include/globals.h ---
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Bounce2.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>

//* ************************************************************************
//* ************************ BOUNCE2 OBJECTS ******************************
//* ************************************************************************
// Bounce2 objects for debounced inputs
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;
extern Bounce startButton;
extern Bounce stage1Signal;
extern Bounce stopSignalStage2;

extern FastAccelStepperEngine engine;
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;

extern Servo swivelArmServo;

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

extern bool vacuumActive;

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
// --- END OF FILE: include/globals.h ---

// --- START OF FILE: src/config/Config.cpp ---
// #include "config/Config.h" // Removed for single-file compilation

//* ************************************************************************
//* ************************* CONFIGURATION *******************************
//* ************************************************************************
// Updated to match original Transfer-Arm_TA-June25 project values

//* ************************************************************************
//* ************************ MECHANICAL SETTINGS **************************
//* ************************************************************************
int STEPS_PER_REV = 400;   // Steps per revolution (1.8° with 1/2 microstepping)
int PULLEY_TEETH = 20;     // Number of teeth on pulleys
float BELT_PITCH = 2.0;        // GT2 belt pitch in mm
float STEPS_PER_MM = ((float)STEPS_PER_REV / (PULLEY_TEETH * BELT_PITCH));
float STEPS_PER_INCH = (STEPS_PER_MM * 25.4);

//* ************************************************************************
//* ************************ POSITION SETTINGS *****************************
//* ************************************************************************
// Position settings (in inches from home)
float X_PICKUP_INCHES = 1.0;     // X pickup position
float X_DROPOFF_INCHES = 20.95;  // X dropoff position (updated from original)
float X_OVERSHOOT_INCHES = (X_DROPOFF_INCHES + 1.75);  // 1.75" past dropoff for servo rotation

float Z_PICKUP_LOWER_INCHES = 7.0;     // Lower Z by 7" for pickup (updated from original)
float Z_DROPOFF_LOWER_INCHES = 5.5;    // Lower Z by 5.5" for dropoff
float Z_SUCTION_START_INCHES = 4.0;    // Start suction when 4" down

//* ************************************************************************
//* ************************ CONVERTED POSITIONS ***************************
//* ************************************************************************
// Converted positions (in steps)
int X_HOME_POS = 0;
int Z_HOME_POS = 0;
int X_PICKUP_POS = (int)(X_PICKUP_INCHES * STEPS_PER_INCH);
int X_DROPOFF_POS = (int)(X_DROPOFF_INCHES * STEPS_PER_INCH); 
int X_OVERSHOOT_POS = (int)(X_OVERSHOOT_INCHES * STEPS_PER_INCH);

// Z-axis positions (original coordinate system: Z_UP = 0, positive = down)
int Z_UP_POS = 0;     // Z-axis fully up position
int Z_PICKUP_POS = (int)(Z_PICKUP_LOWER_INCHES * STEPS_PER_INCH);     // Z down for pickup
int Z_DROPOFF_POS = (int)(Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH);   // Z down for dropoff
int Z_SUCTION_START_POS = (int)(Z_SUCTION_START_INCHES * STEPS_PER_INCH); // Z position to start suction

//* ************************************************************************
//* ************************ SERVO SETTINGS ********************************
//* ************************************************************************
// Servo settings (in degrees)
int SERVO_HOME_POS = 90;      // Neutral position
int SERVO_PICKUP_POS = 10;    // Pickup orientation (updated from original)
int SERVO_TRAVEL_POS = 0;     // Travel position (updated from original)
int SERVO_DROPOFF_POS = 80;   // Dropoff orientation (updated from original)

//* ************************************************************************
//* ************************ TIMING SETTINGS *******************************
//* ************************************************************************
// Timing settings (in milliseconds)  
int PICKUP_HOLD_TIME = 300;     // Hold time at pickup position
int DROPOFF_HOLD_TIME = 100;    // Hold time at dropoff position
int SERVO_ROTATION_TIME = 500;  // Wait time for servo rotation

//* ************************************************************************
//* ************************ STEPPER MOTOR SETTINGS ***********************
//* ************************************************************************
// Stepper motor settings (updated from original)
int X_MAX_SPEED = 7000;  // Steps per second
int X_ACCELERATION = 10000; // Steps per second^2
int X_HOME_SPEED = 1000;    // Homing speed

int Z_MAX_SPEED = 10000; // Steps per second
int Z_ACCELERATION = 10000; // Steps per second^2
int Z_HOME_SPEED = 1000;    // Homing speed
int Z_DROPOFF_SPEED = 10000; // Same speed as normal for dropoff 
// --- END OF FILE: src/config/Config.cpp ---

// --- START OF FILE: src/config/Pins_Definitions.cpp ---
// #include "config/Pins_Definitions.h" // Removed for single-file compilation

//* ************************************************************************
//* ************************ PIN DEFINITIONS ******************************
//* ************************************************************************
// Pin assignments for Freenove ESP32 board
// Updated to match original Transfer-Arm_TA-June25 project

// INPUT PINS (Active HIGH)
int START_BUTTON_PIN = 2;     // Start button input (active high)
int STAGE1_SIGNAL_PIN = 23;   // Stage 1 machine signal input (active high)
int X_HOME_SWITCH_PIN = 15;   // X-axis home limit switch (active high)
int Z_HOME_SWITCH_PIN = 21;   // Z-axis home limit switch (active high)
int STOP_SIGNAL_STAGE_2 = 22; // Stage 2 safety signal (active high, wait for low)

// OUTPUT PINS - STEPPER MOTORS
int X_STEP_PIN = 27;          // X-axis stepper motor step pin
int X_DIR_PIN = 14;           // X-axis stepper motor direction pin
int X_ENABLE_PIN = 4;         // X-axis stepper motor enable pin (active low)
int Z_STEP_PIN = 19;          // Z-axis stepper motor step pin
int Z_DIR_PIN = 18;           // Z-axis stepper motor direction pin

// OUTPUT PINS - ACTUATORS
int SERVO_PIN = 12;           // Servo control pin
int SOLENOID_RELAY_PIN = 33;  // Solenoid relay control pin
int STAGE2_SIGNAL_PIN = 25;   // Signal output to Stage 2 machine (active high) 
// --- END OF FILE: src/config/Pins_Definitions.cpp ---

// --- START OF FILE: src/main.cpp ---
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>

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
  pinMode(START_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(STAGE1_SIGNAL_PIN, INPUT_PULLDOWN);
  pinMode(X_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode(Z_HOME_SWITCH_PIN, INPUT_PULLDOWN);
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
  // Configure limit switches with 2ms debounce (same as Transfer-Arm_TA-June25)
  xHomeSwitch.attach(X_HOME_SWITCH_PIN);
  xHomeSwitch.interval(2);  // 2ms debounce
  
  zHomeSwitch.attach(Z_HOME_SWITCH_PIN);
  zHomeSwitch.interval(2);  // 2ms debounce
  
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
    xStepper->setSpeedInHz((uint32_t)X_MAX_SPEED);
    xStepper->setAcceleration((uint32_t)X_ACCELERATION);
  }
  
  zStepper = engine.stepperConnectToPin(Z_STEP_PIN);
  if (zStepper) {
    zStepper->setDirectionPin(Z_DIR_PIN);
    zStepper->setSpeedInHz((uint32_t)Z_MAX_SPEED);
    zStepper->setAcceleration((uint32_t)Z_ACCELERATION);
  }
  
  vacuumActive = false;
}

/*
void setupServo() {
  swivelArmServo.attach(SERVO_PIN);
  swivelArmServo.write(SERVO_HOME_POS);
  // Note: Removed blocking delay for smooth stepper operation
}
*/

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
// --- END OF FILE: src/main.cpp ---

// --- START OF FILE: src/OTA_Manager.cpp ---
/*
 * OTA_Manager.cpp - Unified Over-The-Air Update Manager for ESP32
 * 
 * This file contains ALL OTA functionality in one place:
 * - WiFi configuration constants
 * - OTA configuration constants  
 * - All OTA function implementations
 * 
 * SETUP INSTRUCTIONS:
 * 1. Include this file in your main.cpp
 * 2. Call initOTA() in setup()
 * 3. Call handleOTA() in loop()
 * 4. Update IP address in platformio.ini to match your ESP32's IP
 * 
 * USAGE:
 * - Use 'pio run -t upload' for OTA uploads
 * - Monitor serial output to see IP address assigned to ESP32
 * - Update platformio.ini upload_port with the assigned IP
 * 
 * NETWORK REQUIREMENTS:
 * - ESP32 and computer must be on same network
 * - Port 3232 must be open for OTA communication
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

//* ************************************************************************
//* ************************ OTA CONFIGURATION ***************************
//* ************************************************************************
// WiFi Configuration
const char* WIFI_SSID = "Everwood";
const char* WIFI_PASSWORD = "Everwood-Staff";

// OTA Configuration
const char* OTA_HOSTNAME = "transfer-arm";
const char* OTA_PASSWORD = "transfer-arm-ota";
const float OTA_PORT = 3232.0;

// Connection timeouts
const unsigned long WIFI_TIMEOUT = 30000;  // 30 seconds
const unsigned long OTA_TIMEOUT = 10000;   // 10 seconds

//* ************************************************************************
//* ************************ WIFI CONNECTION FUNCTIONS ******************
//* ************************************************************************

void initWiFi() {
  Serial.println("\n=== ESP32 OTA Remote Upload Setup ===");
  
  //! Step 1: Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

//* ************************************************************************
//* ************************ OTA SETUP FUNCTIONS ************************
//* ************************************************************************

void initOTA() {
  // Initialize WiFi first
  initWiFi();
  
  //! Step 2: Configure OTA
  ArduinoOTA.setHostname("ESP32-Remote");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  //! Step 3: Start OTA service
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.println("Device ready for remote uploads!");
  Serial.print("Use IP: ");
  Serial.println(WiFi.localIP());
}

//* ************************************************************************
//* ************************ OTA RUNTIME FUNCTIONS **********************
//* ************************************************************************

void handleOTA() {
  //! Handle OTA updates
  ArduinoOTA.handle();
}

void displayIP() {
  //! Display IP every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
    lastPrint = millis();
  }
} 
// --- END OF FILE: src/OTA_Manager.cpp ---

// --- START OF FILE: src/StateMachine/FUNCTIONS/Motors.cpp ---
// This file will contain functions for controlling the stepper motors.
// #include "globals.h" // Removed for single-file compilation

bool isMotorAtTarget(FastAccelStepper* motor) {
  if (!motor) return true;
  return !motor->isRunning();
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
// --- END OF FILE: src/StateMachine/FUNCTIONS/Motors.cpp ---

// --- START OF FILE: src/StateMachine/FUNCTIONS/Pneumatics.cpp ---
// This file will contain functions for controlling the pneumatics (solenoid).
// #include "globals.h" // Removed for single-file compilation

void activateVacuum() {
  digitalWrite(SOLENOID_RELAY_PIN, HIGH);
  vacuumActive = true;
}

void deactivateVacuum() {
  digitalWrite(SOLENOID_RELAY_PIN, LOW);
  vacuumActive = false;
} 
// --- END OF FILE: src/StateMachine/FUNCTIONS/Pneumatics.cpp ---

// --- START OF FILE: src/StateMachine/FUNCTIONS/Sensors.cpp ---
// This file will contain functions for reading sensors and inputs.
// #include "globals.h" // Removed for single-file compilation
// --- END OF FILE: src/StateMachine/FUNCTIONS/Sensors.cpp ---

// --- START OF FILE: src/StateMachine/FUNCTIONS/Servo.cpp ---
// This file will contain functions for controlling the servo motor.
// #include "globals.h" // Removed for single-file compilation
#include <ESP32Servo.h>

// Servo swivelArmServo; // Definition is in main.cpp

void setupServo() {
  swivelArmServo.attach(SERVO_PIN);
  swivelArmServo.write(SERVO_HOME_POS);
  // Note: Removed blocking delay for smooth stepper operation
} 
// --- END OF FILE: src/StateMachine/FUNCTIONS/Servo.cpp ---

// --- START OF FILE: src/StateMachine/STATES/00_IDLE.cpp ---
#include <Arduino.h>
// #include "globals.h" // Removed for single-file compilation

//* ************************************************************************
//* ************************ IDLE STATE ************************************
//* ************************************************************************
// This state waits for trigger signals (start button or stage1 signal)
// When triggered, enables X motor and transitions to pickup sequence

bool handleIdle() {
  // Check for trigger signals using debounced inputs
  if (startButton.read() == HIGH || 
      stage1Signal.read() == HIGH) {
    return true;  // Start pickup sequence
  }
  
  return false;  // Stay in idle
} 
// --- END OF FILE: src/StateMachine/STATES/00_IDLE.cpp ---

// --- START OF FILE: src/StateMachine/STATES/01_HOMING.cpp ---
#include <Arduino.h>
#include <FastAccelStepper.h>
// #include "globals.h" // Removed for single-file compilation

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern Bounce zHomeSwitch;
extern Bounce xHomeSwitch;

//* ************************************************************************
//* ************************ HOMING STATE **********************************
//* ************************************************************************
// This state homes both Z and X axes sequentially
// Z axis homes first, then moves up, then X axis homes and moves to pickup

bool handleHoming() {
  static int homingStep = 0;
  
  switch(homingStep) {
    case 0:  // Start Z homing
      if (zStepper) {
        zStepper->setSpeedInHz((uint32_t)Z_HOME_SPEED);
        zStepper->move(-50000);  // Move negative direction
      }
      homingStep = 1;
      break;
      
    case 1:  // Wait for Z home switch
      if (zHomeSwitch.read() == HIGH) {
        if (zStepper) {
          zStepper->forceStop();
          zStepper->setCurrentPosition((int32_t)Z_HOME_POS);
          zStepper->setSpeedInHz((uint32_t)Z_MAX_SPEED);
          zStepper->moveTo((int32_t)Z_UP_POS);  // Move up 5 inches
        }
        homingStep = 2;
      }
      break;
      
    case 2:  // Wait for Z to reach up position
      if (isMotorAtTarget(zStepper)) {
        if (xStepper) {
          xStepper->setSpeedInHz((uint32_t)X_HOME_SPEED);
          xStepper->move(-50000);  // Move negative direction
        }
        homingStep = 3;
      }
      break;
      
    case 3:  // Wait for X home switch
      if (xHomeSwitch.read() == HIGH) {
        if (xStepper) {
          xStepper->forceStop();
          xStepper->setCurrentPosition((int32_t)X_HOME_POS);
          xStepper->setSpeedInHz((uint32_t)X_MAX_SPEED);
          xStepper->moveTo((int32_t)X_PICKUP_POS);  // Move to pickup
        }
        homingStep = 4;
      }
      break;
      
    case 4:  // Wait for X to reach pickup
      if (isMotorAtTarget(xStepper)) {
        homingStep = 0;   // Reset for next homing
        return true;      // Homing complete
      }
      break;
  }
  
  return false;  // Homing not complete
} 
// --- END OF FILE: src/StateMachine/STATES/01_HOMING.cpp ---

// --- START OF FILE: src/StateMachine/STATES/02_PICKUP.cpp ---
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
// #include "globals.h" // Removed for single-file compilation

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern FastAccelStepper *zStepper;
extern Servo swivelArmServo;
extern unsigned long stateTimer;
extern bool vacuumActive;

//* ************************************************************************
//* ************************ PICKUP STATE **********************************
//* ************************************************************************
// This state handles the pickup sequence:
// Move X to pickup position, set servo, lower Z, activate vacuum, wait, raise Z

bool handlePickup() {
  switch(pickupState) {
    case PICKUP_MOVE_X:
      if (xStepper) {
        xStepper->moveTo((int32_t)X_PICKUP_POS);
      }
      if (isMotorAtTarget(xStepper)) {
        swivelArmServo.write((int)SERVO_PICKUP_POS);
        pickupState = PICKUP_LOWER_Z;
      }
      break;
      
    case PICKUP_LOWER_Z:
      if (zStepper) {
        zStepper->moveTo((int32_t)Z_PICKUP_POS);
        // Activate vacuum when halfway down
        if (zStepper->getCurrentPosition() <= (int32_t)Z_SUCTION_START_POS && !vacuumActive) {
          activateVacuum();
        }
      }
      if (isMotorAtTarget(zStepper)) {
        pickupState = PICKUP_WAIT;
      }
      break;
      
    case PICKUP_WAIT:
      if (waitForTime((unsigned long)PICKUP_HOLD_TIME)) {
        if (zStepper) {
          zStepper->moveTo((int32_t)Z_UP_POS);
        }
        pickupState = PICKUP_RAISE_Z;
      }
      break;
      
    case PICKUP_RAISE_Z:
      if (isMotorAtTarget(zStepper)) {
        pickupState = PICKUP_DONE;
      }
      break;
      
    case PICKUP_DONE:
      pickupState = PICKUP_MOVE_X;  // Reset for next cycle
      return true;  // Pickup complete
  }
  
  return false;  // Pickup not complete
} 
// --- END OF FILE: src/StateMachine/STATES/02_PICKUP.cpp ---

// --- START OF FILE: src/StateMachine/STATES/03_TRANSPORT.cpp ---
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
// #include "globals.h" // Removed for single-file compilation

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern Servo swivelArmServo;
extern unsigned long stateTimer;

//* ************************************************************************
//* ************************ TRANSPORT STATE *******************************
//* ************************************************************************
// This state handles transporting the object from pickup to dropoff:
// Rotate servo, move to overshoot, rotate servo again, move to dropoff

bool handleTransport() {
  switch(transportState) {
    case TRANSPORT_ROTATE_SERVO:
      swivelArmServo.write((int)SERVO_TRAVEL_POS);
      if (xStepper) {
        xStepper->moveTo((int32_t)X_OVERSHOOT_POS);
      }
      transportState = TRANSPORT_MOVE_TO_OVERSHOOT;
      break;
      
    case TRANSPORT_MOVE_TO_OVERSHOOT:
      if (isMotorAtTarget(xStepper)) {
        swivelArmServo.write((int)SERVO_DROPOFF_POS);
        transportState = TRANSPORT_WAIT_SERVO;
      }
      break;
      
    case TRANSPORT_WAIT_SERVO:
      if (waitForTime((unsigned long)SERVO_ROTATION_TIME)) {
        if (xStepper) {
          xStepper->moveTo((int32_t)X_DROPOFF_POS);
        }
        transportState = TRANSPORT_MOVE_TO_DROPOFF;
      }
      break;
      
    case TRANSPORT_MOVE_TO_DROPOFF:
      if (isMotorAtTarget(xStepper)) {
        transportState = TRANSPORT_DONE;
      }
      break;
      
    case TRANSPORT_DONE:
      transportState = TRANSPORT_ROTATE_SERVO;  // Reset for next cycle
      return true;  // Transport complete
  }
  
  return false;  // Transport not complete
} 
// --- END OF FILE: src/StateMachine/STATES/03_TRANSPORT.cpp ---

// --- START OF FILE: src/StateMachine/STATES/04_DROPOFF.cpp ---
#include <Arduino.h>
#include <FastAccelStepper.h>
// #include "globals.h" // Removed for single-file compilation

// External references to objects defined in main file
extern FastAccelStepper *zStepper;
extern unsigned long stateTimer;
extern Bounce stopSignalStage2;

//* ************************************************************************
//* ************************ DROPOFF STATE *********************************
//* ************************************************************************
// This state handles dropping off the object:
// Check safety signal, lower Z, release vacuum, wait, raise Z, signal Stage 2

bool handleDropoff() {
  switch(dropoffState) {
    case DROPOFF_LOWER_Z:
      // Check safety signal before lowering (using debounced input)
      if (stopSignalStage2.read() == HIGH) {
        return false;  // Wait for safety signal to go low
      }
      
      if (zStepper) {
        zStepper->setSpeedInHz((uint32_t)Z_DROPOFF_SPEED);  // Slower for dropoff
        zStepper->moveTo((int32_t)Z_DROPOFF_POS);
      }
      if (isMotorAtTarget(zStepper)) {
        deactivateVacuum();
        dropoffState = DROPOFF_RELEASE;
      }
      break;
      
    case DROPOFF_RELEASE:
      dropoffState = DROPOFF_WAIT;
      break;
      
    case DROPOFF_WAIT:
      if (waitForTime((unsigned long)DROPOFF_HOLD_TIME)) {
        if (zStepper) {
          zStepper->setSpeedInHz((uint32_t)Z_MAX_SPEED);  // Back to normal speed
          zStepper->moveTo((int32_t)Z_UP_POS);
        }
        dropoffState = DROPOFF_RAISE_Z;
      }
      break;
      
    case DROPOFF_RAISE_Z:
      if (isMotorAtTarget(zStepper)) {
        digitalWrite((int)STAGE2_SIGNAL_PIN, HIGH);  // Signal Stage 2
        dropoffState = DROPOFF_DONE;
      }
      break;
      
    case DROPOFF_DONE:
      dropoffState = DROPOFF_LOWER_Z;  // Reset for next cycle
      return true;  // Dropoff complete
  }
  
  return false;  // Dropoff not complete
} 
// --- END OF FILE: src/StateMachine/STATES/04_DROPOFF.cpp ---

// --- START OF FILE: src/StateMachine/STATES/05_RETURN_HOME.cpp ---
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
// #include "globals.h" // Removed for single-file compilation

// External references to objects defined in main file
extern FastAccelStepper *xStepper;
extern Servo swivelArmServo;

//* ************************************************************************
//* ************************ RETURN HOME STATE *****************************
//* ************************************************************************
// This state returns the system to home position:
// Turn off Stage 2 signal, reset servo, move X home, then back to pickup position

bool handleReturnHome() {
  static int returnStep = 0;
  
  switch(returnStep) {
    case 0:  // Signal Stage 2 and move X home
      digitalWrite((int)STAGE2_SIGNAL_PIN, LOW);  // Turn off Stage 2 signal
      swivelArmServo.write((int)SERVO_HOME_POS);    // Reset servo
      if (xStepper) {
        xStepper->moveTo((int32_t)X_HOME_POS);           // Move X home
      }
      returnStep = 1;
      break;
      
    case 1:  // Wait for X to reach home
      if (isMotorAtTarget(xStepper)) {
        if (xStepper) {
          xStepper->moveTo((int32_t)X_PICKUP_POS);
        }
        returnStep = 2;
      }
      break;
      
    case 2:  // Wait for X to reach pickup
      if (isMotorAtTarget(xStepper)) {
        returnStep = 0;   // Reset for next cycle
        return true;      // Return complete
      }
      break;
  }
  
  return false;  // Return not complete
} 
// --- END OF FILE: src/StateMachine/STATES/05_RETURN_HOME.cpp ---