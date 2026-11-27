#pragma once

// LineLink wire protocol: machine ids, frame layout, signal ids, heartbeat flag
// bits and every tunable. This library is an identical copy in all four repos
// (TA, Stage 1, Stage 2, Router) and all four must run the same version.

#include <stddef.h>
#include <stdint.h>

namespace linelink {

// Machine ids
const uint8_t ID_NONE      = 0;
const uint8_t ID_TA        = 1;
const uint8_t ID_STAGE1    = 2;
const uint8_t ID_STAGE2    = 3;
const uint8_t ID_ROUTER    = 4;
const uint8_t ID_SWITCH_CONTROLLER = 5;
const uint8_t ID_BROADCAST = 0xFF;
const uint8_t MAX_ID       = 5;            // real machines are ids 1..MAX_ID
const uint8_t PEER_SLOTS   = MAX_ID + 1;   // tables are indexed by id (slot 0 unused)

// Frame types
const uint8_t FRAME_HEARTBEAT = 1;   // broadcast every HEARTBEAT_INTERVAL_MS: state + flags
const uint8_t FRAME_EVENT     = 2;   // reliable unicast (resent until ACKed)
const uint8_t FRAME_ACK       = 3;   // acknowledges one EVENT by (src, seq)

// Signal ids (EVENT frames)
const uint8_t SIG_NONE            = 0;
const uint8_t SIG_PING            = 1;   // link test; consumed inside LineLink, never reaches machine logic
const uint8_t SIG_START_PICKUP    = 2;   // Stage 1 -> TA : cut piece ready, start pickup
const uint8_t SIG_SUCTION_FAILURE = 3;   // Stage 1 -> TA : suction not confirmed, recover
const uint8_t SIG_STAGE2_START    = 4;   // TA -> Stage 2 : piece placed, start cutting cycle
const uint8_t SIG_ROUTER_START    = 5;   // Stage 2 -> Router : piece dropped, start router cycle

const uint8_t SIG_MANUAL_FEED = 6;        // Switch Controller -> Stage 1
const uint8_t SIG_TA_MANUAL_START = 7;    // Switch Controller -> Transfer Arm
const uint8_t SIG_STAGE2_MANUAL_START = 8; // Switch Controller -> Stage 2

// Switch Controller heartbeat state bits.
const uint8_t SWITCH_RUN_ON = 0x01;
const uint8_t SWITCH_RELOAD_ON = 0x02;
const uint8_t SWITCH_TA_BUTTON_ON = 0x08;
const uint8_t SWITCH_STAGE2_BUTTON_ON = 0x10;
const uint8_t SWITCH_STATE_VALID = 0x80;

// Heartbeat flag bits (a machine publishes its own flags in every heartbeat)
const uint8_t FLAG_READY       = 0x01;   // Router: idle with no start in progress
const uint8_t FLAG_CUT_INHIBIT = 0x02;   // Stage 2: drop-off router-clear sensor active
const uint8_t FLAG_IDLE        = 0x04;   // Any machine: idle, no start pending (flash interlock)
const uint8_t FLAG_FLASHING    = 0x08;   // Any machine: receiving a firmware upload
const uint8_t FLAG_FLASH_GATE  = 0x10;   // Any machine: runs the flash interlock (reports FLAG_IDLE)
const uint8_t FLAG_DROPPING    = 0x20;   // Stage 2: releasing squares at drop-off (router must not retract)
const uint8_t FLAG_FLASH_REQUEST = 0x40; // Any machine: an upload to it is requested (Stage 1 stops starting cycles)

// Frame
const uint8_t FRAME_MAGIC      = 0x4C;   // 'L'
const uint8_t PROTOCOL_VERSION = 2;

struct __attribute__((packed)) Frame {
    uint8_t  magic;
    uint8_t  version;
    uint8_t  srcId;
    uint8_t  dstId;
    uint8_t  type;
    uint16_t seq;      // EVENT/ACK: per-destination sequence; HEARTBEAT: heartbeat counter
    uint8_t  signal;   // EVENT/ACK: signal id
    int32_t  value;    // EVENT: payload (unused in HEARTBEAT/ACK)
    uint8_t  flags;    // HEARTBEAT: FLAG_* bits
    uint8_t  state;    // HEARTBEAT: sender's machine state byte
    uint16_t epoch;    // random per boot, in every frame: a change means the sender rebooted
};

// Timing and sizing tunables
const uint32_t HEARTBEAT_INTERVAL_MS   = 100;    // broadcast period
const uint32_t HEARTBEAT_MIN_GAP_MS    = 5;      // a flag change is broadcast at once, but no sooner than this after the last heartbeat
const uint32_t PEER_STALE_MS           = 500;    // no heartbeat for this long = peer dead (fail-safe)
const uint32_t EVENT_RESEND_INTERVAL_MS = 30;    // resend an unacked EVENT this often
const uint32_t EVENT_GIVE_UP_MS        = 1500;   // bounded window: then count a failure and stop
const uint32_t PING_TEST_MAX_MS        = 20000;  // whole link test is abandoned after this long
const uint16_t PING_TEST_MAX_COUNT     = 200;    // upper bound for one link test

const uint8_t  TX_SLOTS       = 8;    // events in flight (unacked) at once
const uint8_t  RX_QUEUE_SIZE  = 8;    // received events waiting for the machine loop
const uint8_t  ACK_QUEUE_SIZE = 8;    // ACKs waiting to be transmitted
const uint8_t  RESULT_RING    = 8;    // remembered final results of sent events
const uint8_t  DEDUP_WINDOW   = 32;   // per-source duplicate window; the sender never lets one destination's
                                      // oldest unfinished event fall this far behind its newest (see sendEvent)

const uint32_t HOP_MIN_US_UNSET = 0xFFFFFFFFu;   // 'no latency sample yet'
const uint16_t HEARTBEAT_GAP_MAX = 100;           // larger heartbeat seq gaps are restarts, not loss

const uint16_t LINK_TEST_DEFAULT_COUNT = 20;   // pings when ?count= is omitted
const size_t   LINK_STATUS_JSON_CAP    = 2048;  // buffer size for Core::formatStatusJson()

inline const char* machineName(uint8_t id) {
    switch (id) {
        case ID_TA:     return "ta";
        case ID_STAGE1: return "stage1";
        case ID_STAGE2: return "stage2";
        case ID_ROUTER: return "router";
        case ID_SWITCH_CONTROLLER: return "switch-controller";
        default:        return "?";
    }
}

// Parse a machine ID or name; 0 if invalid.
inline uint8_t machineIdFromText(const char* text) {
    if (!text || !*text) return ID_NONE;
    if (text[0] >= '1' && text[0] <= '0' + MAX_ID && text[1] == '\0') return (uint8_t)(text[0] - '0');
    for (uint8_t id = 1; id <= MAX_ID; ++id) {
        const char* n = machineName(id);
        size_t i = 0;
        while (n[i] && text[i] && (text[i] | 0x20) == n[i]) ++i;
        if (!n[i] && !text[i]) return id;
    }
    return ID_NONE;
}

}  // namespace linelink
