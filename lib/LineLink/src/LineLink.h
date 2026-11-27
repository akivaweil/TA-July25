#pragma once

// LineLink for the ESP32: wires the transport-independent Core to ESP-NOW, the
// Arduino clock and a spinlock, and runs Core::service() in its own task so
// heartbeats and ACKs keep flowing while a machine loop is busy.

#include "LineLinkCore.h"
#include "LineLinkFlashGate.h"

namespace linelink {

// The single Core instance used by the firmware (constructed on first use).
Core& link();

// Line-wide flash interlock bound to link(): update() every loop, call
// ArduinoOTA.handle() only while allowed(), setFlashing() from the OTA callbacks.
FlashGate& flashGate();

// Radio transmit power for esp_wifi_set_max_tx_power() (units of 0.25 dBm; the ESP32
// accepts 8..84). It also caps the machine's normal WiFi. The default is the maximum.
const int8_t TX_POWER_MAX = 84;   // 21 dBm

// Call once after WiFi is started in STA mode (setup, after the bounded connect).
// Initialises ESP-NOW, adds the broadcast peer and starts the LineLink task.
// Returns false if ESP-NOW could not start (the machine then runs without links
// and every peer reads as dead).
bool begin(uint8_t selfId, int8_t txPowerQuarterDbm = TX_POWER_MAX);

}  // namespace linelink
