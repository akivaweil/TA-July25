#pragma once

// Header-only HTTP glue for machines that use ESPAsyncWebServer (TA, Stage 1,
// Router). Stage 2 uses the sync WebServer and has its own handler.
//
//   GET /api/linktest?to=<ta|stage1|stage2|router|1-4>&count=<n>
//     Starts sending n PING events to that machine (default LINK_TEST_DEFAULT_COUNT)
//     and replies immediately with the test status ("running": true).
//   GET /api/linktest
//     Returns the current / last test: delivered / failed / retries and min / avg /
//     max ACK latency ("done": true once finished).
//   PING is consumed inside LineLink: it never reaches machine logic.
//
// Two requests instead of one waiting reply: some ESPAsyncWebServer versions drop a
// deferred reply that waits longer than ~0.5 s, which cut long tests off.
//
//   GET /api/flash               upload-slot status (LineLinkFlashGate.h)
//   GET /api/flash?request=1     ask for an upload slot: Stage 1 starts no new cycle,
//                                "ready" turns true once this machine and every
//                                machine before it in the line are idle
//   GET /api/flash?cancel=1      drop the request

#include <ESPAsyncWebServer.h>
#include <stdlib.h>

#include "LineLink.h"

namespace linelink {

const size_t LINK_TEST_BODY_CAP = 384;

inline void sendLinkTestJson(AsyncWebServerRequest* request, int code, const String& body) {
    AsyncWebServerResponse* r = request->beginResponse(code, "application/json", body);
    r->addHeader("Access-Control-Allow-Origin", "*");
    request->send(r);
}

inline void registerLinkTestRoute(AsyncWebServer& server) {
    server.on("/api/linktest", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (request->hasParam("to")) {
            const char* err = nullptr;
            const uint8_t to = machineIdFromText(request->getParam("to")->value().c_str());
            long count = LINK_TEST_DEFAULT_COUNT;
            if (to == ID_NONE || to == link().selfId()) err = "to= must be another machine";
            if (!err && request->hasParam("count")) {
                count = strtol(request->getParam("count")->value().c_str(), nullptr, 10);
                if (count < 1 || count > PING_TEST_MAX_COUNT) err = "count out of range";
            }
            if (!err && !link().startPingTest(to, (uint16_t)count)) err = "a link test is already running";
            if (err) {
                sendLinkTestJson(request, 409, String("{\"ok\":false,\"message\":\"") + err + "\"}");
                return;
            }
        }
        char body[LINK_TEST_BODY_CAP];
        link().formatPingResultJson(body, sizeof(body));
        sendLinkTestJson(request, 200, body);
    });
}

inline void registerFlashRoute(AsyncWebServer& server) {
    server.on("/api/flash", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (request->hasParam("request")) flashGate().request(millis());
        if (request->hasParam("cancel")) flashGate().cancel();
        char body[FLASH_STATUS_BODY_CAP];
        formatFlashStatusJson(flashGate(), link().selfId(), body, sizeof(body));
        sendLinkTestJson(request, 200, body);
    });
}

}  // namespace linelink
