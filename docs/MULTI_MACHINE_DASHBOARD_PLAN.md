# Central Multi-Machine Dashboard (Transfer Arm as hub)

> Saved into the TA codebase so the plan isn't lost. Implementation lives on the
> `feature/central-dashboard` branch of each of the four machine repos.

## Context

There are four independent ESP32-S3 machines that run a single production line and must
work in unison: **Stage 1** (table saw), **Transfer Arm / TA** (pick & place), **Stage 2**
(cutter), and **Router** (flip/feed). Today each machine is configured by hardcoded
compile-time constants (TA, Stage 2) or its own separate local web dashboard (Stage 1 full,
Router/Stage 2 basic). TA/Stage 2 persist nothing, so every reboot reverts to compile-time
defaults.

**Goal:** make the **Transfer Arm the central computer**. TA hosts one dashboard that shows
live status for all four machines and lets the operator view/edit a curated set of each
machine's settings. Each machine persists its own **last-known settings** locally and can
**receive new settings pushed from the TA dashboard** at any time.

Runtime cycle coordination (GPIO pulses Stage1→TA→Stage2, plus ESP-NOW Stage2↔Router) is
**out of scope and must not change** — this work only layers config + status on top.

### Decisions locked with the user
- **Transport:** HTTP REST over the existing "Everwood" WiFi.
- **Dashboard scope:** config sync **+ live status** (read-only). No run-control.
- **Coverage:** all four machines in this pass.
- **Settings:** a **curated subset** per machine (≈3–12 params), not every constant.

## Architecture
Direct-browser topology: TA serves a single-page dashboard; the browser fetches each
machine's REST API directly by static IP. TA does not proxy/poll the others (keeps load off
the most timing-sensitive board). CORS handled by an `Access-Control-Allow-Origin: *`
header on each machine; config writes are `POST` with `Content-Type: text/plain` to avoid
preflight. Offline detection is browser-side via `fetch` + `AbortController` (1.5s).

Persistence: TA & Stage 2 add NVS (`Preferences`); Stage 1 & Router reuse existing EEPROM.
Safety: each machine applies new values only when IDLE/HOMING (`isSafeToApplyConfig()`);
mid-cycle writes persist immediately and apply on next IDLE (`configDirty`).

See `DASHBOARD_API_CONTRACT.md` for the exact shared REST contract that all four machines
implement identically.

## Per-machine curated settings

**TA** (pull from state files into NVS-backed `TASettings`, recompute via `applyTASettings()`):
`X_PICKUP_INCHES`, `Z_PICKUP_LOWER_INCHES`, `SERVO_PICKUP_POS`, `PICKUP_HOLD_TIME`,
`X_DROPOFF_INCHES`, `SERVO_TRAVEL_POS`, `SERVO_DROPOFF_POS`, `Z_DROPOFF_LOWER_INCHES`,
`SERVO_HOME_POS`, `DROPOFF_SETTLE_TIME`, `X_MAX_SPEED`, `Z_DROPOFF_SPEED`.

**Stage 1** (reuse `ConfigurationData` + `loadConfiguration`/`saveConfiguration`):
`CUT_TRAVEL_DISTANCE`, `FEED_TRAVEL_DISTANCE`, `ROTATION_SERVO_HOME_POSITION`,
`ROTATION_SERVO_ACTIVE_HOLD_DURATION_MS`, `ROTATION_CLAMP_EXTEND_DURATION_MS`,
`TA_SIGNAL_DURATION`, `CUT_MOTOR_NORMAL_SPEED`, `FEED_MOTOR_NORMAL_SPEED`.

**Router** (versioned EEPROM struct at addr 0, keep `EEPROM_SIZE=512`):
`SERVO_HOME_ANGLE` + ~2–4 servo offsets / cylinder dwell timings.

**Stage 2** (drop `const` from only this subset; add NVS):
`APPROACH_POSITION`, `CUTTING_POSITION`, `FINAL_POSITION`,
`ALIGNMENT_SHORT_FORWARD_POSITION`, `CUTTING_SPEED`, `APPROACH_SPEED`, `RETURN_SPEED`,
`HOMING_SPEED`, + a cycle-cooldown timing.

## Rollout
Leaf machines first (Stage 1 → Router → Stage 2), TA last. Each OTA-flashed at its static
IP (.250/.249/.251/.228). Do NOT touch ESP-NOW, `routerMAC`, `peer.channel`, or any
WiFi-channel logic on Stage 2/Router. Keep all HTTP handling off the stepper-critical path.

## Verification
Per machine: `curl /api/status`, `/api/config`, and a `POST`; power-cycle to confirm
persistence. TA dashboard at `http://192.168.1.228/`: 4 panels populate, ~2s refresh,
offline detection, save per panel, mid-cycle "pending" applies at next idle. Finally run a
full production cycle to confirm coordination is unaffected.
