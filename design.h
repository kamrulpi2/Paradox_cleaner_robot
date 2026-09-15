#ifndef DESIGN_H
#define DESIGN_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>ESP32 Robot</title>
  <style>
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
      user-select: none;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(160deg, #0f172a, #1e293b);
      color: #f1f5f9;
      min-height: 100vh;
      padding: 16px;
      max-width: 430px;
      margin: 0 auto;
    }
    h1 {
      text-align: center;
      font-size: 1.6rem;
      color: #38bdf8;
      margin: 8px 0 20px;
      font-weight: 700;
    }
    .card {
      background: #1e293b;
      border-radius: 20px;
      padding: 18px;
      margin-bottom: 18px;
      box-shadow: 0 8px 20px rgba(0,0,0,0.35);
    }
    .card-title {
      font-size: 0.95rem;
      color: #94a3b8;
      margin-bottom: 14px;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .angle-display {
      font-size: 2.4rem;
      font-weight: 700;
      color: #4ade80;
      text-align: center;
      margin: 8px 0 12px;
    }
    input[type=range] {
      width: 100%;
      height: 14px;
      -webkit-appearance: none;
      background: #334155;
      border-radius: 10px;
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 32px;
      height: 32px;
      background: #38bdf8;
      border-radius: 50%;
      cursor: pointer;
      box-shadow: 0 2px 8px rgba(56,189,248,0.5);
    }
    .btn-row {
      display: flex;
      gap: 10px;
      justify-content: center;
      flex-wrap: wrap;
      margin-top: 12px;
    }
    .btn {
      border: none;
      border-radius: 14px;
      color: white;
      font-weight: 600;
      font-size: 1rem;
      padding: 14px 12px;
      cursor: pointer;
      transition: 0.12s;
      flex: 1;
      min-width: 70px;
    }
    .btn:active {
      transform: scale(0.94);
      opacity: 0.9;
    }
    .btn-servo  { background: #8b5cf6; }
    .btn-rec    { background: #f59e0b; }
    .btn-play   { background: #22c55e; }
    .btn-home   { background: #64748b; }
    .btn-relay  { background: #f97316; }
    .btn-relay.on { background: #22c55e; }
    .btn-move   { background: #0ea5e9; font-size: 1.5rem; padding: 20px 10px; }
    .btn-stop   { background: #ef4444; font-size: 1.15rem; padding: 16px; width: 100%; margin-top: 8px; }

    .move-grid {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 12px;
      margin-top: 6px;
    }
    .status {
      text-align: center;
      font-size: 0.9rem;
      color: #94a3b8;
      margin-top: 10px;
      min-height: 22px;
    }
    .relay-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
  </style>
</head>
<body>
  <h1>🤖 ESP32 Robot</h1>

  <!-- SERVO -->
  <div class="card">
    <div class="card-title">Servos (Mirrored)</div>
    <div class="angle-display" id="angleVal">0°</div>
    <input type="range" min="0" max="310" value="0" id="angleSlider" oninput="setAngle(this.value)">
    <div class="btn-row">
      <button class="btn btn-servo" onclick="setAngle(0)">0°</button>
      <button class="btn btn-servo" onclick="setAngle(90)">90°</button>
      <button class="btn btn-servo" onclick="setAngle(155)">155°</button>
      <button class="btn btn-servo" onclick="setAngle(310)">310°</button>
    </div>
  </div>

  <!-- RECORD -->
  <div class="card">
    <div class="card-title">Record & Play</div>
    <div class="btn-row">
      <button class="btn btn-rec"  onclick="record()">⏺ Record</button>
      <button class="btn btn-play" onclick="play()">▶️ Play</button>
      <button class="btn btn-home" onclick="home()">🏠 Home</button>
    </div>
    <div class="status" id="status">Ready</div>
  </div>

  <!-- RELAYS -->
  <div class="card">
    <div class="card-title">Relays</div>
    <div class="relay-grid">
      <button class="btn btn-relay" id="r1" onclick="toggleRelay(1)">🔌 Relay 1</button>
      <button class="btn btn-relay" id="r2" onclick="toggleRelay(2)">🔌 Relay 2</button>
      <button class="btn btn-relay" id="r3" onclick="toggleRelay(3)">🔌 Relay 3</button>
      <button class="btn btn-relay" id="r4" onclick="toggleRelay(4)">🔌 Relay 4</button>
    </div>
  </div>

  <!-- MOTORS - Improved Hold Control -->
  <div class="card">
    <div class="card-title">Robot Movement</div>
    <div class="move-grid">
      <div></div>
      <button class="btn btn-move" id="btnF">⬆️</button>
      <div></div>

      <button class="btn btn-move" id="btnL">⬅️</button>
      <button class="btn btn-stop" id="btnS">⏹️ STOP</button>
      <button class="btn btn-move" id="btnR">➡️</button>

      <div></div>
      <button class="btn btn-move" id="btnB">⬇️</button>
      <div></div>
    </div>
  </div>

  <script>
    let relayState = [false, false, false, false];
    let currentDir = 'S';

    // ============ IMPROVED MOTOR CONTROL ============
    function sendMove(dir) {
      if (currentDir === dir) return;
      currentDir = dir;
      fetch('/move?d=' + dir);
    }

    function setupHoldButton(id, dir) {
      const btn = document.getElementById(id);
      
      // Pointer events (best for mobile + desktop)
      btn.addEventListener('pointerdown', (e) => {
        e.preventDefault();
        btn.setPointerCapture(e.pointerId);
        sendMove(dir);
      });
      
      btn.addEventListener('pointerup', (e) => {
        e.preventDefault();
        sendMove('S');
      });
      
      btn.addEventListener('pointercancel', (e) => {
        sendMove('S');
      });
      
      btn.addEventListener('pointerleave', (e) => {
        if (e.buttons === 0) sendMove('S');
      });
    }

    // Setup all movement buttons
    setupHoldButton('btnF', 'F');
    setupHoldButton('btnB', 'B');
    setupHoldButton('btnL', 'L');
    setupHoldButton('btnR', 'R');

    // STOP button
    document.getElementById('btnS').addEventListener('pointerdown', (e) => {
      e.preventDefault();
      sendMove('S');
    });

    // Safety: Auto stop if page loses focus
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) sendMove('S');
    });

    // ============ OTHER FUNCTIONS ============
    function setAngle(a) {
      document.getElementById('angleVal').innerText = a + '°';
      document.getElementById('angleSlider').value = a;
      fetch('/servo?a=' + a);
    }

    function toggleRelay(num) {
      fetch('/relay?n=' + num)
        .then(r => r.text())
        .then(state => {
          relayState[num-1] = (state === "ON");
          const btn = document.getElementById('r' + num);
          if (relayState[num-1]) {
            btn.classList.add('on');
            btn.innerText = '✅ Relay ' + num;
          } else {
            btn.classList.remove('on');
            btn.innerText = '🔌 Relay ' + num;
          }
        });
    }

    function record() {
      document.getElementById('status').innerText = 'Recording... Move servo';
      fetch('/record');
    }

    function play() {
      document.getElementById('status').innerText = 'Playing...';
      fetch('/play').then(r => r.text()).then(t => {
        document.getElementById('status').innerText = t;
      });
    }

    function home() {
      document.getElementById('status').innerText = 'Going Home...';
      setAngle(0);
      fetch('/home').then(() => {
        document.getElementById('status').innerText = 'Home position ✓';
      });
    }
  </script>
</body>
</html>
)rawliteral";

#endif
