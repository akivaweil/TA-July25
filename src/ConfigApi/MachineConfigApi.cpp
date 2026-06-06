#include "MachineConfigApi.h"
#include "MachineSettings.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "globals.h"
#include "config/Pins_Definitions.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🌐 MACHINE CONFIG API IMPLEMENTATION                                 ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝

extern const char DASHBOARD_HTML[] PROGMEM;  // src/WebDashboard/dashboard_html.inc

// Curated setting whose definition stays in Config.cpp.
extern int X_MAX_SPEED;

// Set by a deferred POST; main loop applies on next IDLE entry.
volatile bool configDirty = false;

//* ************************************************************************
//* ******************** MACHINE IDENTITY ********************************
//* ************************************************************************
static const char* MACHINE_ID   = "ta";
static const char* MACHINE_NAME = "Transfer Arm";

//* ************************************************************************
//* ******************** CURATED FIELD TABLE *****************************
//* ************************************************************************
// Self-describing settings. Each field points at its live global so the GET
// reflects current values and the POST writes straight into runtime state.
enum FieldType { FT_INT, FT_FLOAT };

struct Field {
  const char* key;
  const char* label;
  FieldType   type;
  float       minV;
  float       maxV;
  float       step;
  int*        iPtr;     // valid when type == FT_INT
  float*      fPtr;     // valid when type == FT_FLOAT
  const char* nvsKey;   // NVS key used by MachineSettings persistence
};

static const Field FIELDS[] = {
  { "X_PICKUP_INCHES",       "X Pickup (in)",        FT_FLOAT, 0.1f,  5.0f,   0.05f, nullptr, &X_PICKUP_INCHES,        "xPickIn" },
  { "Z_PICKUP_LOWER_INCHES", "Z Pickup Lower (in)",  FT_FLOAT, 3.0f,  10.0f,  0.1f,  nullptr, &Z_PICKUP_LOWER_INCHES,  "zPickLowIn" },
  { "SERVO_PICKUP_POS",      "Servo Pickup (deg)",   FT_INT,   0,     180,    1,     &SERVO_PICKUP_POS, nullptr,        "servoPick" },
  { "PICKUP_HOLD_TIME",      "Pickup Hold (ms)",     FT_INT,   10,    500,    10,    &PICKUP_HOLD_TIME, nullptr,        "pickHold" },
  { "X_DROPOFF_INCHES",      "X Dropoff (in)",       FT_FLOAT, 15.0f, 30.0f,  0.25f, nullptr, &X_DROPOFF_INCHES,       "xDropIn" },
  { "SERVO_TRAVEL_POS",      "Servo Travel (deg)",   FT_INT,   0,     180,    1,     &SERVO_TRAVEL_POS, nullptr,        "servoTravel" },
  { "SERVO_DROPOFF_POS",     "Servo Dropoff (deg)",  FT_INT,   0,     180,    1,     &SERVO_DROPOFF_POS, nullptr,       "servoDrop" },
  { "Z_DROPOFF_LOWER_INCHES","Z Dropoff Lower (in)", FT_FLOAT, 3.0f,  10.0f,  0.1f,  nullptr, &Z_DROPOFF_LOWER_INCHES, "zDropLowIn" },
  { "SERVO_HOME_POS",        "Servo Home (deg)",     FT_INT,   0,     180,    1,     &SERVO_HOME_POS, nullptr,         "servoHome" },
  { "DROPOFF_SETTLE_TIME",   "Dropoff Settle (ms)",  FT_INT,   10,    200,    5,     &DROPOFF_SETTLE_TIME, nullptr,     "dropSettle" },
  { "X_MAX_SPEED",           "X Max Speed (st/s)",   FT_INT,   1000,  15000,  100,   &X_MAX_SPEED, nullptr,            "xMaxSpeed" },
  { "Z_DROPOFF_SPEED",       "Z Dropoff Speed (st/s)",FT_INT,  5000,  20000,  500,   &Z_DROPOFF_SPEED, nullptr,        "zDropSpeed" },
};
static const size_t FIELD_COUNT = sizeof(FIELDS) / sizeof(FIELDS[0]);

static const Field* findField(const char* key) {
  for (size_t i = 0; i < FIELD_COUNT; i++) {
    if (strcmp(FIELDS[i].key, key) == 0) return &FIELDS[i];
  }
  return nullptr;
}

