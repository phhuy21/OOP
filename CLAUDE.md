# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

SkyGate is an Airport Management System: a C++17 OOP core driven by **two separate
frontends built from the same `src/` tree** — an interactive console menu app and a
REST-API web server with a vanilla-JS dashboard. University OOP coursework; all
code comments and user-facing strings are in **Vietnamese**, code is in `namespace skygate`.

**Scope (current):** the system models **one home airport (PHG)**. Flights depart PHG to a
**destination city** — `Flight::destCode()` holds a city name (e.g. `"Da Nang"`), not an
`Airport` object, so only the origin must be a real airport. The product focus is **passenger
monitoring** (booking → check-in → boarding journey, search, per-flight/gate counts, alerts).
The crew/pilot/ground-staff classes still exist for OOP coverage but are **not seeded, hidden
from the UI, and optional** — `Flight::advance()` only needs aircraft + gate to reach `Ready`.
The web frontend has **no map** (the old Leaflet dashboard and the `weather`/`people` tabs were
removed); `web/*_map.*`, `web/*_old.*` remain as dead backups.

## Build & Run

Requires the **MSYS2 UCRT64 toolchain** (`g++` 15.x at `C:\msys64\ucrt64`). Before any
build or run, put it first on PATH — this is the single most common failure mode:

```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
```

If you skip it, Git-for-Windows' mismatched `libstdc++`/`libgcc` DLLs get loaded and the
exe dies with exit `3221225785` (`0xC0000139`, entrypoint-not-found) or bash exit `127`,
usually with no output.

Two entry points, two build scripts (both `-std=c++17 -Wall -Wextra -O2`):

| Target | Build | Entry | Run |
|---|---|---|---|
| Console app | `bash build.sh` → `skygate.exe` | `src/main.cpp` | `./skygate.exe` |
| Web server | `bash build_web.sh` → `skygate_web.exe` | `src/web/web_main.cpp` | `./skygate_web.exe [port]` (default 8080) |

`build.sh` compiles **every** `src/**/*.cpp`. `build_web.sh` compiles every `src/**/*.cpp`
**except** `main.cpp` and `web_main.cpp`, then adds `web_main.cpp` back explicitly — both
files define `main()`, so including both would double-link. If you add a new entry point,
follow the same exclude pattern. The web build also needs `-Ithird_party` and (on Windows)
`-static -static-libgcc -static-libstdc++ -lws2_32`.

If a build fails with `cannot open output file ... Permission denied`, a previous run still
holds the exe: `taskkill //F //IM skygate.exe` (or `skygate_web.exe`), then rebuild.

The web server serves `./web` statically and exposes `/api/*`; open `http://localhost:<port>`.
`MoSkyGate.bat` launches the prebuilt `skygate_web.exe` on port 3000.

## Test

There is no unit-test framework. End-to-end checks drive the **console app** via scripted
stdin. The canonical harness is the `run-skygate` skill (`.claude/skills/run-skygate/`) —
**read its `SKILL.md` before running or testing**; it documents the PATH/encoding gotchas
and the menu-driving rules. Quick reference:

```bash
python .claude/skills/run-skygate/driver.py smoke   # seed + list + exit (read-only)
python .claude/skills/run-skygate/driver.py list     # list all entities (read-only)
python .claude/skills/run-skygate/driver.py create-save  # MUTATES data/
```

Driving gotcha: every console action returns to a submenu that calls `pause()`, so each
add/list/save input line must be followed by a bare empty line to answer
`Nhấn Enter để tiếp tục...`. A missing pause line desyncs the whole sequence (looks like
"exits clean but did nothing"). The root `test_run.py` is legacy and unreliable — prefer the driver.

## Architecture

**`AirportSystem` (`src/system/`) is the single source of truth and the only place business
logic lives.** It owns every entity list (`vector<shared_ptr<T>>` of airports, aircraft,
pilots, ground staff, passengers, crews, flights), enforces all rules, runs the simulation
clock, dispatch queues, weather simulation, and file persistence. It is a deliberate ~900-line
god-class controller.

