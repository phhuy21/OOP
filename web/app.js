// SkyGate Web — quản lý 1 sân bay nhà + giám sát hành khách.
const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => document.querySelectorAll(sel);
let STATE = null;

// ---------- Phiên đăng nhập ----------
let CURRENT_USER = null;
function loadUser() {
  try { CURRENT_USER = JSON.parse(localStorage.getItem("skygate_user") || "null"); }
  catch (e) { CURRENT_USER = null; }
}
function can(tab) { return !!(CURRENT_USER && CURRENT_USER.menu && CURRENT_USER.menu.includes(tab)); }
function isRole(...roles) { return !!(CURRENT_USER && roles.includes(CURRENT_USER.role)); }

async function api(path, params) {
  const opt = { method: params ? "POST" : "GET" };
  if (params) {
    // Tự gắn 'actor' (tài khoản đang đăng nhập) cho mọi thao tác ghi để backend phân quyền.
    if (CURRENT_USER && !("actor" in params)) params = { ...params, actor: CURRENT_USER.username };
    opt.headers = { "Content-Type": "application/x-www-form-urlencoded" };
    opt.body = new URLSearchParams(params).toString();
  }
  const res = await fetch(path, opt);
  return res.json();
}

function toast(msg, isErr) {
  const t = $("#toast");
  t.innerHTML = `<i data-lucide="${isErr ? 'alert-triangle' : 'check-circle-2'}"></i> <span>${esc(msg)}</span>`;
  t.className = "toast show" + (isErr ? " err" : "");
  lucide.createIcons();
  setTimeout(() => (t.className = "toast"), 3200);
}