//* ************************************************************************
//* ******************** STATE NAME / HEALTH *****************************
//* ************************************************************************
static const char* stateName() {
  switch (systemState) {
    case STATE_IDLE:      return "IDLE";
    case STATE_HOMING:    return "HOMING";
    case STATE_PICKUP:    return "PICKUP";
    case STATE_TRANSPORT: return "TRANSPORT";
    case STATE_DROPOFF:   return "DROPOFF";
    default:              return "UNKNOWN";
  }
}

bool isSafeToApplyConfig() {
  // Only the truly-motionless IDLE state is safe. HOMING actively drives the
  // steppers (speed-sensitive limit-switch seeks), so a live apply mid-home
  // could retarget an in-flight move; defer during HOMING and every cycle state.
  return systemState == STATE_IDLE;
}

//* ************************************************************************
//* ******************** STATUS JSON *************************************
//* ************************************************************************
String buildStatusJson() {
  JsonDocument doc;
  doc["id"]       = MACHINE_ID;
  doc["name"]     = MACHINE_NAME;
  doc["state"]    = stateName();
  // WARNING until first homing completes (i.e. while still homing), else HEALTHY.
  doc["health"]   = (systemState == STATE_HOMING) ? "WARNING" : "HEALTHY";
  doc["uptimeMs"] = (uint32_t)millis();
  doc["freeHeap"] = (uint32_t)ESP.getFreeHeap();
  doc["rssi"]     = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

  JsonObject sensors = doc["sensors"].to<JsonObject>();
  sensors["xHome"]       = (digitalRead(X_HOME_SWITCH_PIN) == HIGH);
  sensors["zHome"]       = (zHomeSwitch.read() == HIGH);
  sensors["startButton"] = (startButton.read() == HIGH);
  sensors["stage1"]      = (stage1Signal.read() == HIGH);
  sensors["stopStage2"]  = (stopSignalStage2.read() == HIGH);
  sensors["vacuum"]      = vacuumActive;

  String out;
  serializeJson(doc, out);
  return out;
}

//* ************************************************************************
//* ******************** CONFIG JSON (GET) *******************************
//* ************************************************************************
String buildConfigJson() {
  JsonDocument doc;
  doc["id"]     = MACHINE_ID;
  doc["schema"] = 1;
  JsonArray fields = doc["fields"].to<JsonArray>();
  for (size_t i = 0; i < FIELD_COUNT; i++) {
    const Field& f = FIELDS[i];
    JsonObject o = fields.add<JsonObject>();
    o["key"]   = f.key;
    o["label"] = f.label;
    o["type"]  = (f.type == FT_INT) ? "int" : "float";
    if (f.type == FT_INT) {
      o["value"] = *f.iPtr;
      o["step"]  = (int)f.step;
      o["min"]   = (int)f.minV;
      o["max"]   = (int)f.maxV;
    } else {
      o["value"] = *f.fPtr;
      o["step"]  = f.step;
      o["min"]   = f.minV;
      o["max"]   = f.maxV;
    }
  }
  String out;
  serializeJson(doc, out);
  return out;
}

