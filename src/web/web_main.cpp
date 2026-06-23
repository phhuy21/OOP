// SkyGate Web — bọc AirportSystem (C++ OOP) thành REST API + phục vụ web tĩnh.
// KHÔNG sửa logic nghiệp vụ: chỉ gọi lại các hàm public sẵn có của AirportSystem.
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <string>

#include "../../third_party/httplib.h"
#include "Json.h"

#include <filesystem>
#include "../system/AirportSystem.h"
#include "../common/Enums.h"

namespace sg = skygate;
using skygate::json::Obj;
using skygate::json::Arr;

namespace {

std::string dt(const sg::DateTime& d) { return d.isValid() ? d.toString() : ""; }

std::string jAirport(const std::shared_ptr<sg::Airport>& a) {
    Arr runways, gates;
    for (const auto& r : a->runways())
        runways.push(Obj().str("code", r.code()).num("length", (long long)r.lengthMeters()).dump());
    for (const auto& g : a->gates())
        gates.push(Obj().str("code", g.code()).str("type", sg::toString(g.type())).dump());
    return Obj()
        .str("code", a->code()).str("name", a->name()).str("note", a->note())
        .num("longestRunway", (long long)a->longestRunway())
        .raw("runways", runways.dump()).raw("gates", gates.dump())
        .dump();
}

std::string jAircraft(const std::shared_ptr<sg::Aircraft>& ac) {
    return Obj()
        .str("registration", ac->registration()).str("model", ac->model())
        .str("category", sg::toString(ac->category())).num("capacity", (long long)ac->capacity())
        .num("requiredRunway", (long long)ac->requiredRunwayLength())
        .num("turnaround", (long long)ac->minTurnaroundMinutes())
        .str("preferredGate", sg::toString(ac->preferredGateType()))
        .dump();
}

std::string jPilot(const std::shared_ptr<sg::Pilot>& p) {
    return Obj()
        .str("id", p->id()).str("name", p->fullName()).num("age", (long long)p->age())
        .str("phone", p->phone()).str("base", p->baseAirport())
        .str("certs", p->certificationList()).num("monthlyHours", p->monthlyFlightHours())
        .dump();
}

std::string jGround(const std::shared_ptr<sg::GroundStaff>& g) {
    return Obj()
        .str("id", g->id()).str("name", g->fullName()).num("age", (long long)g->age())
        .str("phone", g->phone()).str("base", g->baseAirport()).str("department", g->department())
        .dump();
}

std::string jPassenger(const std::shared_ptr<sg::Passenger>& p) {
    return Obj()
        .str("id", p->id()).str("name", p->fullName()).num("age", (long long)p->age())
        .str("phone", p->phone())
        .str("passport", p->passport()).str("flight", p->flightCode()).str("seat", p->seat())
        .boolean("checkedIn", p->checkedIn()).boolean("boarded", p->boarded())
        .num("bagPieces", (long long)p->baggage().pieces())
        .num("bagWeight", p->baggage().totalWeightKg())
        .boolean("bagOverweight", p->baggage().isOverweight())
        .dump();
}

std::string jCrew(const std::shared_ptr<sg::Crew>& c) {
    Arr pilots, ground;
    for (const auto& p : c->pilots()) pilots.pushStr(p->id());
    for (const auto& g : c->groundStaff()) ground.pushStr(g->id());
    return Obj().str("id", c->id())
        .raw("pilots", pilots.dump()).raw("ground", ground.dump()).dump();
}

std::string jFlight(const std::shared_ptr<sg::Flight>& f) {
    Arr pax;
    for (const auto& p : f->passengers()) pax.pushStr(p->id());
    return Obj()
        .str("code", f->code()).str("origin", f->originCode()).str("dest", f->destCode())
        .str("departure", dt(f->departure())).str("arrival", dt(f->arrival()))
        .str("status", f->statusName()).boolean("emergency", f->emergency())
        .str("note", f->note())
        .str("aircraft", f->aircraft() ? f->aircraft()->registration() : "")
        .str("crew", f->crew() ? f->crew()->id() : "")
        .str("gate", f->gateCode())
        .num("checkedIn", (long long)f->checkedInCount())
        .num("boarded", (long long)f->boardedCount())
        .num("paxCount", (long long)f->passengers().size())
        .num("capacity", (long long)(f->aircraft() ? f->aircraft()->capacity() : 0))
        .num("availableSeats",
             (long long)(f->aircraft()
                             ? f->aircraft()->capacity() - (int)f->passengers().size()
                             : -1))
        .raw("passengers", pax.dump())
        .dump();
}

std::string jEvent(const sg::LogEntry& e) {
    return Obj().str("time", e.time).str("text", e.text).dump();
}

std::string jUser(const std::shared_ptr<sg::auth::User>& u) {
    Arr menu;
    for (const auto& m : u->menu()) menu.pushStr(m);
    return Obj()
        .str("username", u->username()).str("fullName", u->fullName())
        .str("role", u->role()).raw("menu", menu.dump())
        .dump();
}

std::string jTicket(const std::shared_ptr<sg::Ticket>& t) {
    return Obj()
        .str("ticketId", t->ticketId()).str("flight", t->flightCode())
        .str("pid", t->passengerId()).str("passengerName", t->passengerName())
        .str("owner", t->ownerUsername()).str("seatClass", t->seatClass())
        .dump();
}

std::string fullState(const sg::AirportSystem& s) {
    Arr airports, aircrafts, pilots, ground, passengers, crews, flights, depQ, arrQ, tickets;
    for (const auto& a : s.airports())   airports.push(jAirport(a));
    for (const auto& a : s.aircrafts())  aircrafts.push(jAircraft(a));
    for (const auto& p : s.pilots())     pilots.push(jPilot(p));
    for (const auto& g : s.groundStaff())ground.push(jGround(g));
    for (const auto& p : s.passengers()) passengers.push(jPassenger(p));
    for (const auto& c : s.crews())      crews.push(jCrew(c));
    for (const auto& f : s.flights())    flights.push(jFlight(f));
    for (const auto& t : s.tickets())    tickets.push(jTicket(t));
    for (const auto& c : s.departureQueue()) depQ.pushStr(c);
    for (const auto& c : s.arrivalQueue())   arrQ.pushStr(c);
    // Nhật ký: render mới nhất trước -> đảo ngược ở đây.
    Arr events;
    const auto& log = s.eventLog();
    for (auto it = log.rbegin(); it != log.rend(); ++it) events.push(jEvent(*it));
    return Obj()
        .str("summary", s.summary())
        .str("currentTime", dt(s.currentTime()))
        .raw("airports", airports.dump()).raw("aircrafts", aircrafts.dump())
        .raw("pilots", pilots.dump()).raw("ground", ground.dump())
        .raw("passengers", passengers.dump()).raw("crews", crews.dump())
        .raw("flights", flights.dump())
        .raw("tickets", tickets.dump())
        .raw("departureQueue", depQ.dump()).raw("arrivalQueue", arrQ.dump())
        .raw("eventLog", events.dump())
        .dump();
}

std::string jResult(const sg::OpResult& r) {
    return Obj().boolean("ok", r.ok).str("message", r.message).dump();
}

// Lấy 1 query param dạng chuỗi (rỗng nếu thiếu).
std::string q(const httplib::Request& req, const char* key) {
    auto it = req.params.find(key);
    return it != req.params.end() ? it->second : std::string();
}

}  // namespace

