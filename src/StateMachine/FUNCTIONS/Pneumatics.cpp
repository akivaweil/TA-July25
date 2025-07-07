// This file will contain functions for controlling the pneumatics (solenoid).
#include "globals.h"

void activateVacuum() {
  digitalWrite(SOLENOID_RELAY_PIN, HIGH);
  vacuumActive = true;
}

void deactivateVacuum() {
  digitalWrite(SOLENOID_RELAY_PIN, LOW);
  vacuumActive = false;
} 