#include <Arduino.h>
#include "globals.h"

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