int main(int argc, char** argv) {
    int port = (argc > 1) ? std::atoi(argv[1]) : 8080;

    const std::string kDataDir = "data";

    sg::AirportSystem system;
    // Kiểm tra xem đã có cơ sở dữ liệu SQLite hoặc file .txt cũ chưa
    bool hasDb = std::filesystem::exists(kDataDir) && std::filesystem::exists(kDataDir + "/skygate.db");
    bool hasTxt = std::filesystem::exists(kDataDir) && std::filesystem::exists(kDataDir + "/airports.txt");
    if (hasDb || hasTxt) {
        if (system.loadAll(kDataDir).ok) {
            system.seedDefaultAccounts();
            system.saveAll(kDataDir);
            std::printf("Da nap du lieu tu thu muc '%s'.\n", kDataDir.c_str());
        } else {
            std::printf("Loi: Khong the nap du lieu tu '%s'.\n", kDataDir.c_str());
        }
    } else {
        system.seedDemoData();
        system.saveAll(kDataDir);
        std::printf("Da khoi tao du lieu mau va luu vao '%s'.\n", kDataDir.c_str());
    }

    httplib::Server svr;

    auto sendJson = [](httplib::Response& res, const std::string& body) {
        res.set_content(body, "application/json; charset=utf-8");
    };

    // Vai trò của tài khoản 'actor' (rỗng nếu không có / không hợp lệ).
    auto actorRole = [&](const std::string& username) -> std::string {
        auto u = system.findUser(username);
        return u ? u->role() : std::string();
    };
    // Trả về true nếu actor có một trong các vai trò cho phép.
    auto hasRole = [&](const std::string& username,
                       std::initializer_list<const char*> allowed) -> bool {
        std::string r = actorRole(username);
        for (const char* a : allowed) if (r == a) return true;
        return false;
    };

    // ===== Đọc toàn bộ trạng thái =====
    svr.Get("/api/state", [&](const httplib::Request&, httplib::Response& res) {
        sendJson(res, fullState(system));
    });

    // ===== Hành động: đẩy chuyến bay sang trạng thái kế tiếp =====
    svr.Post("/api/flight/advance", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.advanceFlight(q(req, "code"))));
    });

    // ===== Check-in một hành khách =====
    svr.Post("/api/flight/checkin", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.checkIn(q(req, "code"), q(req, "pid"), q(req, "seat"))));
    });

    // ===== Cho hành khách lên máy bay =====
    svr.Post("/api/flight/board", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.board(q(req, "code"), q(req, "pid"))));
    });

    // ===== Thao tác hàng loạt: check-in / lên máy bay toàn bộ chuyến =====
    svr.Post("/api/flight/checkin-all", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.checkInAll(q(req, "code"))));
    });
    svr.Post("/api/flight/board-all", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.boardAll(q(req, "code"))));
    });

    // ===== Hoãn / huỷ chuyến =====
    svr.Post("/api/flight/delay", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được hoãn chuyến bay.")));
            return;
        }
        long long m = std::atoll(q(req, "minutes").c_str());
        sendJson(res, jResult(system.delayFlight(q(req, "code"), m, q(req, "reason"))));
    });
    svr.Post("/api/flight/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được huỷ chuyến bay.")));
            return;
        }
        sendJson(res, jResult(system.cancelFlight(q(req, "code"), q(req, "reason"))));
    });

    // ===== Tạo chuyến + gán tài nguyên + đặt chỗ (trước đây chỉ có ở CLI) =====
    svr.Post("/api/flight/create", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được tạo chuyến bay.")));
            return;
        }
        sendJson(res, jResult(system.createFlight(
            q(req, "code"), q(req, "origin"), q(req, "dest"),
            q(req, "departure"), q(req, "arrival"))));
    });
    svr.Post("/api/flight/assign-aircraft", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.assignAircraft(q(req, "code"), q(req, "reg"))));
    });
    svr.Post("/api/flight/assign-crew", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.assignCrew(q(req, "code"), q(req, "crewId"))));
    });
    svr.Post("/api/flight/assign-gate", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.assignGate(q(req, "code"), q(req, "gate"))));  // "" = tự động
    });
    svr.Post("/api/flight/book", [&](const httplib::Request& req, httplib::Response& res) {
        sendJson(res, jResult(system.bookPassenger(q(req, "code"), q(req, "pid"))));
    });

    // ===== Đồng hồ mô phỏng: tua thời gian ảo =====
    svr.Post("/api/sim/tick", [&](const httplib::Request& req, httplib::Response& res) {
        long long m = std::atoll(q(req, "minutes").c_str());
        sendJson(res, jResult(system.tickSimulation(m)));
    });

    // ===== Mô phỏng thời tiết xấu =====
    svr.Post("/api/weather", [&](const httplib::Request& req, httplib::Response& res) {
        bool cancel = q(req, "cancel") == "1" || q(req, "cancel") == "true";
        int n = system.simulateBadWeather(q(req, "airport"), q(req, "start"), q(req, "end"), cancel);
        sendJson(res, Obj().boolean("ok", true)
                 .str("message", "Số chuyến bị ảnh hưởng: " + std::to_string(n))
                 .num("affected", (long long)n).dump());
    });

    // ===== Reset về dữ liệu mẫu =====
    svr.Post("/api/reset", [&](const httplib::Request&, httplib::Response& res) {
        system = sg::AirportSystem();
        system.seedDemoData();
        sendJson(res, jResult(sg::OpResult::success("Đã nạp lại dữ liệu mẫu.")));
    });

    // ===== Đăng nhập =====
    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        auto u = system.authenticate(q(req, "username"), q(req, "password"));
        if (!u) {
            sendJson(res, Obj().boolean("ok", false)
                     .str("message", "Sai tên đăng nhập hoặc mật khẩu.").dump());
            return;
        }
        sendJson(res, Obj().boolean("ok", true)
                 .str("message", "Đăng nhập thành công.")
                 .raw("user", jUser(u)).dump());
    });

    // ===== Quản lý tài khoản (chỉ Admin) =====
    svr.Get("/api/users", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, Obj().boolean("ok", false).str("message", "Chỉ Admin được xem tài khoản.").dump());
            return;
        }
        Arr arr;
        for (const auto& u : system.users()) arr.push(jUser(u));
        sendJson(res, Obj().boolean("ok", true).raw("users", arr.dump()).dump());
    });
    svr.Post("/api/user/create", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được tạo tài khoản.")));
            return;
        }
        sendJson(res, jResult(system.addUser(q(req, "role"), q(req, "username"),
                                             q(req, "password"), q(req, "fullName"))));
    });
    svr.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        std::string username = q(req, "username");
        std::string password = q(req, "password");
        std::string fullName = q(req, "fullName");
        if (username.empty() || password.empty() || fullName.empty()) {
            sendJson(res, jResult(sg::OpResult::failure("Vui lòng điền đầy đủ thông tin đăng ký.")));
            return;
        }
        sendJson(res, jResult(system.addUser("Customer", username, password, fullName)));
    });
    svr.Post("/api/user/delete", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được xoá tài khoản.")));
            return;
        }
        sendJson(res, jResult(system.deleteUser(q(req, "username"))));
    });

    // ===== Vé: mua / huỷ (Customer & Staff & Admin) =====
    svr.Post("/api/ticket/book", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Customer", "Staff", "Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Bạn không có quyền mua vé.")));
            return;
        }
        sendJson(res, jResult(system.bookTicket(
            q(req, "actor"), q(req, "code"), q(req, "passengerName"), q(req, "seat"),
            std::atoi(q(req, "bagPieces").c_str()), std::atof(q(req, "bagWeight").c_str()),
            q(req, "phone"))));
    });
    svr.Post("/api/ticket/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Customer", "Staff", "Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Bạn không có quyền huỷ vé.")));
            return;
        }
        sendJson(res, jResult(system.cancelTicket(q(req, "ticketId"), q(req, "actor"))));
    });

    // ===== Thêm tài nguyên hạ tầng (chỉ Admin) =====
    svr.Post("/api/aircraft/create", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được thêm máy bay.")));
            return;
        }
        sg::AircraftCategory cat = sg::aircraftCategoryFromString(q(req, "category"));
        sendJson(res, jResult(system.addAircraft(cat, q(req, "reg"), q(req, "model"),
                                                 std::atoi(q(req, "capacity").c_str()))));
    });
    svr.Post("/api/runway/create", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được thêm đường băng.")));
            return;
        }
        sendJson(res, jResult(system.addRunway(q(req, "airport"), q(req, "code"),
                                               std::atoi(q(req, "length").c_str()))));
    });

    // ===== Xoá tài nguyên (chỉ Admin) =====
    svr.Post("/api/aircraft/delete", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được xoá máy bay.")));
            return;
        }
        sendJson(res, jResult(system.deleteAircraft(q(req, "reg"))));
    });
    svr.Post("/api/runway/delete", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được xoá đường băng.")));
            return;
        }
        sendJson(res, jResult(system.deleteRunway(q(req, "airport"), q(req, "code"))));
    });
    svr.Post("/api/flight/delete", [&](const httplib::Request& req, httplib::Response& res) {
        if (!hasRole(q(req, "actor"), {"Admin"})) {
            sendJson(res, jResult(sg::OpResult::failure("Chỉ Admin được xoá chuyến bay.")));
            return;
        }
        sendJson(res, jResult(system.deleteFlight(q(req, "code"))));
    });

    // ===== Tự động lưu file sau mỗi thao tác ghi (POST), trừ đăng nhập =====
    svr.set_post_routing_handler([&](const httplib::Request& req, httplib::Response&) {
        if (req.method == "POST" && req.path != "/api/login")
            system.saveAll(kDataDir);
    });

    // ===== Phục vụ web tĩnh =====
    svr.set_mount_point("/", "./web");

    std::printf("SkyGate Web chay tai http://localhost:%d\n", port);
    std::printf("Nhan Ctrl+C de dung.\n");
    if (!svr.listen("0.0.0.0", port)) {
        std::fprintf(stderr, "Khong mo duoc cong %d (co the dang ban).\n", port);
        return 1;
    }
    return 0;
}
