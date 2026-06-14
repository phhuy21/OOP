// SkyGate Web — gọi REST API của backend C++ và render giao diện.
const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => document.querySelectorAll(sel);
let STATE = null;

async function api(path, params) {
  const opt = { method: params ? "POST" : "GET" };
  if (params) {
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
  renderPeople();
  initWeatherForm();

  // Đồng hồ mô phỏng + nhật ký + form tạo chuyến
  if (STATE.currentTime) $("#sim-now").textContent = STATE.currentTime;
  renderLog();
  initCreateForm();

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
    { id: 'sc', label: 'Check-in', matches: ['Scheduled', 'Delayed', 'CheckIn'] },
    { id: 'bd', label: 'Lên máy bay', matches: ['Boarding'] },
    { id: 'ia', label: 'Đang bay', matches: ['Takeoff', 'InAir'] },
    { id: 'ld', label: 'Hạ cánh', matches: ['Landed', 'Completed'] }
  ];
  
  let currentIdx = -1;
  if (['Scheduled', 'Delayed', 'CheckIn'].includes(status)) currentIdx = 0;
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
    const isDelay = f.status === "Delayed";
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
          <span class="route-code">${esc(f.dest)}</span>
          <span class="route-name">${esc(getAirportName(f.dest))}</span>
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
      
      ${f.note ? `
        <div class="row warn-text" style="background: rgba(239, 68, 68, 0.08); padding: 8px; border-radius: 6px; border: 1px solid rgba(239, 68, 68, 0.12); margin-top: 10px;">
          <i data-lucide="alert-circle" style="width:14px; height:14px; flex-shrink: 0;"></i> 
          <span>${esc(f.note)}</span>
        </div>` : ""}
      
      <div class="ops">
        <button class="btn btn-sm" onclick="doAdvance('${f.code}')">
          <i data-lucide="chevron-right"></i> Tiến trạng thái
        </button>
        <button class="btn btn-sm btn-warn" onclick="openModal('${f.code}')">
          <i data-lucide="settings"></i> Thao tác chuyến
        </button>
      </div>
    </div>`;
  }).join("");
}

async function doAdvance(code) {
  const r = await api("/api/flight/advance", { code });
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
        <div class="row"><i data-lucide="route" style="width:14px; height:14px;"></i> Đường băng: ${a.runways.map((r) => `<b>${esc(r.code)}</b> (${r.length}m)`).join(", ")}</div>
        <div class="row" style="margin-top:14px; font-weight:600; color:var(--txt); border-top: 1px solid rgba(255,255,255,0.05); padding-top:12px;">
          <i data-lucide="door-open" style="width:14px; height:14px;"></i> Cổng ra máy bay (${a.gates.length})
        </div>
        <div class="gate-grid">${gatesHtml}</div>
      </div>`;
  }).join("");
}

// ---------- Máy bay ----------
function renderAircrafts() {
  $("#aircrafts-table tbody").innerHTML = STATE.aircrafts.map((a) => `
    <tr>
      <td><b>${esc(a.registration)}</b></td>
      <td>${esc(a.model)}</td>
      <td><span class="badge scheduled">${esc(a.category)}</span></td>
      <td><b>${a.capacity}</b> ghế</td>
      <td>${a.requiredRunway}m</td>
      <td>${a.turnaround}'</td>
      <td><span class="badge checkin">${esc(a.preferredGate)}</span></td>
    </tr>`).join("");
}

// ---------- Nhân sự + hành khách ----------
function checkMark(val) {
  return val 
    ? `<span style="color:var(--green); font-weight:bold; font-size:15px;"><i data-lucide="check-circle-2" style="width:16px; height:16px; display:inline-block; vertical-align:middle;"></i></span>` 
    : `<span style="color:var(--muted); font-size:15px;"><i data-lucide="minus" style="width:14px; height:14px; display:inline-block; vertical-align:middle;"></i></span>`;
}

