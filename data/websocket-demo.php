<?php
// index.php SLT Vendor Test (PHP + AJAX + HTML, no external JS libs)
// The browser connects to OBS via WebSocket (JS). PHP only serves the page.
?><!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Lower Thirds WebSocket Vendor Test (PHP)</title>
  <style>
  :root{
    --bg0:#0b0f17;
    --bg1:#0f1623;
    --card:rgba(255,255,255,.06);
    --card2:rgba(255,255,255,.08);
    --stroke:rgba(255,255,255,.10);
    --stroke2:rgba(255,255,255,.14);
    --text:#eaf0ff;
    --muted:rgba(234,240,255,.70);
    --muted2:rgba(234,240,255,.55);

    --good:#39d98a;
    --bad:#ff4d6d;
    --warn:#ffcc66;

    --shadow: 0 20px 60px rgba(0,0,0,.55);
    --radius: 18px;
    --radiusSm: 14px;

    --focus: 0 0 0 3px rgba(120,168,255,.30);
  }

  *{ box-sizing:border-box; }
  html,body{ height:100%; }

  body{
    margin:0;
    font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif;
    color:var(--text);
    background:
      radial-gradient(1200px 700px at 10% -10%, rgba(108,99,255,.35), transparent 60%),
      radial-gradient(900px 700px at 90% 0%, rgba(0,212,255,.22), transparent 55%),
      radial-gradient(900px 700px at 60% 110%, rgba(57,217,138,.16), transparent 60%),
      linear-gradient(180deg, var(--bg0), var(--bg1));
  }

  .wrap{
    max-width: 1100px;
    margin: 0 auto;
    padding: 26px 18px 40px;
  }

  /* Header */
  .hero{
    display:flex;
    flex-wrap:wrap;
    align-items:flex-end;
    justify-content:space-between;
    gap:14px;
    margin-bottom: 18px;
  }
  h1{
    margin:0;
    font-size: 22px;
    letter-spacing: .2px;
    line-height: 1.15;
  }
  .sub{
    margin-top:6px;
    color:var(--muted);
    font-size: 13px;
  }
  .badge{
    display:inline-flex;
    align-items:center;
    gap:8px;
    padding: 8px 12px;
    border-radius: 999px;
    background: rgba(255,255,255,.06);
    border:1px solid var(--stroke);
    color: var(--muted);
    backdrop-filter: blur(12px);
  }
  .dot{
    width:10px; height:10px; border-radius:50%;
    background: var(--warn);
    box-shadow: 0 0 0 3px rgba(255,204,102,.18);
  }

  /* Panels */
  .panel{
    background: var(--card);
    border: 1px solid var(--stroke);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    backdrop-filter: blur(16px);
    overflow:hidden;
  }
  .panel__hd{
    padding: 14px 16px;
    display:flex;
    justify-content:space-between;
    align-items:center;
    border-bottom: 1px solid var(--stroke);
    background: linear-gradient(180deg, rgba(255,255,255,.06), transparent);
  }
  .panel__ttl{
    font-weight: 650;
    letter-spacing: .2px;
    font-size: 14px;
  }
  .panel__bd{
    padding: 16px;
  }

  /* Form grid */
  .grid{
    display:grid;
    gap: 12px;
    grid-template-columns: 1.3fr 1fr auto auto auto;
    align-items:end;
  }
  @media (max-width: 860px){
    .grid{
      grid-template-columns: 1fr 1fr;
    }
    .grid .span2{ grid-column: span 2; }
  }
  @media (max-width: 520px){
    .grid{ grid-template-columns: 1fr; }
    .grid .span2{ grid-column: auto; }
  }

  .field{
    display:flex;
    flex-direction:column;
    gap: 6px;
    min-width: 0;
  }
  label{
    font-size: 12px;
    color: var(--muted2);
    display:flex;
    align-items:center;
    gap:8px;
  }

  input{
    width: 100%;
    padding: 10px 12px;
    border-radius: 14px;
    border: 1px solid var(--stroke);
    background: rgba(0,0,0,.25);
    color: var(--text);
    outline: none;
    transition: border-color .15s ease, box-shadow .15s ease, background .15s ease;
  }
  input::placeholder{ color: rgba(234,240,255,.35); }
  input:focus{
    border-color: rgba(120,168,255,.55);
    box-shadow: var(--focus);
    background: rgba(0,0,0,.35);
  }

  /* Buttons */
  button{
    appearance:none;
    border:1px solid var(--stroke2);
    border-radius: 14px;
    padding: 10px 12px;
    background: rgba(255,255,255,.06);
    color: var(--text);
    cursor:pointer;
    transition: transform .05s ease, background .15s ease, border-color .15s ease, opacity .15s ease;
    white-space:nowrap;
    font-weight: 600;
    letter-spacing: .2px;
  }
  button:hover{ background: rgba(255,255,255,.10); }
  button:active{ transform: translateY(1px); }
  button:focus{ outline:none; box-shadow: var(--focus); }
  button:disabled{
    opacity:.45;
    cursor:not-allowed;
  }
  .btn-primary{
    background: linear-gradient(180deg, rgba(120,168,255,.85), rgba(120,168,255,.50));
    border-color: rgba(120,168,255,.55);
  }
  .btn-primary:hover{
    background: linear-gradient(180deg, rgba(120,168,255,.92), rgba(120,168,255,.58));
  }
  .btn-ghost{
    background: rgba(255,255,255,.04);
  }
  .btn-danger{
    border-color: rgba(255,77,109,.35);
    background: rgba(255,77,109,.10);
  }
  .btn-danger:hover{ background: rgba(255,77,109,.16); }

  /* Preferences row */
  .prefs{
    display:flex;
    flex-wrap:wrap;
    gap: 12px 16px;
    align-items:center;
    justify-content:space-between;
    margin-top: 12px;
    padding-top: 12px;
    border-top: 1px solid var(--stroke);
  }
  .prefs-left{
    display:flex;
    flex-wrap:wrap;
    gap: 12px 16px;
    align-items:center;
  }
  .check{
    display:inline-flex;
    align-items:center;
    gap: 10px;
    padding: 8px 10px;
    border-radius: 999px;
    border:1px solid var(--stroke);
    background: rgba(0,0,0,.18);
  }
  .check input{ width: 16px; height: 16px; margin:0; }

  .hint{
    color: var(--muted2);
    font-size: 12px;
  }

  /* Status */
  #status{
    margin-top: 12px;
    padding: 12px 14px;
    border-radius: var(--radiusSm);
    border: 1px solid var(--stroke);
    background: rgba(0,0,0,.22);
    color: var(--muted);
    display:flex;
    align-items:center;
    justify-content:space-between;
    gap: 12px;
  }
  #status::before{
    content:"";
    width: 10px;
    height: 10px;
    border-radius:50%;
    background: var(--warn);
    box-shadow: 0 0 0 3px rgba(255,204,102,.18);
    flex:0 0 auto;
  }

  /* Section titles */
  h2{
    margin: 18px 0 10px;
    font-size: 14px;
    color: var(--muted);
    letter-spacing: .25px;
    font-weight: 700;
    text-transform: uppercase;
  }

  /* Items grid */
  #items{
    margin-top: 10px;
    display:grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 12px;
  }
  @media (max-width: 860px){
    #items{ grid-template-columns: 1fr; }
  }

  .card{
    border:1px solid var(--stroke);
    border-radius: var(--radius);
    padding: 14px;
    background: rgba(255,255,255,.05);
    box-shadow: 0 10px 30px rgba(0,0,0,.35);
  }
  .card h3{
    margin:0 0 6px 0;
    font-size: 15px;
    letter-spacing: .2px;
  }
  .muted{ color: var(--muted); }

  .meta{
    margin-top: 10px;
    color: var(--muted2);
    font-size: 12px;
    line-height: 1.35;
  }

  .row{
    display:flex;
    gap: 10px;
    flex-wrap:wrap;
    align-items:center;
  }

  /* Pill states */
  .pill{
    display:inline-flex;
    align-items:center;
    gap: 8px;
    padding: 6px 10px;
    border-radius: 999px;
    font-size: 12px;
    border: 1px solid var(--stroke);
    background: rgba(0,0,0,.18);
    color: var(--muted);
    margin-top: 10px;
    width: fit-content;
  }
  .pill::before{
    content:"";
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--warn);
    box-shadow: 0 0 0 3px rgba(255,204,102,.14);
  }
  .pill.on{
    border-color: rgba(57,217,138,.35);
    color: rgba(234,240,255,.85);
  }
  .pill.on::before{
    background: var(--good);
    box-shadow: 0 0 0 3px rgba(57,217,138,.14);
  }
  .pill.off{
    border-color: rgba(255,77,109,.35);
  }
  .pill.off::before{
    background: var(--bad);
    box-shadow: 0 0 0 3px rgba(255,77,109,.14);
  }

  code{
    background: rgba(255,255,255,.08);
    padding: 2px 8px;
    border-radius: 999px;
    border: 1px solid var(--stroke);
    color: rgba(234,240,255,.88);
    font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace;
    font-size: 12px;
  }

  /* Log */
  pre{
    margin: 10px 0 0;
    padding: 14px;
    border-radius: var(--radius);
    border: 1px solid var(--stroke);
    background: rgba(0,0,0,.35);
    overflow:auto;
    max-height: 320px;
    color: rgba(234,240,255,.80);
    font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace;
    font-size: 12px;
    line-height: 1.45;
    box-shadow: 0 10px 30px rgba(0,0,0,.35);
  }

  /* Small helpers */
  .mini{ font-size:12px; color: var(--muted2); }
