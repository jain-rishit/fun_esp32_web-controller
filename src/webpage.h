#pragma once

// the page shell (links the two assets below)
static const char PAGE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Web-Control</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="card">
    <h1>ESP32 Web-Control <span id="ledDot" class="dot"></span></h1>
    <p class="sub">PWM LED on GPIO2 &middot; scans all WiFi in range</p>

    <div class="row">
      <button id="ledOnBtn"     onclick="setLed('on')">ON</button>
      <button id="ledOffBtn"    onclick="setLed('off')">OFF</button>
      <button id="ledBlinkBtn"  onclick="setLed('blink')">BLINK</button>
      <button id="ledBreathBtn" onclick="setLed('breathe')">BREATHE</button>
    </div>

    <div class="slider-block">
      <label>Brightness <span id="dimVal">255</span></label>
      <input type="range" id="dim" min="0" max="255" value="255">
    </div>

    <div class="slider-block">
      <label>Speed (ms) <span id="speedVal">500</span></label>
      <input type="range" id="speed" min="100" max="3000" step="50" value="500">
    </div>

    <button id="scanBtn" onclick="startScan()">Scan WiFi networks</button>
    <div id="status">Tap "Scan WiFi networks" to list everything in range.</div>

    <table id="tbl">
      <thead>
        <tr><th>SSID</th><th>RSSI</th><th>CH</th><th>Sec</th></tr>
      </thead>
      <tbody></tbody>
    </table>
  </div>

  <script src="/app.js"></script>
</body>
</html>
)rawliteral";

// styling
static const char STYLE_CSS[] PROGMEM = R"rawliteral(
:root { color-scheme: dark; }
* { box-sizing: border-box; }

body {
  margin: 0;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  font-family: system-ui, sans-serif;
  background: #0f172a;
  color: #e2e8f0;
  padding: 16px;
}

.card {
  width: 100%;
  max-width: 480px;
  background: #1e293b;
  border-radius: 16px;
  padding: 20px;
  box-shadow: 0 8px 30px rgba(0, 0, 0, .4);
}

h1 { margin: 0 0 4px; font-size: 1.25rem; }
.sub { color: #94a3b8; margin: 0 0 16px; font-size: .85rem; }

.row { display: flex; gap: 8px; flex-wrap: wrap; }

button {
  flex: 1;
  min-width: 90px;
  border: none;
  border-radius: 10px;
  padding: 12px;
  font-size: .95rem;
  font-weight: 600;
  color: #fff;
  cursor: pointer;
  background: #334155;
  transition: filter .15s;
}
button:active { filter: brightness(1.3); }

#ledOnBtn     { background: #16a34a; }
#ledOffBtn    { background: #dc2626; }
#ledBlinkBtn  { background: #2563eb; }
#ledBreathBtn { background: #d946ef; }
#scanBtn      { margin-top: 16px; background: #7c3aed; width: 100%; }

.slider-block { margin-top: 14px; }
.slider-block label {
  display: flex;
  justify-content: space-between;
  color: #94a3b8;
  font-size: .8rem;
  margin-bottom: 4px;
}
input[type=range] { width: 100%; accent-color: #38bdf8; }

table {
  width: 100%;
  margin-top: 16px;
  border-collapse: collapse;
  font-size: .85rem;
}
th, td { text-align: left; padding: 8px 6px; border-bottom: 1px solid #334155; }
th { color: #94a3b8; font-weight: 600; }

#status { margin-top: 12px; color: #94a3b8; font-size: .8rem; min-height: 1.2em; }

.pill {
  display: inline-block;
  padding: 2px 8px;
  border-radius: 999px;
  background: #334155;
  font-size: .7rem;
}

.dot {
  display: inline-block;
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: #475569;
  margin-right: 6px;
}
.dot.on { background: #22c55e; box-shadow: 0 0 8px #22c55e; }
)rawliteral";

// front-end logic (talks to the /state /led /set /scan endpoints)
static const char APP_JS[] PROGMEM = R"rawliteral(
const tbl = document.querySelector('#tbl tbody');
const statusEl = document.getElementById('status');
const dot = document.getElementById('ledDot');
const dim = document.getElementById('dim');
const speed = document.getElementById('speed');
const dimVal = document.getElementById('dimVal');
const speedVal = document.getElementById('speedVal');

async function get(url) {
  const res = await fetch(url);
  return res.json();
}

async function setLed(mode) {
  await get('/led?mode=' + mode);
  refreshState();
}

dim.addEventListener('input', () => dimVal.textContent = dim.value);
dim.addEventListener('change', () => get('/set?dim=' + dim.value));

speed.addEventListener('input', () => speedVal.textContent = speed.value + ' ms');
speed.addEventListener('change', () => get('/set?speed=' + speed.value));

async function refreshState() {
  try {
    const s = await get('/state');
    dot.className = (s.mode !== 'off') ? 'dot on' : 'dot';
    dim.value = s.dim;
    dimVal.textContent = s.dim;
    speed.value = s.speed;
    speedVal.textContent = s.speed + ' ms';
  } catch (err) {}
}

async function pollScan() {
  const s = await get('/scan');
  if (s.status === 'running') {
    statusEl.textContent = 'Scanning… (1–4 s)';
    setTimeout(pollScan, 1500);
    return;
  }
  statusEl.textContent = s.length + ' network(s) found.';
  tbl.innerHTML = '';
  s.sort((a, b) => b.rssi - a.rssi);
  for (const n of s) {
    const tr = document.createElement('tr');
    const name = (n.ssid === '' || n.ssid === '\x00') ? '<i>hidden</i>' : n.ssid;
    tr.innerHTML =
      '<td>' + name + '</td>' +
      '<td>' + n.rssi + ' dBm</td>' +
      '<td>' + n.channel + '</td>' +
      '<td><span class="pill">' + n.security + '</span></td>';
    tbl.appendChild(tr);
  }
}

function startScan() { pollScan(); }

refreshState();
setInterval(refreshState, 2000);
setInterval(() => { if (document.hidden === false) pollScan(); }, 15000);
)rawliteral";