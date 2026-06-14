// SkyGate Map Web — bọc giao diện giám sát bản đồ bay Việt Nam.
const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => document.querySelectorAll(sel);
let STATE = null;

// Leaflet map instances
let map = null;
let airportMarkers = {};
let flightPaths = [];
let flightMarkers = {};
let weatherOverlays = [];

// Tọa độ địa lý của các sân bay tại Việt Nam
const AIRPORT_COORDS = {
  PHG: [21.2212, 105.8072],   // Sân bay Quốc tế Phương Hoàng (Hà Nội)
  CLG: [10.8184, 106.6588],   // Sân bay Quốc tế Cửu Long (TP.HCM)
  HAU: [10.2244, 103.9608],   // Sân bay Đảo Hải Âu (Phú Quốc)
  MHA: [22.3364, 103.8438]    // Sân bay Thung Lũng Mường Hoa (Sa Pa)
};

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

// Helper to parse dates in simulated time format (robust cross-browser parsing)
function parseSimDate(dateStr) {
  if (!dateStr) return null;
  
  const cleanedStr = dateStr.trim();
  
  // Try YYYY/MM/DD HH:MM:SS format first (highly cross-browser compatible)
  let d = new Date(cleanedStr.replace(/-/g, '/'));
  if (d && !isNaN(d.getTime())) return d;
  
  // Try ISO format with T
  d = new Date(cleanedStr.replace(' ', 'T'));
  if (d && !isNaN(d.getTime())) return d;
  
  // Direct new Date
  d = new Date(dateStr);
  if (d && !isNaN(d.getTime())) return d;
  
  // Manual string split fallback
  const parts = cleanedStr.split(' ');
  if (parts.length >= 2) {
    const dateParts = parts[0].split('-');
    const timeParts = parts[1].split(':');
    if (dateParts.length === 3 && timeParts.length >= 2) {
      const year = parseInt(dateParts[0], 10);
      const month = parseInt(dateParts[1], 10) - 1;
      const day = parseInt(dateParts[2], 10);
      const hour = parseInt(timeParts[0], 10);
      const minute = parseInt(timeParts[1], 10);
      const second = timeParts[2] ? parseInt(timeParts[2], 10) : 0;
      const parsedDate = new Date(year, month, day, hour, minute, second);
      if (parsedDate && !isNaN(parsedDate.getTime())) return parsedDate;
    }
  }
  return null;
}

// Calculate flight progress fraction between 0.0 and 1.0 (safeguarded against NaN)
function getFlightProgress(flight, currentTimeStr) {
  const dep = parseSimDate(flight.departure);
  const arr = parseSimDate(flight.arrival);
  const now = parseSimDate(currentTimeStr);
  
  if (!dep || !arr || !now) return 0;
  if (isNaN(dep.getTime()) || isNaN(arr.getTime()) || isNaN(now.getTime())) return 0;
  
  if (now <= dep) return 0;
  if (now >= arr) return 1;
  
  const total = arr.getTime() - dep.getTime();
  const elapsed = now.getTime() - dep.getTime();
  
  if (isNaN(total) || isNaN(elapsed) || total <= 0) return 0;
  
  return Math.min(1, Math.max(0, elapsed / total));
}

// Generate quadratic Bezier points for curved lines
function getBezierPoints(p1, p2, offset = 0.15) {
  const midX = (p1[0] + p2[0]) / 2;
  const midY = (p1[1] + p2[1]) / 2;
  const dx = p2[0] - p1[0];
  const dy = p2[1] - p1[1];
  
  // Perpendicular control point
  const controlX = midX - dy * offset;
  const controlY = midY + dx * offset;
  
  const points = [];
  const steps = 30;
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    const x = (1-t)*(1-t)*p1[0] + 2*(1-t)*t*controlX + t*t*p2[0];
    const y = (1-t)*(1-t)*p1[1] + 2*(1-t)*t*controlY + t*t*p2[1];
    points.push([x, y]);
  }
  return points;
}

function initMap() {
  // Center map on Central Vietnam
  map = L.map('map', {
    zoomControl: true,
    maxZoom: 12,
    minZoom: 4
  }).setView([16.4, 106.5], 5.5);

  // CartoDB Dark Matter map layer (very sleek dark theme)
  L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors &copy; <a href="https://carto.com/attributions">CartoDB</a>',
    subdomains: 'abcd',
    maxZoom: 20
  }).addTo(map);
}