</style>

</head>
<body>
<div class="wrap">
  <div class="hero">
    <div>
      <h1>Smart Lower Thirds Vendor API Test</h1>
      <div class="sub">Connect to OBS WebSocket v5 and test vendor calls for Smart Lower Thirds.</div>
    </div>
    <div class="badge"><span class="dot"></span><span>OBS WebSocket • Vendor API</span></div>
  </div>

  <div class="panel">
    <div class="panel__hd">
      <div class="panel__ttl">Connection</div>
    </div>

    <div class="panel__bd">
      <div class="grid">
        <div class="field span2">
          <label for="wsUrl">OBS WS URL</label>
          <input id="wsUrl" value="ws://127.0.0.1:4455" size="28">
        </div>

        <div class="field">
          <label for="wsPass">Password</label>
          <input id="wsPass" type="password" placeholder="optional" size="16">
        </div>

        <button id="btnConnect" class="btn-primary">Connect</button>
        <button id="btnDisconnect" class="btn-ghost" disabled>Disconnect</button>
        <button id="btnList" class="btn-ghost" disabled>List Lower Thirds</button>
      </div>

      <div class="prefs">
        <div class="prefs-left">
          <label class="check"><input id="rememberUrl" type="checkbox" checked> Remember URL</label>
          <label class="check"><input id="rememberPass" type="checkbox"> Remember password</label>
          <button id="btnForget" class="btn-danger" type="button">Forget saved</button>
        </div>
        <div class="hint">Password is stored in browser localStorage only if you enable it.</div>
      </div>

      <div id="status">Not connected</div>
    </div>
  </div>

  <h2>Lower Thirds</h2>
  <div id="items"></div>

  <h2>Raw Log</h2>
  <pre id="log"></pre>
