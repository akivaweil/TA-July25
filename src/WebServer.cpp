#include "WebServer.h"
#include "dashboard_html.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "globals.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🌐 WEB SERVER                                                       ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

unsigned long systemStartTime = 0;

String getSystemStateString() {
  switch (systemState) {
    case STATE_IDLE: return "IDLE";
    case STATE_PICKUP: return "PICKUP";
    case STATE_TRANSPORT: return "TRANSPORT";
    case STATE_DROPOFF: return "DROPOFF";
    case STATE_RETURN_HOME: return "RETURN HOME";
    case STATE_HOMING: return "HOMING";
    default: return "UNKNOWN";
  }
}

void broadcastSystemStatus() {
    if (ws.count() > 0) {
        JsonDocument doc;
        doc["type"] = "system_status";
        doc["currentState"] = getSystemStateString();
        doc["uptime"] = millis() - systemStartTime;
        
        String message;
        serializeJson(doc, message);
        ws.textAll(message);
    }
}

void broadcastSensorStatus() {
    if (ws.count() > 0) {
        JsonDocument doc;
        doc["type"] = "sensor_status";
        doc["xHome"] = xHomeSwitch.read();
        doc["zHome"] = zHomeSwitch.read();
        doc["startButton"] = startButton.read();
        doc["stage1Signal"] = stage1Signal.read();
        doc["stopSignalStage2"] = stopSignalStage2.read();
        
        String message;
        serializeJson(doc, message);
        ws.textAll(message);
    }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT: {
            broadcastSystemStatus();
            broadcastSensorStatus();
            break;
        }
            
        case WS_EVT_DISCONNECT:
            break;
            
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0;
                String message = (char*)data;
                
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, message);
                
                if (!error) {
                    String msgType = doc["type"];
                    if (msgType == "ping") {
                        String pongMessage = "{\"type\":\"pong\"}";
                        client->text(pongMessage);
                    } else if (msgType == "request_all_data") {
                        broadcastSystemStatus();
                        broadcastSensorStatus();
                    }
                }
            }
            break;
        }
            
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void setupWebServer() {
    systemStartTime = millis();
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", dashboardHTML);
        response->addHeader("Connection", "close");
        request->send(response);
    });
    
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    
    server.begin();
    Serial.println("Web server and WebSocket started");
}

void updateDashboardStatus() {
    if (ws.count() > 0) {
        broadcastSystemStatus();
        broadcastSensorStatus();
    }
}
