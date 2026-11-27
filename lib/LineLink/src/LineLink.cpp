#if defined(ARDUINO)

#include "LineLink.h"

#include <Arduino.h>
#include <esp_now.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <string.h>

namespace linelink {
namespace {

const uint32_t     LINK_TASK_STACK_BYTES = 4096;
const UBaseType_t  LINK_TASK_PRIORITY    = 2;    // above the Arduino loop task (1)
const BaseType_t   LINK_TASK_CORE        = 0;    // WiFi core; leaves core 1 for the machine loop
const TickType_t   LINK_TASK_PERIOD_TICKS = 1;   // service() cadence (1 tick = 1 ms)

// Bind the Switch Controller ID to the commissioned ESP-NOW station address.
const uint8_t SWITCH_CONTROLLER_MAC[6] = {0x94, 0xA9, 0x90, 0x0C, 0xDE, 0x88};

const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

class EspNowRadio : public IRadio {
public:
    bool addPeer(const uint8_t mac[6]) {
        if (esp_now_is_peer_exist(mac)) return true;
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, mac, 6);
        peer.channel = 0;   // 0 = follow the current WiFi channel
        peer.encrypt = false;
        return esp_now_add_peer(&peer) == ESP_OK;
    }
    bool sendBroadcast(const uint8_t* data, size_t len) {
        return esp_now_send(BROADCAST_MAC, data, len) == ESP_OK;
    }
    bool sendUnicast(const uint8_t mac[6], const uint8_t* data, size_t len) {
        return esp_now_send(mac, data, len) == ESP_OK;
    }
};

class ArduinoClock : public IClock {
public:
    uint32_t millis() { return ::millis(); }
    uint32_t micros() { return (uint32_t)::micros(); }
};

class SpinLock : public ILock {
public:
    SpinLock() : mux_(portMUX_INITIALIZER_UNLOCKED) {}
    void lock()   { portENTER_CRITICAL(&mux_); }
    void unlock() { portEXIT_CRITICAL(&mux_); }
private:
    portMUX_TYPE mux_;
};

void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (len == static_cast<int>(sizeof(Frame)) && data &&
        data[offsetof(Frame, srcId)] == ID_SWITCH_CONTROLLER &&
        (!mac || memcmp(mac, SWITCH_CONTROLLER_MAC, sizeof(SWITCH_CONTROLLER_MAC)) != 0)) {
        return;
    }
    link().onReceive(mac, data, len);
}

void linkTask(void*) {
    for (;;) {
        link().service();
        vTaskDelay(LINK_TASK_PERIOD_TICKS);
    }
}

}  // namespace

Core& link() {
    static EspNowRadio radio;
    static ArduinoClock clock;
    static SpinLock lock;
    static Core core(radio, clock, lock);
    return core;
}

FlashGate& flashGate() {
    static FlashGate gate(link());
    return gate;
}

bool begin(uint8_t selfId, int8_t txPowerQuarterDbm) {
    static bool started = false;
    if (started) return true;

    esp_wifi_set_max_tx_power(txPowerQuarterDbm);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[LineLink] esp_now_init failed - links disabled");
        return false;
    }
    esp_now_register_recv_cb(onEspNowReceive);

    esp_now_peer_info_t broadcast = {};
    memcpy(broadcast.peer_addr, BROADCAST_MAC, 6);
    broadcast.channel = 0;
    broadcast.encrypt = false;
    esp_now_add_peer(&broadcast);

    link().begin(selfId, (uint16_t)esp_random(), (int32_t)esp_random());
    xTaskCreatePinnedToCore(linkTask, "linelink", LINK_TASK_STACK_BYTES, nullptr,
                            LINK_TASK_PRIORITY, nullptr, LINK_TASK_CORE);
    started = true;
    Serial.printf("[LineLink] up as %s\n", machineName(selfId));
    return true;
}

}  // namespace linelink

#endif  // ARDUINO