</div>

<script>
/**
 * obs-websocket v5 minimal client (no external libs).
 * Implements:
 * - Identify (and optional auth)
 * - Request/Response by requestId
 * - CallVendorRequest
 *
 * op codes: 0 Hello, 1 Identify, 2 Identified, 6 Request, 7 RequestResponse
 */

const VENDOR = "smart-lower-thirds";

// Storage keys
const LS_URL  = "slt_obs_ws_url";
const LS_PASS = "slt_obs_ws_pass";
const LS_REM_URL  = "slt_remember_url";
const LS_REM_PASS = "slt_remember_pass";

const elUrl = document.getElementById("wsUrl");
const elPass = document.getElementById("wsPass");
const elRememberUrl = document.getElementById("rememberUrl");
const elRememberPass = document.getElementById("rememberPass");
const btnForget = document.getElementById("btnForget");

const elStatus = document.getElementById("status");
const elItems = document.getElementById("items");
const elLog = document.getElementById("log");

const btnConnect = document.getElementById("btnConnect");
const btnDisconnect = document.getElementById("btnDisconnect");
const btnList = document.getElementById("btnList");

let ws = null;
let hello = null;
let identified = false;
let reqSeq = 1;
const pending = new Map(); // requestId -> {resolve,reject,timeout}

function log(...args) {
  const line = args.map(a => typeof a === "string" ? a : JSON.stringify(a, null, 2)).join(" ");
  elLog.textContent = (elLog.textContent + line + "\n").slice(-20000);
  elLog.scrollTop = elLog.scrollHeight;
  console.log(...args);
}

function setStatus(t) { elStatus.textContent = t; }

function setUiConnected(on) {
  btnConnect.disabled = on;
  btnDisconnect.disabled = !on;
  btnList.disabled = !on;
}

function wsSend(obj) {
  ws.send(JSON.stringify(obj));
}

function newRequestId() {
  return "req_" + (reqSeq++) + "_" + Date.now();
}

/* -------------------------
   Persistence (localStorage)
-------------------------- */