function updateMap() {
  if (!map || !STATE) return;

  // Clear previous flights lines
  flightPaths.forEach(p => map.removeLayer(p));
  flightPaths = [];

  // Clear previous weather overlays
  weatherOverlays.forEach(o => map.removeLayer(o));
  weatherOverlays = [];

  // Track active flights in the air
  const activeFlightCodes = new Set();

  // Update or render Airports
  STATE.airports.forEach(ap => {
    const coords = AIRPORT_COORDS[ap.code];
    if (!coords) return;

    let weatherBad = false;
    let emergency = false;

    // Check bad weather state via notes
    if (ap.note && ap.note !== "Thời tiết tốt" && ap.note !== "Bình thường" && ap.note !== "") {
      weatherBad = true;
    }

    // Check emergency flights targeting this airport
    const activeEmergencies = STATE.flights.filter(f => 
      (f.origin === ap.code || f.dest === ap.code) && 
      f.emergency && 
      f.status !== "Completed" && 
      f.status !== "Landed" && 
      f.status !== "Cancelled"
    );
    if (activeEmergencies.length > 0) {
      emergency = true;
    }

    let marker = airportMarkers[ap.code];
    if (!marker) {
      const icon = L.divIcon({
        className: 'airport-marker-container',
        html: `<div class="airport-marker ${emergency ? 'emergency' : (weatherBad ? 'weather-bad' : '')}" id="marker-${ap.code}">
                <div class="airport-pulse"></div>
                <div class="airport-dot"></div>
                <div class="airport-label">${ap.code}</div>
               </div>`,
        iconSize: [14, 14],
        iconAnchor: [7, 7]
      });

      marker = L.marker(coords, { icon }).addTo(map);
      airportMarkers[ap.code] = marker;
    } else {
      const el = document.getElementById(`marker-${ap.code}`);
      if (el) {
        el.className = `airport-marker ${emergency ? 'emergency' : (weatherBad ? 'weather-bad' : '')}`;
      }
    }

    // Render Windy-style weather radar sweep overlay if weather is bad
    if (weatherBad) {
      const isSevere = ap.note.toLowerCase().includes('bão') || 
                       ap.note.toLowerCase().includes('đóng cửa') || 
                       ap.note.toLowerCase().includes('huỷ') || 
                       ap.note.toLowerCase().includes('khẩn');
      
      const weatherOverlayHtml = `
        <div class="radar-sweep-container" style="pointer-events:none;">
          <div class="weather-storm-cell ${isSevere ? 'severe' : ''}"></div>
          <div class="radar-sweep"></div>
          <div class="weather-icon-badge ${isSevere ? 'severe' : ''}">
            <i data-lucide="${isSevere ? 'cloud-lightning' : 'cloud-rain'}"></i>
          </div>
        </div>
      `;
      
      const weatherIcon = L.divIcon({
        className: 'weather-radar-overlay',
        html: weatherOverlayHtml,
        iconSize: [0, 0],
        iconAnchor: [0, 0]
      });
      
      const wOverlay = L.marker(coords, { icon: weatherIcon }).addTo(map);
      weatherOverlays.push(wOverlay);
    }

    // Bind popup with airport details
    const popupHtml = `
      <div style="font-size:12px; line-height:1.4; min-width: 180px;">
        <h3 style="margin:0 0 6px 0; color:var(--accent); font-size:13px; font-weight:700; display:flex; align-items:center; gap:6px;">
          <i data-lucide="building-2" style="width:14px; height:14px;"></i> ${ap.code} — ${ap.name}
        </h3>
        <p style="margin:2px 0;"><b>Tình trạng:</b> <span style="color:${emergency ? 'var(--red)' : (weatherBad ? 'var(--amber)' : 'var(--green)')}; font-weight:600;">${esc(ap.note || "Bình thường")}</span></p>
        <p style="margin:2px 0;"><b>Đường băng dài nhất:</b> ${ap.longestRunway}m</p>
        <p style="margin:2px 0;"><b>Đường băng:</b> ${ap.runways.map(r => r.code).join(', ')}</p>
        <p style="margin:2px 0;"><b>Cổng đỗ:</b> ${ap.gates.length} cổng</p>
      </div>
    `;
    marker.bindPopup(popupHtml);

    // On click, switch to Airport tab and scroll to card
    marker.off('click');
    marker.on('click', () => {
      const tabBtn = document.querySelector('.tab[data-tab="airports"]');
      if (tabBtn) {
        tabBtn.click();
        setTimeout(() => {
          const cards = document.querySelectorAll('#airports-list .card');
          for (const card of cards) {
            if (card.innerHTML.includes(ap.code)) {
              card.scrollIntoView({ behavior: 'smooth', block: 'center' });
              card.style.borderColor = 'var(--accent)';
              setTimeout(() => card.style.borderColor = '', 2000);
              break;
            }
          }
        }, 150);
      }
    });
  });

  // Render Flights and Moving Plane Markers
  STATE.flights.forEach(f => {
    if (f.status === "Cancelled" || f.status === "Landed" || f.status === "Completed") return;

    const startCoords = AIRPORT_COORDS[f.origin];
    const endCoords = AIRPORT_COORDS[f.dest];
    if (!startCoords || !endCoords) return;

    // Create curved flight path
    const codeHash = [...f.code].reduce((a, c) => a + c.charCodeAt(0), 0);
    const curvature = 0.12 + (codeHash % 8) * 0.02; // Curved differently to avoid overlays
    const points = getBezierPoints(startCoords, endCoords, curvature);

    const isFlying = f.status === "Takeoff" || f.status === "InAir";
    const pathColor = f.emergency ? 'var(--red)' : 'var(--accent)';

    // Polyline glow and main line
    const flightPath = L.polyline(points, {
      color: pathColor,
      weight: isFlying ? 2.5 : 1.5,
      opacity: isFlying ? 0.8 : 0.35,
      dashArray: isFlying ? '' : '6, 6'
    }).addTo(map);
    flightPaths.push(flightPath);

    // Add popup info on line
    const popupHtml = `
      <div style="font-size:12px; line-height:1.4;">
        <h3 style="margin:0 0 6px 0; color:${f.emergency ? 'var(--red)' : 'var(--accent)'}; font-size:13px; font-weight:700;">Chuyến ${f.code}</h3>
        <p style="margin:2px 0;"><b>Tuyến:</b> ${f.origin} &rarr; ${f.dest}</p>
        <p style="margin:2px 0;"><b>Trạng thái:</b> <span class="${statusClass(f.status)}">${f.status}</span></p>
        <p style="margin:2px 0;"><b>Khởi hành:</b> ${f.departure || '—'}</p>
        <p style="margin:2px 0;"><b>Dự kiến đến:</b> ${f.arrival || '—'}</p>
        ${f.note ? `<p style="margin:2px 0; color:var(--red);"><b>Chú ý:</b> ${esc(f.note)}</p>` : ''}
      </div>
    `;
    flightPath.bindPopup(popupHtml);

    flightPath.on('mouseover', () => {
      flightPath.setStyle({ weight: 4, opacity: 1 });
    });
    flightPath.on('mouseout', () => {
      flightPath.setStyle({ weight: isFlying ? 2.5 : 1.5, opacity: isFlying ? 0.8 : 0.35 });
    });

    // If flight is currently in the air, compute and animate its position
    if (isFlying) {
      activeFlightCodes.add(f.code);
      const targetProgress = getFlightProgress(f, STATE.currentTime);
      const planeColor = f.emergency ? 'var(--red)' : 'var(--accent)';
      const planeStroke = '#ffffff';

      let planeMarker = flightMarkers[f.code];
      
      if (!planeMarker) {
        // Create marker initially at progress 0 or targetProgress (glide out of origin)
        const startProgress = 0;
        const initialPos = points[0];

        const planeIconHtml = `
          <div class="plane-icon-wrapper" style="transform: rotate(0deg);">
            <svg class="plane-icon-svg" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="22" height="22" fill="${planeColor}" stroke="${planeStroke}" stroke-width="1.2">
              <path d="M21 16v-2l-8-5V3.5c0-.83-.67-1.5-1.5-1.5S10 2.67 10 3.5V9l-8 5v2l8-2.5V19l-2 1.5V22l3.5-1 3.5 1v-1.5L14 19v-5.5l8 2.5z"/>
            </svg>
            <div class="plane-label">${f.code}</div>
          </div>
        `;

        const planeIcon = L.divIcon({
          className: 'plane-marker-icon',
          html: planeIconHtml,
          iconSize: [24, 24],
          iconAnchor: [12, 12]
        });

        planeMarker = L.marker(initialPos, { icon: planeIcon }).addTo(map);
        planeMarker.displayedProgress = startProgress;
        flightMarkers[f.code] = planeMarker;

        // On click, switch to Flight tab and scroll to flight card
        planeMarker.on('click', () => {
          const tabBtn = document.querySelector('.tab[data-tab="flights"]');
          if (tabBtn) {
            tabBtn.click();
            setTimeout(() => {
              const cards = document.querySelectorAll('#flights-list .card');
              for (const card of cards) {
                if (card.innerHTML.includes(f.code)) {
                  card.scrollIntoView({ behavior: 'smooth', block: 'center' });
                  card.style.borderColor = 'var(--accent)';
                  setTimeout(() => card.style.borderColor = '', 2000);
                  break;
                }
              }
            }, 150);
          }
        });
      }

      planeMarker.bindPopup(popupHtml);

      // Cancel previous animation frame if still running
      if (planeMarker.animationFrameId) {
        cancelAnimationFrame(planeMarker.animationFrameId);
      }

      // Animate marker from current displayedProgress to targetProgress (safeguarded against NaN)
      let startProgress = planeMarker.displayedProgress || 0;
      if (isNaN(startProgress)) startProgress = 0;
      let finalTargetProgress = targetProgress || 0;
      if (isNaN(finalTargetProgress)) finalTargetProgress = 0;
      
      // If target progress is behind current displayed progress (simulation reset or rewind)
      if (finalTargetProgress < startProgress) {
        startProgress = finalTargetProgress;
        planeMarker.displayedProgress = finalTargetProgress;
      }

      const progressDiff = finalTargetProgress - startProgress;
      const animDuration = 1500; // Animate over 1.5 seconds
      const startTime = performance.now();

      function animatePlane(nowTime) {
        const elapsed = nowTime - startTime;
        const t = Math.min(1, elapsed / animDuration);
        
        // Glide along path linearly
        let currentProgress = startProgress + progressDiff * t;
        if (isNaN(currentProgress)) currentProgress = finalTargetProgress;
        planeMarker.displayedProgress = currentProgress;

        const idx = Math.floor(currentProgress * (points.length - 1));
        const currentPos = points[idx];

        // Rotation angle
        const nextIdx = Math.min(idx + 1, points.length - 1);
        const prevIdx = Math.max(idx - 1, 0);
        const p1 = points[prevIdx];
        const p2 = points[nextIdx];

        const pixel1 = map.project(p1, map.getZoom());
        const pixel2 = map.project(p2, map.getZoom());
        const dx = pixel2.x - pixel1.x;
        const dy = pixel2.y - pixel1.y;
        let angle = Math.atan2(dy, dx) * 180 / Math.PI + 90;

        // Set marker position
        planeMarker.setLatLng(currentPos);

        // Update rotation transform
        const iconEl = planeMarker.getElement();
        if (iconEl) {
          const wrapper = iconEl.querySelector('.plane-icon-wrapper');
          if (wrapper) {
            wrapper.style.transform = `rotate(${angle}deg)`;
          }
        }

        if (t < 1) {
          planeMarker.animationFrameId = requestAnimationFrame(animatePlane);
        } else {
          planeMarker.animationFrameId = null;
        }
      }

      if (progressDiff > 0.0001) {
        planeMarker.animationFrameId = requestAnimationFrame(animatePlane);
      } else {
        // Direct update if no difference
        let safeTargetProgress = targetProgress || 0;
        if (isNaN(safeTargetProgress)) safeTargetProgress = 0;
        const idx = Math.floor(safeTargetProgress * (points.length - 1));
        const currentPos = points[idx];
        if (currentPos) planeMarker.setLatLng(currentPos);

        const nextIdx = Math.min(idx + 1, points.length - 1);
        const prevIdx = Math.max(idx - 1, 0);
        const p1 = points[prevIdx];
        const p2 = points[nextIdx];

        const pixel1 = map.project(p1, map.getZoom());
        const pixel2 = map.project(p2, map.getZoom());
        let angle = Math.atan2(pixel2.y - pixel1.y, pixel2.x - pixel1.x) * 180 / Math.PI + 90;

        const iconEl = planeMarker.getElement();
        if (iconEl) {
          const wrapper = iconEl.querySelector('.plane-icon-wrapper');
          if (wrapper) {
            wrapper.style.transform = `rotate(${angle}deg)`;
          }
        }
      }
    }
  });

  // Cleanup of inactive flights
  for (const code in flightMarkers) {
    if (!activeFlightCodes.has(code)) {
      const marker = flightMarkers[code];
      if (marker.animationFrameId) {
        cancelAnimationFrame(marker.animationFrameId);
      }
      map.removeLayer(marker);
      delete flightMarkers[code];
    }
  }

  lucide.createIcons();
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

  // Simulated Clock & Event Logs
  if (STATE.currentTime) $("#sim-now").textContent = STATE.currentTime;
  renderLog();
  initCreateForm();

  // Render Map elements (Airports, flight routes, active planes)
  updateMap();

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
  if (!r.ok) { toast(r.message, true); return; }

  const msgs = [r.message];
  const reg = $("#c-aircraft").value;
  const crewId = $("#c-crew").value;
  
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
  simTimer = setInterval(doTick, 2000); // Step every 2 seconds real-time
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
  initMap();
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

  $("#btn-step").addEventListener("click", doTick);
  $("#btn-play").addEventListener("click", togglePlay);
  $("#btn-create").addEventListener("click", doCreateFlight);

  $$(".log-filter").forEach((btn) => btn.addEventListener("click", () => {
    $$(".log-filter").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    logFilter = btn.dataset.filter;
    renderLog();
  }));

  load();
});