**Both frontends are thin wrappers that only call `AirportSystem`'s public methods** — they
contain no business logic:
- `src/main.cpp` — console menu; each choice calls a method and prints the result.
- `src/web/web_main.cpp` — `httplib` routes; each route calls the same method and serializes
  the return value to JSON. It holds one in-memory `AirportSystem` for the process lifetime.

So **to add a feature: implement the rule in `AirportSystem` (+ the relevant domain class),
then expose it in the console menu, the web route, and the frontend.**

### Key cross-cutting conventions

- **`OpResult` everywhere** (`src/common/OpResult.h`): mutating operations return
  `OpResult{ bool ok, string message }`, never throw for business-rule failures. The console
  prints `[OK]`/`[LỖI]`; the web layer maps it to `{ ok, message }` JSON via `jResult()`.
- **Enum⇄string is centralized in `src/common/Enums.cpp`** (`toString` / `*FromString`) and is
  used for *both* display and file persistence. These strings also cross the API boundary as
  **exact-match** values — e.g. `FlightStatus` serializes to hyphenated `"Check-In"`, and the
  frontend matches that literal. Changing a status string means changing the C++ and the JS
  together (see git history).
- Entities are passed and stored as `shared_ptr`. Header guards are `SKYGATE_*_H`.

### Accounts, roles & ticket booking

A login/role layer sits on top of the system (added after the original coursework core):

- **`User` hierarchy** lives in `src/auth/` under **`namespace skygate::auth`** (deliberately
  separate from the unrelated `skygate::Staff` crew class): abstract `User` → {`Admin`, `Staff`,
  `Customer`}. Each role overrides `role()` and `menu()` (the list of web tab keys it may see) —
  `menu()` is the polymorphic stand-in for the spec's `virtual showMenu()`. Build via
  `auth::makeUser(role, …)`.
- **`Ticket`** (`src/operations/Ticket.h`) is the booking unit. `AirportSystem::bookTicket()`
  creates a `Passenger` **and** a `Ticket` and adds the passenger to the flight, so available
  seats = `aircraft->capacity() - flight->passengers().size()` decrements naturally;
  `cancelTicket()` removes both and frees the seat. Ticket ids are `TKnnnn` from a persisted
  `ticketCounter_`; auto-generated passengers are `PX1xxxx`.
- **Auth is enforced server-side**, not in the UI: every mutating web route reads an `actor`
  username param and checks role via the `hasRole(...)` lambda in `web_main.cpp`. The frontend
  auto-attaches `actor` (from `localStorage`) to every POST and hides tabs/buttons per `menu()`,
  but that's convenience — the C++ check is the real gate.
- **Demo accounts** (seeded by `seedDefaultAccounts()`, idempotent): `admin/admin123`,
  `staff/staff123`, `customer/cus123`. Passwords are stored plaintext in `users.txt` — fine for
  the coursework, not for production.

### Domain model (OOP)

- **People:** `Person` → `Staff` → {`Pilot`, `GroundStaff`}; `Passenger` derives `Person` directly.
- **Aircraft:** abstract `Aircraft` → {`WideBodyAircraft`, `NarrowBodyAircraft`, `TurbopropAircraft`},
  with pure-virtual `category()` / `requiredRunwayLength()` / `minTurnaroundMinutes()` /
  `preferredGateType()` / `minGateRank()`. **Always construct via `AircraftFactory::create(category, …)`**,
  not the concrete subclasses.
- **`Flight`** is the central composition: it references an `Aircraft`, an optional `Crew`, a gate,
  and its passenger list, and owns the lifecycle **state machine**. `originCode()` is the home
  airport; `destCode()` is a destination **city name** (not an airport).

### Flight lifecycle & simulation clock