function loadPrefs() {
  try {
    const remUrl = localStorage.getItem(LS_REM_URL);
    const remPass = localStorage.getItem(LS_REM_PASS);

    elRememberUrl.checked = (remUrl === null) ? true : (remUrl === "1");
    elRememberPass.checked = (remPass === "1");

    if (elRememberUrl.checked) {
      const savedUrl = localStorage.getItem(LS_URL);
      if (savedUrl) elUrl.value = savedUrl;
    }

    if (elRememberPass.checked) {
      const savedPass = localStorage.getItem(LS_PASS);
      if (savedPass) elPass.value = savedPass;
    }
  } catch (e) {
    log("localStorage not available:", String(e));
  }
}

function savePrefs() {
  try {
    localStorage.setItem(LS_REM_URL, elRememberUrl.checked ? "1" : "0");
    localStorage.setItem(LS_REM_PASS, elRememberPass.checked ? "1" : "0");

    if (elRememberUrl.checked) {
      localStorage.setItem(LS_URL, elUrl.value.trim());
    } else {
      localStorage.removeItem(LS_URL);
    }

    if (elRememberPass.checked) {
      localStorage.setItem(LS_PASS, elPass.value);
    } else {
      localStorage.removeItem(LS_PASS);
    }
  } catch (e) {
    log("Failed saving prefs:", String(e));
  }
}

function forgetSaved() {
  try {
    localStorage.removeItem(LS_URL);
    localStorage.removeItem(LS_PASS);
    localStorage.setItem(LS_REM_URL, "1");
    localStorage.setItem(LS_REM_PASS, "0");
  } catch {}
  elRememberUrl.checked = true;
  elRememberPass.checked = false;
  elPass.value = "";
  setStatus("Saved settings cleared.");
  log("Cleared saved URL/password.");
}

elRememberUrl.addEventListener("change", savePrefs);
elRememberPass.addEventListener("change", () => {
  if (!elRememberPass.checked) {
    try { localStorage.removeItem(LS_PASS); } catch {}
  }
  savePrefs();
});
elUrl.addEventListener("change", savePrefs);
elPass.addEventListener("change", savePrefs);
btnForget.addEventListener("click", forgetSaved);

loadPrefs();

/* -------------------------
   Crypto helpers
-------------------------- */

async function sha256Base64(str) {
  const data = new TextEncoder().encode(str);
  const hash = await crypto.subtle.digest("SHA-256", data);
  const bytes = new Uint8Array(hash);
  let bin = "";
  for (let i = 0; i < bytes.length; i++) bin += String.fromCharCode(bytes[i]);
  return btoa(bin);
}

// obs-websocket v5 auth: secret = base64( sha256(password + salt) ), auth = base64( sha256(secret + challenge) )
async function makeAuth(password, salt, challenge) {
  const secret = await sha256Base64(password + salt);
  const auth = await sha256Base64(secret + challenge);
  return auth;
}

/* -------------------------
   Requests
-------------------------- */

function request(requestType, requestData) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    return Promise.reject(new Error("WebSocket not connected"));
  }
  const requestId = newRequestId();

  const payload = {
    op: 6,
    d: {
      requestType,
      requestId,
      requestData: requestData || {}
    }
  };

  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      pending.delete(requestId);
      reject(new Error("Request timeout: " + requestType));
    }, 8000);

    pending.set(requestId, { resolve, reject, timeout });
    wsSend(payload);
  });
}

async function callVendor(requestType, requestData) {
  return await request("CallVendorRequest", {
    vendorName: VENDOR,
    requestType,
    requestData: requestData || {}
  });
}

function normalizeItems(res) {
  const top = res?.d?.responseData;
  if (top && Array.isArray(top.items)) return top.items;

  const nested = top?.responseData;
  if (nested && Array.isArray(nested.items)) return nested.items;

  const nested2 = nested?.responseData;
  if (nested2 && Array.isArray(nested2.items)) return nested2.items;

  return [];
}

/* -------------------------
   UI rendering
-------------------------- */

function escapeHtml(s) {
  return String(s ?? "").replace(/[&<>"']/g, c => ({
    "&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#039;"
  }[c]));
}

