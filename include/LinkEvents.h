#pragma once

// Transfer Arm side of the LineLink (ESP-NOW) machine links:
//   in : START_PICKUP from Stage 1 (replaces the Stage 1 start wire)
//        SUCTION_FAILURE from Stage 1 (replaces the shortened wire pulse)
//   out: STAGE2_START to Stage 2 (replaces the Stage 2 start wire)

// Start the LineLink layer (call once after WiFi is up).
void setupLineLink();

// Call once per loop: drains received events and publishes the machine state.
void updateLineLink();

// Stage 1 pickup request: set by START_PICKUP, valid for PICKUP_REQUEST_HOLD_MS
// (the old wire stayed HIGH 5 s), cleared when the pickup actually starts.
bool pickupRequestActive();
void clearPickupRequest();

// Reliable STAGE2_START event (call where the old Stage 2 wire went HIGH).
void sendStage2Start();