`FlightStatus` (`Enums.h`): `Scheduled → CheckIn → Boarding → GateClosed → Ready → Takeoff →
InAir → Landed → Turnaround → Completed`, plus `Delayed` / `Cancelled`. `Flight::advance()`
performs guarded transitions: reaching `Ready` requires aircraft + gate (**crew is optional** in
the current single-airport scope; if a crew *is* assigned it's still validated). `AirportSystem::
tickSimulation(minutes)` advances a *virtual* clock and auto-promotes flight statuses based on
virtual-now vs. departure/arrival — used by the monitoring alerts.

Representative business rules still in `AirportSystem` (the crew-related ones only fire when a
crew is assigned, which the UI no longer does): a pilot needs a certification matching the
aircraft category and ≤ 100 monthly flight hours; aircraft↔gate compatibility uses a gate rank
(`RemoteStand`=0, `SingleJetBridge`=1, `DoubleJetBridge`=2) the aircraft must meet; the **origin
(home) airport** runway must satisfy the aircraft; baggage warns over 23 kg/piece.

### Persistence — and the console/web split

State is saved as **pipe (`|`)-delimited plain text**, one entity type per file in `./data/`
(`airports.txt`, `aircraft.txt`, `pilots.txt`, `groundstaff.txt`, `crews.txt`, `flights.txt`,
`passengers.txt`, `gates.txt`, `runways.txt`, `queues.txt`, `users.txt`, `tickets.txt`) via
`saveAll(dir)` / `loadAll(dir)`. `tickets.txt` starts with a `COUNTER|n` line that restores the
ticket-id sequence; load order matters — users/tickets are read after flights so references resolve.

- **Console** auto-loads `./data` on startup (if `data/airports.txt` exists) and saves on demand.
- **Web server now persists too**: on startup it `loadAll("data")` (falling back to
  `seedDemoData()` if empty), and a `set_post_routing_handler` calls `saveAll("data")` after every
  successful POST (except `/api/login`). So web state survives restarts. `seedDefaultAccounts()`
  runs on load to backfill the demo accounts into pre-existing data dirs that lack `users.txt`.

### Web layer specifics

- **No external JSON library**: hand-rolled `Obj` / `Arr` string builders in `src/web/Json.h`;
  per-entity `j*()` serializers in `web_main.cpp`; `GET /api/state` returns the whole snapshot.
- HTTP server is the single-header `third_party/httplib.h`.
- API: `GET /api/state`; `POST /api/flight/{create,assign-aircraft,assign-crew,assign-gate,book,
  checkin,board,advance,delay,cancel,delete}`; `POST /api/sim/tick`; `POST /api/weather`;
  `POST /api/reset`. Auth/role routes: `POST /api/login`; `GET /api/users` + `POST /api/user/{create,
  delete}` (Admin); `POST /api/ticket/{book,cancel}`; `POST /api/{aircraft,runway}/{create,delete}`
  (Admin). Mutating params come as `application/x-www-form-urlencoded`, including the `actor` username
  used for the server-side role check.

### Frontend (`web/`)

Vanilla JS, **no framework and no build step** — edit and reload. The live files are
`index.html` / `app.js` / `style.css` — a tabbed dashboard (Chuyến bay, Giám sát, Hành khách,
Sân bay, Máy bay, Tạo chuyến, Đặt vé, Tài khoản, Nhật ký). `app.js` calls `api(path, params)`
(GET when no params, POST form-encoded otherwise, auto-attaching the logged-in `actor`), reads
everything from `GET /api/state`, and mutates through the POST endpoints. `applyAuth()` gates the
login overlay and shows/hides tabs per the user's `menu()`. `renderMonitor()` computes alerts
(un-checked-in passengers near departure, full/near-full flights, overweight bags) purely
client-side from `/api/state`. The `*_old.*` and `*_map.*` files in `web/` are dead backups
(the map version) — **edit the live trio, not those.**