function renderItems(items) {
  elItems.innerHTML = "";
  if (!items || !items.length) {
    elItems.innerHTML = `<div class="muted">No items returned. Check output_dir + lt-state.json loaded.</div>`;
    return;
  }

  for (const it of items) {
    const vis = !!it.isVisible;
    const pillClass = vis ? "pill on" : "pill off";
    const pillText = vis ? "Visible" : "Hidden";

    const card = document.createElement("div");
    card.className = "card";
    card.innerHTML = `
      <h3>${escapeHtml(it.title || it.id)}</h3>
      <div class="muted">${escapeHtml(it.subtitle || "")}</div>
      <div style="margin-top:8px" class="${pillClass}">${pillText}</div>
      <div class="muted" style="margin-top:8px">
        Repeat: <code>${escapeHtml(it.repeatEverySec)}</code> sec • Visible: <code>${escapeHtml(it.repeatVisibleSec)}</code> sec • Hotkey: <code>${escapeHtml(it.hotkey || "")}</code>
      </div>
      <div style="margin-top:10px" class="row">
        <button data-act="toggle">Toggle</button>
        <button data-act="show">Show</button>
        <button data-act="hide">Hide</button>
        <span class="muted">id: <code>${escapeHtml(it.id)}</code></span>
      </div>
    `;

    card.querySelector('[data-act="toggle"]').addEventListener("click", async () => {
      await doToggle(it.id);
      await doList();
    });
    card.querySelector('[data-act="show"]').addEventListener("click", async () => {
      await doSetVisible(it.id, true);
      await doList();
    });
    card.querySelector('[data-act="hide"]').addEventListener("click", async () => {
      await doSetVisible(it.id, false);
      await doList();
    });

    elItems.appendChild(card);
  }
}

/* -------------------------
   Actions
-------------------------- */

async function doList() {
  try {
    setStatus("Listing lower thirds…");
    const res = await callVendor("ListLowerThirds", {});
    log("ListLowerThirds:", res);

    const items = normalizeItems(res);
    renderItems(items);

    setStatus(`Listed ${items.length} item(s).`);
  } catch (e) {
    log("List error:", String(e));
    setStatus("List failed (see log).");
  }
}

async function doToggle(id) {
  try {
    setStatus(`Toggling ${id}…`);
    const res = await callVendor("ToggleVisible", { id });
    log("ToggleVisible:", res);
    setStatus(`Toggled ${id}.`);
  } catch (e) {
    log("Toggle error:", String(e));
    setStatus("Toggle failed (see log).");
  }
}

async function doSetVisible(id, visible) {
  try {
    setStatus(`${visible ? "Showing" : "Hiding"} ${id}…`);
    const res = await callVendor("SetVisible", { id, visible });
    log("SetVisible:", res);
    setStatus(`${visible ? "Shown" : "Hidden"} ${id}.`);
  } catch (e) {
    log("SetVisible error:", String(e));
    setStatus("SetVisible failed (see log).");
  }
}

/* -------------------------
   Connect / Disconnect
-------------------------- */

async function connectObs() {
  const url = elUrl.value.trim();
  const password = elPass.value;

  savePrefs();

  identified = false;
  hello = null;

  setStatus("Connecting…");
  ws = new WebSocket(url);

  ws.onopen = () => {
    setStatus("Socket open. Waiting for Hello…");
  };

  ws.onclose = () => {
    setStatus("Disconnected.");
    setUiConnected(false);
    ws = null;
    identified = false;
  };

  ws.onerror = () => {
    log("WebSocket error event (browser provides no detail).");
  };

  ws.onmessage = async (msg) => {
    let data;
    try { data = JSON.parse(msg.data); } catch { return; }

    if (data.op === 0) {
      hello = data.d;
      log("Hello:", hello);

      const auth = hello?.authentication;
      const identifyPayload = {
        op: 1,
        d: {
          rpcVersion: hello.rpcVersion || 1,
          eventSubscriptions: 0
        }
      };

      if (auth && password) {
        identifyPayload.d.authentication = await makeAuth(password, auth.salt, auth.challenge);
      }

      wsSend(identifyPayload);
      setStatus("Sent Identify…");
      return;
    }

    if (data.op === 2) {
      identified = true;
      setUiConnected(true);
      setStatus("Connected & Identified.");
      log("Identified:", data.d);
      await doList();
      return;
    }

    if (data.op === 7) {
      const requestId = data?.d?.requestId;
      const entry = pending.get(requestId);
      if (entry) {
        clearTimeout(entry.timeout);
        pending.delete(requestId);

        const status = data?.d?.requestStatus;
        if (!status || status.result !== true) {
          entry.reject(new Error(`${data.d.requestType} failed: ${status?.code} ${status?.comment || ""}`));
        } else {
          entry.resolve(data);
        }
      }
    }
  };
}

btnConnect.addEventListener("click", async () => {
  try {
    await connectObs();
  } catch (e) {
    log("Connect failed:", String(e));
    setStatus("Connect failed (see log).");
    setUiConnected(false);
  }
});

btnDisconnect.addEventListener("click", () => {
  try { ws && ws.close(); } catch {}
});

btnList.addEventListener("click", doList);

setUiConnected(false);
</script>
</body>
</html>
