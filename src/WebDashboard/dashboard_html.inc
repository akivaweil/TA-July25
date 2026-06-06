// Auto-served single-page dashboard for the central multi-machine view.
// Hosted by TA at GET "/". Vanilla HTML/CSS/JS, phone-friendly. Apple
// "Liquid Glass" aesthetic: one frosted tab per machine, default = Stage 1.
extern const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Machine Dashboard</title>
<style>
  :root {
    --txt:#f3f6fb; --muted:rgba(232,238,248,.62);
    --glass:rgba(255,255,255,.07); --glass-2:rgba(255,255,255,.12);
    --hair:rgba(255,255,255,.16); --hair-soft:rgba(255,255,255,.08);
    --green:#34d27b; --green-t:rgba(52,210,123,.16);
    --amber:#ffd23f; --amber-t:rgba(255,210,63,.16);
    --red:#ff5c5c; --red-t:rgba(255,92,92,.16);
    --accent:#7aa8ff; --blur:saturate(180%) blur(20px);
  }
  * { box-sizing:border-box; }
  html, body { margin:0; min-height:100%; }
  body {
    font-family:-apple-system,BlinkMacSystemFont,"SF Pro Text","Segoe UI",Roboto,sans-serif;
    color:var(--txt); -webkit-font-smoothing:antialiased; padding-bottom:env(safe-area-inset-bottom);
    background:
      radial-gradient(1100px 760px at 8% -12%, rgba(108,99,255,.40), transparent 60%),
      radial-gradient(980px 700px at 112% 6%, rgba(0,196,170,.30), transparent 55%),
      radial-gradient(900px 900px at 52% 118%, rgba(255,72,158,.26), transparent 55%),
      #070a11;
    background-attachment:fixed;
  }

  .topbar { position:sticky; top:0; z-index:10; padding:max(12px,env(safe-area-inset-top)) 0 0;
            background:rgba(12,16,24,.55); -webkit-backdrop-filter:var(--blur); backdrop-filter:var(--blur);
            border-bottom:1px solid var(--hair-soft); }
  header { display:flex; align-items:center; justify-content:space-between; padding:4px 18px 12px; }
  .h-title { font-size:18px; font-weight:700; letter-spacing:.2px; }
  .h-summary { display:flex; align-items:center; gap:8px; font-size:13px; color:var(--muted);
               font-variant-numeric:tabular-nums; }

  /* Segmented glass tab strip — one tab per machine, shows live connectivity. */
  .tabs { display:flex; gap:5px; margin:0 14px 12px; padding:5px; border-radius:16px;
          background:var(--glass); border:1px solid var(--hair);
          -webkit-backdrop-filter:var(--blur); backdrop-filter:var(--blur);
          overflow-x:auto; scrollbar-width:none; }
  .tabs::-webkit-scrollbar { display:none; }
  .tab { display:flex; align-items:center; gap:8px; flex:1 0 auto; justify-content:center;
         padding:9px 14px; border-radius:11px; border:1px solid transparent; background:transparent;
         color:var(--muted); font-size:14px; font-weight:600; white-space:nowrap; cursor:pointer;
         transition:background .18s, color .18s, box-shadow .18s; }
  .tab:hover { color:var(--txt); }
  .tab.active { background:var(--glass-2); color:var(--txt);
                box-shadow:inset 0 1px 0 rgba(255,255,255,.28), 0 1px 3px rgba(0,0,0,.25); }
  .tdot { width:9px; height:9px; border-radius:50%; background:#6b7480; flex:none; transition:background .25s, box-shadow .25s; }
  .tdot.online { background:var(--green); box-shadow:0 0 0 3px rgba(52,210,123,.22); }

  .stage { max-width:680px; margin:0 auto; padding:16px; }

  .panel { background:var(--glass); border:1px solid var(--hair); border-radius:22px; padding:18px 20px;
           -webkit-backdrop-filter:var(--blur); backdrop-filter:var(--blur);
           box-shadow:0 12px 34px rgba(0,0,0,.38), inset 0 1px 0 rgba(255,255,255,.18);
           transition:opacity .25s; }
  .panel.offline { opacity:.62; }

  .row { display:flex; align-items:center; justify-content:space-between; gap:10px; }
  .name { display:flex; align-items:center; gap:10px; font-size:17px; font-weight:650; }
  .sub { font-size:12px; color:var(--muted); margin-top:3px; }
  .dot { width:11px; height:11px; border-radius:50%; display:inline-block; flex:none;
         background:#6b7480; transition:background .25s, box-shadow .25s; }
  .dot.online { background:var(--green); box-shadow:0 0 0 3px rgba(52,210,123,.22); }
  .dot.partial { background:var(--amber); box-shadow:0 0 0 3px rgba(255,210,63,.22); }
  .dot.down { background:var(--red); box-shadow:0 0 0 3px rgba(255,92,92,.22); }

  .pill { padding:4px 11px; border-radius:999px; font-size:11px; font-weight:700;
          letter-spacing:.5px; text-transform:uppercase; border:1px solid var(--hair); }
  .pill.online { background:var(--green-t); color:var(--green); border-color:rgba(52,210,123,.3); }
  .pill.offline { background:rgba(255,255,255,.05); color:var(--muted); }

  .staterow { display:flex; align-items:center; gap:11px; margin:16px 0 2px; }
  .state { font-size:26px; font-weight:750; letter-spacing:.2px; }
  .badge { padding:3px 10px; border-radius:999px; font-size:11px; font-weight:700; border:1px solid transparent; }
  .HEALTHY { background:var(--green-t); color:var(--green); border-color:rgba(52,210,123,.3); }
  .WARNING { background:var(--amber-t); color:var(--amber); border-color:rgba(255,210,63,.3); }
  .ERROR { background:var(--red-t); color:var(--red); border-color:rgba(255,92,92,.3); }
  .UNK { background:rgba(255,255,255,.05); color:var(--muted); }

  .meta { font-size:12.5px; color:var(--muted); margin-top:10px; display:flex; flex-wrap:wrap;
          gap:6px 16px; font-variant-numeric:tabular-nums; }
  .sensors { display:flex; flex-wrap:wrap; gap:7px; margin-top:12px; }
  .sensor { display:flex; align-items:center; gap:7px; padding:5px 10px; font-size:12px;
            background:rgba(255,255,255,.05); border:1px solid var(--hair-soft); border-radius:9px; }
  .sdot { width:8px; height:8px; border-radius:50%; flex:none; }
  .s-on { background:var(--green); box-shadow:0 0 6px rgba(52,210,123,.6); } .s-off { background:#6b7480; }

  .cfg { margin-top:16px; border-top:1px solid var(--hair-soft); padding-top:14px; }
  .cfg-head { display:flex; align-items:center; justify-content:space-between; font-size:12px;
              color:var(--muted); margin-bottom:8px; }
  .field { display:flex; align-items:center; gap:10px; margin:8px 0; }
  .field label { font-size:13px; color:var(--muted); flex:1; }
  .field input { width:108px; background:rgba(255,255,255,.06); color:var(--txt); border:1px solid var(--hair);
                 border-radius:10px; padding:7px 10px; font-size:13px; font-variant-numeric:tabular-nums;
                 transition:border-color .15s, box-shadow .15s; }
  .field input:focus { outline:none; border-color:var(--accent); box-shadow:0 0 0 3px rgba(122,168,255,.2); }
  .field input:disabled { opacity:.45; }
  .field input.err { border-color:var(--red); box-shadow:0 0 0 3px rgba(255,92,92,.18); }
  .fstat { width:18px; height:18px; flex:none; display:flex; align-items:center; justify-content:center;
           font-size:14px; line-height:1; }
  .fstat.ok { color:var(--green); } .fstat.warn { color:var(--amber); } .fstat.err { color:var(--red); }
  .spin { width:13px; height:13px; border:2px solid var(--hair); border-top-color:var(--accent);
          border-radius:50%; animation:spin .7s linear infinite; }
  @keyframes spin { to { transform:rotate(360deg); } }
  .fade { animation:fade 2.4s forwards; }
  @keyframes fade { 0%,55% { opacity:1; } 100% { opacity:0; } }
  .cfg-empty { font-size:12.5px; color:var(--muted); padding:6px 0; }
</style>
</head>
<body>
<div class="topbar">
  <header>
    <div class="h-title">Machine Dashboard</div>
    <div class="h-summary"><span class="dot" id="sum-dot"></span><span id="sum-txt">connecting…</span></div>
  </header>
  <div class="tabs" id="tabs"></div>
</div>
<div class="stage" id="stage"></div>
<script>
const MACHINES = [
  { id:"ta",     name:"Transfer Arm", base:"" },
  { id:"stage1", name:"Stage 1",      base:"http://192.168.1.250" },
  { id:"router", name:"Router",       base:"http://192.168.1.249" },
  { id:"stage2", name:"Stage 2",      base:"http://192.168.1.251" },
];
const DEFAULT_TAB = "stage1";
const POLL_MS = 2000, TIMEOUT_MS = 1500;
const RECONCILE_EVERY = 8;  // every Nth poll, re-pull /api/config to catch reboots + applied deferrals
const ST = {};  // id -> per-machine runtime state (DOM refs + flags)
let active = DEFAULT_TAB;
let pollCount = 0;

function el(tag, cls, txt) { const e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; }
function fmtUptime(ms) { const s = Math.floor(ms/1000), h = Math.floor(s/3600), m = Math.floor((s%3600)/60); return h+"h "+m+"m "+(s%60)+"s"; }

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

//* ******************** PANEL CONSTRUCTION ********************
function buildPanel(m) {
  const s = ST[m.id] = { m, online:false, configLoaded:false, configLoading:false };
  const p = el("div", "panel offline"); s.panel = p;

  const head = el("div", "row");
  const left = el("div");
  const nameRow = el("div", "name");
  s.dot = el("span", "dot");
  nameRow.appendChild(s.dot); nameRow.appendChild(el("span", null, m.name));
  left.appendChild(nameRow);
  left.appendChild(el("div", "sub", m.base || "same origin"));
  s.pill = el("span", "pill offline", "Offline");
  head.appendChild(left); head.appendChild(s.pill);
  p.appendChild(head);

  const stateRow = el("div", "staterow");
  s.state = el("span", "state", "—");
  s.badge = el("span", "badge UNK", "—");
  stateRow.appendChild(s.state); stateRow.appendChild(s.badge);
  p.appendChild(stateRow);

  s.meta = el("div", "meta"); p.appendChild(s.meta);
  s.sensors = el("div", "sensors"); p.appendChild(s.sensors);

  const cfg = el("div", "cfg");
  const cfgHead = el("div", "cfg-head");
  cfgHead.appendChild(el("span", null, "Settings"));
  cfgHead.appendChild(el("span", null, "edits save on blur"));
  cfg.appendChild(cfgHead);
  s.fields = el("div");
  s.fields.appendChild(el("div", "cfg-empty", "waiting for connection…"));
  cfg.appendChild(s.fields);
  p.appendChild(cfg);

  return p;
}

//* ******************** TABS ********************
function setActive(id) {
  active = id;
  MACHINES.forEach(m => {
    const on = m.id === id;
    ST[m.id].panel.style.display = on ? "" : "none";
    ST[m.id].tab.classList.toggle("active", on);
  });
}

//* ******************** LIVE STATUS POLL ********************
async function poll(m) {
  const s = ST[m.id];
  let r = null;
  try { r = await fetchJSON(m.base + "/api/status"); } catch (e) {}
  if (!r || !r.ok || !r.json) { setOffline(s); return; }
  setOnline(s, r.json);
  if (!s.configLoaded && !s.configLoading) loadConfig(s);  // auto-load curated fields once online
}

function setOnline(s, d) {
  s.online = true;
  s.panel.className = "panel";
  s.dot.className = "dot online";
  s.tabDot.className = "tdot online";
  s.pill.className = "pill online"; s.pill.textContent = "Online";
  s.state.textContent = d.state || "?";
  const h = d.health || "UNK";
  s.badge.className = "badge " + (["HEALTHY","WARNING","ERROR"].includes(h) ? h : "UNK");
  s.badge.textContent = h;

  s.meta.innerHTML = "";
  if (d.freeHeap != null) s.meta.appendChild(el("span", null, "heap " + Math.round(d.freeHeap/1024) + " kB"));
  if (d.rssi != null && d.rssi !== 0) s.meta.appendChild(el("span", null, "rssi " + d.rssi + " dBm"));
  if (d.uptimeMs != null) s.meta.appendChild(el("span", null, "up " + fmtUptime(d.uptimeMs)));

  s.sensors.innerHTML = "";
  const map = d.sensors || {};
  Object.keys(map).forEach(k => {
    const v = map[k];
    const chip = el("div", "sensor");
    if (typeof v === "boolean") {
      chip.appendChild(el("span", "sdot " + (v ? "s-on" : "s-off")));
      chip.appendChild(el("span", null, k));
    } else {
      chip.appendChild(el("span", null, k + ": " + v));
    }
    s.sensors.appendChild(chip);
  });

  s.fields.querySelectorAll("input").forEach(i => i.disabled = false);  // re-enable once reachable
}

function setOffline(s) {
  s.online = false;
  s.panel.className = "panel offline";
  s.dot.className = "dot";
  s.tabDot.className = "tdot";
  s.pill.className = "pill offline"; s.pill.textContent = "Offline";
  s.state.textContent = "Offline";
  s.badge.className = "badge UNK"; s.badge.textContent = "—";
  s.meta.innerHTML = ""; s.sensors.innerHTML = "";
  s.fields.querySelectorAll("input").forEach(i => i.disabled = true);
}

//* ******************** CURATED SETTINGS ********************
async function loadConfig(s) {
  s.configLoading = true;
  let r = null;
  try { r = await fetchJSON(s.m.base + "/api/config"); } catch (e) {}
  s.configLoading = false;
  if (!r || !r.ok || !r.json || !Array.isArray(r.json.fields)) return;  // retry on a later poll
  s.configLoaded = true;
  s.fields.innerHTML = "";
  r.json.fields.forEach(f => {
    const row = el("div", "field");
    row.appendChild(el("label", null, f.label || f.key));
    const inp = document.createElement("input");
    inp.type = "number"; inp.inputMode = "decimal";
    inp.dataset.key = f.key;
    inp.dataset.orig = String(f.value);
    if (f.min != null) inp.min = f.min;
    if (f.max != null) inp.max = f.max;
    if (f.step != null) inp.step = f.step;
    inp.value = f.value;
    if (!s.online) inp.disabled = true;
    const stat = el("span", "fstat");
    inp._stat = stat;
    inp.addEventListener("blur", () => commitField(s, inp, stat, f));
    inp.addEventListener("keydown", e => { if (e.key === "Enter") inp.blur(); });
    row.appendChild(inp); row.appendChild(stat);
    s.fields.appendChild(row);
  });
}

// Periodically re-pull /api/config so displayed values track the device after a reboot,
// another client's edit, or a deferred value applying at next IDLE. Never touches the field
// being edited or one with an uncommitted local change.
async function reconcileConfig(s) {
  if (!s.configLoaded || !s.online) return;
  let r = null;
  try { r = await fetchJSON(s.m.base + "/api/config"); } catch (e) {}
  if (!r || !r.ok || !r.json || !Array.isArray(r.json.fields)) return;
  r.json.fields.forEach(f => {
    const inp = s.fields.querySelector('input[data-key="' + f.key + '"]');
    if (!inp || inp === document.activeElement) return;
    const live = String(f.value);
    if (inp._pending) {  // a deferred save has landed once the device reports the staged value
      if (inp.dataset.orig === live) { inp._pending = false; inp.value = live; setStat(inp._stat); }
      return;
    }
    if (String(inp.value).trim() !== inp.dataset.orig) return;  // uncommitted local edit — leave it
    if (inp.dataset.orig !== live) { inp.value = live; inp.dataset.orig = live; }
  });
}

// Render an inline per-field status icon next to the input the user just left.
// kind: "spin" | "ok" | "warn" | "err".  fade=true auto-dismisses (saved confirmation).
function setStat(stat, kind, glyph, title, fade) {
  stat.className = "fstat" + (kind && kind !== "spin" ? " " + kind : "");
  stat.title = title || "";
  stat.innerHTML = "";
  if (kind === "spin") { stat.appendChild(el("span", "spin")); return; }
  if (!glyph) return;
  const g = el("span", fade ? "fade" : null, glyph);
  // The saved "✓" fades out; once gone, drop the glyph + tooltip so the slot is truly empty.
  if (fade) g.addEventListener("animationend", () => { if (stat.firstChild === g) { stat.innerHTML = ""; stat.title = ""; } });
  stat.appendChild(g);
}

// Save-on-blur: POST just this field if it changed and is in range, then show
// the result as an icon right beside the box (saved / pending / error).
async function commitField(s, inp, stat, f) {
  // Went offline mid-edit (setOffline disables inputs, firing this blur): restore the last good value.
  if (inp.disabled) { inp.value = inp.dataset.orig; inp.classList.remove("err"); setStat(stat); return; }
  const raw = inp.value.trim();
  const isInt = f.type === "int";
  const val = parseFloat(raw);

  if (raw === "" || isNaN(val)) { inp.value = inp.dataset.orig; inp.classList.remove("err"); setStat(stat); return; }
  // Int fields reject fractional input (the server would 400) rather than silently truncating it.
  if (isInt && !Number.isInteger(val)) { inp.classList.add("err"); setStat(stat, "err", "✕", "must be a whole number"); return; }
  if (String(val) === inp.dataset.orig) { inp.classList.remove("err"); setStat(stat); return; }
  if ((f.min != null && val < f.min) || (f.max != null && val > f.max)) {
    inp.classList.add("err");
    setStat(stat, "err", "✕", "must be " + f.min + "–" + f.max);
    return;
  }
  inp.classList.remove("err");

  // In-flight token: if the user edits + blurs again before this POST returns, only the
  // newest commit may settle the icon / dataset.orig — a stale earlier response is dropped.
  const gen = inp._gen = (inp._gen || 0) + 1;
  setStat(stat, "spin");
  const payload = {}; payload[f.key] = val;
  let r = null;
  try {
    r = await fetchJSON(s.m.base + "/api/config", {
      method:"POST", headers:{ "Content-Type":"text/plain" }, body:JSON.stringify(payload),
    });
  } catch (e) {}
  if (inp._gen !== gen) return;  // superseded by a newer edit
  const j = (r && r.json) || {};
  if (r && r.ok && j.ok) {
    inp.dataset.orig = String(val);
    if (j.deferred) { inp._pending = true; setStat(stat, "warn", "⏱", "pending — applies at next idle"); }
    else { inp._pending = false; setStat(stat, "ok", "✓", "saved", true); }
  } else if (!r) {
    setStat(stat, "warn", "⚠", "machine unreachable — not saved");  // transport failure, not a server rejection
  } else {
    setStat(stat, "err", "✕", j.message || "save failed");
  }
}

//* ******************** HEADER SUMMARY ********************
function updateSummary() {
  const total = MACHINES.length;
  const online = MACHINES.filter(m => ST[m.id].online).length;
  const sumDot = document.getElementById("sum-dot");
  sumDot.className = "dot " + (online === total ? "online" : online > 0 ? "partial" : "down");
  document.getElementById("sum-txt").textContent = online + " / " + total + " online";
}

//* ******************** BOOT + POLL LOOP ********************
const tabsEl = document.getElementById("tabs");
const stage = document.getElementById("stage");
MACHINES.forEach(m => {
  const p = buildPanel(m);
  stage.appendChild(p);
  const tab = el("button", "tab");
  const tdot = el("span", "tdot");
  tab.appendChild(tdot); tab.appendChild(el("span", null, m.name));
  tab.onclick = () => setActive(m.id);
  tabsEl.appendChild(tab);
  ST[m.id].tab = tab; ST[m.id].tabDot = tdot;
});
setActive(active);
async function tick() {
  await Promise.all(MACHINES.map(poll));
  updateSummary();
  if (pollCount++ % RECONCILE_EVERY === RECONCILE_EVERY - 1) await Promise.all(MACHINES.map(reconcileConfig));
}
tick();
setInterval(tick, POLL_MS);
</script>
</body>
</html>)rawliteral";