function renderPeople() {
  $("#pilots-table tbody").innerHTML = STATE.pilots.map((p) => `
    <tr>
      <td><b>${esc(p.id)}</b></td>
      <td>${esc(p.name)}</td>
      <td>${p.age}</td>
      <td><span class="badge scheduled">${esc(p.base)}</span></td>
      <td>${esc(p.certs).split(';').map(c => `<span class="badge checkin" style="margin-right:2px; font-size:9px; padding:2px 6px;">${c}</span>`).join('') || "—"}</td>
      <td><b>${p.monthlyHours}h</b></td>
    </tr>`).join("");
    
  $("#ground-table tbody").innerHTML = STATE.ground.map((g) => `
    <tr>
      <td><b>${esc(g.id)}</b></td>
      <td>${esc(g.name)}</td>
      <td><span class="badge scheduled">${esc(g.base)}</span></td>
      <td><span class="badge checkin">${esc(g.department)}</span></td>
    </tr>`).join("");
    
  $("#passengers-table tbody").innerHTML = STATE.passengers.map((p) => `
    <tr>
      <td><b>${esc(p.id)}</b></td>
      <td>${esc(p.name)}</td>
      <td><code>${esc(p.passport)}</code></td>
      <td><b>${esc(p.flight) || "—"}</b></td>
      <td><span class="badge boarding">${esc(p.seat) || "—"}</span></td>
      <td>${checkMark(p.checkedIn)}</td>
      <td>${checkMark(p.boarded)}</td>
      <td class="${p.bagOverweight ? "warn-text" : ""}">
        <i data-lucide="luggage" style="width:14px; height:14px; display:inline-block; vertical-align:middle; margin-right:4px;"></i>
        ${p.bagPieces} kiện / ${p.bagWeight}kg ${p.bagOverweight ? " ⚠" : ""}
      </td>
    </tr>`).join("");
}

// ---------- Weather Form Initialization ----------
function initWeatherForm() {
  const select = $("#w-airport");
  if (!select || !STATE) return;
  const currentVal = select.value;
  select.innerHTML = STATE.airports.map(a => `<option value="${a.code}">${a.code} — ${a.name}</option>`).join('');
  if (currentVal && STATE.airports.some(a => a.code === currentVal)) {
    select.value = currentVal;
  }
  
  const now = new Date();
  const startInput = $("#w-start");
  const endInput = $("#w-end");
  
  if (startInput && !startInput.value) {
    const formatDateTime = (d) => {
      const pad = (n) => String(n).padStart(2, '0');
      return `${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())}T${pad(d.getHours())}:${pad(d.getMinutes())}`;
    };
    
    const tomorrowStart = new Date(now);
    tomorrowStart.setDate(now.getDate() + 1);
    tomorrowStart.setHours(8, 0, 0, 0);
    
    const tomorrowEnd = new Date(now);
    tomorrowEnd.setDate(now.getDate() + 1);
    tomorrowEnd.setHours(12, 0, 0, 0);
    
    startInput.value = formatDateTime(tomorrowStart);
    endInput.value = formatDateTime(tomorrowEnd);
  }
}

