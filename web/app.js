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

// "YYYY-MM-DD HH:MM" -> "DD/MM/YYYY HH:MM" (hiển thị ngày-tháng-năm).
function formatDMY(s) {
  if (!s) return s;
  const [d, t] = String(s).split(" ");
  const parts = (d || "").split("-");
  if (parts.length !== 3) return s;
  return `${parts[2]}/${parts[1]}/${parts[0]}${t ? " " + t : ""}`;
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
  renderAircrafts();
  renderPassengers();
  renderMonitor();

  // Simulated Clock & Event Logs
  if (STATE.currentTime) $("#sim-now").textContent = formatDMY(STATE.currentTime);
  renderLog();
  initCreateForm();

  // Đặt vé & quản lý tài khoản
  initBookingForm();
  renderTickets();
  if (isRole("Admin")) {
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
    { id: 'bd', label: 'Lên máy bay', matches: ['Boarding', 'GateClosed', 'Ready'] },
    { id: 'ia', label: 'Đang bay', matches: ['Takeoff', 'InAir'] },
    { id: 'ld', label: 'Hạ cánh', matches: ['Landed', 'Turnaround', 'Completed'] }
  ];
  
  let currentIdx = -1;
  if (['Scheduled', 'Delayed', 'CheckIn', 'Check-in'].includes(status)) currentIdx = 0;
  else if (['Boarding', 'GateClosed', 'Ready'].includes(status)) currentIdx = 1;
  else if (['Takeoff', 'InAir'].includes(status)) currentIdx = 2;
  else if (['Landed', 'Turnaround', 'Completed'].includes(status)) currentIdx = 3;
  
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
      
      <div class="row"><i data-lucide="clock" style="width:14px; height:14px;"></i> Khởi hành: <b>${esc(formatDMY(f.departure)) || "—"}</b></div>
      <div class="row"><i data-lucide="clock-arrow-up" style="width:14px; height:14px;"></i> Dự kiến đến: <b>${esc(formatDMY(f.arrival)) || "—"}</b></div>
      <div class="row">
        <i data-lucide="info" style="width:14px; height:14px;"></i> 
        <span>Máy bay: <b>${esc(f.aircraft) || "(chưa gán)"}</b> · Cổng: <b>${esc(f.gate) || "—"}</b></span>
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
        <button class="btn btn-sm btn-warn" onclick="openModal('${f.code}')">
          <i data-lucide="settings"></i> Thao tác chuyến
        </button>` : ""}
        ${isRole("Admin") ? `
        <button class="btn btn-sm btn-warn" onclick="doDeleteFlight('${f.code}')">
          <i data-lucide="trash-2"></i> Xoá
        </button>` : ""}
        ${can("booking") && ['Scheduled', 'Check-in', 'CheckIn', 'Boarding', 'Delayed'].includes(f.status) ? `
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

// ---------- Máy bay ----------
function renderAircrafts() {
  const admin = isRole("Admin");
  $("#aircrafts-table tbody").innerHTML = STATE.aircrafts.map((a) => {
    const currentFlight = STATE.flights.find(f => f.aircraft === a.registration && !['Completed', 'Cancelled'].includes(f.status));
    let statusBadge = `<span class="badge completed">Sẵn sàng</span>`;
    if (currentFlight) {
      if (['Takeoff', 'InAir'].includes(currentFlight.status)) {
        statusBadge = `<span class="badge boarding">Đang bay (${currentFlight.code})</span>`;
      } else if (['Landed', 'Turnaround'].includes(currentFlight.status)) {
        statusBadge = `<span class="badge scheduled" style="background-color: var(--accent); color: white;">Quay đầu (${currentFlight.code})</span>`;
      } else {
        statusBadge = `<span class="badge checkin">Đang đỗ (${currentFlight.code})</span>`;
      }
    }
    return `
      <tr>
        <td><b>${esc(a.registration)}</b></td>
        <td>${esc(a.model)}</td>
        <td><span class="badge scheduled">${esc(a.category)}</span></td>
        <td><b>${a.capacity}</b> ghế</td>
        <td>${a.requiredRunway}m</td>
        <td>${statusBadge}</td>
        <td>${admin ? `<button class="btn btn-sm btn-warn" onclick="doDeleteAircraft('${esc(a.registration)}')"><i data-lucide="trash-2"></i></button>` : ""}</td>
      </tr>`;
  }).join("");
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

// ---------- Hành khách ----------
function checkMark(val) {
  return val
    ? `<span style="color:var(--green); font-weight:bold; font-size:15px;"><i data-lucide="check-circle-2" style="width:16px; height:16px; display:inline-block; vertical-align:middle;"></i></span>`
    : `<span style="color:var(--muted); font-size:15px;"><i data-lucide="minus" style="width:14px; height:14px; display:inline-block; vertical-align:middle;"></i></span>`;
}

// Nhãn hành trình của một hành khách theo trạng thái.
function journeyBadge(p) {
  if (!p.flight) return `<span class="badge scheduled">Chưa đặt chỗ</span>`;
  
  const f = STATE.flights.find(x => x.code === p.flight);
  if (!f) return `<span class="badge checkin">Chờ check-in</span>`;
  
  if (f.status === 'Cancelled') {
    return `<span class="badge cancelled" style="background-color: var(--border); color: var(--muted);">Chuyến bị huỷ</span>`;
  }
  
  const isPastBoarding = ['GateClosed', 'Ready', 'Takeoff', 'InAir', 'Landed', 'Turnaround', 'Completed'].includes(f.status);
  
  if (p.boarded) {
    if (['Takeoff', 'InAir'].includes(f.status)) {
      return `<span class="badge boarding">Đang bay</span>`;
    }
    if (['Landed', 'Turnaround', 'Completed'].includes(f.status)) {
      return `<span class="badge completed">Đã hoàn thành</span>`;
    }
    return `<span class="badge completed">Đã lên máy bay</span>`;
  }
  
  if (isPastBoarding) {
    return `<span class="badge cancelled" style="background-color: rgba(239, 68, 68, 0.1); color: var(--warn); border: 1px solid rgba(239, 68, 68, 0.2);">Trễ chuyến (No-show)</span>`;
  }
  
  if (p.checkedIn) {
    return `<span class="badge boarding">Đã check-in</span>`;
  }
  
  return `<span class="badge checkin">Chờ check-in</span>`;
}

let paxSearchTerm = "";
function renderPassengers() {
  const tbody = $("#passengers-table tbody");
  if (!tbody) return;
  const term = paxSearchTerm.trim().toLowerCase();
  const list = STATE.passengers.filter((p) => {
    if (!term) return true;
    return [p.id, p.name, p.flight, p.phone]
      .some((v) => String(v || "").toLowerCase().includes(term));
  });
  if (!list.length) {
    tbody.innerHTML = `<tr><td colspan="6" class="hint">Không có hành khách khớp.</td></tr>`;
    return;
  }
  tbody.innerHTML = list.map((p) => `
    <tr>
      <td><b>${esc(p.id)}</b></td>
      <td>
        ${esc(p.name)}
        ${p.phone ? `<br/><small style="color:var(--muted); font-size:11px; display:inline-flex; align-items:center; gap:2px; margin-top:2px;"><i data-lucide="phone" style="width:10px; height:10px;"></i>${esc(p.phone)}</small>` : ""}
      </td>
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
    // Chuyến gần đầy / hết ghế.
    if (f.availableSeats === 0) {
      alerts.push({ level: "warn", icon: "users",
        text: `Chuyến <b>${esc(f.code)}</b> đã <b>hết ghế</b> (${f.capacity}/${f.capacity}).` });
    } else if (f.availableSeats > 0 && f.capacity > 0 && f.availableSeats <= Math.max(1, Math.round(f.capacity * 0.1))) {
      alerts.push({ level: "info", icon: "users",
        text: `Chuyến <b>${esc(f.code)}</b> sắp đầy — còn <b>${f.availableSeats}</b> ghế.` });
    }
  }
  // Hành khách hành lý quá cân hoặc quá số kiện.
  const overweight = STATE.passengers.filter((p) => p.bagOverweight && p.flight);
  for (const p of overweight) {
    let warningText = "";
    if (p.bagPieces > 2 && p.bagWeight > 46.0) {
      warningText = `Hành khách <b>${esc(p.name)}</b> (${esc(p.flight)}) hành lý vượt quá số kiện (${p.bagPieces} kiện) và quá cân (${p.bagWeight}kg).`;
    } else if (p.bagPieces > 2) {
      warningText = `Hành khách <b>${esc(p.name)}</b> (${esc(p.flight)}) mang quá số kiện cho phép: <b>${p.bagPieces} kiện</b> (cho phép tối đa 2 kiện).`;
    } else if (p.bagWeight > 46.0) {
      warningText = `Hành khách <b>${esc(p.name)}</b> (${esc(p.flight)}) hành lý quá cân: <b>${p.bagWeight}kg</b> (cho phép tối đa 46kg).`;
    } else {
      warningText = `Hành khách <b>${esc(p.name)}</b> (${esc(p.flight)}) hành lý vượt tiêu chuẩn: ${p.bagPieces} kiện, ${p.bagWeight}kg.`;
    }
    alerts.push({ level: "info", icon: "luggage", text: warningText });
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

  const box = $("#monitor-list");
  if (box) {
    const monitored = activeFlights;
    box.innerHTML = monitored.length
      ? monitored.map((f) => {
          const pct = f.capacity ? Math.round((f.paxCount / f.capacity) * 100) : 0;
          return `
          <div class="card monitor-card">
            <h3>
              <span class="card-title-left"><i data-lucide="plane-takeoff" style="color:var(--accent);"></i> <b>${esc(f.code)}</b> → ${esc(f.dest)}</span>
              <span class="${statusClass(f.status)}">${esc(f.status)}</span>
            </h3>
            <div class="row"><i data-lucide="door-open" style="width:14px;height:14px;"></i> Cổng: <b>${esc(f.gate) || "—"}</b> · Khởi hành: <b>${esc(formatDMY(f.departure)) || "—"}</b></div>
            <div class="monitor-stats">
              <div class="mstat"><span class="mstat-num">${f.paxCount}</span><span class="mstat-lbl">Đã đặt</span></div>
              <div class="mstat"><span class="mstat-num">${f.availableSeats >= 0 ? f.availableSeats : "—"}</span><span class="mstat-lbl">Ghế trống</span></div>
            </div>
            <div class="loadbar"><div class="loadbar-fill" style="--fill-pct: ${pct / 100};"></div></div>
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
  
  const paxList = f.passengers.map((pid) => {
    const p = STATE.passengers.find((x) => x.id === pid);
    const name = p ? p.name : pid;
    let stText = "Chờ check-in";
    let statusClass = "checkin";
    if (f.status === 'Cancelled') {
      stText = "Chuyến bị huỷ";
      statusClass = "cancelled";
    } else {
      const isPastBoarding = ['GateClosed', 'Ready', 'Takeoff', 'InAir', 'Landed', 'Turnaround', 'Completed'].includes(f.status);
      if (p && p.boarded) {
        if (['Takeoff', 'InAir'].includes(f.status)) {
          stText = "Đang bay";
          statusClass = "boarding";
        } else if (['Landed', 'Turnaround', 'Completed'].includes(f.status)) {
          stText = "Đã hoàn thành";
          statusClass = "completed";
        } else {
          stText = "Đã lên máy bay";
          statusClass = "completed";
        }
      } else if (isPastBoarding) {
        stText = "Trễ chuyến (No-show)";
        statusClass = "cancelled";
      } else if (p && p.checkedIn) {
        stText = `Đã check-in${p.seat ? ' (Ghế ' + p.seat + ')' : ''}`;
        statusClass = "boarding";
      }
    }
    
    const checkinDisabled = (!['Check-in', 'Boarding'].includes(f.status)) || (p && p.checkedIn);
    const boardDisabled = (f.status !== 'Boarding') || (p && (!p.checkedIn || p.boarded));
    
    return `
      <div class="pax-line">
        <div class="pax-meta">
          <div class="pax-user-info">
            <span class="pax-name">${esc(name)}</span>
            <span class="pax-id">${esc(pid)}</span>
          </div>
          <div class="pax-status-wrap">
            <span class="badge ${statusClass}">${stText}</span>
          </div>
        </div>
        <span class="pax-actions">
          <button class="btn btn-sm" onclick="doCheckin('${code}','${pid}')" ${checkinDisabled ? 'disabled' : ''}>
            <i data-lucide="check-square" style="width:12px; height:12px; vertical-align:middle; margin-right:2px;"></i> Check-in
          </button>
          <button class="btn btn-sm" onclick="doBoard('${code}','${pid}')" ${boardDisabled ? 'disabled' : ''}>
            <i data-lucide="navigation-2" style="width:12px; height:12px; vertical-align:middle; margin-right:2px;"></i> Lên máy bay
          </button>
        </span>
      </div>`;
  }).join("") || "<p class='hint'><i data-lucide='users'></i> Chuyến chưa có hành khách nào đăng ký.</p>";

  let bulkHtml = "";
  if (f.passengers.length > 0) {
    const bulkCheckinDisabled = !['Check-in', 'Boarding'].includes(f.status);
    const bulkBoardDisabled = f.status !== 'Boarding';
    bulkHtml = `
      <div style="display: flex; gap: 8px; margin-bottom: 12px;">
        <button class="btn btn-sm" onclick="doCheckinAll('${code}')" ${bulkCheckinDisabled ? 'disabled' : ''} style="flex: 1;">
          <i data-lucide="check-square"></i> Check-in toàn bộ
        </button>
        <button class="btn btn-sm" onclick="doBoardAll('${code}')" ${bulkBoardDisabled ? 'disabled' : ''} style="flex: 1;">
          <i data-lucide="navigation-2"></i> Cho lên toàn bộ
        </button>
      </div>`;
  }

  let adminOpsHtml = "";
  const isPastTakeoff = ['Takeoff', 'InAir', 'Landed', 'Turnaround', 'Completed', 'Cancelled'].includes(f.status);
  if (isRole("Admin") && !isPastTakeoff) {
    adminOpsHtml = `
      <h4><i data-lucide="clock" style="width:14px; height:14px; vertical-align:middle;"></i> Hoãn chuyến</h4>
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
  }

  $("#modal-body").innerHTML = `
    <h4 style="margin-top:0"><i data-lucide="users" style="width:14px; height:14px; vertical-align:middle; margin-right:4px;"></i> Danh sách hành khách</h4>
    ${bulkHtml}
    <div style="max-height: 250px; overflow-y: auto; margin-bottom: 16px; border: 1px solid var(--border); padding: 8px; border-radius: 8px; background: rgba(15, 23, 42, 0.02);">
      ${paxList}
    </div>
    ${adminOpsHtml}`;
    
  $("#modal").classList.remove("hidden");
  lucide.createIcons();
}

function closeModal() { 
  $("#modal").classList.add("hidden"); 
}

// Check-in: ghế đã gán khi đặt vé nên chỉ cần xác nhận (không gửi seat).
async function doCheckin(code, pid) {
  const r = await api("/api/flight/checkin", { code, pid });
  toast(r.message, !r.ok);
  await load();
  openModal(code);
}

async function doCheckinAll(code) {
  const r = await api("/api/flight/checkin-all", { code });
  toast(r.message, !r.ok);
  await load();
  openModal(code);
}

// Check-in những khách đã tích chọn (dùng khi ít khách).
async function doCheckinSelected(code) {
  const pids = [...document.querySelectorAll(".pax-check:checked")].map((c) => c.value);
  if (!pids.length) { toast("Chưa tích khách nào để check-in.", true); return; }
  let ok = 0, fail = 0;
  for (const pid of pids) {
    const r = await api("/api/flight/checkin", { code, pid });
    r.ok ? ok++ : fail++;
  }
  toast(`Đã check-in ${ok} khách đã chọn${fail ? `, ${fail} lỗi` : ""}.`, fail > 0 && ok === 0);
  await load();
  openModal(code);
}

async function doBoardAll(code) {
  const r = await api("/api/flight/board-all", { code });
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
    const day = formatDMY(timeParts[0] || "");
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

// Thành phố đích từ sân bay nhà SKG + thời lượng bay (phút) -> dùng để tự tính giờ đến.
const CITY_DURATIONS = [
  { name: "Ha Noi", min: 120 },
  { name: "Da Nang", min: 60 },
  { name: "Hue", min: 55 },
  { name: "Nha Trang", min: 75 },
  { name: "Da Lat", min: 70 },
  { name: "Phu Quoc", min: 110 },
  { name: "Can Tho", min: 100 },
  { name: "Hai Phong", min: 115 },
  { name: "Vinh", min: 90 },
  { name: "Quy Nhon", min: 70 },
  { name: "Buon Ma Thuot", min: 80 },
];

// Cộng `mins` phút vào chuỗi datetime-local "YYYY-MM-DDTHH:MM".
function addMinutesToLocal(local, mins) {
  if (!local) return "";
  const d = new Date(local);
  if (isNaN(d)) return "";
  d.setMinutes(d.getMinutes() + mins);
  const p = (n) => String(n).padStart(2, "0");
  return `${d.getFullYear()}-${p(d.getMonth() + 1)}-${p(d.getDate())}T${p(d.getHours())}:${p(d.getMinutes())}`;
}

// Phân tích chuỗi thời gian "YYYY-MM-DD HH:MM" hoặc "YYYY-MM-DDTHH:MM" thành đối tượng Date
function parseDateStr(s) {
  if (!s) return null;
  const clean = s.replace("T", " ");
  const [d, t] = clean.split(" ");
  if (!d || !t) return null;
  const [Y, M, D] = d.split("-").map(Number);
  const [h, m] = t.split(":").map(Number);
  return new Date(Y, (M || 1) - 1, D || 1, h || 0, m || 0);
}

// Cập nhật danh sách máy bay rảnh rỗi dựa trên lịch bay đã nhập
function updateAvailableAircrafts() {
  if (!STATE) return;
  const cDepVal = $("#c-dep").value;
  const cArrVal = $("#c-arr").value;
  const newDep = parseDateStr(cDepVal);
  const newArr = parseDateStr(cArrVal);

  const availableAircrafts = STATE.aircrafts.filter((a) => {
    // Kiểm tra xem máy bay có bị trùng lịch với chuyến bay nào đang hoạt động không
    for (const f of STATE.flights) {
      if (!f.aircraft || f.aircraft !== a.registration) continue;
      if (["Completed", "Cancelled"].includes(f.status)) continue;

      if (newDep && newArr) {
        const otherDep = parseDateStr(f.departure);
        const otherArr = parseDateStr(f.arrival);
        if (otherDep && otherArr) {
          const turnaroundMs = 0; // Đã bỏ thời gian quay đầu
          const newArrWithTurnaround = new Date(newArr.getTime() + turnaroundMs);
          const otherArrWithTurnaround = new Date(otherArr.getTime() + turnaroundMs);
          if (!(newArrWithTurnaround <= otherDep || otherArrWithTurnaround <= newDep)) {
            return false; // Bị trùng lịch
          }
        }
      } else {
        // Nếu chưa nhập đủ giờ bay mới, coi như bận nếu đang chạy chuyến nào đó
        return false;
      }
    }
    return true;
  });

  const currentSelected = $("#c-aircraft").value;
  setOpts("#c-aircraft", `<option value="">(chưa gán)</option>` +
    availableAircrafts.map((a) =>
      `<option value="${a.registration}">${a.registration} · ${esc(a.category)}</option>`).join(""));
  
  if (availableAircrafts.some(a => a.registration === currentSelected)) {
    $("#c-aircraft").value = currentSelected;
  } else {
    $("#c-aircraft").value = "";
  }
}

// Tự điền giờ đến = giờ đi + thời lượng bay của thành phố đã chọn.
function updateArrival() {
  const city = $("#c-dest").value;
  const dep = $("#c-dep").value;
  const item = CITY_DURATIONS.find((c) => c.name === city);
  $("#c-arr").value = item && dep ? addMinutesToLocal(dep, item.min) : "";
  updateAvailableAircrafts();
}

function initCreateForm() {
  if (!STATE) return;
  // Sân bay đi là sân bay nhà (chỉ 1) — chọn sẵn; đích chọn từ danh sách thành phố.
  setOpts("#c-origin", STATE.airports.map((a) =>
    `<option value="${a.code}">${a.code} — ${esc(a.name)}</option>`).join(""));
  setOpts("#c-dest", CITY_DURATIONS.map((c) =>
    `<option value="${esc(c.name)}">${esc(c.name)} · ${c.min}'</option>`).join(""));

  updateAvailableAircrafts();
  setOpts("#c-gate", `<option value="">Tự động</option>`);
  updateArrival();
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
  $("#c-dep").value = "";
  $("#c-arr").value = "";
  $("#c-aircraft").value = "";
  $("#c-gate").value = "";
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

function showRegister(e) {
  if (e) e.preventDefault();
  $("#login-form-wrap").style.display = "none";
  $("#login-form-wrap").classList.add("hidden");
  $("#register-form-wrap").style.display = "flex";
  $("#register-form-wrap").classList.remove("hidden");
  $("#login-error").textContent = "";
  $("#reg-error").textContent = "";
}

function showLogin(e) {
  if (e) e.preventDefault();
  $("#register-form-wrap").style.display = "none";
  $("#register-form-wrap").classList.add("hidden");
  $("#login-form-wrap").style.display = "flex";
  $("#login-form-wrap").classList.remove("hidden");
  $("#login-error").textContent = "";
  $("#reg-error").textContent = "";
}

async function doRegister() {
  const username = $("#reg-user").value.trim();
  const password = $("#reg-pass").value;
  const fullName = $("#reg-fullname").value.trim();
  if (!username || !password || !fullName) {
    $("#reg-error").textContent = "Nhập đủ thông tin đăng ký.";
    return;
  }
  const r = await api("/api/register", { username, password, fullName });
  toast(r.message, !r.ok);
  if (r.ok) {
    $("#reg-user").value = "";
    $("#reg-pass").value = "";
    $("#reg-fullname").value = "";
    $("#login-user").value = username;
    showLogin();
  } else {
    $("#reg-error").textContent = r.message;
  }
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
  $$(".admin-only").forEach((el) => { el.style.display = isRole("Admin") ? (el.classList.contains("form") ? "flex" : "block") : "none"; });

  // Chọn tab hợp lệ đầu tiên.
  const firstTab = [...$$(".tab")].find((t) => t.style.display !== "none");
  if (firstTab) activateTab(firstTab.dataset.tab);

  lucide.createIcons();
  load();
}

// ---------- Đặt vé ----------
const SEAT_COLS = ["A", "B", "C", "D", "E", "F"];
const MAX_SEATS = 100;
const BUSINESS_ROWS = 2;        // 2 hàng đầu = Thương gia
let BOOK_SEAT = "";             // ghế đang chọn ở form đặt vé
let BOOK_PHONE = "";            // số điện thoại của hành khách đang đặt

// Hạng vé suy từ mã ghế (giống backend): hàng <= 2 -> Thương gia.
function seatClassOf(seat) {
  const row = parseInt(String(seat).match(/^\d+/), 10);
  return row && row <= BUSINESS_ROWS ? "Thương gia" : "Thường";
}

let BOOK_CODE = "";             // chuyến đang đặt trong modal chọn ghế

function initBookingForm() {
  if (!STATE) return;
  const opts = STATE.flights
    .filter((f) => f.availableSeats > 0 && ['Scheduled', 'Check-in', 'CheckIn', 'Boarding', 'Delayed'].includes(f.status))
    .map((f) => `<option value="${f.code}">${f.code} · ${esc(f.origin)}→${esc(f.dest)} · còn ${f.availableSeats} ghế</option>`)
    .join("");
  setOpts("#t-flight", opts || `<option value="">(không có chuyến còn ghế)</option>`);
}

// Cập nhật nhãn ghế đang chọn + hạng (trong modal chọn ghế).
function updateSeatLabel() {
  const lbl = $("#bk-seat-label");
  if (!lbl) return;
  lbl.textContent = BOOK_SEAT || "—";
  const cl = $("#bk-class-label");
  if (BOOK_SEAT) {
    const k = seatClassOf(BOOK_SEAT);
    cl.textContent = k;
    cl.className = "badge " + (k === "Thương gia" ? "boarding" : "checkin");
  } else { cl.textContent = ""; cl.className = ""; }
}

// Mở modal chọn ghế + hành lý sau khi nhập tên và bấm "Chọn ghế".
function openBookingModal() {
  const code = $("#t-flight").value;
  const name = $("#t-name").value.trim();
  const phone = $("#t-phone").value.trim();
  if (!code) { toast("Chưa chọn chuyến bay.", true); return; }
  if (!name) { toast("Nhập tên hành khách.", true); return; }
  BOOK_CODE = code;
  BOOK_SEAT = "";
  BOOK_PHONE = phone;
  const f = STATE.flights.find((x) => x.code === code);
  $("#modal-title").innerHTML = `<i data-lucide="armchair" style="display:inline-block; vertical-align:middle; margin-right:6px;"></i> Đặt vé — ${esc(name)} · ${esc(code)} → ${esc(f ? f.dest : "")}`;
  $("#modal-body").innerHTML = `
    <div id="bk-seatboard"></div>
    <h4><i data-lucide="luggage" style="width:14px; height:14px; vertical-align:middle;"></i> Hành lý ký gửi</h4>
    <div class="field" style="gap: 16px;">
      <label style="flex: 1; display: flex; flex-direction: column; gap: 6px; font-size: 13px; color: var(--muted);">
        Số kiện hành lý (tối đa 3)
        <input id="bk-bagpieces" type="number" min="0" max="3" value="0" placeholder="Tối đa 3 kiện" style="width: 100%;" />
      </label>
      <label style="flex: 1; display: flex; flex-direction: column; gap: 6px; font-size: 13px; color: var(--muted);">
        Tổng cân nặng (kg) (tối đa 50)
        <input id="bk-bagweight" type="number" min="0" max="50" step="0.1" value="0" placeholder="Tối đa 50 kg" style="width: 100%;" />
      </label>
    </div>
    <div class="field" style="align-items:center; justify-content:space-between; margin-top: 14px;">
      <span>Ghế đang chọn: <b id="bk-seat-label">—</b> <span id="bk-class-label"></span></span>
      <button class="btn" onclick="confirmBookTicket()"><i data-lucide="shopping-cart"></i> Mua vé</button>
    </div>`;
  $("#modal").classList.remove("hidden");
  renderBookingSeats(code);
  lucide.createIcons();
}

// Sơ đồ ghế: 6 cột, hàng = ceil(sốGhế/6) (tối đa 100); ghế đã có khách bị khoá.
function renderBookingSeats(code) {
  const box = $("#bk-seatboard");
  if (!box) return;
  const f = STATE.flights.find((x) => x.code === code);
  if (!f || !f.capacity) { box.innerHTML = ""; updateSeatLabel(); return; }
  const total = Math.min(f.capacity, MAX_SEATS);
  const rows = Math.ceil(total / SEAT_COLS.length);
  const occupied = new Set(
    STATE.passengers.filter((p) => p.flight === code && p.seat).map((p) => p.seat.toUpperCase()));

  let html = `<div class="seat-map-container">
      <p style="font-weight:600; font-size:13px; margin-bottom:10px; display:flex; align-items:center; gap:6px;">
        <i data-lucide="armchair" style="width:16px; height:16px; color:var(--accent);"></i> Chọn ghế (${total} ghế · 2 hàng đầu hạng Thương gia)
      </p>
      <div class="seat-map">
        <div class="seat-map-header">
          <div class="seat-map-col-label">A</div><div class="seat-map-col-label">B</div>
          <div class="seat-map-col-label">C</div><div class="seat-map-col-label aisle-gap"></div>
          <div class="seat-map-col-label">D</div><div class="seat-map-col-label">E</div>
          <div class="seat-map-col-label">F</div>
        </div>`;
  for (let r = 1; r <= rows; r++) {
    html += `<div class="seat-row"><div class="seat-row-label">${r}</div>`;
    SEAT_COLS.forEach((c, idx) => {
      const seatIdx = (r - 1) * SEAT_COLS.length + idx;
      if (seatIdx < total) {
        const seat = `${r}${c}`;
        const cls = ["seat"];
        if (r <= BUSINESS_ROWS) cls.push("business");
        if (occupied.has(seat)) cls.push("occupied");
        if (seat === BOOK_SEAT) cls.push("selected");
        const onclick = occupied.has(seat) ? "" : `onclick="selectBookingSeat('${seat}')"`;
        html += `<button type="button" class="${cls.join(" ")}" ${onclick}>${seat}</button>`;
      } else {
        html += `<span class="seat" style="visibility:hidden;"></span>`;
      }
      if (idx === 2) html += `<div class="aisle-gap"></div>`;
    });
    html += `</div>`;
  }
  html += `<div class="seat-legend">
        <div class="legend-item"><div class="legend-dot available"></div> Trống</div>
        <div class="legend-item"><div class="legend-dot business"></div> Thương gia</div>
        <div class="legend-item"><div class="legend-dot occupied"></div> Đã có khách</div>
      </div></div></div>`;
  box.innerHTML = html;
  updateSeatLabel();
  lucide.createIcons();
}

function selectBookingSeat(seat) {
  console.log("selectBookingSeat called with:", seat);
  BOOK_SEAT = seat;
  renderBookingSeats(BOOK_CODE);
}

// Xác nhận mua vé từ modal chọn ghế.
async function confirmBookTicket() {
  console.log("confirmBookTicket called. BOOK_SEAT =", BOOK_SEAT, "BOOK_CODE =", BOOK_CODE, "BOOK_PHONE =", BOOK_PHONE);
  if (!BOOK_SEAT) { toast("Chưa chọn ghế trên sơ đồ.", true); return; }
  const passengerName = $("#t-name").value.trim();
  if (!passengerName) { toast("Nhập tên hành khách.", true); return; }
  const bagPieces = parseInt($("#bk-bagpieces").value) || 0;
  const bagWeight = parseFloat($("#bk-bagweight").value) || 0;
  if (bagPieces < 0 || bagPieces > 3) { toast("Hành lý vượt quá số kiện tối đa cho phép (tối đa 3 kiện).", true); return; }
  if (bagWeight < 0 || bagWeight > 50.0) { toast("Hành lý vượt quá khối lượng tối đa cho phép (tối đa 50 kg).", true); return; }
  const r = await api("/api/ticket/book", {
    code: BOOK_CODE, passengerName, seat: BOOK_SEAT,
    phone: BOOK_PHONE,
    bagPieces: $("#bk-bagpieces").value || "0",
    bagWeight: $("#bk-bagweight").value || "0",
  });
  toast(r.message, !r.ok);
  if (r.ok) {
    $("#t-name").value = "";
    $("#t-phone").value = "";
    BOOK_SEAT = ""; BOOK_CODE = ""; BOOK_PHONE = "";
    closeModal();
  }
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
    tbody.innerHTML = `<tr><td colspan="7" class="hint">Chưa có vé.</td></tr>`;
    return;
  }
  tbody.innerHTML = list.map((t) => {
    const p = (STATE.passengers || []).find((x) => x.id === t.pid);
    const seat = p && p.seat ? p.seat : "—";
    const cls = t.seatClass || "Thường";
    return `
    <tr>
      <td><b>${esc(t.ticketId)}</b></td>
      <td>${esc(t.passengerName)}</td>
      <td>${esc(t.flight)}</td>
      <td><span class="badge boarding">${esc(seat)}</span></td>
      <td><span class="badge ${cls === "Thương gia" ? "boarding" : "checkin"}">${esc(cls)}</span></td>
      <td>${esc(t.owner)}</td>
      <td><button class="btn btn-sm btn-warn" onclick="doCancelTicket('${t.ticketId}')"><i data-lucide="x"></i> Huỷ</button></td>
    </tr>`; }).join("");
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

  // Toggle đăng nhập / đăng ký & submit đăng ký
  $("#link-to-register").addEventListener("click", showRegister);
  $("#link-to-login").addEventListener("click", showLogin);
  $("#btn-register-submit").addEventListener("click", doRegister);
  $("#reg-pass").addEventListener("keydown", (e) => { if (e.key === "Enter") doRegister(); });
  $("#reg-user").addEventListener("keydown", (e) => { if (e.key === "Enter") doRegister(); });
  $("#reg-fullname").addEventListener("keydown", (e) => { if (e.key === "Enter") doRegister(); });

  // Đặt vé & quản lý tài khoản & hạ tầng (Admin)
  $("#t-phone").addEventListener("input", (e) => {
    e.target.value = e.target.value.replace(/\D/g, "");
  });
  $("#btn-book").addEventListener("click", openBookingModal);
  $("#btn-adduser").addEventListener("click", doAddUser);
  $("#btn-addaircraft").addEventListener("click", doAddAircraft);

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
  $("#c-dest").addEventListener("change", updateArrival);
  $("#c-dep").addEventListener("input", updateArrival);

  $$(".log-filter").forEach((btn) => btn.addEventListener("click", () => {
    $$(".log-filter").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    logFilter = btn.dataset.filter;
    renderLog();
  }));

  // Hiện dashboard nếu đã đăng nhập, ngược lại hiện màn hình đăng nhập.
  applyAuth();
});