function esc(s) {
  return String(s ?? "").replace(/[&<>"]/g, (c) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
}

// ---------- Tải & render toàn bộ trạng thái ----------
async function load() {
  STATE = await api("/api/state");
  
  // Render summary as pills
  if (STATE.summary) {
    const pills = STATE.summary.split('|').map(s => {
      const clean = s.trim();
      return `<span class="summary-pill">${esc(clean)}</span>`;
    }).join('');
    $("#summary").innerHTML = pills;
  }
  
  renderFlights();
  renderAirports();
  renderAircrafts();
  renderPassengers();
  renderMonitor();

  // Simulated Clock & Event Logs
  if (STATE.currentTime) $("#sim-now").textContent = STATE.currentTime;
  renderLog();
  initCreateForm();

  // Đặt vé & quản lý tài khoản
  initBookingForm();
  renderTickets();
  if (isRole("Admin")) {
    setOpts("#rw-airport", STATE.airports.map((a) => `<option value="${a.code}">${a.code}</option>`).join(""));
    renderUsers();
  }

  // Initialize Lucide Icons for dynamic content
  lucide.createIcons();
}

// ---------- Chuyến bay ----------
function statusClass(s) {
  return "badge " + s.toLowerCase().replace(/[^a-z]/g, "");
}

function getTimelineHtml(status) {
  if (status === "Cancelled") {
    return `<div class="warn-text" style="text-align:center; font-size:12px; margin: 16px 0 8px;">
      <i data-lucide="ban" style="width:14px; height:14px; vertical-align:middle; display:inline-block; margin-right:4px;"></i> Chuyến bay đã bị huỷ
    </div>`;
  }
  
  const stages = [
    { id: 'sc', label: 'Check-in', matches: ['Scheduled', 'Delayed', 'CheckIn', 'Check-in'] },
    { id: 'bd', label: 'Lên máy bay', matches: ['Boarding'] },
    { id: 'ia', label: 'Đang bay', matches: ['Takeoff', 'InAir'] },
    { id: 'ld', label: 'Hạ cánh', matches: ['Landed', 'Completed'] }
  ];
  
  let currentIdx = -1;
  if (['Scheduled', 'Delayed', 'CheckIn', 'Check-in'].includes(status)) currentIdx = 0;
  else if (status === 'Boarding') currentIdx = 1;
  else if (['Takeoff', 'InAir'].includes(status)) currentIdx = 2;
  else if (['Landed', 'Completed'].includes(status)) currentIdx = 3;
  
  return `<div class="timeline">
    ${stages.map((st, idx) => {
      let stateClass = "";
      if (idx < currentIdx) stateClass = "passed";
      else if (idx === currentIdx) stateClass = "active";
      return `
        <div class="timeline-step ${stateClass}">
          <div class="timeline-dot"></div>
          <div class="timeline-label">${st.label}</div>
        </div>`;
    }).join('')}
  </div>`;
}

function getAirportName(code) {
  const ap = STATE.airports.find(a => a.code === code);
  return ap ? ap.name : "Sân bay " + code;
}

function renderFlights() {
  const box = $("#flights-list");
  if (!STATE.flights.length) { 
    box.innerHTML = "<p class='hint'><i data-lucide='inbox'></i> Chưa có chuyến bay.</p>"; 
    return; 
  }
  
  box.innerHTML = STATE.flights.map((f) => {
    const isEmer = f.emergency;
    return `
    <div class="card">
      <h3>
        <span class="card-title-left">
          <i data-lucide="plane-takeoff" style="color:var(--accent);"></i>
          <b>${esc(f.code)}</b>
        </span>
        <span>
          <span class="${statusClass(f.status)}">${esc(f.status)}</span>
          ${isEmer ? '<span class="badge emer"><i data-lucide="alert-octagon"></i> KHẨN</span>' : ""}
        </span>
      </h3>
      
      <div class="flight-route">
        <div class="route-node">
          <span class="route-code">${esc(f.origin)}</span>
          <span class="route-name">${esc(getAirportName(f.origin))}</span>
        </div>
        <div class="route-line">
          <i data-lucide="plane"></i>
        </div>
        <div class="route-node">
          <span class="route-code"><i data-lucide="map-pin" style="width:12px;height:12px;"></i></span>
          <span class="route-name">${esc(f.dest)}</span>
        </div>
      </div>
      
      ${getTimelineHtml(f.status)}
      
      <div class="row"><i data-lucide="clock" style="width:14px; height:14px;"></i> Khởi hành: <b>${esc(f.departure) || "—"}</b></div>
      <div class="row"><i data-lucide="clock-arrow-up" style="width:14px; height:14px;"></i> Dự kiến đến: <b>${esc(f.arrival) || "—"}</b></div>
      <div class="row">
        <i data-lucide="info" style="width:14px; height:14px;"></i> 
        <span>Máy bay: <b>${esc(f.aircraft) || "(chưa gán)"}</b> · Cổng: <b>${esc(f.gate) || "—"}</b> · Tổ bay: <b>${esc(f.crew) || "—"}</b></span>
      </div>
      <div class="row">
        <i data-lucide="users" style="width:14px; height:14px;"></i>
        <span>Hành khách: <b>${f.paxCount}</b> · Đã Check-in: <b>${f.checkedIn}</b> · Lên máy bay: <b>${f.boarded}</b></span>
      </div>
      <div class="row">
        <i data-lucide="armchair" style="width:14px; height:14px;"></i>
        <span>Ghế trống: <b>${f.availableSeats >= 0 ? f.availableSeats : "—"}</b>${f.capacity ? " / " + f.capacity : ""}</span>
      </div>

      ${f.note ? `
        <div class="row warn-text" style="background: rgba(239, 68, 68, 0.08); padding: 8px; border-radius: 6px; border: 1px solid rgba(239, 68, 68, 0.12); margin-top: 10px;">
          <i data-lucide="alert-circle" style="width:14px; height:14px; flex-shrink: 0;"></i>
          <span>${esc(f.note)}</span>
        </div>` : ""}

      <div class="ops">
        ${isRole("Admin", "Staff") ? `
        <button class="btn btn-sm" onclick="doAdvance('${f.code}')">
          <i data-lucide="chevron-right"></i> Tiến trạng thái
        </button>
        <button class="btn btn-sm btn-warn" onclick="openModal('${f.code}')">
          <i data-lucide="settings"></i> Thao tác chuyến
        </button>
        <button class="btn btn-sm btn-warn" onclick="doDeleteFlight('${f.code}')">
          <i data-lucide="trash-2"></i> Xoá
        </button>` : ""}
        ${can("booking") ? `
        <button class="btn btn-sm" onclick="bookFromFlight('${f.code}')">
          <i data-lucide="ticket"></i> Mua vé
        </button>` : ""}
      </div>
    </div>`;
  }).join("");
}

async function doAdvance(code) {
  const r = await api("/api/flight/advance", { code });
  toast(r.message, !r.ok);
  await load();
}

async function doDeleteFlight(code) {
  if (!confirm("Xoá chuyến bay " + code + "? Mọi vé của chuyến sẽ bị huỷ.")) return;
  const r = await api("/api/flight/delete", { code });
  toast(r.message, !r.ok);
  await load();
}

// ---------- Sân bay ----------
function gateClass(type) {
  if (type.includes("Double")) return "double";
  if (type.includes("Single")) return "single";
  return "remote";
}

function renderAirports() {
  $("#airports-list").innerHTML = STATE.airports.map((a) => {
    const gatesHtml = a.gates.map((g) => {
      const typeLabel = g.type.replace("JetBridge", "").replace("Stand", "");
      return `
        <div class="gate-badge ${gateClass(g.type)}">
          <span class="gate-code">${esc(g.code)}</span>
          <span class="gate-type">${esc(typeLabel)}</span>
        </div>`;
    }).join('');
    
    return `
      <div class="card">
        <h3 style="color:var(--accent); font-weight:700;">
          <span class="card-title-left"><i data-lucide="building-2"></i> ${esc(a.code)}</span>
        </h3>
        <div class="row" style="color:var(--txt); font-weight:600; font-size:14px; margin-bottom:8px;">${esc(a.name)}</div>
        <div class="row" style="align-items: flex-start;">
          <i data-lucide="info" style="width:14px; height:14px; margin-top: 2px;"></i> 
          <span>${esc(a.note)}</span>
        </div>
        <div class="row"><i data-lucide="arrow-right-left" style="width:14px; height:14px;"></i> Runway dài nhất: <b>${a.longestRunway}m</b></div>
        <div class="row" style="align-items: flex-start;"><i data-lucide="route" style="width:14px; height:14px; margin-top:3px;"></i>
          <span>Đường băng: ${a.runways.map((r) => `<b>${esc(r.code)}</b> (${r.length}m)${isRole("Admin") ? ` <a class="rw-del" title="Xoá đường băng" onclick="doDeleteRunway('${esc(a.code)}','${esc(r.code)}')">✕</a>` : ""}`).join(", ") || "—"}</span>
        </div>
        <div class="row" style="margin-top:14px; font-weight:600; color:var(--txt); border-top: 1px solid rgba(255,255,255,0.05); padding-top:12px;">
          <i data-lucide="door-open" style="width:14px; height:14px;"></i> Cổng ra máy bay (${a.gates.length})
        </div>
        <div class="gate-grid">${gatesHtml}</div>
      </div>`;
  }).join("");
}

// ---------- Máy bay ----------
function renderAircrafts() {
  const admin = isRole("Admin");
  $("#aircrafts-table tbody").innerHTML = STATE.aircrafts.map((a) => `
    <tr>
      <td><b>${esc(a.registration)}</b></td>
      <td>${esc(a.model)}</td>
      <td><span class="badge scheduled">${esc(a.category)}</span></td>
      <td><b>${a.capacity}</b> ghế</td>
      <td>${a.requiredRunway}m</td>
      <td>${a.turnaround}'</td>
      <td><span class="badge checkin">${esc(a.preferredGate)}</span></td>
      <td>${admin ? `<button class="btn btn-sm btn-warn" onclick="doDeleteAircraft('${esc(a.registration)}')"><i data-lucide="trash-2"></i></button>` : ""}</td>
    </tr>`).join("");
  lucide.createIcons();
}

async function doAddAircraft() {
  const r = await api("/api/aircraft/create", {
    category: $("#ac-category").value,
    reg: $("#ac-reg").value.trim(),
    model: $("#ac-model").value.trim(),
    capacity: $("#ac-capacity").value,
  });
  toast(r.message, !r.ok);
  if (r.ok) { $("#ac-reg").value = ""; $("#ac-model").value = ""; $("#ac-capacity").value = ""; }
  await load();
}

async function doDeleteAircraft(reg) {
  if (!confirm("Xoá máy bay " + reg + "?")) return;
  const r = await api("/api/aircraft/delete", { reg });
  toast(r.message, !r.ok);
  await load();
}

async function doAddRunway() {
  const r = await api("/api/runway/create", {
    airport: $("#rw-airport").value,
    code: $("#rw-code").value.trim(),
    length: $("#rw-length").value,
  });
  toast(r.message, !r.ok);
  if (r.ok) { $("#rw-code").value = ""; $("#rw-length").value = ""; }
  await load();
}

async function doDeleteRunway(airport, code) {
  if (!confirm("Xoá đường băng " + code + " tại " + airport + "?")) return;
  const r = await api("/api/runway/delete", { airport, code });
  toast(r.message, !r.ok);
  await load();
}

// ---------- Hành khách ----------
function checkMark(val) {
  return val
    ? `<span style="color:var(--green); font-weight:bold; font-size:15px;"><i data-lucide="check-circle-2" style="width:16px; height:16px; display:inline-block; vertical-align:middle;"></i></span>`
    : `<span style="color:var(--muted); font-size:15px;"><i data-lucide="minus" style="width:14px; height:14px; display:inline-block; vertical-align:middle;"></i></span>`;
}

// Nhãn hành trình của một hành khách theo trạng thái.
function journeyBadge(p) {
  if (p.boarded) return `<span class="badge completed">Đã lên máy bay</span>`;
  if (p.checkedIn) return `<span class="badge boarding">Đã check-in</span>`;
  if (p.flight) return `<span class="badge checkin">Đã đặt chỗ</span>`;
  return `<span class="badge scheduled">Chưa đặt chỗ</span>`;
}

let paxSearchTerm = "";
function renderPassengers() {
  const tbody = $("#passengers-table tbody");
  if (!tbody) return;
  const term = paxSearchTerm.trim().toLowerCase();
  const list = STATE.passengers.filter((p) => {
    if (!term) return true;
    return [p.id, p.name, p.passport, p.flight]
      .some((v) => String(v || "").toLowerCase().includes(term));
  });
  if (!list.length) {
    tbody.innerHTML = `<tr><td colspan="7" class="hint">Không có hành khách khớp.</td></tr>`;
    return;
  }
  tbody.innerHTML = list.map((p) => `
    <tr>
      <td><b>${esc(p.id)}</b></td>
      <td>${esc(p.name)}</td>
      <td><code>${esc(p.passport) || "—"}</code></td>
      <td><b>${esc(p.flight) || "—"}</b></td>
      <td><span class="badge boarding">${esc(p.seat) || "—"}</span></td>
      <td>${journeyBadge(p)}</td>
      <td class="${p.bagOverweight ? "warn-text" : ""}">
        <i data-lucide="luggage" style="width:14px; height:14px; display:inline-block; vertical-align:middle; margin-right:4px;"></i>
        ${p.bagPieces} kiện / ${p.bagWeight}kg ${p.bagOverweight ? " ⚠" : ""}
      </td>
    </tr>`).join("");
  lucide.createIcons();
}

// ---------- Giám sát hành khách: cảnh báo + theo dõi theo chuyến ----------
// Số phút từ thời gian mô phỏng tới giờ khởi hành của chuyến (âm = đã qua giờ).
function minutesToDeparture(f) {
  const parse = (s) => {
    if (!s) return null;
    const [d, t] = s.split(" ");
    if (!d || !t) return null;
    const [Y, M, D] = d.split("-").map(Number);
    const [h, m] = t.split(":").map(Number);
    return new Date(Y, (M || 1) - 1, D || 1, h || 0, m || 0);
  };
  const now = parse(STATE.currentTime);
  const dep = parse(f.departure);
  if (!now || !dep) return null;
  return Math.round((dep - now) / 60000);
}

function renderMonitor() {
  // ----- Cảnh báo -----
  const alerts = [];
  const activeFlights = STATE.flights.filter(
    (f) => f.status !== "Completed" && f.status !== "Cancelled");

  for (const f of activeFlights) {
    const mins = minutesToDeparture(f);
    // Khách chưa check-in khi sắp tới giờ bay (<=60') và chuyến chưa cất cánh.
    const notReady = ["Scheduled", "CheckIn", "Check-in", "Boarding", "Delayed"].includes(f.status);
    if (mins !== null && mins >= 0 && mins <= 60 && notReady) {
      const notCheckedIn = f.paxCount - f.checkedIn;
      if (notCheckedIn > 0) {
        alerts.push({ level: "warn", icon: "clock-alert",
          text: `Chuyến <b>${esc(f.code)}</b> còn ${mins}' tới giờ bay nhưng <b>${notCheckedIn}</b> khách chưa check-in.` });
      }
    }
    // Chuyến gần đầy / hết ghế.
    if (f.availableSeats === 0) {
      alerts.push({ level: "warn", icon: "users",
        text: `Chuyến <b>${esc(f.code)}</b> đã <b>hết ghế</b> (${f.capacity}/${f.capacity}).` });
    } else if (f.availableSeats > 0 && f.capacity > 0 && f.availableSeats <= Math.max(1, Math.round(f.capacity * 0.1))) {
      alerts.push({ level: "info", icon: "users",
        text: `Chuyến <b>${esc(f.code)}</b> sắp đầy — còn <b>${f.availableSeats}</b> ghế.` });
    }
  }
  // Hành khách hành lý quá cân.
  const overweight = STATE.passengers.filter((p) => p.bagOverweight && p.flight);
  for (const p of overweight) {
    alerts.push({ level: "info", icon: "luggage",
      text: `Hành khách <b>${esc(p.name)}</b> (${esc(p.flight)}) hành lý quá cân: ${p.bagWeight}kg.` });
  }

  const alertsBox = $("#alerts-list");
  if (alertsBox) {
    alertsBox.innerHTML = alerts.length
      ? alerts.map((a) => `
        <div class="alert ${a.level}">
          <i data-lucide="${a.icon}"></i> <span>${a.text}</span>
        </div>`).join("")
      : `<div class="alert ok"><i data-lucide="check-circle-2"></i> <span>Không có cảnh báo. Mọi chuyến đang ổn định.</span></div>`;
  }

  // ----- Theo dõi theo chuyến / cổng -----
  const box = $("#monitor-list");
  if (box) {
    const monitored = activeFlights;
    box.innerHTML = monitored.length
      ? monitored.map((f) => {
          const waiting = f.paxCount - f.checkedIn;
          const pct = f.capacity ? Math.round((f.paxCount / f.capacity) * 100) : 0;
          return `
          <div class="card monitor-card">
            <h3>
              <span class="card-title-left"><i data-lucide="plane-takeoff" style="color:var(--accent);"></i> <b>${esc(f.code)}</b> → ${esc(f.dest)}</span>
              <span class="${statusClass(f.status)}">${esc(f.status)}</span>
            </h3>
            <div class="row"><i data-lucide="door-open" style="width:14px;height:14px;"></i> Cổng: <b>${esc(f.gate) || "—"}</b> · Khởi hành: <b>${esc(f.departure) || "—"}</b></div>
            <div class="monitor-stats">
              <div class="mstat"><span class="mstat-num">${f.paxCount}</span><span class="mstat-lbl">Đã đặt</span></div>
              <div class="mstat"><span class="mstat-num">${f.checkedIn}</span><span class="mstat-lbl">Check-in</span></div>
              <div class="mstat"><span class="mstat-num">${f.boarded}</span><span class="mstat-lbl">Đã lên</span></div>
              <div class="mstat ${waiting > 0 ? 'warn-text' : ''}"><span class="mstat-num">${waiting}</span><span class="mstat-lbl">Chờ check-in</span></div>
              <div class="mstat"><span class="mstat-num">${f.availableSeats >= 0 ? f.availableSeats : "—"}</span><span class="mstat-lbl">Ghế trống</span></div>
            </div>
            <div class="loadbar"><div class="loadbar-fill" style="width:${pct}%;"></div></div>
            <div class="row" style="font-size:11px; color:var(--muted);">Lấp đầy ${pct}% (${f.paxCount}/${f.capacity || "?"})</div>
          </div>`;
        }).join("")
      : `<p class="hint"><i data-lucide="inbox"></i> Không có chuyến đang hoạt động để giám sát.</p>`;
  }
  lucide.createIcons();
}

// ---------- Modal thao tác ----------
function openModal(code) {
  const f = STATE.flights.find((x) => x.code === code);
  if (!f) return;
  
  $("#modal-title").innerHTML = `<i data-lucide="settings" style="display:inline-block; vertical-align:middle; margin-right:6px;"></i> Chuyến ${f.code} — ${f.status}`;
  
  const canCheckIn = f.status === "CheckIn" || f.status === "Check-in" || f.status === "Boarding";
  const pax = f.passengers.map((pid) => {
    const p = STATE.passengers.find((x) => x.id === pid);
    const name = p ? p.name : pid;
    let stText = "Chưa check-in";
    let statusClass = "scheduled";
    if (p && p.boarded) { stText = "Đã lên máy bay"; statusClass = "completed"; }
    else if (p && p.checkedIn) { stText = `Đã check-in (Ghế ${p.seat})`; statusClass = "boarding"; }
    
    const isBtnDisabled = !canCheckIn || (p && p.checkedIn);
    const btnTitle = !canCheckIn ? "Chỉ có thể check-in khi trạng thái chuyến bay là Check-in hoặc Boarding" : "";
    
    return `
      <div class="pax-line">
        <span><b>${esc(name)}</b> <small>(${esc(pid)} · <span class="badge ${statusClass}" style="font-size:9px; padding:1px 5px;">${stText}</span>)</small></span>
        <span class="pax-actions">
          <button class="btn btn-sm" onclick="showSeatPicker('${code}','${pid}')" ${isBtnDisabled ? 'disabled style="opacity:0.5; cursor:not-allowed;"' : ''} title="${esc(btnTitle)}">
            <i data-lucide="check-square"></i> Check-in
          </button>
          <button class="btn btn-sm" onclick="doBoard('${code}','${pid}')" ${p && (!p.checkedIn || p.boarded) ? 'disabled style="opacity:0.5; cursor:not-allowed;"' : ''}>
            <i data-lucide="navigation-2"></i> Lên máy bay
          </button>
        </span>
      </div>`;
  }).join("") || "<p class='hint'><i data-lucide='users'></i> Chuyến chưa có hành khách nào đăng ký.</p>";

  $("#modal-body").innerHTML = `
    <h4 style="margin-top:0"><i data-lucide="users" style="width:14px; height:14px; vertical-align:middle;"></i> Hành khách</h4>
    ${pax}
    
    <div id="seat-picker-container"></div>
    
    <h4><i data-lucide="clock-alert" style="width:14px; height:14px; vertical-align:middle;"></i> Hoãn chuyến</h4>
    <div class="field">
      <input id="m-min" placeholder="Số phút (ví dụ 60)" type="text" />
      <input id="m-reason" placeholder="Lý do hoãn chuyến" type="text" />
      <button class="btn btn-sm" onclick="doDelay('${code}')">Hoãn</button>
    </div>
    
    <h4><i data-lucide="ban" style="width:14px; height:14px; vertical-align:middle;"></i> Huỷ chuyến</h4>
    <div class="field">
      <input id="m-creason" placeholder="Lý do huỷ chuyến" type="text" />
      <button class="btn btn-sm btn-warn" onclick="doCancel('${code}')">Huỷ chuyến</button>
    </div>`;
    
  $("#modal").classList.remove("hidden");
  lucide.createIcons();
}

function closeModal() { 
  $("#modal").classList.add("hidden"); 
}

// Show seat selector grid in modal
function showSeatPicker(code, pid) {
  const occupied = STATE.passengers
    .filter(p => p.flight === code && p.seat)
    .map(p => p.seat.toUpperCase());
  
  const rows = [1, 2, 3, 4, 5];
  const cols = ['A', 'B', 'C', 'D', 'E', 'F'];
  
  let html = `
    <div class="seat-map-container" style="animation: fadeIn 0.25s ease;">
      <p style="font-weight:600; font-size:13px; margin-bottom:12px; display:flex; align-items:center; gap:6px;">
        <i data-lucide="armchair" style="width:16px; height:16px; color:var(--accent);"></i> Chọn ghế ngồi cho khách: <b style="color:var(--accent);">${pid}</b>
      </p>
      <div class="seat-map">
        <div class="seat-map-header">
          <div class="seat-map-col-label">A</div>
          <div class="seat-map-col-label">B</div>
          <div class="seat-map-col-label">C</div>
          <div class="seat-map-col-label aisle-gap"></div>
          <div class="seat-map-col-label">D</div>
          <div class="seat-map-col-label">E</div>
          <div class="seat-map-col-label">F</div>
        </div>`;
        
  rows.forEach(r => {
    html += `<div class="seat-row">
      <div class="seat-row-label">${r}</div>`;
    
    cols.forEach((c, idx) => {
      const seatCode = `${r}${c}`;
      const isOccupied = occupied.includes(seatCode);
      const seatClass = isOccupied ? 'seat occupied' : 'seat';
      const onclickAttr = isOccupied ? '' : `onclick="selectSeat('${code}', '${pid}', '${seatCode}')"`;
      
      html += `<button class="${seatClass}" ${onclickAttr}>${seatCode}</button>`;
      if (idx === 2) {
        html += `<div class="aisle-gap"></div>`;
      }
    });
    html += `</div>`;
  });
  
  html += `
        <div class="seat-legend">
          <div class="legend-item"><div class="legend-dot available"></div> Ghế trống</div>
          <div class="legend-item"><div class="legend-dot occupied"></div> Đã chọn</div>
        </div>
      </div>
    </div>`;
    
  $("#seat-picker-container").innerHTML = html;
  lucide.createIcons();
}

async function selectSeat(code, pid, seatCode) {
  const r = await api("/api/flight/checkin", { code, pid, seat: seatCode });
  toast(r.message, !r.ok);
  await load();
  openModal(code);
}

async function doBoard(code, pid) {
  const r = await api("/api/flight/board", { code, pid });
  toast(r.message, !r.ok); 
  await load(); 
  openModal(code);
}

async function doDelay(code) {
  const r = await api("/api/flight/delay", { code, minutes: $("#m-min").value, reason: $("#m-reason").value });
  toast(r.message, !r.ok); 
  closeModal(); 
  await load();
}

async function doCancel(code) {
  const r = await api("/api/flight/cancel", { code, reason: $("#m-creason").value });
  toast(r.message, !r.ok); 
  closeModal(); 
  await load();
}

// ---------- Nhật ký sự kiện ----------
function classifyLog(text) {
  if (/→/.test(text) && /:/.test(text) && !/Tạo chuyến|gán |đặt chỗ/.test(text))
    return "status";
  if (/Tua thời gian|Tua th/.test(text))
    return "tick";
  return "assign";
}

const LOG_STYLE = {
  status: { icon: "git-commit-horizontal", color: "var(--accent)" },
  tick:   { icon: "timer",                 color: "var(--muted)" },
  assign: { icon: "package-plus",          color: "var(--green)" },
};

let logFilter = "all";

function renderLog() {
  const box = $("#log-list");
  if (!box) return;
  if (!STATE.eventLog || !STATE.eventLog.length) {
    box.innerHTML = "<p class='hint'><i data-lucide='inbox'></i> Chưa có sự kiện.</p>";
    lucide.createIcons();
    return;
  }

  const items = STATE.eventLog
    .map((e) => ({ ...e, cat: classifyLog(e.text) }))
    .filter((e) => logFilter === "all" || e.cat === logFilter);

  if (!items.length) {
    box.innerHTML = "<p class='hint'>Không có sự kiện phù hợp bộ lọc.</p>";
    return;
  }

  let html = '<div class="log-timeline">';
  items.forEach((e) => {
    const st = LOG_STYLE[e.cat] || LOG_STYLE.assign;
    const timeParts = e.time.split(" ");
    const day = timeParts[0] || "";
    const hour = timeParts[1] || "";

    html += `
      <div class="log-entry" data-cat="${e.cat}">
        <div class="log-dot" style="border-color:${st.color};box-shadow:0 0 6px ${st.color}40"></div>
        <div class="log-content">
          <span class="log-time">${esc(hour)}<small>${esc(day)}</small></span>
          <span class="log-text">${esc(e.text)}</span>
        </div>
      </div>`;
  });
  html += "</div>";
  box.innerHTML = html;
  lucide.createIcons();
}

// ---------- Form tạo chuyến bay ----------
function setOpts(sel, html) {
  const el = $(sel);
  if (!el) return;
  const prev = el.value;
  el.innerHTML = html;
  if ([...el.options].some((o) => o.value === prev)) el.value = prev;
}

function initCreateForm() {
  if (!STATE) return;
  // Sân bay đi là sân bay nhà (chỉ 1) — chọn sẵn, đích là thành phố nhập tay.
  setOpts("#c-origin", STATE.airports.map((a) =>
    `<option value="${a.code}">${a.code} — ${esc(a.name)}</option>`).join(""));
  setOpts("#c-aircraft", `<option value="">(chưa gán)</option>` +
    STATE.aircrafts.map((a) =>
      `<option value="${a.registration}">${a.registration} · ${esc(a.category)}</option>`).join(""));
  setOpts("#c-gate", `<option value="">Tự động</option>`);
}

async function doCreateFlight() {
  const code = $("#c-code").value.trim();
  const r = await api("/api/flight/create", {
    code,
    origin: $("#c-origin").value,
    dest: $("#c-dest").value.trim(),
    departure: $("#c-dep").value.replace("T", " "),
    arrival: $("#c-arr").value.replace("T", " "),
  });
  if (!r.ok) { toast(r.message, true); return; }

  const msgs = [r.message];
  const reg = $("#c-aircraft").value;
  if (reg) {
    msgs.push((await api("/api/flight/assign-aircraft", { code, reg })).message);
    msgs.push((await api("/api/flight/assign-gate", { code, gate: $("#c-gate").value })).message);
  }

  toast(msgs.join(" | "), false);
  $("#c-code").value = "";
  $("#c-dest").value = "";
  await load();
}

// ---------- Đồng hồ mô phỏng ----------
let simTimer = null;

async function doTick() {
  const r = await api("/api/sim/tick", { minutes: $("#sim-step").value });
  await load();
  if (!r.ok) toast(r.message, true);
}

function setPlayLabel(running) {
  $("#btn-play").innerHTML = running
    ? `<i data-lucide="pause"></i> Dừng`
    : `<i data-lucide="play"></i> Chạy`;
  lucide.createIcons();
}

function stopPlay() {
  if (simTimer) { clearInterval(simTimer); simTimer = null; }
  setPlayLabel(false);
}

function togglePlay() {
  if (simTimer) { stopPlay(); return; }
  simTimer = setInterval(doTick, 2000); // Step every 2 seconds real-time
  setPlayLabel(true);
}

// ---------- Tabs + nút toàn cục ----------
function activateTab(name) {
  $$(".tab").forEach((x) => x.classList.remove("active"));
  $$(".panel").forEach((x) => x.classList.remove("active"));
  const tabBtn = [...$$(".tab")].find((t) => t.dataset.tab === name);
  if (tabBtn) tabBtn.classList.add("active");
  const panel = $("#tab-" + name);
  if (panel) panel.classList.add("active");
}

function initTabs() {
  $$(".tab").forEach((t) => t.addEventListener("click", () => activateTab(t.dataset.tab)));
}

// ---------- Đăng nhập / phân quyền ----------
async function doLogin() {
  const username = $("#login-user").value.trim();
  const password = $("#login-pass").value;
  if (!username || !password) { $("#login-error").textContent = "Nhập đủ tên đăng nhập và mật khẩu."; return; }
  const r = await api("/api/login", { username, password });
  if (!r.ok) { $("#login-error").textContent = r.message; return; }
  CURRENT_USER = r.user;
  localStorage.setItem("skygate_user", JSON.stringify(r.user));
  $("#login-error").textContent = "";
  $("#login-pass").value = "";
  applyAuth();
}

function doLogout() {
  stopPlay();
  localStorage.removeItem("skygate_user");
  CURRENT_USER = null;
  applyAuth();
}

// Áp dụng trạng thái đăng nhập: hiện/ẩn dashboard, ẩn tab/nút theo vai trò.
function applyAuth() {
  const logged = !!CURRENT_USER;
  $("#login-screen").style.display = logged ? "none" : "flex";
  document.body.classList.toggle("logged-in", logged);
  if (!logged) return;

  $("#user-info").innerHTML = `<i data-lucide="user"></i> ${esc(CURRENT_USER.fullName)} · <b>${esc(CURRENT_USER.role)}</b>`;

  // Ẩn các tab vai trò không có quyền.
  $$(".tab").forEach((t) => { t.style.display = can(t.dataset.tab) ? "" : "none"; });
  // Đồng hồ mô phỏng & reset: chỉ Admin/Staff; logout: luôn hiện.
  $("#clock-bar").style.display = isRole("Admin", "Staff") ? "" : "none";
  $("#btn-reset").style.display = isRole("Admin") ? "" : "none";
  // Công cụ quản trị (thêm máy bay/đường băng): chỉ Admin.
  $$(".admin-only").forEach((el) => { el.style.display = isRole("Admin") ? "" : "none"; });

  // Chọn tab hợp lệ đầu tiên.
  const firstTab = [...$$(".tab")].find((t) => t.style.display !== "none");
  if (firstTab) activateTab(firstTab.dataset.tab);

  lucide.createIcons();
  load();
}

// ---------- Đặt vé ----------
function initBookingForm() {
  if (!STATE) return;
  const opts = STATE.flights
    .filter((f) => f.availableSeats > 0 && f.status !== "Cancelled" && f.status !== "Completed")
    .map((f) => `<option value="${f.code}">${f.code} · ${esc(f.origin)}→${esc(f.dest)} · còn ${f.availableSeats} ghế</option>`)
    .join("");
  setOpts("#t-flight", opts || `<option value="">(không có chuyến còn ghế)</option>`);
}

async function doBookTicket() {
  const code = $("#t-flight").value;
  const passengerName = $("#t-name").value.trim();
  if (!code) { toast("Chưa chọn chuyến bay.", true); return; }
  if (!passengerName) { toast("Nhập tên hành khách.", true); return; }
  const r = await api("/api/ticket/book", { code, passengerName });
  toast(r.message, !r.ok);
  if (r.ok) $("#t-name").value = "";
  await load();
}

// Bấm "Mua vé" trên thẻ chuyến bay: nhảy sang tab Đặt vé và chọn sẵn chuyến.
function bookFromFlight(code) {
  activateTab("booking");
  const sel = $("#t-flight");
  if (sel && [...sel.options].some((o) => o.value === code)) sel.value = code;
  $("#t-name").focus();
}

async function doCancelTicket(ticketId) {
  if (!confirm("Huỷ vé " + ticketId + "?")) return;
  const r = await api("/api/ticket/cancel", { ticketId });
  toast(r.message, !r.ok);
  await load();
}

function renderTickets() {
  const tbody = $("#tickets-table tbody");
  if (!tbody) return;
  let list = STATE.tickets || [];
  // Customer chỉ thấy vé của mình; Staff/Admin thấy tất cả.
  if (isRole("Customer")) {
    list = list.filter((t) => t.owner === CURRENT_USER.username);
    $("#tickets-title").textContent = "Vé của tôi";
  } else {
    $("#tickets-title").textContent = "Tất cả vé đã bán";
  }
  if (!list.length) {
    tbody.innerHTML = `<tr><td colspan="5" class="hint">Chưa có vé.</td></tr>`;
    return;
  }
  tbody.innerHTML = list.map((t) => `
    <tr>
      <td><b>${esc(t.ticketId)}</b></td>
      <td>${esc(t.passengerName)}</td>
      <td>${esc(t.flight)}</td>
      <td>${esc(t.owner)}</td>
      <td><button class="btn btn-sm btn-warn" onclick="doCancelTicket('${t.ticketId}')"><i data-lucide="x"></i> Huỷ</button></td>
    </tr>`).join("");
  lucide.createIcons();
}

// ---------- Quản lý tài khoản (Admin) ----------
async function doAddUser() {
  const r = await api("/api/user/create", {
    role: $("#u-role").value,
    username: $("#u-username").value.trim(),
    password: $("#u-password").value,
    fullName: $("#u-fullname").value.trim(),
  });
  toast(r.message, !r.ok);
  if (r.ok) { $("#u-username").value = ""; $("#u-password").value = ""; $("#u-fullname").value = ""; }
  await renderUsers();
}

async function doDeleteUser(username) {
  if (!confirm("Xoá tài khoản " + username + "?")) return;
  const r = await api("/api/user/delete", { username });
  toast(r.message, !r.ok);
  await renderUsers();
}

async function renderUsers() {
  const tbody = $("#users-table tbody");
  if (!tbody || !isRole("Admin")) return;
  const r = await api("/api/users?actor=" + encodeURIComponent(CURRENT_USER.username));
  const list = (r && r.users) || [];
  tbody.innerHTML = list.map((u) => `
    <tr>
      <td><b>${esc(u.username)}</b></td>
      <td>${esc(u.fullName)}</td>
      <td>${esc(u.role)}</td>
      <td><button class="btn btn-sm btn-warn" onclick="doDeleteUser('${esc(u.username)}')"><i data-lucide="trash-2"></i> Xoá</button></td>
    </tr>`).join("");
  lucide.createIcons();
}

window.addEventListener("DOMContentLoaded", () => {
  initTabs();

  // Đăng nhập / đăng xuất
  loadUser();
  $("#btn-login").addEventListener("click", doLogin);
  $("#login-pass").addEventListener("keydown", (e) => { if (e.key === "Enter") doLogin(); });
  $("#login-user").addEventListener("keydown", (e) => { if (e.key === "Enter") doLogin(); });
  $("#btn-logout").addEventListener("click", doLogout);

  // Đặt vé & quản lý tài khoản & hạ tầng (Admin)
  $("#btn-book").addEventListener("click", doBookTicket);
  $("#btn-adduser").addEventListener("click", doAddUser);
  $("#btn-addaircraft").addEventListener("click", doAddAircraft);
  $("#btn-addrunway").addEventListener("click", doAddRunway);

  // Tìm kiếm hành khách (lọc tại chỗ, không gọi lại server).
  $("#pax-search").addEventListener("input", (e) => {
    paxSearchTerm = e.target.value;
    if (STATE) renderPassengers();
  });

  $("#btn-reload").addEventListener("click", load);
  $("#modal-close").addEventListener("click", closeModal);
  $("#modal").addEventListener("click", (e) => { if (e.target.id === "modal") closeModal(); });

  $("#btn-reset").addEventListener("click", async () => {
    if (!confirm("Nạp lại dữ liệu mẫu? Thao tác hiện tại sẽ mất.")) return;
    stopPlay();
    const r = await api("/api/reset", {});
    toast(r.message, !r.ok);
    await load();
  });

  $("#btn-step").addEventListener("click", doTick);
  $("#btn-play").addEventListener("click", togglePlay);
  $("#btn-create").addEventListener("click", doCreateFlight);

  $$(".log-filter").forEach((btn) => btn.addEventListener("click", () => {
    $$(".log-filter").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    logFilter = btn.dataset.filter;
    renderLog();
  }));

  // Hiện dashboard nếu đã đăng nhập, ngược lại hiện màn hình đăng nhập.
  applyAuth();
});
