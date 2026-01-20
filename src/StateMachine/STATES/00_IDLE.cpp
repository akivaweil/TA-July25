#include <Arduino.h>
#include "globals.h"

// External reference to state timer
extern unsigned long stateTimer;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE CONFIG                                                  ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
const int START_SIGNAL_DELAY = 325;   // Delay after start signal before beginning pick cycle (ms)

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE                                                         ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state waits for trigger signals (start button or stage1 signal)
// When triggered, waits for START_SIGNAL_DELAY before transitioning to pickup sequence

bool handleIdle() {
  // Check for trigger signals using debounced inputs
  if (startButton.read() == HIGH || 
      stage1Signal.read() == HIGH) {
    // Start delay timer and wait for START_SIGNAL_DELAY
    if (waitForTime(START_SIGNAL_DELAY)) {
      return true;  // Start pickup sequence after delay
    }
  }
  
  return false;  // Stay in idle
} 