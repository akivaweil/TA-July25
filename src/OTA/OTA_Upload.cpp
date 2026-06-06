#include "OTA/OTA_Upload.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

// OTA upload implementation
// Barebones WiFi connection and Over-The-Air updates for the ESP32.

const char* ssid = "Everwood";
const char* password = "Everwood-Staff";

void setupOTA() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
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
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi unavailable — running standalone on saved settings.");
  }

  // Print IP address on startup
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("stage1-esp32s3");

  // STEP 1: Setup progress callbacks to reduce logging to 25% intervals
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Started");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Only log every 25% progress to reduce serial output
    static int lastLoggedProgress = -1;
    int currentProgress = (progress * 100) / total;
    
    if (currentProgress >= 25 && lastLoggedProgress < 25) {
      Serial.printf("OTA Progress: 25%%\n");
      lastLoggedProgress = 25;
    } else if (currentProgress >= 50 && lastLoggedProgress < 50) {
      Serial.printf("OTA Progress: 50%%\n");
      lastLoggedProgress = 50;
    } else if (currentProgress >= 75 && lastLoggedProgress < 75) {
      Serial.printf("OTA Progress: 75%%\n");
      lastLoggedProgress = 75;
    } else if (currentProgress >= 100 && lastLoggedProgress < 100) {
      Serial.printf("OTA Progress: 100%%\n");
      lastLoggedProgress = 100;
    }
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA Update Complete");
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error: %u\n", error);
  });
  
  ArduinoOTA.begin();
}

void handleOTA() {
  ArduinoOTA.handle();
} 