#include "OTA_Upload.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

//* ************************************************************************
//* *********************** OTA UPLOAD IMPLEMENTATION *********************
//* ************************************************************************
// Barebones WiFi connection and Over-The-Air updates for the ESP32.

const char* ssid = "Everwood";
const char* password = "Everwood-Staff";

void setupOTA() {
  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                       ESP32-S3 STARTUP                      ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.println("Attempting connection...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Try to connect for up to 30 seconds
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
    if (attempts % 5 == 0) {  // Print status every 5 seconds
      Serial.printf(" (attempt %d/30) ", attempts);
    }
  }
  Serial.println(); // New line after dots

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ WiFi connection FAILED after 30 seconds!");
    Serial.println("Possible issues:");
    Serial.println("- Wrong WiFi credentials");
    Serial.println("- WiFi network not available");
    Serial.println("- ESP32-S3 not in range of WiFi");
    Serial.println("- Wrong WiFi channel/frequency");
    Serial.println("\nRestarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }

  // Print IP address on startup with clear banner
  Serial.println("\n✅ WiFi connected successfully!");
  Serial.println("╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                     ESP32-S3 IP ADDRESS                     ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Signal Strength: " + String(WiFi.RSSI()) + " dBm");
  Serial.println();
  Serial.println("🎯 Use the IP address above for OTA updates!");
  Serial.println("   Example: pio run -e esp32_s3_ota -t upload --upload-port 192.168.1.XXX");
  Serial.println();

  ArduinoOTA.setHostname("stage1-esp32s3");
  
  //! ************************************************************************
  //! STEP 1: SETUP PROGRESS CALLBACKS TO REDUCE LOGGING TO 25% INTERVALS
  //! ************************************************************************
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

void printIPAddress() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 30000) { // Print every 30 seconds
    Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                     ESP32-S3 IP ADDRESS                     ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("^ Use this IP for OTA updates ^");
    Serial.println();
    lastPrint = millis();
  }
} 