//* ************************************************************************
//* ******************** CONFIG POST CORE ********************************
//* ************************************************************************
// Returns true on success (HTTP 200). On false, outMsg holds the error and
// NOTHING is changed/persisted. On success, outDeferred reflects apply vs defer.
bool applyConfigJson(const String& body, bool& outDeferred, String& outMsg) {
  outDeferred = false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    outMsg = "invalid JSON";
    return false;
  }
  if (!doc.is<JsonObject>()) {
    outMsg = "expected JSON object";
    return false;
  }
  JsonObject obj = doc.as<JsonObject>();

  // PASS 1: validate every key is known and within [min,max]. Change nothing yet.
  for (JsonPair kv : obj) {
    const Field* f = findField(kv.key().c_str());
    if (!f) {
      outMsg = String(kv.key().c_str()) + " invalid/out of range";
      return false;
    }
    if (!kv.value().is<float>() && !kv.value().is<int>()) {
      outMsg = String(f->key) + " invalid/out of range";
      return false;
    }
    float v = kv.value().as<float>();
    if (v < f->minV || v > f->maxV) {
      outMsg = String(f->key) + " invalid/out of range";
      return false;
    }
    // Int-typed fields must receive integral values (no silent 52.9 -> 52 truncation).
    if (f->type == FT_INT && v != (float)(int)v) {
      outMsg = String(f->key) + " invalid/out of range";
      return false;
    }
  }

  // Decide the path BEFORE touching anything: only the motionless IDLE state may
  // write live runtime variables. On the deferred path we persist the new values
  // to NVS but leave EVERY live global untouched (the cycle/homing code reads
  // tunables continuously); the main loop copies persisted -> live and recomputes
  // on the next IDLE entry.
  const bool safe = isSafeToApplyConfig();

  if (safe) {
    // SAFE PATH: write live globals, persist them, then recompute derived state.
    for (JsonPair kv : obj) {
      const Field* f = findField(kv.key().c_str());
      if (f->type == FT_INT) {
        *f->iPtr = kv.value().as<int>();
      } else {
        *f->fPtr = kv.value().as<float>();
      }
    }
    saveSettings();      // persist live globals
    applyTASettings();   // recompute derived positions + re-apply motor speeds
    outDeferred = false;
    outMsg = "saved";
  } else {
    // DEFERRED PATH: persist staged values straight to NVS WITHOUT writing any
    // live global, then flag the main loop to load + apply on next IDLE entry.
    for (JsonPair kv : obj) {
      const Field* f = findField(kv.key().c_str());
      if (f->type == FT_INT) {
        persistSettingInt(f->nvsKey, kv.value().as<int>());
      } else {
        persistSettingFloat(f->nvsKey, kv.value().as<float>());
      }
    }
    configDirty = true;
    outDeferred = true;
    outMsg = "pending — applies at next idle";
  }
  return true;
}

//* ************************************************************************
//* ******************** ROUTE REGISTRATION ******************************
//* ************************************************************************
static void addCors(AsyncWebServerResponse* res) {
  res->addHeader("Access-Control-Allow-Origin", "*");
}

void setupConfigApi(AsyncWebServer& server) {
  // GET /api/status
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* res =
        request->beginResponse(200, "application/json", buildStatusJson());
    addCors(res);
    request->send(res);
  });

  // GET /api/config
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* res =
        request->beginResponse(200, "application/json", buildConfigJson());
    addCors(res);
    request->send(res);
  });

  // POST /api/config — body arrives as text/plain (CORS simple request). Use a
  // raw body accumulator (identical pattern across all async machines).
  server.on(
      "/api/config", HTTP_POST,
      [](AsyncWebServerRequest* request) {
        // A zero-length-body POST never triggers onBody, so the real response is
        // never sent and the connection hangs. Guard on contentLength()==0 here
        // (NOT a _tempObject sentinel — the fork nulls that after onBody, which
        // would make every real POST fall through to this empty-body 400).
        if (request->contentLength() == 0) {
          AsyncWebServerResponse* res = request->beginResponse(
              400, "application/json", "{\"ok\":false,\"message\":\"empty body\"}");
          addCors(res);
          request->send(res);
        }
        // Otherwise the body handler (onBody) sends the real response on the
        // final chunk.
      },
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        String* acc = reinterpret_cast<String*>(request->_tempObject);
        if (index == 0) {
          acc = new String();
          acc->reserve(total + 1);
          request->_tempObject = acc;
        }
        if (acc) {
          acc->concat(reinterpret_cast<const char*>(data), len);
        }
        if (index + len == total) {
          bool deferred = false;
          String msg;
          bool ok = applyConfigJson(acc ? *acc : String(""), deferred, msg);

          JsonDocument doc;
          doc["ok"] = ok;
          if (ok) {
            doc["applied"]  = !deferred;
            doc["deferred"] = deferred;
          }
          doc["message"] = msg;
          String out;
          serializeJson(doc, out);

          AsyncWebServerResponse* res =
              request->beginResponse(ok ? 200 : 400, "application/json", out);
          addCors(res);
          request->send(res);

          if (acc) {
            delete acc;
            request->_tempObject = nullptr;
          }
        }
      });

  // GET / — central dashboard SPA (served from PROGMEM).
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* res =
        request->beginResponse_P(200, "text/html", DASHBOARD_HTML);
    request->send(res);
  });
}

//* ************************************************************************
//* ******************** GLOBAL ASYNC SERVER *****************************
//* ************************************************************************
// Single AsyncWebServer on port 80. begin() only after WiFi is up (caller
// guarantees this by calling setupWebServer() after setupOTA()).
static AsyncWebServer webServer(80);

void setupWebServer() {
  setupConfigApi(webServer);
  webServer.begin();
}
