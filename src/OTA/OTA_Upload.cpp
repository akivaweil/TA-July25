#include "OTA/OTA_Upload.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "esp_task_wdt.h"

// OTA upload implementation
// Barebones WiFi connection and Over-The-Air updates for the ESP32.
//
// NOTE: This is the Transfer Arm (TA) firmware. The four line machines (TA .228,
// Router .250, Stage 1 .251, Stage 2 .253) are managed from the TA hub, but each
// has its OWN codebase and is flashed from there — this repo flashes TA only.
// See the IP/hostname/repo map in platformio.ini.

const char* ssid = "Everwood";
const char* password = "Everwood-Staff";

// Unique per-machine OTA/mDNS hostname. Must NOT collide with the other machines
// (Stage 1 uses "stage1-esp32s3") or OTA targeting/mDNS becomes ambiguous.
const char* OTA_HOSTNAME = "ta-esp32s3";

void setupOTA() {
  WiFi.mode(WIFI_STA);
  // Disable WiFi modem power-save. The Arduino default (WIFI_PS_MIN_MODEM) makes
  // the radio sleep between DTIM beacons, so a fresh client's first packets see
  // ~1s latency or get dropped entirely — desktop browsers retry through it, but
  // mobile Safari gives up and the dashboard appears unreachable from phones.
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  // Try to connect for a bounded time, then proceed regardless so the machine
  // still boots and runs on its locally-saved settings if the network/TA is
  // down. WiFi keeps retrying in the background after the loop falls through.
  const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
  const uint32_t WIFI_CONNECT_POLL_MS = 250;
  WiFi.setAutoReconnect(true);
  uint32_t wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - wifiConnectStart < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_CONNECT_POLL_MS);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[TA] wifi ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[TA] wifi unavailable - standalone");
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);

  ArduinoOTA.onStart([]() {
    Serial.println("[TA] OTA start");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Feed the watchdog: ArduinoOTA.handle() blocks for the whole upload in one
    // loop iteration, so without this a large/slow upload would trip the WDT.
    esp_task_wdt_reset();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("[TA] OTA done");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[TA] OTA error %u\n", error);
  });
  
  ArduinoOTA.begin();
}

void handleOTA() {
  ArduinoOTA.handle();
} 