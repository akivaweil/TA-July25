// Auto-served single-page dashboard for the central multi-machine view.
// Hosted by TA at GET "/". Vanilla HTML/CSS/JS, phone-friendly.
extern const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Machine Dashboard</title>
<style>
  :root { --bg:#0f1419; --panel:#1b232c; --line:#2c3946; --txt:#e6edf3; --muted:#8b98a5; }
  * { box-sizing:border-box; }
  body { margin:0; font-family:-apple-system,Segoe UI,Roboto,sans-serif; background:var(--bg); color:var(--txt); }
  header { padding:14px 16px; font-size:18px; font-weight:600; border-bottom:1px solid var(--line); }
  .wrap { display:grid; gap:14px; padding:14px; grid-template-columns:repeat(auto-fill,minmax(300px,1fr)); }
  .panel { background:var(--panel); border:1px solid var(--line); border-radius:10px; padding:12px 14px; }
  .row { display:flex; align-items:center; justify-content:space-between; gap:8px; }
  .title { font-size:16px; font-weight:600; }
  .sub { font-size:12px; color:var(--muted); }
  .dot { width:10px; height:10px; border-radius:50%; display:inline-block; vertical-align:middle; }
  .online { background:#2ea043; } .offline { background:#6e7681; }
  .badge { padding:2px 8px; border-radius:999px; font-size:11px; font-weight:600; }
  .HEALTHY { background:#16341f; color:#3fb950; } .WARNING { background:#3a3413; color:#d4a72c; }
  .ERROR { background:#3a1717; color:#f85149; } .UNK { background:#2c3946; color:#8b98a5; }
  .meta { font-size:12px; color:var(--muted); margin-top:6px; display:flex; flex-wrap:wrap; gap:10px; }
  .sensors { display:flex; flex-wrap:wrap; gap:6px; margin-top:8px; }
  .sensor { display:flex; align-items:center; gap:5px; background:#0f1722; border:1px solid var(--line);
            border-radius:6px; padding:3px 7px; font-size:12px; }
  .s-on { background:#2ea043; } .s-off { background:#6e7681; }
  .state { font-size:20px; font-weight:700; margin:6px 0; }
  button { background:#21303f; color:var(--txt); border:1px solid var(--line); border-radius:7px;
           padding:7px 12px; font-size:13px; cursor:pointer; }
  button:hover { background:#2a3d50; }
  button:disabled { opacity:.5; cursor:default; }
  .cfg { margin-top:10px; border-top:1px solid var(--line); padding-top:10px; }
  .field { display:flex; align-items:center; justify-content:space-between; gap:8px; margin:5px 0; }
  .field label { font-size:12px; color:var(--muted); flex:1; }
  .field input { width:110px; background:#0f1722; color:var(--txt); border:1px solid var(--line);
                 border-radius:6px; padding:5px 7px; font-size:13px; }
  .msg { font-size:12px; margin-top:8px; min-height:16px; }
  .msg.ok { color:#3fb950; } .msg.warn { color:#d4a72c; } .msg.err { color:#f85149; }
  .btns { display:flex; gap:8px; margin-top:8px; }
</style>
</head>
<body>
<header>Machine Dashboard</header>
<div class="wrap" id="wrap"></div>
<script>
const MACHINES = [
  { id:"ta",     name:"Transfer Arm", base:"" },
  { id:"stage1", name:"Stage 1",      base:"http://192.168.1.250" },
  { id:"router", name:"Router",       base:"http://192.168.1.249" },
  { id:"stage2", name:"Stage 2",      base:"http://192.168.1.251" },
];
const POLL_MS = 2000, TIMEOUT_MS = 1500;

function el(tag, cls, txt) { const e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; }
function fmtUptime(ms) { const s = Math.floor(ms/1000); const h = Math.floor(s/3600); const m = Math.floor((s%3600)/60); return h+"h "+m+"m "+(s%60)+"s"; }

async function fetchJSON(url, opts) {
  const ctrl = new AbortController();
  const t = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const r = await fetch(url, Object.assign({ signal:ctrl.signal }, opts || {}));
    const body = await r.text();
    let json = null; try { json = JSON.parse(body); } catch (e) {}
    return { ok:r.ok, status:r.status, json };
  } finally { clearTimeout(t); }
}

function buildPanel(m) {
  const p = el("div", "panel"); p.id = "panel-" + m.id;
  const head = el("div", "row");
  const left = el("div");
  const titleRow = el("div", "row");
  const dot = el("span", "dot offline"); dot.id = "dot-" + m.id;
  const title = el("span", "title", " " + m.name);
  titleRow.appendChild(dot); titleRow.appendChild(title);
  left.appendChild(titleRow);
  left.appendChild(el("div", "sub", m.base || "same origin"));
  const badge = el("span", "badge UNK", "—"); badge.id = "badge-" + m.id;
  head.appendChild(left); head.appendChild(badge);
  p.appendChild(head);

  const state = el("div", "state", "offline"); state.id = "state-" + m.id; p.appendChild(state);
  const meta = el("div", "meta"); meta.id = "meta-" + m.id; p.appendChild(meta);
  const sensors = el("div", "sensors"); sensors.id = "sensors-" + m.id; p.appendChild(sensors);

  const cfg = el("div", "cfg");
  const btns = el("div", "btns");
  const loadBtn = el("button", null, "Load settings");
  const saveBtn = el("button", null, "Save"); saveBtn.disabled = true;
  btns.appendChild(loadBtn); btns.appendChild(saveBtn);
  cfg.appendChild(btns);
  const fields = el("div"); fields.id = "fields-" + m.id; cfg.appendChild(fields);
  const msg = el("div", "msg"); msg.id = "msg-" + m.id; cfg.appendChild(msg);
  p.appendChild(cfg);

  loadBtn.onclick = () => loadConfig(m, fields, saveBtn, msg);
  saveBtn.onclick = () => saveConfig(m, fields, saveBtn, msg);
  return p;
}

async function poll(m) {
  const dot = document.getElementById("dot-" + m.id);
  const badge = document.getElementById("badge-" + m.id);
  const state = document.getElementById("state-" + m.id);
  const meta = document.getElementById("meta-" + m.id);
  const sensors = document.getElementById("sensors-" + m.id);
  try {
    const r = await fetchJSON(m.base + "/api/status");
    if (!r.ok || !r.json) throw new Error("bad");
    const s = r.json;
    dot.className = "dot online";
    state.textContent = s.state || "?";
    const h = (s.health || "UNK");
    badge.className = "badge " + (["HEALTHY","WARNING","ERROR"].includes(h) ? h : "UNK");
    badge.textContent = h;
    meta.innerHTML = "";
    if (s.freeHeap != null) meta.appendChild(el("span", null, "heap " + Math.round(s.freeHeap/1024) + "kB"));
    if (s.rssi != null) meta.appendChild(el("span", null, "rssi " + s.rssi + "dBm"));
    if (s.uptimeMs != null) meta.appendChild(el("span", null, "up " + fmtUptime(s.uptimeMs)));
    sensors.innerHTML = "";
    const map = s.sensors || {};
    Object.keys(map).forEach(k => {
      const v = map[k];
      const chip = el("div", "sensor");
      if (typeof v === "boolean") {
        chip.appendChild(el("span", "dot " + (v ? "s-on" : "s-off")));
        chip.appendChild(el("span", null, k));
      } else {
        chip.appendChild(el("span", null, k + ": " + v));
      }
      sensors.appendChild(chip);
    });
  } catch (e) {
    dot.className = "dot offline";
    badge.className = "badge UNK"; badge.textContent = "—";
    state.textContent = "offline";
    meta.innerHTML = ""; sensors.innerHTML = "";
  }
}

async function loadConfig(m, fields, saveBtn, msg) {
  msg.className = "msg"; msg.textContent = "loading…";
  try {
    const r = await fetchJSON(m.base + "/api/config");
    if (!r.ok || !r.json || !Array.isArray(r.json.fields)) throw new Error("bad");
    fields.innerHTML = "";
    r.json.fields.forEach(f => {
      const row = el("div", "field");
      row.appendChild(el("label", null, f.label || f.key));
      const inp = document.createElement("input");
      inp.type = "number"; inp.dataset.key = f.key; inp.dataset.type = f.type;
      if (f.min != null) inp.min = f.min;
      if (f.max != null) inp.max = f.max;
      if (f.step != null) inp.step = f.step;
      inp.value = f.value;
      inp.dataset.orig = String(f.value);
      row.appendChild(inp);
      fields.appendChild(row);
    });
    saveBtn.disabled = false;
    msg.textContent = "";
  } catch (e) {
    msg.className = "msg err"; msg.textContent = "failed to load settings";
  }
}

async function saveConfig(m, fields, saveBtn, msg) {
  const payload = {};
  fields.querySelectorAll("input").forEach(inp => {
    if (String(inp.value) !== inp.dataset.orig) {
      payload[inp.dataset.key] = inp.dataset.type === "int" ? parseInt(inp.value, 10) : parseFloat(inp.value);
    }
  });
  if (Object.keys(payload).length === 0) { msg.className = "msg warn"; msg.textContent = "no changes"; return; }
  msg.className = "msg"; msg.textContent = "saving…"; saveBtn.disabled = true;
  try {
    const r = await fetchJSON(m.base + "/api/config", {
      method:"POST",
      headers:{ "Content-Type":"text/plain" },
      body: JSON.stringify(payload),
    });
    const j = r.json || {};
    if (r.ok && j.ok) {
      if (j.deferred) { msg.className = "msg warn"; msg.textContent = "pending — applies at next idle"; }
      else { msg.className = "msg ok"; msg.textContent = j.message || "saved"; }
      fields.querySelectorAll("input").forEach(inp => { inp.dataset.orig = String(inp.value); });
    } else {
      msg.className = "msg err"; msg.textContent = (j.message || "save failed");
    }
  } catch (e) {
    msg.className = "msg err"; msg.textContent = "save failed (offline?)";
  } finally { saveBtn.disabled = false; }
}

const wrap = document.getElementById("wrap");
MACHINES.forEach(m => wrap.appendChild(buildPanel(m)));
function tick() { MACHINES.forEach(poll); }
tick();
setInterval(tick, POLL_MS);
</script>
</body>
</html>)rawliteral";
