#include <Arduino.h>
#include "globals.h"

// External reference to state timer
extern unsigned long stateTimer;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE CONFIG                                                  ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
const int START_SIGNAL_DELAY = 325;   // Delay after start signal before beginning pick cycle (ms)
const unsigned long AUTO_HOME_IDLE_TIMEOUT = 10000;  // Auto-home if no start command after cycle complete (ms)

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE                                                         ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state waits for trigger signals (start button or stage1 signal)
// When triggered, waits for START_SIGNAL_DELAY before transitioning to pickup sequence

bool handleIdle() {
  const bool startCommandReceived = (startButton.read() == HIGH || stage1Signal.read() == HIGH);

  // Check for trigger signals using debounced inputs
  if (startCommandReceived) {
    // Start delay timer and wait for START_SIGNAL_DELAY
    if (waitForTime(START_SIGNAL_DELAY)) {
      autoHomePending = false;  // New cycle is starting, cancel post-cycle auto-home
      return true;  // Start pickup sequence after delay
    }
  } else {
    stateTimer = 0;  // Require a fresh, continuous start signal
  }

  // Run one-time auto-home if machine stays idle after a completed cycle
  if (autoHomePending && (millis() - lastCycleCompleteTime >= AUTO_HOME_IDLE_TIMEOUT)) {
    autoHomePending = false;  // One-shot behavior: do not repeatedly re-home
    systemState = STATE_HOMING;
  }
  
  return false;  // Stay in idle
} 