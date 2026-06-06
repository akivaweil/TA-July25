#ifndef MACHINE_CONFIG_API_H
#define MACHINE_CONFIG_API_H

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🌐 MACHINE CONFIG API (shared cross-machine REST contract)           ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝
// Implements the canonical dashboard REST contract for this machine (TA):
//   GET  /api/status   live read-only status + flat sensors map
//   GET  /api/config   self-describing curated settings (fields[])
//   POST /api/config   validate + persist + apply-or-defer
// All /api/* responses carry Access-Control-Allow-Origin: *.

#include <Arduino.h>

class AsyncWebServer;

// Register the three /api routes (and the dashboard "/" route) on the server.
void setupConfigApi(AsyncWebServer& server);

// Create the global AsyncWebServer on port 80, register routes, and begin().
// Call from setup() AFTER setupOTA() (WiFi must already be connected).
void setupWebServer();

// Body builders / POST core (kept identical in name across all four repos).
String buildStatusJson();                                                  // GET /api/status body
String buildConfigJson();                                                  // GET /api/config body
bool   applyConfigJson(const String& body, bool& outDeferred, String& outMsg); // POST core
bool   isSafeToApplyConfig();                                              // true only in the motionless IDLE state

// Set true by a deferred POST; the main loop applies it on next IDLE entry.
extern volatile bool configDirty;

#endif // MACHINE_CONFIG_API_H
