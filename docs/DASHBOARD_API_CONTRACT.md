# Dashboard REST API Contract (CANONICAL — all four machines implement this identically)

This is the single source of truth for the cross-machine config + status API. Every
machine (TA, Stage 1, Router, Stage 2) exposes the SAME three routes on port 80 with the
SAME JSON envelope. Only the plumbing (async vs sync server, persistence backend, field
list) differs per machine. Do not deviate from the shapes below.

## Routes (port 80)

### `GET /api/status` — live, read-only
Response body (Content-Type `application/json`):
```json
{
  "id": "<machine-id>",
  "name": "<Human Name>",
  "state": "<STATE_NAME>",
  "health": "HEALTHY",
  "uptimeMs": 1234567,
  "freeHeap": 210000,
  "rssi": -54,
  "sensors": { "<key>": true, "<key2>": 12 }
}
```
- `id` ∈ `ta | stage1 | router | stage2`.
- `health` ∈ `HEALTHY | WARNING | ERROR`. Use the machine's existing health/error concept
  if present (e.g. Stage 1 `getSystemHealth()`); otherwise `HEALTHY`, or `WARNING` if not
  yet homed, `ERROR` if in an error state.
- `state` is the current state-machine state name as a string (reuse the machine's existing
  state-name switch/helper).
- `sensors` is a FLAT object of bool/number values. Keys differ per machine — the dashboard
  renders whatever keys arrive, so just include the machine's meaningful inputs/flags.

### `GET /api/config` — current editable settings (self-describing)
```json
{
  "id": "<machine-id>",
  "schema": 1,
  "fields": [
    { "key": "X_DROPOFF_INCHES", "label": "X Dropoff (in)", "type": "float", "value": 20.75, "min": 0, "max": 30, "step": 0.05 },
    { "key": "SERVO_PICKUP_POS", "label": "Servo Pickup (deg)", "type": "int", "value": 52, "min": 0, "max": 180, "step": 1 }
  ]
}
```
- `type` ∈ `float | int`. `value` is the CURRENT live/persisted value.
- `min`/`max`/`step` drive the dashboard's number inputs and clamping. Choose sane ranges.
- The `fields` array IS the curated settings list for that machine.

### `POST /api/config` — apply + persist
- Request body is a FLAT key→value JSON object containing only the changed keys. The
  dashboard sends it with `Content-Type: text/plain` (intentional — see CORS below).
```json
{ "X_DROPOFF_INCHES": 21.0, "SERVO_PICKUP_POS": 50 }
```
- Behavior:
  1. Validate each key is known and the value is within `[min,max]`. Unknown key or
     out-of-range → respond `{ "ok": false, "message": "<key> invalid/out of range" }`
     (HTTP 400) and change nothing.
  2. ALWAYS persist accepted values immediately to NVS/EEPROM.
  3. If `isSafeToApplyConfig()` (machine IDLE/HOMING) → apply to the live runtime variables
     now and respond `{ "ok": true, "applied": true, "deferred": false, "message": "saved" }`.
  4. If NOT safe (mid-cycle) → set a `configDirty` flag, do NOT touch live motion variables,
     respond `{ "ok": true, "applied": false, "deferred": true, "message": "pending — applies at next idle" }`.
     The main loop must apply `configDirty` on next entry to IDLE.

## CORS (identical on every machine)
- Add header `Access-Control-Allow-Origin: *` to ALL `/api/*` responses.
- The dashboard sends `POST` as `Content-Type: text/plain` so it is a CORS "simple request"
  and NO preflight `OPTIONS` is needed. Do NOT add an OPTIONS handler. The device parses the
  body as JSON regardless of the declared content type.

## C++ shape (keep names identical across repos)
Create `src/ConfigApi/MachineConfigApi.{h,cpp}` and `src/ConfigApi/MachineSettings.{h,cpp}`.

`MachineConfigApi.h` declares (server type varies per machine):
```cpp
// Async machines (TA, Stage 1, Router): pass AsyncWebServer&
// Sync machine  (Stage 2):              pass WebServer&
void setupConfigApi(<ServerType>& server);
String buildStatusJson();                 // GET /api/status body
String buildConfigJson();                 // GET /api/config body
bool   applyConfigJson(const String& body, bool& outDeferred, String& outMsg); // POST handler core
bool   isSafeToApplyConfig();             // true only when IDLE/HOMING
```

`MachineSettings.{h,cpp}` owns the persisted struct + `loadSettings()` / `saveSettings()` /
`applySettings()` (NVS via `Preferences` on TA/Stage2; EEPROM passthrough on Stage1/Router).
On first boot (sentinel/magic absent) seed from current compile-time defaults and persist.

### JSON build/parse: use ArduinoJson 7
Build responses with a `JsonDocument` + `serializeJson(doc, out)`. Parse the POST body with
`deserializeJson(doc, body)`. Iterate `doc.as<JsonObject>()` for the flat key→value POST.

### POST body reading (the only async/sync difference)
- **Async (TA / Stage 1 / Router, ESPAsyncWebServer):** register the POST route with an
  `onBody` accumulator (accumulate `data`/`len`/`index`/`total` into a `String`, then on the
  final chunk call `applyConfigJson(...)` and `request->send(...)`). Do NOT rely on
  `AsyncCallbackJsonWebHandler` (the `me-no-dev` fork on Router may not expose it) — use the
  raw body accumulator so all three async machines use the identical pattern.
- **Sync (Stage 2, WebServer.h):** read `server.arg("plain")` inside the handler and call
  `applyConfigJson(...)`.

## Machine table (used by the TA dashboard SPA only)
| id | name | base URL |
|----|------|----------|
| `ta` | Transfer Arm | (same origin — empty base) |
| `stage1` | Stage 1 | `http://192.168.1.250` |
| `router` | Router | `http://192.168.1.249` |
| `stage2` | Stage 2 | `http://192.168.1.251` |
