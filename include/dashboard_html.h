#ifndef DASHBOARD_HTML_H
#define DASHBOARD_HTML_H

#include <Arduino.h>

const char dashboardHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Transfer Arm Dashboard</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>⚙️</text></svg>">
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&family=Space+Mono:wght@400;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-main: #1e293b;
            --bg-card: #334155;
            --bg-card-gradient: linear-gradient(145deg, #334155 0%, #273548 100%);
            --bg-card-hover: #475569;
            
            --accent-primary: #60a5fa;
            --accent-secondary: #a78bfa;
            --accent-success: #34d399;
            --accent-warning: #fbbf24;
            --accent-danger: #f87171;
            
            --text-main: #f8fafc;
            --text-muted: #cbd5e1;
            --text-dim: #94a3b8;
            
            --border-subtle: rgba(255, 255, 255, 0.08);
            --border-active: rgba(255, 255, 255, 0.25);
            
            --shadow-card: 0 4px 6px -1px rgba(0, 0, 0, 0.3), 0 2px 4px -1px rgba(0, 0, 0, 0.15);
            --shadow-hover: 0 20px 25px -5px rgba(0, 0, 0, 0.4), 0 8px 10px -6px rgba(0, 0, 0, 0.2);
            --shadow-glow: 0 0 20px rgba(96, 165, 250, 0.15);
            
            --radius-lg: 24px;
            --radius-md: 16px;
            --radius-sm: 8px;
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }
        
        body {
            font-family: 'Outfit', system-ui, -apple-system, sans-serif;
            background: var(--bg-main);
            color: var(--text-main);
            min-height: 100vh;
            line-height: 1.5;
            overflow-x: hidden;
            padding: 2rem;
        }

        .dashboard-grid {
            display: grid;
            grid-template-columns: repeat(12, 1fr);
            gap: 1.5rem;
            max-width: 1600px;
            margin: 0 auto;
        }

        .col-span-4 { grid-column: span 4; }
        .col-span-6 { grid-column: span 6; }
        .col-span-8 { grid-column: span 8; }
        .col-span-12 { grid-column: span 12; }

        header {
            grid-column: 1 / -1;
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1rem;
            padding: 0 0.5rem;
        }

        h1 {
            font-size: 1.5rem;
            font-weight: 700;
            letter-spacing: -0.02em;
            color: var(--text-main);
            text-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }

        .card {
            background: var(--bg-card-gradient);
            border: 1px solid var(--border-subtle);
            border-radius: var(--radius-lg);
            padding: 1.5rem;
            position: relative;
            transition: all 0.4s cubic-bezier(0.175, 0.885, 0.32, 1.275);
            box-shadow: var(--shadow-card);
            display: flex;
            flex-direction: column;
        }

        .card::before {
            content: '';
            position: absolute;
            top: 0; left: 0; right: 0; height: 1px;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.15), transparent);
            opacity: 0.6;
            pointer-events: none;
        }

        .card:hover {
            transform: translateY(-5px) scale(1.01);
            border-color: var(--border-active);
            box-shadow: var(--shadow-hover);
            z-index: 10;
        }

        .card-header {
            display: flex;
            align-items: center;
            gap: 0.75rem;
            margin-bottom: 1.5rem;
            color: var(--text-muted);
            font-size: 0.9rem;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }

        .card-header svg { width: 18px; height: 18px; stroke-width: 2.5; }

        .status-badge {
            display: flex;
            align-items: center;
            gap: 0.5rem;
            padding: 0.5rem 1rem;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid var(--border-subtle);
            border-radius: 100px;
            font-size: 0.85rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }

        .status-badge:hover { 
            background: rgba(255, 255, 255, 0.1); 
            transform: translateY(-1px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }
        
        .status-dot {
            width: 8px; height: 8px;
            border-radius: 50%;
            background: var(--text-dim);
            transition: all 0.3s;
        }

        .status-badge.connected .status-dot {
            background: var(--accent-success);
            box-shadow: 0 0 0 3px rgba(52, 211, 153, 0.2);
        }
        
        .status-badge.disconnected .status-dot {
            background: var(--accent-danger);
            box-shadow: 0 0 0 3px rgba(248, 113, 113, 0.2);
        }

        .value-huge {
            font-size: 3rem;
            font-weight: 700;
            line-height: 1;
            letter-spacing: -0.03em;
            color: var(--text-main);
            font-variant-numeric: tabular-nums;
            text-shadow: 0 2px 10px rgba(0,0,0,0.2);
        }

        .value-large {
            font-size: 1.75rem;
            font-weight: 600;
            letter-spacing: -0.02em;
            color: var(--text-main);
        }

        .label-sm {
            font-size: 0.75rem;
            color: var(--text-dim);
            font-weight: 600;
            margin-top: 0.25rem;
            text-transform: uppercase;
        }

        .state-display {
            text-align: center;
            padding: 2rem 0;
            background: radial-gradient(circle at center, rgba(96, 165, 250, 0.08) 0%, transparent 70%);
            border-radius: var(--radius-md);
            margin: -0.5rem -0.5rem 0.5rem -0.5rem;
        }
        .state-value {
            font-size: 2rem;
            font-weight: 800;
            color: var(--accent-primary);
            text-shadow: 0 0 25px rgba(96, 165, 250, 0.4);
            margin-bottom: 0.5rem;
        }

        .sensor-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 1rem;
        }

        .sensor-item {
            background: rgba(0, 0, 0, 0.2);
            padding: 1rem;
            border-radius: var(--radius-md);
            display: flex;
            align-items: center;
            justify-content: space-between;
            border: 1px solid transparent;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            box-shadow: inset 0 1px 3px rgba(0,0,0,0.2);
        }

        .sensor-item:hover {
            background: rgba(255, 255, 255, 0.03);
            transform: translateY(-1px);
        }

        .sensor-item.active {
            background: rgba(52, 211, 153, 0.1);
            border-color: rgba(52, 211, 153, 0.3);
            box-shadow: 0 4px 12px rgba(52, 211, 153, 0.1);
        }

        .sensor-label { font-size: 0.9rem; font-weight: 500; color: var(--text-muted); }
        .sensor-item.active .sensor-label { color: var(--text-main); font-weight: 600; }
        
        .sensor-led {
            width: 12px; height: 6px;
            border-radius: 10px;
            background: var(--bg-card-hover);
            transition: all 0.3s;
        }

        .sensor-item.active .sensor-led {
            background: var(--accent-success);
            box-shadow: 0 0 8px rgba(52, 211, 153, 0.6);
        }

        @media (max-width: 1024px) {
            .dashboard-grid { grid-template-columns: 1fr 1fr; }
            .col-span-4, .col-span-6, .col-span-8, .col-span-12 { grid-column: span 2; }
        }
        
        @media (max-width: 768px) {
            body { padding: 1rem; }
            .dashboard-grid { grid-template-columns: 1fr; gap: 1rem; }
            .col-span-4, .col-span-6, .col-span-8, .col-span-12 { grid-column: span 1; }
            .sensor-grid { grid-template-columns: 1fr; }
        }
        
        .fade-in { animation: fadeIn 0.5s ease forwards; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
    </style>
</head>
<body>
    <div class="dashboard-grid">
        <header>
            <h1>Transfer Arm Controller</h1>
            <div class="status-badge disconnected" id="connectionStatus">
                <div class="status-dot"></div>
                <span id="connectionText">Offline</span>
            </div>
        </header>

        <div class="card col-span-4 fade-in" style="animation-delay: 0.1s">
            <div class="card-header">
                <svg fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z"/></svg>
                System State
            </div>
            <div class="state-display">
                <div class="value-large" id="uptime" style="font-size: 3rem; font-weight: 700; font-family: 'Space Mono'; color: var(--text-main);">00:00:00</div>
                <div class="label-sm">UPTIME</div>
            </div>
            <div style="margin-top: auto; display: flex; justify-content: space-between; padding-top: 1rem; border-top: 1px solid var(--border-subtle);">
                <div>
                    <div class="state-value" id="currentState" style="font-size: 1rem; text-shadow: none; margin-bottom: 0.25rem;">INITIALIZING</div>
                    <div class="label-sm">CURRENT PROCESS</div>
                </div>
            </div>
        </div>

        <div class="card col-span-8 fade-in" style="animation-delay: 0.2s">
            <div class="card-header">
                <svg fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"/><path stroke-linecap="round" stroke-linejoin="round" d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z"/></svg>
                Sensor Matrix
            </div>
            <div class="sensor-grid">
                <div class="sensor-item" id="sensor_x_home">
                    <span class="sensor-label">X Home Switch</span>
                    <div class="sensor-led"></div>
                </div>
                <div class="sensor-item" id="sensor_z_home">
                    <span class="sensor-label">Z Home Switch</span>
                    <div class="sensor-led"></div>
                </div>
                <div class="sensor-item" id="sensor_start">
                    <span class="sensor-label">Start Button</span>
                    <div class="sensor-led"></div>
                </div>
                <div class="sensor-item" id="sensor_stage1">
                    <span class="sensor-label">Stage 1 Signal</span>
                    <div class="sensor-led"></div>
                </div>
                <div class="sensor-item" id="sensor_stop">
                    <span class="sensor-label">Stop Signal Stage 2</span>
                    <div class="sensor-led"></div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let ws;
        let reconnectTimeout;
        let heartbeatInterval;
        let heartbeatTimeout;
        let isConnected = false;
        let reconnectAttempts = 0;
        
        let lastUptimeMs = 0;
        let lastUptimeUpdateTime = 0;
        let uptimeUpdateInterval = null;

        function updateConnectionStatus(connected) {
            const badge = document.getElementById('connectionStatus');
            const text = document.getElementById('connectionText');
            if (connected) {
                badge.classList.remove('disconnected');
                badge.classList.add('connected');
                text.textContent = 'Online';
            } else {
                badge.classList.remove('connected');
                badge.classList.add('disconnected');
                text.textContent = 'Offline';
            }
        }

        function connect() {
            if (reconnectTimeout) clearTimeout(reconnectTimeout);
            
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.hostname}/ws`;
            
            try {
                ws = new WebSocket(wsUrl);
                
                ws.onopen = () => {
                    isConnected = true;
                    reconnectAttempts = 0;
                    updateConnectionStatus(true);
                    startHeartbeat();
                    startUptimeUpdates();
                };
                
                ws.onmessage = (event) => {
                    try {
                        const data = JSON.parse(event.data);
                        handleMessage(data);
                    } catch (e) { console.error(e); }
                };
                
                ws.onclose = () => {
                    isConnected = false;
                    updateConnectionStatus(false);
                    cleanup();
                    attemptReconnect();
                };
                
                ws.onerror = () => { if (ws.readyState !== 1) ws.close(); };
            } catch (e) {
                console.error(e);
                attemptReconnect();
            }
        }
        
        function cleanup() {
            if (heartbeatInterval) clearInterval(heartbeatInterval);
            if (heartbeatTimeout) clearTimeout(heartbeatTimeout);
            stopUptimeUpdates();
        }

        function attemptReconnect() {
            if (reconnectAttempts > 50) {
                setTimeout(() => { reconnectAttempts = 0; attemptReconnect(); }, 10000);
                return;
            }
            reconnectAttempts++;
            reconnectTimeout = setTimeout(connect, Math.min(1000 + (reconnectAttempts * 500), 5000));
        }

        function startHeartbeat() {
            heartbeatInterval = setInterval(() => {
                if (ws && ws.readyState === 1) {
                    ws.send(JSON.stringify({type: 'ping'}));
                    heartbeatTimeout = setTimeout(() => { ws.close(); }, 2000);
                }
            }, 1000);
        }

        function handleMessage(data) {
            if (data.type === 'pong') {
                if (heartbeatTimeout) clearTimeout(heartbeatTimeout);
            }
            else if (data.type === 'system_status') {
                document.getElementById('currentState').textContent = data.currentState;
                
                const now = Date.now();
                const estimated = lastUptimeMs + (now - lastUptimeUpdateTime);
                if (lastUptimeMs === 0 || Math.abs(estimated - data.uptime) > 2000) {
                    lastUptimeMs = data.uptime;
                    lastUptimeUpdateTime = now;
                    document.getElementById('uptime').textContent = formatUptime(data.uptime);
                }
            }
            else if (data.type === 'sensor_status') {
                updateSensor('sensor_x_home', data.xHome);
                updateSensor('sensor_z_home', data.zHome);
                updateSensor('sensor_start', data.startButton);
                updateSensor('sensor_stage1', data.stage1Signal);
                updateSensor('sensor_stop', data.stopSignalStage2);
            }
        }

        function updateSensor(id, active) {
            const el = document.getElementById(id);
            if (active) el.classList.add('active');
            else el.classList.remove('active');
        }

        function formatUptime(ms) {
            const s = Math.floor(ms / 1000);
            const d = Math.floor(s / 86400);
            const h = Math.floor((s % 86400) / 3600);
            const m = Math.floor((s % 3600) / 60);
            const sec = s % 60;
            
            if (d > 0) return `${d}d ${h}h ${m}m`;
            return `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(sec).padStart(2,'0')}`;
        }

        function startUptimeUpdates() {
            if (uptimeUpdateInterval) clearInterval(uptimeUpdateInterval);
            uptimeUpdateInterval = setInterval(() => {
                if (lastUptimeMs > 0) {
                    const now = Date.now();
                    const diff = now - lastUptimeUpdateTime;
                    document.getElementById('uptime').textContent = formatUptime(lastUptimeMs + diff);
                }
            }, 1000);
        }
        
        function stopUptimeUpdates() {
            if (uptimeUpdateInterval) clearInterval(uptimeUpdateInterval);
        }

        connect();
        document.getElementById('connectionStatus').addEventListener('click', connect);
        setInterval(() => {
            if (isConnected && ws.readyState === 1) ws.send(JSON.stringify({type: 'request_all_data'}));
        }, 2000);
    </script>
</body>
</html>
)rawliteral";

#endif // DASHBOARD_HTML_H

