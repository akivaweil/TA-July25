#include <Arduino.h>
#include "globals.h"

// External reference to state timer
extern unsigned long stateTimer;
extern unsigned long lastCycleEndTime;

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE CONFIG                                                  ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
const int START_SIGNAL_DELAY = 325;              // Delay after start signal before beginning pick cycle (ms)
const unsigned long MIN_CYCLE_INTERVAL_MS = 2000; // Minimum time between consecutive cycles to avoid slamming Stage 2

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE                                                         ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state waits for trigger signals (start button or stage1 signal)
// When triggered, waits for START_SIGNAL_DELAY before transitioning to pickup sequence
// Also enforces a minimum interval between consecutive cycles

bool handleIdle() {
  // Enforce minimum interval between cycles (skip on first cycle after boot/homing)
  if (lastCycleEndTime != 0 && (millis() - lastCycleEndTime) < MIN_CYCLE_INTERVAL_MS) {
    stateTimer = 0;  // Reset start delay timer while waiting on cooldown
    return false;
  }

  const bool startCommandReceived = (startButton.read() == HIGH || stage1Signal.read() == HIGH);

  // Check for trigger signals using debounced inputs
  if (startCommandReceived) {
    // Start delay timer and wait for START_SIGNAL_DELAY
    if (waitForTime(START_SIGNAL_DELAY)) {
      return true;  // Start pickup sequence after delay
    }
  } else {
    stateTimer = 0;  // Require a fresh, continuous start signal
  }

  return false;  // Stay in idle
} 