// ---------- Modal thao tác ----------
function openModal(code) {
  const f = STATE.flights.find((x) => x.code === code);
  if (!f) return;
  
  $("#modal-title").innerHTML = `<i data-lucide="settings" style="display:inline-block; vertical-align:middle; margin-right:6px;"></i> Chuyến ${f.code} — ${f.status}`;
  
  const pax = f.passengers.map((pid) => {
    const p = STATE.passengers.find((x) => x.id === pid);
    const name = p ? p.name : pid;
    let stText = "Chưa check-in";
    let statusClass = "scheduled";
    if (p && p.boarded) { stText = "Đã lên máy bay"; statusClass = "completed"; }
    else if (p && p.checkedIn) { stText = `Đã check-in (Ghế ${p.seat})`; statusClass = "boarding"; }
    
    return `
      <div class="pax-line">
        <span><b>${esc(name)}</b> <small>(${esc(pid)} · <span class="badge ${statusClass}" style="font-size:9px; padding:1px 5px;">${stText}</span>)</small></span>
        <span class="pax-actions">
          <button class="btn btn-sm" onclick="showSeatPicker('${code}','${pid}')" ${p && p.checkedIn ? 'disabled style="opacity:0.5; cursor:not-allowed;"' : ''}>
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
  // Find seats occupied by passengers of this flight
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
  openModal(code); // Refresh modal view
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

// Classify a log entry text into a category for filtering & styling.
function classifyLog(text) {
  if (/→/.test(text) && /:/.test(text) && !/Tạo chuyến|gán |đặt chỗ/.test(text))
    return "status";        // e.g. "SG100: Scheduled → Check-in"
  if (/Tua thời gian|Tua th/.test(text))
    return "tick";           // time advance
  return "assign";           // creation, assignment, etc.
}

// Icon + color per category
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

  // Classify & filter
  const items = STATE.eventLog
    .map((e) => ({ ...e, cat: classifyLog(e.text) }))
    .filter((e) => logFilter === "all" || e.cat === logFilter);

  if (!items.length) {
    box.innerHTML = "<p class='hint'>Không có sự kiện phù hợp bộ lọc.</p>";
    return;
  }

  // Build timeline HTML
  let html = '<div class="log-timeline">';
  items.forEach((e) => {
    const st = LOG_STYLE[e.cat] || LOG_STYLE.assign;
    // Extract just the time portion (HH:MM) for compact display
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
  const apOpts = STATE.airports.map((a) =>
    `<option value="${a.code}">${a.code} — ${esc(a.name)}</option>`).join("");
  setOpts("#c-origin", apOpts);
  setOpts("#c-dest", apOpts);
  setOpts("#c-aircraft", `<option value="">(chưa gán)</option>` +
    STATE.aircrafts.map((a) =>
      `<option value="${a.registration}">${a.registration} · ${esc(a.category)}</option>`).join(""));
  setOpts("#c-crew", `<option value="">(chưa gán)</option>` +
    STATE.crews.map((c) => `<option value="${c.id}">${c.id}</option>`).join(""));
  setOpts("#c-gate", `<option value="">Tự động</option>`);
  setOpts("#c-pax", STATE.passengers.map((p) =>
    `<option value="${p.id}">${p.id} · ${esc(p.name)}</option>`).join(""));
}

async function doCreateFlight() {
  const code = $("#c-code").value.trim();
  const r = await api("/api/flight/create", {
    code,
    origin: $("#c-origin").value,
    dest: $("#c-dest").value,
    departure: $("#c-dep").value.replace("T", " "),
    arrival: $("#c-arr").value.replace("T", " "),
  });
  if (!r.ok) { toast(r.message, true); return; }  // tạo thất bại -> dừng

  const msgs = [r.message];
  const reg = $("#c-aircraft").value;
  const crewId = $("#c-crew").value;
  // Backend yêu cầu có máy bay trước khi gán tổ bay & gate.
  if (reg) msgs.push((await api("/api/flight/assign-aircraft", { code, reg })).message);
  if (reg && crewId) msgs.push((await api("/api/flight/assign-crew", { code, crewId })).message);
  if (reg) msgs.push((await api("/api/flight/assign-gate", { code, gate: $("#c-gate").value })).message);
  for (const opt of [...$("#c-pax").selectedOptions])
    msgs.push((await api("/api/flight/book", { code, pid: opt.value })).message);

  toast(msgs.join(" | "), false);
  $("#c-code").value = "";
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
  simTimer = setInterval(doTick, 2000);  // 2 giây thật / bước
  setPlayLabel(true);
}

// ---------- Tabs + nút toàn cục ----------
function initTabs() {
  $$(".tab").forEach((t) => t.addEventListener("click", () => {
    $$(".tab").forEach((x) => x.classList.remove("active"));
    $$(".panel").forEach((x) => x.classList.remove("active"));
    t.classList.add("active");
    $("#tab-" + t.dataset.tab).classList.add("active");
  }));
}

window.addEventListener("DOMContentLoaded", () => {
  initTabs();
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

  $("#btn-weather").addEventListener("click", async () => {
    const startVal = $("#w-start").value.replace("T", " ");
    const endVal = $("#w-end").value.replace("T", " ");
    const r = await api("/api/weather", {
      airport: $("#w-airport").value,
      start: startVal,
      end: endVal,
      cancel: $("#w-cancel").checked ? "1" : "0",
    });
    toast(r.message, !r.ok);
    await load();
  });

  // Đồng hồ mô phỏng + tạo chuyến
  $("#btn-step").addEventListener("click", doTick);
  $("#btn-play").addEventListener("click", togglePlay);
  $("#btn-create").addEventListener("click", doCreateFlight);

  // Log filter buttons
  $$(".log-filter").forEach((btn) => btn.addEventListener("click", () => {
    $$(".log-filter").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    logFilter = btn.dataset.filter;
    renderLog();
  }));

  load();
});
