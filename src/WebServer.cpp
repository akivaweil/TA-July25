#include "WebServer.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "globals.h"

//╔═══╗ ════════════════════════════════════════════════════════════════ ╔═══╗
//║ 🌐 WEB SERVER                                                       ║
//╚═══╝ ════════════════════════════════════════════════════════════════ ╚═══╝

AsyncWebServer server(80);

// HTML Content
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Machine Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; margin: 0; padding: 20px; }
    h1 { color: #333; }
    .container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    .card { background: #eee; padding: 15px; margin: 10px 0; border-radius: 5px; text-align: left; }
    .card h3 { margin-top: 0; }
    .status-val { font-weight: bold; color: #007BFF; }
    .sensor { display: flex; justify-content: space-between; margin: 5px 0; padding: 5px; background: #fff; border-radius: 3px; }
    .sensor.active { background-color: #d4edda; border-left: 5px solid #28a745; }
    .sensor.inactive { background-color: #f8d7da; border-left: 5px solid #dc3545; }
    .state-indicator { font-size: 1.2em; padding: 10px; background: #e2e3e5; border-radius: 5px; margin-bottom: 20px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Machine Dashboard</h1>
    
    <div class="state-indicator">
      Current State: <span id="systemState" class="status-val">LOADING...</span>
    </div>

    <div class="card">
      <h3>Sensors</h3>
      <div id="sensors-list">
        <div class="sensor" id="sensor-x-home"><span>X Home Switch</span><span class="status">...</span></div>
        <div class="sensor" id="sensor-z-home"><span>Z Home Switch</span><span class="status">...</span></div>
        <div class="sensor" id="sensor-start"><span>Start Button</span><span class="status">...</span></div>
        <div class="sensor" id="sensor-stage1"><span>Stage 1 Signal</span><span class="status">...</span></div>
        <div class="sensor" id="sensor-stop"><span>Stop Signal Stage 2</span><span class="status">...</span></div>
      </div>
    </div>
  </div>

<script>
  function updateStatus() {
    fetch('/status')
      .then(response => response.json())
      .then(data => {
        document.getElementById('systemState').innerText = data.state;
        
        updateSensor('sensor-x-home', data.sensors.xHome);
        updateSensor('sensor-z-home', data.sensors.zHome);
        updateSensor('sensor-start', data.sensors.startButton);
        updateSensor('sensor-stage1', data.sensors.stage1Signal);
        updateSensor('sensor-stop', data.sensors.stopSignalStage2);
      })
      .catch(error => console.error('Error:', error));
  }

  function updateSensor(id, active) {
    const el = document.getElementById(id);
    const statusSpan = el.querySelector('.status');
    if (active) {
      el.className = 'sensor active';
      statusSpan.innerText = 'TRIGGERED';
    } else {
      el.className = 'sensor inactive';
      statusSpan.innerText = 'OFF';
    }
  }

  setInterval(updateStatus, 500); // Update every 500ms
  updateStatus(); // Initial call
</script>
</body>
</html>
)rawliteral";

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

void setupWebServer() {
  
  //* ************************************************************************
  //* ************************ ROUTE HANDLERS *******************************
  //* ************************************************************************
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"state\":\"" + getSystemStateString() + "\",";
    json += "\"sensors\":{";
    json += "\"xHome\":" + String(xHomeSwitch.read() ? "true" : "false") + ",";
    json += "\"zHome\":" + String(zHomeSwitch.read() ? "true" : "false") + ",";
    json += "\"startButton\":" + String(startButton.read() ? "true" : "false") + ",";
    json += "\"stage1Signal\":" + String(stage1Signal.read() ? "true" : "false") + ",";
    json += "\"stopSignalStage2\":" + String(stopSignalStage2.read() ? "true" : "false");
    json += "}";
    json += "}";
    request->send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web Server started");
}

