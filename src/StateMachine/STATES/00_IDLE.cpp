#include <Arduino.h>
#include "globals.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE CONFIG                                                  ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
const unsigned long IDLE_REHOME_TIMEOUT = 5000;  // Auto re-home after idle for this long (ms)

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ ⏸️  IDLE STATE                                                         ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// This state waits for trigger signals (start button or stage1 signal)
// When triggered, enables X motor and transitions to pickup sequence
// If idle longer than IDLE_REHOME_TIMEOUT, re-home the machine

bool handleIdle() {
  static unsigned long idleEnteredAt = 0;
  if (idleEnteredAt == 0) {
    idleEnteredAt = millis();
  }

  // Check for trigger signals using debounced inputs
  if (startButton.read() == HIGH ||
      stage1Signal.read() == HIGH) {
    idleEnteredAt = 0;
    return true;  // Start pickup sequence
  }

  // Auto re-home after idle timeout
  if (millis() - idleEnteredAt >= IDLE_REHOME_TIMEOUT) {
    idleEnteredAt = 0;
    systemState = STATE_HOMING;
  }

  return false;  // Stay in idle
} 