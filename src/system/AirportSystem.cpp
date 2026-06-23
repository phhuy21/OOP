#include "AirportSystem.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../aircraft/AircraftFactory.h"
#include "../common/Utils.h"

namespace skygate {

namespace fs = std::filesystem;
using utils::buildRecord;
using utils::parseRecord;

// ===========================================================================
//  Tiện ích nội bộ
// ===========================================================================
namespace {

std::vector<std::string> splitList(const std::string& s) {
    std::vector<std::string> out;
    if (s.empty()) return out;
    for (auto& part : utils::split(s, ',')) {
        std::string t = utils::trim(part);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

// --- Sơ đồ ghế: 6 cột A-F, số hàng = ceil(sốGhế/6), tối đa 100 ghế. ---
constexpr int kSeatCols = 6;
constexpr int kMaxSeats = 100;
constexpr int kBusinessRows = 2;  // 2 hàng đầu là hạng Thương gia

int seatCapacity(int capacity) { return capacity < kMaxSeats ? capacity : kMaxSeats; }
int seatRows(int capacity) { return (seatCapacity(capacity) + kSeatCols - 1) / kSeatCols; }

// Tách mã ghế "<hàng><cột>" -> (hàng>=1, chỉ-số-cột 0..5). false nếu sai định dạng.
bool parseSeat(const std::string& seat, int& row, int& col) {
    if (seat.size() < 2) return false;
    size_t i = 0;
    while (i < seat.size() && std::isdigit(static_cast<unsigned char>(seat[i]))) ++i;
    if (i == 0 || i != seat.size() - 1) return false;  // số ở đầu + đúng 1 ký tự cột
    char c = static_cast<char>(std::toupper(static_cast<unsigned char>(seat.back())));
    if (c < 'A' || c >= 'A' + kSeatCols) return false;
    row = std::atoi(seat.substr(0, i).c_str());
    col = c - 'A';
    return row >= 1;
}

// Ghế nằm trong sơ đồ của chuyến có sức chứa `capacity`?
bool seatInRange(const std::string& seat, int capacity) {
    int row, col;
    if (!parseSeat(seat, row, col)) return false;
    if (row > seatRows(capacity)) return false;
    int idx = (row - 1) * kSeatCols + col;
    return idx < seatCapacity(capacity);
}

// Hạng vé suy ra từ vị trí ghế: 2 hàng đầu = Thương gia, còn lại = Thường.
std::string seatClassFromSeat(const std::string& seat) {
    int row, col;
    if (parseSeat(seat, row, col) && row <= kBusinessRows) return "Thương gia";
    return "Thường";
}

}  // namespace

// ===========================================================================
//  Sân bay / gate / đường băng
// ===========================================================================
OpResult AirportSystem::addAirport(const std::string& code, const std::string& name,
                                   const std::string& note) {
    if (code.empty()) return OpResult::failure("Mã sân bay không được rỗng.");
    if (findAirport(code)) return OpResult::failure("Mã sân bay đã tồn tại: " + code);
    airports_.push_back(std::make_shared<Airport>(code, name, note));
    return OpResult::success("Đã thêm sân bay " + code + ".");
}

OpResult AirportSystem::addRunway(const std::string& airportCode, const std::string& runwayCode,
                                  int lengthMeters) {
    Airport* a = findAirport(airportCode);
    if (!a) return OpResult::failure("Không tìm thấy sân bay " + airportCode);
    if (lengthMeters <= 0) return OpResult::failure("Chiều dài đường băng phải > 0.");
    a->addRunway(Runway(runwayCode, lengthMeters));
    return OpResult::success("Đã thêm đường băng " + runwayCode + " cho " + airportCode + ".");
}

OpResult AirportSystem::addGate(const std::string& airportCode, const std::string& gateCode,
                                GateType type) {
    Airport* a = findAirport(airportCode);
    if (!a) return OpResult::failure("Không tìm thấy sân bay " + airportCode);
    if (a->findGate(gateCode)) return OpResult::failure("Gate đã tồn tại: " + gateCode);
    a->addGate(Gate(gateCode, type));
    return OpResult::success("Đã thêm gate " + gateCode + " (" + toString(type) + ").");
}

Airport* AirportSystem::findAirport(const std::string& code) {
    for (auto& a : airports_) if (a->code() == code) return a.get();
    return nullptr;
}

// ===========================================================================
//  Máy bay
// ===========================================================================
OpResult AirportSystem::addAircraft(AircraftCategory category, const std::string& registration,
                                    const std::string& model, int capacity) {
    if (registration.empty()) return OpResult::failure("Số hiệu máy bay không được rỗng.");
    if (findAircraft(registration)) return OpResult::failure("Máy bay đã tồn tại: " + registration);
    if (capacity <= 0) return OpResult::failure("Sức chứa phải > 0.");
    aircrafts_.push_back(AircraftFactory::create(category, registration, model, capacity));
    return OpResult::success("Đã thêm máy bay " + registration + " (" + toString(category) + ").");
}

std::shared_ptr<Aircraft> AirportSystem::findAircraft(const std::string& registration) {
    for (auto& a : aircrafts_) if (a->registration() == registration) return a;
    return nullptr;
}

// ===========================================================================
//  Con người
// ===========================================================================
OpResult AirportSystem::addPilot(const std::string& id, const std::string& name, int age,
                                 const std::string& phone, const std::string& base) {
    if (id.empty()) return OpResult::failure("Mã phi công không được rỗng.");
    if (findPilot(id)) return OpResult::failure("Phi công đã tồn tại: " + id);
    pilots_.push_back(std::make_shared<Pilot>(id, name, age, phone, base));
    return OpResult::success("Đã thêm phi công " + id + ".");
}

OpResult AirportSystem::addPilotCertification(const std::string& pilotId, AircraftCategory cat) {
    auto p = findPilot(pilotId);
    if (!p) return OpResult::failure("Không tìm thấy phi công " + pilotId);
    p->addCertification(cat);
    return OpResult::success("Đã cấp chứng chỉ " + toString(cat) + " cho " + pilotId + ".");
}

OpResult AirportSystem::addGroundStaff(const std::string& id, const std::string& name, int age,
                                       const std::string& phone, const std::string& base,
                                       const std::string& department) {
    if (id.empty()) return OpResult::failure("Mã nhân viên không được rỗng.");
    if (findGroundStaff(id)) return OpResult::failure("Nhân viên đã tồn tại: " + id);
    ground_.push_back(std::make_shared<GroundStaff>(id, name, age, phone, base, department));
    return OpResult::success("Đã thêm nhân viên mặt đất " + id + ".");
}

OpResult AirportSystem::addPassenger(const std::string& id, const std::string& name, int age,
                                     const std::string& phone, const std::string& passport) {
    if (id.empty()) return OpResult::failure("Mã hành khách không được rỗng.");
    if (findPassenger(id)) return OpResult::failure("Hành khách đã tồn tại: " + id);
    passengers_.push_back(std::make_shared<Passenger>(id, name, age, phone, passport));
    return OpResult::success("Đã thêm hành khách " + id + ".");
}

OpResult AirportSystem::setPassengerBaggage(const std::string& passengerId, int pieces,
                                            double weightKg) {
    auto p = findPassenger(passengerId);
    if (!p) return OpResult::failure("Không tìm thấy hành khách " + passengerId);
    if (pieces > 3) return OpResult::failure("Hành lý vượt quá số kiện tối đa cho phép (3 kiện).");
    if (weightKg > 50.0) return OpResult::failure("Hành lý vượt quá khối lượng tối đa cho phép (50.0 kg).");
    p->setBaggage(Baggage(pieces, weightKg));
    std::string msg = "Đã cập nhật hành lý cho " + passengerId + ": " + p->baggage().describe() + ".";
    return OpResult::success(msg);
}

std::shared_ptr<Pilot> AirportSystem::findPilot(const std::string& id) {
    for (auto& p : pilots_) if (p->id() == id) return p;
    return nullptr;
}
std::shared_ptr<GroundStaff> AirportSystem::findGroundStaff(const std::string& id) {
    for (auto& g : ground_) if (g->id() == id) return g;
    return nullptr;
}
std::shared_ptr<Passenger> AirportSystem::findPassenger(const std::string& id) {
    for (auto& p : passengers_) if (p->id() == id) return p;
    return nullptr;
}

// ===========================================================================
//  Tổ bay
// ===========================================================================
OpResult AirportSystem::createCrew(const std::string& crewId) {
    if (crewId.empty()) return OpResult::failure("Mã tổ bay không được rỗng.");
    if (findCrew(crewId)) return OpResult::failure("Tổ bay đã tồn tại: " + crewId);
    crews_.push_back(std::make_shared<Crew>(crewId));
    return OpResult::success("Đã tạo tổ bay " + crewId + ".");
}

OpResult AirportSystem::addPilotToCrew(const std::string& crewId, const std::string& pilotId) {
    auto c = findCrew(crewId);
    if (!c) return OpResult::failure("Không tìm thấy tổ bay " + crewId);
    auto p = findPilot(pilotId);
    if (!p) return OpResult::failure("Không tìm thấy phi công " + pilotId);
    c->addPilot(p);
    return OpResult::success("Đã thêm phi công " + pilotId + " vào tổ bay " + crewId + ".");
}

OpResult AirportSystem::addGroundToCrew(const std::string& crewId, const std::string& groundId) {
    auto c = findCrew(crewId);
    if (!c) return OpResult::failure("Không tìm thấy tổ bay " + crewId);
    auto g = findGroundStaff(groundId);
    if (!g) return OpResult::failure("Không tìm thấy nhân viên " + groundId);
    c->addGroundStaff(g);
    return OpResult::success("Đã thêm NV mặt đất " + groundId + " vào tổ bay " + crewId + ".");
}

std::shared_ptr<Crew> AirportSystem::findCrew(const std::string& id) {
    for (auto& c : crews_) if (c->id() == id) return c;
    return nullptr;
}

// ===========================================================================
//  Chuyến bay
// ===========================================================================
OpResult AirportSystem::createFlight(const std::string& code, const std::string& origin,
                                     const std::string& dest, const std::string& departure,
                                     const std::string& arrival) {
    if (code.empty()) return OpResult::failure("Mã chuyến bay không được rỗng.");
    if (findFlight(code)) return OpResult::failure("Chuyến bay đã tồn tại: " + code);
    // Mô hình 1 sân bay nhà: sân bay đi phải là sân bay ta vận hành; điểm đến
    // chỉ là tên thành phố (không cần là sân bay trong hệ thống).
    if (!findAirport(origin)) return OpResult::failure("Sân bay đi không tồn tại: " + origin);
    if (dest.empty()) return OpResult::failure("Điểm đến không được rỗng.");
    if (origin == dest) return OpResult::failure("Điểm đi và đến phải khác nhau.");

    DateTime dep = DateTime::parse(departure);
    DateTime arr = DateTime::parse(arrival);
    if (!dep.isValid()) return OpResult::failure("Giờ đi không hợp lệ (định dạng YYYY-MM-DD HH:MM).");
    if (!arr.isValid()) return OpResult::failure("Giờ đến không hợp lệ (định dạng YYYY-MM-DD HH:MM).");
    if (arr <= dep) return OpResult::failure("Giờ đến phải sau giờ đi.");

    flights_.push_back(std::make_shared<Flight>(code, origin, dest, dep, arr));
    logEvent("Tạo chuyến " + code + " (" + origin + "->" + dest + ").");
    return OpResult::success("Đã tạo chuyến bay " + code + " (" + origin + "->" + dest + ").");
}

std::shared_ptr<Flight> AirportSystem::findFlight(const std::string& code) {
    for (auto& f : flights_) if (f->code() == code) return f;
    return nullptr;
}

OpResult AirportSystem::assignAircraft(const std::string& flightCode,
                                       const std::string& registration) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    auto ac = findAircraft(registration);
    if (!ac) return OpResult::failure("Không tìm thấy máy bay " + registration);

    Airport* origin = findAirport(f->originCode());
    if (!origin) return OpResult::failure("Thiếu sân bay đi cho chuyến.");

    // Quy tắc đường băng: sân bay nhà phải có đường băng đủ dài cho máy bay.
    if (!origin->hasRunwayFor(*ac)) {
        return OpResult::failure("Sân bay đi " + origin->code() +
                                 " không có đường băng đủ dài (cần >= " +
                                 std::to_string(ac->requiredRunwayLength()) + "m).");
    }
    // Quy tắc sức chứa.
    if (static_cast<int>(f->passengers().size()) > ac->capacity()) {
        return OpResult::failure("Số hành khách (" + std::to_string(f->passengers().size()) +
                                 ") vượt sức chứa máy bay (" + std::to_string(ac->capacity()) + ").");
    }

    // Kiểm tra trùng lịch bay của máy bay
    for (const auto& other : flights_) {
        if (other->code() == flightCode) continue;
        if (other->isFinished()) continue;
        if (other->aircraft() && other->aircraft()->registration() == registration) {
            int turnaround = 0; // Đã bỏ thời gian quay đầu
            if (!(f->arrival().addMinutes(turnaround) <= other->departure() ||
                  other->arrival().addMinutes(turnaround) <= f->departure())) {
                return OpResult::failure("Máy bay " + registration + " đang bận thực hiện chuyến bay " +
                                         other->code() + " (Lịch bay: " + other->departure().toString() + 
                                         " -> " + other->arrival().toString() + ").");
            }
        }
    }

    f->setAircraft(ac);
    logEvent(flightCode + ": gán máy bay " + registration + ".");
    return OpResult::success("Đã gán máy bay " + registration + " cho chuyến " + flightCode + ".");
}

OpResult AirportSystem::assignCrew(const std::string& flightCode, const std::string& crewId) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    auto c = findCrew(crewId);
    if (!c) return OpResult::failure("Không tìm thấy tổ bay " + crewId);
    if (!f->aircraft()) {
        return OpResult::failure("Hãy gán máy bay trước để kiểm tra chứng chỉ tổ bay.");
    }

    auto issues = c->validateForFlight(f->aircraft()->category(), f->departure(), f->flightHours());
    if (!issues.empty()) {
        std::string m = "Không thể gán tổ bay:";
        for (const auto& i : issues) m += "\n      - " + i;
        return OpResult::failure(m);
    }
    f->setCrew(c);
    logEvent(flightCode + ": gán tổ bay " + crewId + ".");
    return OpResult::success("Đã gán tổ bay " + crewId + " cho chuyến " + flightCode + ".");
}

void AirportSystem::gateWindow(const Flight& flight, DateTime& outStart, DateTime& outEnd) {
    long long turnaround = 0;  // Đã bỏ thời gian quay đầu
    outStart = flight.departure().addMinutes(-turnaround);
    outEnd = flight.departure().addMinutes(30);
}

OpResult AirportSystem::assignGate(const std::string& flightCode, const std::string& gateCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    if (!f->aircraft()) return OpResult::failure("Hãy gán máy bay trước khi gán gate.");
    Airport* origin = findAirport(f->originCode());
    if (!origin) return OpResult::failure("Không tìm thấy sân bay đi.");

    DateTime start, end;
    gateWindow(*f, start, end);

    // Giải phóng gate cũ của chuyến này (nếu đổi gate).
    for (auto& g : origin->gates()) g.releaseFlight(flightCode);

    Gate* gate = nullptr;
    if (gateCode.empty()) {
        gate = origin->findAvailableGate(*f->aircraft(), start, end);
        if (!gate) return OpResult::failure("Không còn gate phù hợp & trống tại " + origin->code() + ".");
    } else {
        gate = origin->findGate(gateCode);
        if (!gate) return OpResult::failure("Không tìm thấy gate " + gateCode + ".");
        if (!f->aircraft()->canUseGateType(gate->type())) {
            return OpResult::failure("Gate " + gateCode + " (" + toString(gate->type()) +
                                     ") không phù hợp máy bay loại " + f->aircraft()->categoryName() + ".");
        }
        if (!gate->isFreeDuring(start, end)) {
            return OpResult::failure("Gate " + gateCode + " đã bị chiếm trong khung giờ này.");
        }
    }
    gate->reserve(flightCode, start, end);
    f->setGateCode(gate->code());
    logEvent(flightCode + ": gán gate " + gate->code() + ".");
    return OpResult::success("Đã gán gate " + gate->code() + " cho chuyến " + flightCode + ".");
}

OpResult AirportSystem::bookPassenger(const std::string& flightCode,
                                      const std::string& passengerId) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    auto p = findPassenger(passengerId);
    if (!p) return OpResult::failure("Không tìm thấy hành khách " + passengerId);
    if (!p->flightCode().empty() && p->flightCode() != flightCode) {
        return OpResult::failure("Hành khách đang thuộc chuyến khác: " + p->flightCode());
    }
    if (f->aircraft() &&
        static_cast<int>(f->passengers().size()) >= f->aircraft()->capacity()) {
        return OpResult::failure("Chuyến đã đầy (sức chứa " +
                                 std::to_string(f->aircraft()->capacity()) + ").");
    }
    f->addPassenger(p);
    logEvent(flightCode + ": đặt chỗ hành khách " + passengerId + ".");
    return OpResult::success("Đã thêm hành khách " + passengerId + " vào chuyến " + flightCode + ".");
}

OpResult AirportSystem::checkIn(const std::string& flightCode, const std::string& passengerId,
                                const std::string& seat) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    return f->checkInPassenger(passengerId, seat);
}

OpResult AirportSystem::board(const std::string& flightCode, const std::string& passengerId) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    return f->boardPassenger(passengerId);
}

OpResult AirportSystem::checkInAll(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    int done = 0, skipped = 0;
    for (const auto& p : f->passengers()) {
        if (p->checkedIn()) continue;
        // Giữ ghế đã gán khi đặt vé (seat rỗng -> không đổi ghế).
        if (f->checkInPassenger(p->id(), "").ok) ++done;
        else ++skipped;
    }
    if (done == 0)
        return OpResult::failure("Không check-in được khách nào (trạng thái " + f->statusName() +
                                 " hoặc mọi khách đã check-in).");
    std::string msg = "Đã check-in " + std::to_string(done) + " khách của chuyến " + flightCode + ".";
    if (skipped) msg += " (" + std::to_string(skipped) + " khách không hợp lệ.)";
    logEvent(flightCode + ": check-in hàng loạt " + std::to_string(done) + " khách.");
    return OpResult::success(msg);
}

OpResult AirportSystem::boardAll(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    int done = 0, skipped = 0;
    for (const auto& p : f->passengers()) {
        if (!p->checkedIn() || p->boarded()) { ++skipped; continue; }
        if (f->boardPassenger(p->id()).ok) ++done;
        else ++skipped;
    }
    if (done == 0)
        return OpResult::failure("Không cho khách nào lên máy bay được (cửa khởi hành chưa mở "
                                 "hoặc khách chưa check-in). Trạng thái: " + f->statusName() + ".");
    std::string msg = "Đã cho " + std::to_string(done) + " khách lên máy bay (chuyến " + flightCode + ").";
    if (skipped) msg += " (" + std::to_string(skipped) + " khách chưa check-in/đã lên.)";
    logEvent(flightCode + ": cho lên máy bay hàng loạt " + std::to_string(done) + " khách.");
    return OpResult::success(msg);
}

std::vector<std::string> AirportSystem::validateCrewForFlight(const Flight& flight) const {
    if (!flight.crew()) return {"Chưa gán tổ bay."};
    if (!flight.aircraft()) return {"Chưa gán máy bay."};
    return flight.crew()->validateForFlight(flight.aircraft()->category(),
                                            flight.departure(), flight.flightHours());
}

OpResult AirportSystem::advanceFlight(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    auto issues = validateCrewForFlight(*f);
    std::string oldStatus = f->statusName();
    OpResult r = f->advance(issues);
    if (r.ok) logEvent(flightCode + ": " + oldStatus + " -> " + f->statusName() + ".");
    return r;
}

OpResult AirportSystem::delayFlight(const std::string& flightCode, long long minutes,
                                    const std::string& reason) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    OpResult r = f->delay(minutes, reason);
    if (r.ok)
        logEvent(flightCode + ": HOÃN " + std::to_string(minutes) + " phút — " + reason + ".");
    return r;
}

OpResult AirportSystem::cancelFlight(const std::string& flightCode, const std::string& reason) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    OpResult r = f->cancel(reason);
    if (r.ok) {
        logEvent(flightCode + ": HUỶ — " + reason + ".");
        // Trả gate và loại khỏi hàng đợi.
        Airport* origin = findAirport(f->originCode());
        if (origin) for (auto& g : origin->gates()) g.releaseFlight(flightCode);
        auto rm = [&](std::vector<std::string>& q) {
            q.erase(std::remove(q.begin(), q.end(), flightCode), q.end());
        };
        rm(departureQueue_);
        rm(arrivalQueue_);
    }
    return r;
}

void AirportSystem::setMarkEmergency(const std::string& flightCode, bool value) {
    auto f = findFlight(flightCode);
    if (f) f->setEmergency(value);
}

// ===========================================================================
//  Xoá tài nguyên (dành cho Admin)
// ===========================================================================
OpResult AirportSystem::deleteAircraft(const std::string& registration) {
    auto ac = findAircraft(registration);
    if (!ac) return OpResult::failure("Không tìm thấy máy bay " + registration);
    // Không cho xoá nếu đang gán cho một chuyến chưa kết thúc.
    for (auto& f : flights_) {
        if (!f->isFinished() && f->aircraft() && f->aircraft()->registration() == registration) {
            return OpResult::failure("Máy bay đang gán cho chuyến " + f->code() +
                                     " (chưa kết thúc), không thể xoá.");
        }
    }
    aircrafts_.erase(std::remove(aircrafts_.begin(), aircrafts_.end(), ac), aircrafts_.end());
    logEvent("Xoá máy bay " + registration + ".");
    return OpResult::success("Đã xoá máy bay " + registration + ".");
}

OpResult AirportSystem::deleteRunway(const std::string& airportCode,
                                     const std::string& runwayCode) {
    Airport* a = findAirport(airportCode);
    if (!a) return OpResult::failure("Không tìm thấy sân bay " + airportCode);
    if (!a->removeRunway(runwayCode))
        return OpResult::failure("Không tìm thấy đường băng " + runwayCode + " tại " + airportCode);
    logEvent("Xoá đường băng " + runwayCode + " tại " + airportCode + ".");
    return OpResult::success("Đã xoá đường băng " + runwayCode + ".");
}

OpResult AirportSystem::deleteFlight(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);

    // Giải phóng hành khách & gỡ vé liên quan.
    std::vector<std::string> pids;
    for (auto& p : f->passengers()) pids.push_back(p->id());

    for (const auto& pid : pids) {
        passengers_.erase(std::remove_if(passengers_.begin(), passengers_.end(),
                                         [&](const std::shared_ptr<Passenger>& p) {
                                             return p->id() == pid;
                                         }),
                          passengers_.end());
    }

    tickets_.erase(std::remove_if(tickets_.begin(), tickets_.end(),
                                  [&](const std::shared_ptr<Ticket>& t) {
                                      return t->flightCode() == flightCode;
                                  }),
                   tickets_.end());
    // Trả gate và loại khỏi hàng đợi.
    Airport* origin = findAirport(f->originCode());
    if (origin) for (auto& g : origin->gates()) g.releaseFlight(flightCode);
    auto rm = [&](std::vector<std::string>& q) {
        q.erase(std::remove(q.begin(), q.end(), flightCode), q.end());
    };
    rm(departureQueue_);
    rm(arrivalQueue_);

    flights_.erase(std::remove(flights_.begin(), flights_.end(), f), flights_.end());
    logEvent("Xoá chuyến bay " + flightCode + ".");
    return OpResult::success("Đã xoá chuyến bay " + flightCode + ".");
}

// ===========================================================================
//  Tài khoản người dùng (Admin / Staff / Customer)
// ===========================================================================
OpResult AirportSystem::addUser(const std::string& role, const std::string& username,
                                const std::string& password, const std::string& fullName) {
    if (username.empty()) return OpResult::failure("Tên đăng nhập không được rỗng.");
    if (password.empty()) return OpResult::failure("Mật khẩu không được rỗng.");
    if (role != "Admin" && role != "Staff" && role != "Customer")
        return OpResult::failure("Vai trò không hợp lệ: " + role);
    if (findUser(username)) return OpResult::failure("Tên đăng nhập đã tồn tại: " + username);
    users_.push_back(auth::makeUser(role, username, password, fullName));
    logEvent("Thêm tài khoản " + role + " '" + username + "'.");
    return OpResult::success("Đã tạo tài khoản " + role + " '" + username + "'.");
}

OpResult AirportSystem::deleteUser(const std::string& username) {
    auto u = findUser(username);
    if (!u) return OpResult::failure("Không tìm thấy tài khoản " + username);
    if (u->role() == "Admin") {
        int admins = 0;
        for (auto& x : users_) if (x->role() == "Admin") ++admins;
        if (admins <= 1) return OpResult::failure("Không thể xoá Admin cuối cùng.");
    }
    users_.erase(std::remove(users_.begin(), users_.end(), u), users_.end());
    logEvent("Xoá tài khoản '" + username + "'.");
    return OpResult::success("Đã xoá tài khoản " + username + ".");
}

std::shared_ptr<auth::User> AirportSystem::findUser(const std::string& username) {
    for (auto& u : users_) if (u->username() == username) return u;
    return nullptr;
}

std::shared_ptr<auth::User> AirportSystem::authenticate(const std::string& username,
                                                        const std::string& password) {
    auto u = findUser(username);
    if (u && u->checkPassword(password)) return u;
    return nullptr;
}

// ===========================================================================
//  Vé (Ticket): trọng tâm đặt / huỷ vé theo ghế trống
// ===========================================================================
int AirportSystem::availableSeats(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f || !f->aircraft()) return -1;
    return f->aircraft()->capacity() - static_cast<int>(f->passengers().size());
}

OpResult AirportSystem::bookTicket(const std::string& ownerUsername,
                                   const std::string& flightCode,
                                   const std::string& passengerName,
                                   const std::string& seat, int bagPieces,
                                   double bagWeightKg, const std::string& phone) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    if (f->isFinished()) return OpResult::failure("Chuyến bay đã kết thúc/huỷ, không thể đặt vé.");
    
    FlightStatus st = f->status();
    if (st != FlightStatus::Scheduled && st != FlightStatus::CheckIn && 
        st != FlightStatus::Boarding && st != FlightStatus::Delayed) {
        return OpResult::failure("Chuyến bay đã đóng cửa vé (đã đóng quầy, cất cánh hoặc hoàn tất).");
    }
    if (simulationTime_ >= f->departure()) {
        return OpResult::failure("Chuyến bay đã qua giờ khởi hành, không thể đặt vé.");
    }
    if (passengerName.empty()) return OpResult::failure("Tên hành khách không được rỗng.");
    if (bagPieces > 3) return OpResult::failure("Hành lý vượt quá số kiện tối đa cho phép (3 kiện).");
    if (bagWeightKg > 50.0) return OpResult::failure("Hành lý vượt quá khối lượng tối đa cho phép (50.0 kg).");
    if (!f->aircraft())
        return OpResult::failure("Chuyến chưa gán máy bay nên chưa mở bán vé.");
    if (static_cast<int>(f->passengers().size()) >= f->aircraft()->capacity())
        return OpResult::failure("Chuyến đã hết ghế (sức chứa " +
                                 std::to_string(f->aircraft()->capacity()) + ").");

    // --- Chọn & khoá ghế ---
    int cap = f->aircraft()->capacity();
    std::string chosenSeat = seat;
    for (char& ch : chosenSeat) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    if (!chosenSeat.empty()) {
        if (!seatInRange(chosenSeat, cap))
            return OpResult::failure("Mã ghế không hợp lệ: " + seat);
        for (const auto& p : f->passengers())
            if (p->seat() == chosenSeat)
                return OpResult::failure("Ghế " + chosenSeat + " đã có người chọn, mời chọn ghế khác.");
    } else {
        // Tự gán ghế trống đầu tiên (giữ cho console/đặt nhanh không cần sơ đồ).
        std::vector<std::string> taken;
        for (const auto& p : f->passengers())
            if (!p->seat().empty()) taken.push_back(p->seat());
        int total = seatCapacity(cap);
        for (int idx = 0; idx < total && chosenSeat.empty(); ++idx) {
            std::string s = std::to_string(idx / kSeatCols + 1) +
                            static_cast<char>('A' + idx % kSeatCols);
            if (std::find(taken.begin(), taken.end(), s) == taken.end()) chosenSeat = s;
        }
        if (chosenSeat.empty()) return OpResult::failure("Không còn ghế trống.");
    }
    std::string cls = seatClassFromSeat(chosenSeat);

    // Sinh mã hành khách & mã vé duy nhất.
    std::string pid;
    do {
        std::ostringstream o;
        o << "PX" << std::setw(5) << std::setfill('0') << (++ticketCounter_ + 10000);
        pid = o.str();
    } while (findPassenger(pid));
    std::ostringstream tk;
    tk << "TK" << std::setw(4) << std::setfill('0') << ticketCounter_;
    std::string tid = tk.str();

    passengers_.push_back(std::make_shared<Passenger>(pid, passengerName, 0, phone, ""));
    auto np = findPassenger(pid);
    np->setSeat(chosenSeat);
    np->setBaggage(Baggage(bagPieces, bagWeightKg));
    f->addPassenger(np);
    tickets_.push_back(
        std::make_shared<Ticket>(tid, flightCode, pid, passengerName, ownerUsername, cls));
    logEvent(flightCode + ": bán vé " + tid + " (" + cls + ", ghế " + chosenSeat + ") cho '" +
             passengerName + "' (" + ownerUsername + ").");
    return OpResult::success("Đã mua vé " + tid + " (" + cls + ", ghế " + chosenSeat + ") cho '" +
                             passengerName + "' trên chuyến " + flightCode + ". Ghế còn lại: " +
                             std::to_string(availableSeats(flightCode)) + ".");
}

OpResult AirportSystem::cancelTicket(const std::string& ticketId,
                                     const std::string& requesterUsername) {
    auto t = findTicket(ticketId);
    if (!t) return OpResult::failure("Không tìm thấy vé " + ticketId);
    // Customer chỉ huỷ vé của mình; Staff/Admin (chuỗi rỗng = bỏ qua kiểm tra) huỷ được mọi vé.
    if (!requesterUsername.empty() && t->ownerUsername() != requesterUsername) {
        auto u = findUser(requesterUsername);
        bool privileged = u && (u->role() == "Admin" || u->role() == "Staff");
        if (!privileged) return OpResult::failure("Bạn chỉ có thể huỷ vé của chính mình.");
    }

    std::string flightCode = t->flightCode();
    std::string pid = t->passengerId();
    auto f = findFlight(flightCode);
    if (f) f->removePassenger(pid);
    // Hành khách được tạo riêng cho vé -> xoá luôn khỏi hệ thống.
    passengers_.erase(std::remove_if(passengers_.begin(), passengers_.end(),
                                     [&](const std::shared_ptr<Passenger>& p) {
                                         return p->id() == pid;
                                     }),
                      passengers_.end());
    tickets_.erase(std::remove(tickets_.begin(), tickets_.end(), t), tickets_.end());
    logEvent(flightCode + ": huỷ vé " + ticketId + ".");
    return OpResult::success("Đã huỷ vé " + ticketId + " trên chuyến " + flightCode + ".");
}

std::shared_ptr<Ticket> AirportSystem::findTicket(const std::string& ticketId) {
    for (auto& t : tickets_) if (t->ticketId() == ticketId) return t;
    return nullptr;
}

// ===========================================================================
//  Đồng hồ mô phỏng & nhật ký sự kiện (chức năng mở rộng)
// ===========================================================================
void AirportSystem::logEvent(const std::string& text) {
    std::string stamp = simulationTime_.isValid() ? simulationTime_.toString() : "--";
    eventLog_.push_back(LogEntry{stamp, text});
    // Giới hạn ~200 dòng gần nhất để nhật ký không phình vô hạn.
    if (eventLog_.size() > 200)
        eventLog_.erase(eventLog_.begin(),
                        eventLog_.begin() + (eventLog_.size() - 200));
}

// Vị trí trên trục vòng đời tuyến tính (Scheduled..Completed = 0..9).
// Delayed coi như mức Boarding (2) để advance() đưa nó về Boarding rồi tiếp tục;
// Cancelled là sentinel kết thúc (100).
int AirportSystem::lifecycleOrdinal(FlightStatus s) {
    switch (s) {
        case FlightStatus::Scheduled:  return 0;
        case FlightStatus::CheckIn:    return 1;
        case FlightStatus::Boarding:   return 2;
        case FlightStatus::GateClosed: return 3;
        case FlightStatus::Ready:      return 4;
        case FlightStatus::Takeoff:    return 5;
        case FlightStatus::InAir:      return 6;
        case FlightStatus::Landed:     return 7;
        case FlightStatus::Turnaround: return 8;
        case FlightStatus::Completed:  return 9;
        case FlightStatus::Delayed:    return 2;
        case FlightStatus::Cancelled:  return 100;
    }
    return 0;
}

// Trạng thái mục tiêu của chuyến theo thời gian ảo hiện tại so với giờ đi/đến.
// Cố ý nhắm InAir (không Takeoff) và Turnaround (không Landed) để vòng lặp ở
// tickSimulation đi XUYÊN QUA các bước trung gian — Landed vẫn được kích hoạt
// nên giờ bay của tổ bay vẫn được cộng dồn.
FlightStatus AirportSystem::targetStatusFor(const Flight& f) const {
    const DateTime& now = simulationTime_;
    long long toDep = now.minutesUntil(f.departure());  // dep - now (>0: dep ở tương lai)
    long long toArr = now.minutesUntil(f.arrival());    // arr - now

    if (toDep > 120) return FlightStatus::Scheduled;
    if (toDep > 40)  return FlightStatus::CheckIn;
    if (toDep > 15)  return FlightStatus::Boarding;
    if (toDep > 5)   return FlightStatus::GateClosed;
    if (toDep > 0)   return FlightStatus::Ready;

    if (toArr > 0)   return FlightStatus::InAir;  // đã tới giờ đi, chưa tới giờ đến

    long long sinceArr = -toArr;  // số phút sau giờ đến
    long long turn = f.aircraft() ? f.aircraft()->minTurnaroundMinutes() : 60;
    if (sinceArr < turn) return FlightStatus::Turnaround;
    return FlightStatus::Completed;
}

OpResult AirportSystem::tickSimulation(long long minutes) {
    if (minutes < 0) return OpResult::failure("Số phút tua phải >= 0.");
    if (!simulationTime_.isValid())
        simulationTime_ = DateTime(2026, 6, 10, 6, 0);  // phòng thủ
    simulationTime_ = simulationTime_.addMinutes(minutes);

    int moved = 0;
    for (auto& f : flights_) {
        FlightStatus st = f->status();
        if (st == FlightStatus::Cancelled || st == FlightStatus::Completed) continue;
        // Chuyến đang hoãn: đưa về Boarding trước (tránh kẹt vì cùng ordinal).
        if (st == FlightStatus::Delayed) advanceFlight(f->code());

        FlightStatus target = targetStatusFor(*f);
        int guard = 0;  // chặn vòng lặp vô hạn (vòng đời tối đa ~10 bước)
        while (lifecycleOrdinal(f->status()) < lifecycleOrdinal(target) && guard++ < 12) {
            OpResult r = advanceFlight(f->code());  // tái dùng: kiểm tra tổ bay + ghi log
            if (!r.ok) break;  // vd chưa gán máy bay -> kẹt ở GateClosed, KHÔNG spam
            ++moved;
        }
    }
    logEvent("Tua thời gian +" + std::to_string(minutes) + " phút. " +
             std::to_string(moved) + " bước chuyển trạng thái.");
    return OpResult::success("Đồng hồ -> " + simulationTime_.toString() + " (" +
                             std::to_string(moved) + " chuyển trạng thái).");
}

// ===========================================================================
//  Hàng đợi cất / hạ cánh (chức năng mở rộng)
// ===========================================================================
OpResult AirportSystem::enqueueDeparture(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    if (f->isFinished()) return OpResult::failure("Chuyến đã kết thúc.");
    if (std::find(departureQueue_.begin(), departureQueue_.end(), flightCode) != departureQueue_.end())
        return OpResult::failure("Chuyến đã ở trong hàng đợi cất cánh.");
    departureQueue_.push_back(flightCode);
    return OpResult::success("Đã đưa " + flightCode + " vào hàng đợi cất cánh.");
}

OpResult AirportSystem::enqueueArrival(const std::string& flightCode) {
    auto f = findFlight(flightCode);
    if (!f) return OpResult::failure("Không tìm thấy chuyến bay " + flightCode);
    if (std::find(arrivalQueue_.begin(), arrivalQueue_.end(), flightCode) != arrivalQueue_.end())
        return OpResult::failure("Chuyến đã ở trong hàng đợi hạ cánh.");
    arrivalQueue_.push_back(flightCode);
    return OpResult::success("Đã đưa " + flightCode + " vào hàng đợi hạ cánh.");
}

int AirportSystem::pickPriorityIndex(const std::vector<std::string>& queue) const {
    int best = -1;
    bool bestEmergency = false;
    DateTime bestDep;
    for (size_t i = 0; i < queue.size(); ++i) {
        std::shared_ptr<Flight> f;
        for (auto& fl : flights_) if (fl->code() == queue[i]) { f = fl; break; }
        if (!f) continue;
        bool em = f->emergency();
        const DateTime& dep = f->departure();
        bool take = false;
        if (best == -1) {
            take = true;
        } else if (em != bestEmergency) {
            take = em;  // ưu tiên chuyến khẩn cấp
        } else {
            take = dep < bestDep;  // cùng mức ưu tiên: giờ đi sớm hơn trước
        }
        if (take) {
            best = static_cast<int>(i);
            bestEmergency = em;
            bestDep = dep;
        }
    }
    return best;
}

OpResult AirportSystem::processNextDeparture() {
    if (departureQueue_.empty()) return OpResult::failure("Hàng đợi cất cánh trống.");
    int idx = pickPriorityIndex(departureQueue_);
    if (idx < 0) return OpResult::failure("Không có chuyến hợp lệ trong hàng đợi.");
    std::string code = departureQueue_[static_cast<size_t>(idx)];
    auto f = findFlight(code);
    if (f->status() != FlightStatus::Ready) {
        return OpResult::failure("Chuyến " + code + " chưa ở trạng thái Ready (hiện: " +
                                 f->statusName() + "). Hãy hoàn tất chuẩn bị trước.");
    }
    OpResult r1 = advanceFlight(code);  // Ready -> Takeoff
    OpResult r2 = advanceFlight(code);  // Takeoff -> InAir
    departureQueue_.erase(departureQueue_.begin() + idx);
    arrivalQueue_.push_back(code);
    std::string em = f->emergency() ? " [KHẨN CẤP]" : "";
    return OpResult::success("Cất cánh chuyến " + code + em + ". " + r1.message + " | " + r2.message +
                             ". Đã chuyển sang hàng đợi hạ cánh.");
}

OpResult AirportSystem::processNextArrival() {
    if (arrivalQueue_.empty()) return OpResult::failure("Hàng đợi hạ cánh trống.");
    int idx = pickPriorityIndex(arrivalQueue_);
    if (idx < 0) return OpResult::failure("Không có chuyến hợp lệ trong hàng đợi.");
    std::string code = arrivalQueue_[static_cast<size_t>(idx)];
    auto f = findFlight(code);
    if (f->status() != FlightStatus::InAir) {
        return OpResult::failure("Chuyến " + code + " chưa ở trạng thái InAir (hiện: " +
                                 f->statusName() + ").");
    }
    advanceFlight(code);  // InAir -> Landed (cộng giờ bay tổ bay)
    advanceFlight(code);  // Landed -> Turnaround
    advanceFlight(code);  // Turnaround -> Completed
    arrivalQueue_.erase(arrivalQueue_.begin() + idx);
    std::string em = f->emergency() ? " [KHẨN CẤP]" : "";
    return OpResult::success("Hạ cánh & hoàn tất chuyến " + code + em +
                             " (trạng thái: " + f->statusName() + ").");
}

// ===========================================================================
//  Mô phỏng thời tiết xấu (chức năng mở rộng)
// ===========================================================================
int AirportSystem::simulateBadWeather(const std::string& airportCode, const std::string& start,
                                      const std::string& end, bool cancel) {
    DateTime ws = DateTime::parse(start);
    DateTime we = DateTime::parse(end);
    if (!ws.isValid() || !we.isValid() || we <= ws) return -1;  // tham số sai

    int affected = 0;
    for (auto& f : flights_) {
        if (f->isFinished()) continue;
        bool touches = (f->originCode() == airportCode || f->destCode() == airportCode);
        if (!touches) continue;
        // Chuyến bị ảnh hưởng nếu lịch bay giao với khung giờ thời tiết xấu.
        if (!DateTime::overlaps(f->departure(), f->arrival(), ws, we)) continue;
        // Đang trên không / đã cất cánh thì không can thiệp.
        if (f->status() == FlightStatus::InAir || f->status() == FlightStatus::Takeoff) continue;

        std::string reason = "Thời tiết xấu tại " + airportCode;
        if (cancel) {
            cancelFlight(f->code(), reason);
        } else {
            f->delay(120, reason);  // hoãn 120 phút
        }
        ++affected;
    }
    if (affected > 0)
        logEvent("Thời tiết xấu tại " + airportCode + ": " + std::to_string(affected) +
                 " chuyến bị ảnh hưởng.");
    return affected;
}

// ===========================================================================
//  Lưu / đọc dữ liệu (SQLite WAL mode)
// ===========================================================================
namespace {

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    if (!in) return lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

std::string dtStore(const DateTime& dt) { return dt.isValid() ? dt.toString() : ""; }

// ---------------------------------------------------------------------------
//  CREATE TABLE (gọi 1 lần khi mở db lần đầu)
// ---------------------------------------------------------------------------
OpResult createTables(Database& db) {
    db.execute("CREATE TABLE IF NOT EXISTS airports ("
               "code TEXT PRIMARY KEY, name TEXT NOT NULL, note TEXT NOT NULL DEFAULT '')");
    db.execute("CREATE TABLE IF NOT EXISTS runways ("
               "airport_code TEXT NOT NULL, code TEXT NOT NULL, length_meters INTEGER NOT NULL,"
               "PRIMARY KEY (airport_code, code))");
    db.execute("CREATE TABLE IF NOT EXISTS gates ("
               "airport_code TEXT NOT NULL, code TEXT NOT NULL,"
               "type TEXT NOT NULL DEFAULT 'RemoteStand',"
               "PRIMARY KEY (airport_code, code))");
    db.execute("CREATE TABLE IF NOT EXISTS aircraft ("
               "registration TEXT PRIMARY KEY, category TEXT NOT NULL,"
               "model TEXT NOT NULL, capacity INTEGER NOT NULL)");
    db.execute("CREATE TABLE IF NOT EXISTS pilots ("
               "id TEXT PRIMARY KEY, name TEXT NOT NULL, age INTEGER NOT NULL,"
               "phone TEXT NOT NULL DEFAULT '', base TEXT NOT NULL DEFAULT '',"
               "certifications TEXT NOT NULL DEFAULT '', monthly_hours REAL NOT NULL DEFAULT 0.0,"
               "last_flight_end TEXT NOT NULL DEFAULT '')");
    db.execute("CREATE TABLE IF NOT EXISTS groundstaff ("
               "id TEXT PRIMARY KEY, name TEXT NOT NULL, age INTEGER NOT NULL,"
               "phone TEXT NOT NULL DEFAULT '', base TEXT NOT NULL DEFAULT '',"
               "department TEXT NOT NULL DEFAULT '')");
    db.execute("CREATE TABLE IF NOT EXISTS passengers ("
               "id TEXT PRIMARY KEY, name TEXT NOT NULL, age INTEGER NOT NULL DEFAULT 0,"
               "phone TEXT NOT NULL DEFAULT '', passport TEXT NOT NULL DEFAULT '',"
               "flight_code TEXT NOT NULL DEFAULT '', seat TEXT NOT NULL DEFAULT '',"
               "checked_in INTEGER NOT NULL DEFAULT 0, boarded INTEGER NOT NULL DEFAULT 0,"
               "bag_pieces INTEGER NOT NULL DEFAULT 0, bag_weight REAL NOT NULL DEFAULT 0.0)");
    db.execute("CREATE TABLE IF NOT EXISTS crews ("
               "id TEXT PRIMARY KEY)");
    db.execute("CREATE TABLE IF NOT EXISTS crew_pilots ("
               "crew_id TEXT NOT NULL, pilot_id TEXT NOT NULL,"
               "PRIMARY KEY (crew_id, pilot_id))");
    db.execute("CREATE TABLE IF NOT EXISTS crew_ground ("
               "crew_id TEXT NOT NULL, ground_id TEXT NOT NULL,"
               "PRIMARY KEY (crew_id, ground_id))");
    db.execute("CREATE TABLE IF NOT EXISTS flights ("
               "code TEXT PRIMARY KEY, origin TEXT NOT NULL, dest TEXT NOT NULL,"
               "departure TEXT NOT NULL, arrival TEXT NOT NULL,"
               "status TEXT NOT NULL DEFAULT 'Scheduled', emergency INTEGER NOT NULL DEFAULT 0,"
               "note TEXT NOT NULL DEFAULT '', aircraft_reg TEXT NOT NULL DEFAULT '',"
               "crew_id TEXT NOT NULL DEFAULT '', gate_code TEXT NOT NULL DEFAULT '')");
    db.execute("CREATE TABLE IF NOT EXISTS flight_passengers ("
               "flight_code TEXT NOT NULL, passenger_id TEXT NOT NULL, pos INTEGER NOT NULL DEFAULT 0,"
               "PRIMARY KEY (flight_code, passenger_id))");
    db.execute("CREATE TABLE IF NOT EXISTS users ("
               "role TEXT NOT NULL, username TEXT PRIMARY KEY,"
               "password TEXT NOT NULL, full_name TEXT NOT NULL DEFAULT '')");
    db.execute("CREATE TABLE IF NOT EXISTS tickets ("
               "ticket_id TEXT PRIMARY KEY, flight_code TEXT NOT NULL DEFAULT '',"
               "passenger_id TEXT NOT NULL DEFAULT '', passenger_name TEXT NOT NULL DEFAULT '',"
               "owner_username TEXT NOT NULL DEFAULT '', seat_class TEXT NOT NULL DEFAULT 'Thường')");
    db.execute("CREATE TABLE IF NOT EXISTS queues ("
               "type TEXT NOT NULL, pos INTEGER NOT NULL DEFAULT 0, code TEXT NOT NULL,"
               "PRIMARY KEY (type, pos))");
    db.execute("CREATE TABLE IF NOT EXISTS metadata ("
               "key TEXT PRIMARY KEY, value TEXT NOT NULL DEFAULT '')");
    db.execute("CREATE TABLE IF NOT EXISTS event_log ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, time TEXT NOT NULL, text TEXT NOT NULL)");
    return OpResult::success();
}

// Xoá toàn bộ dữ liệu cũ trong db (theo thứ tự ngược phụ thuộc).
OpResult clearTables(Database& db) {
    db.execute("DELETE FROM event_log");
    db.execute("DELETE FROM flight_passengers");
    db.execute("DELETE FROM crew_pilots");
    db.execute("DELETE FROM crew_ground");
    db.execute("DELETE FROM queues");
    db.execute("DELETE FROM tickets");
    db.execute("DELETE FROM flights");
    db.execute("DELETE FROM crews");
    db.execute("DELETE FROM passengers");
    db.execute("DELETE FROM groundstaff");
    db.execute("DELETE FROM pilots");
    db.execute("DELETE FROM aircraft");
    db.execute("DELETE FROM runways");
    db.execute("DELETE FROM gates");
    db.execute("DELETE FROM users");
    db.execute("DELETE FROM airports");
    db.execute("DELETE FROM metadata");
    return OpResult::success();
}

}  // namespace

// =================================== saveToDatabase ========================
OpResult AirportSystem::saveToDatabase(const std::string& dir) {
    std::string dbPath = dir + "/skygate.db";
    OpResult openResult = db_.open(dbPath);
    if (!openResult.ok) return openResult;

    OpResult tblResult = createTables(db_);
    if (!tblResult.ok) return tblResult;

    return db_.transaction([&]() -> OpResult {
        OpResult clearResult = clearTables(db_);
        if (!clearResult.ok) return clearResult;

        // --- Airports ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO airports(code, name, note) VALUES(?1, ?2, ?3)");
            for (auto& ap : airports_) {
                stmt.bindText(1, ap->code()).bindText(2, ap->name()).bindText(3, ap->note());
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Runways ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO runways(airport_code, code, length_meters) VALUES(?1, ?2, ?3)");
            for (auto& ap : airports_)
                for (auto& rw : ap->runways()) {
                    stmt.bindText(1, ap->code()).bindText(2, rw.code())
                         .bindInt(3, rw.lengthMeters());
                    OpResult r = stmt.execute(); if (!r.ok) return r;
                }
        }
        // --- Gates ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO gates(airport_code, code, type) VALUES(?1, ?2, ?3)");
            for (auto& ap : airports_)
                for (auto& ga : ap->gates()) {
                    stmt.bindText(1, ap->code()).bindText(2, ga.code())
                         .bindText(3, toString(ga.type()));
                    OpResult r = stmt.execute(); if (!r.ok) return r;
                }
        }
        // --- Aircraft ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO aircraft(registration, category, model, capacity) "
                "VALUES(?1, ?2, ?3, ?4)");
            for (auto& ac : aircrafts_) {
                stmt.bindText(1, ac->registration()).bindText(2, toString(ac->category()))
                     .bindText(3, ac->model()).bindInt(4, ac->capacity());
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Pilots ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO pilots(id, name, age, phone, base, certifications, "
                "monthly_hours, last_flight_end) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)");
            for (auto& p : pilots_) {
                stmt.bindText(1, p->id()).bindText(2, p->fullName())
                     .bindInt(3, p->age()).bindText(4, p->phone())
                     .bindText(5, p->baseAirport())
                     .bindText(6, p->certificationList())
                     .bindDouble(7, p->monthlyFlightHours())
                     .bindText(8, dtStore(p->lastFlightEnd()));
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Ground staff ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO groundstaff(id, name, age, phone, base, department) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6)");
            for (auto& g : ground_) {
                stmt.bindText(1, g->id()).bindText(2, g->fullName())
                     .bindInt(3, g->age()).bindText(4, g->phone())
                     .bindText(5, g->baseAirport()).bindText(6, g->department());
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Passengers ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO passengers(id, name, age, phone, passport, flight_code, "
                "seat, checked_in, boarded, bag_pieces, bag_weight) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)");
            for (auto& p : passengers_) {
                stmt.bindText(1, p->id()).bindText(2, p->fullName())
                     .bindInt(3, p->age()).bindText(4, p->phone())
                     .bindText(5, p->passport())
                     .bindText(6, p->flightCode()).bindText(7, p->seat())
                     .bindInt(8, p->checkedIn() ? 1 : 0)
                     .bindInt(9, p->boarded() ? 1 : 0)
                     .bindInt(10, p->baggage().pieces())
                     .bindDouble(11, p->baggage().totalWeightKg());
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Crews + junction tables ---
        {
            auto stmtC = db_.prepare("INSERT INTO crews(id) VALUES(?1)");
            auto stmtP = db_.prepare("INSERT INTO crew_pilots(crew_id, pilot_id) VALUES(?1, ?2)");
            auto stmtG = db_.prepare("INSERT INTO crew_ground(crew_id, ground_id) VALUES(?1, ?2)");
            for (auto& c : crews_) {
                stmtC.bindText(1, c->id());
                OpResult r = stmtC.execute(); if (!r.ok) return r;
                for (auto& p : c->pilots()) {
                    stmtP.bindText(1, c->id()).bindText(2, p->id());
                    r = stmtP.execute(); if (!r.ok) return r;
                }
                for (auto& g : c->groundStaff()) {
                    stmtG.bindText(1, c->id()).bindText(2, g->id());
                    r = stmtG.execute(); if (!r.ok) return r;
                }
            }
        }
        // --- Flights + flight_passengers ---
        {
            auto stmtF = db_.prepare(
                "INSERT INTO flights(code, origin, dest, departure, arrival, status, "
                "emergency, note, aircraft_reg, crew_id, gate_code) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)");
            auto stmtFP = db_.prepare(
                "INSERT INTO flight_passengers(flight_code, passenger_id, pos) VALUES(?1, ?2, ?3)");
            for (auto& f : flights_) {
                stmtF.bindText(1, f->code()).bindText(2, f->originCode())
                     .bindText(3, f->destCode())
                     .bindText(4, dtStore(f->departure()))
                     .bindText(5, dtStore(f->arrival()))
                     .bindText(6, toString(f->status()))
                     .bindInt(7, f->emergency() ? 1 : 0)
                     .bindText(8, f->note())
                     .bindText(9, f->aircraft() ? f->aircraft()->registration() : "")
                     .bindText(10, f->crew() ? f->crew()->id() : "")
                     .bindText(11, f->gateCode());
                OpResult r = stmtF.execute(); if (!r.ok) return r;
                int pos = 0;
                for (auto& p : f->passengers()) {
                    stmtFP.bindText(1, f->code()).bindText(2, p->id()).bindInt(3, pos++);
                    r = stmtFP.execute(); if (!r.ok) return r;
                }
            }
        }
        // --- Queues ---
        {
            auto stmt = db_.prepare("INSERT INTO queues(type, pos, code) VALUES(?1, ?2, ?3)");
            for (size_t i = 0; i < departureQueue_.size(); ++i) {
                stmt.bindText(1, "DEP").bindInt(2, static_cast<int>(i))
                     .bindText(3, departureQueue_[i]);
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
            for (size_t i = 0; i < arrivalQueue_.size(); ++i) {
                stmt.bindText(1, "ARR").bindInt(2, static_cast<int>(i))
                     .bindText(3, arrivalQueue_[i]);
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Users ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO users(role, username, password, full_name) VALUES(?1, ?2, ?3, ?4)");
            for (auto& u : users_) {
                stmt.bindText(1, u->role()).bindText(2, u->username())
                     .bindText(3, u->password()).bindText(4, u->fullName());
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }
        // --- Tickets ---
        {
            auto stmt = db_.prepare(
                "INSERT OR REPLACE INTO metadata(key, value) VALUES(?1, ?2)");
            stmt.bindText(1, "ticket_counter").bindText(2, std::to_string(ticketCounter_));
            OpResult r = stmt.execute(); if (!r.ok) return r;

            auto stmtT = db_.prepare(
                "INSERT INTO tickets(ticket_id, flight_code, passenger_id, "
                "passenger_name, owner_username, seat_class) VALUES(?1, ?2, ?3, ?4, ?5, ?6)");
            for (auto& t : tickets_) {
                stmtT.bindText(1, t->ticketId()).bindText(2, t->flightCode())
                     .bindText(3, t->passengerId()).bindText(4, t->passengerName())
                     .bindText(5, t->ownerUsername()).bindText(6, t->seatClass());
                r = stmtT.execute(); if (!r.ok) return r;
            }
        }
        // --- Simulation time ---
        {
            auto stmt = db_.prepare(
                "INSERT OR REPLACE INTO metadata(key, value) VALUES(?1, ?2)");
            stmt.bindText(1, "simulation_time")
                 .bindText(2, simulationTime_.isValid() ? simulationTime_.toString() : "");
            OpResult r = stmt.execute(); if (!r.ok) return r;
        }
        // --- Event log ---
        {
            auto stmt = db_.prepare(
                "INSERT INTO event_log(time, text) VALUES(?1, ?2)");
            for (auto& e : eventLog_) {
                stmt.bindText(1, e.time).bindText(2, e.text);
                OpResult r = stmt.execute(); if (!r.ok) return r;
            }
        }

        return OpResult::success();
    }) ? OpResult::success("Đã lưu toàn bộ dữ liệu vào '" + dir + "'.")
       : OpResult::failure("Lỗi khi lưu dữ liệu.");
}

// =================================== loadFromDatabase ======================
OpResult AirportSystem::loadFromDatabase(const std::string& dir) {
    std::string dbPath = dir + "/skygate.db";
    OpResult openResult = db_.open(dbPath);
    if (!openResult.ok) return openResult;

    OpResult tblResult = createTables(db_);
    if (!tblResult.ok) return tblResult;

    // Xoá trạng thái cũ trước khi nạp.
    airports_.clear(); aircrafts_.clear(); pilots_.clear(); ground_.clear();
    passengers_.clear(); crews_.clear(); flights_.clear();
    users_.clear(); tickets_.clear(); ticketCounter_ = 0;
    departureQueue_.clear(); arrivalQueue_.clear();
    eventLog_.clear();

    // Nạp theo thứ tự phụ thuộc (giống thứ tự file .txt cũ).
    db_.query("SELECT code, name, note FROM airports",
        [&](int argc, char** vals, char**) {
            if (argc >= 3) addAirport(vals[0] ? vals[0] : "",
                                       vals[1] ? vals[1] : "",
                                       vals[2] ? vals[2] : "");
        });
    db_.query("SELECT airport_code, code, length_meters FROM runways",
        [&](int argc, char** vals, char**) {
            if (argc >= 3) addRunway(vals[0] ? vals[0] : "",
                                      vals[1] ? vals[1] : "",
                                      utils::toInt(vals[2] ? vals[2] : ""));
        });
    db_.query("SELECT airport_code, code, type FROM gates",
        [&](int argc, char** vals, char**) {
            if (argc >= 3) addGate(vals[0] ? vals[0] : "",
                                    vals[1] ? vals[1] : "",
                                    gateTypeFromString(vals[2] ? vals[2] : ""));
        });
    db_.query("SELECT registration, category, model, capacity FROM aircraft",
        [&](int argc, char** vals, char**) {
            if (argc >= 4) addAircraft(aircraftCategoryFromString(vals[1] ? vals[1] : ""),
                                        vals[0] ? vals[0] : "",
                                        vals[2] ? vals[2] : "",
                                        utils::toInt(vals[3] ? vals[3] : ""));
        });
    db_.query("SELECT id, name, age, phone, base, certifications, monthly_hours, last_flight_end FROM pilots",
        [&](int argc, char** vals, char**) {
            if (argc >= 8) {
                addPilot(vals[0] ? vals[0] : "", vals[1] ? vals[1] : "",
                          utils::toInt(vals[2] ? vals[2] : ""),
                          vals[3] ? vals[3] : "", vals[4] ? vals[4] : "");
                auto p = findPilot(vals[0] ? vals[0] : "");
                if (p) {
                    std::string certs = vals[5] ? vals[5] : "";
                    for (auto& cert : utils::split(certs, ';')) {
                        std::string t = utils::trim(cert);
                        if (!t.empty()) p->addCertification(aircraftCategoryFromString(t));
                    }
                    p->setMonthlyFlightHours(utils::toDouble(vals[6] ? vals[6] : ""));
                    DateTime last = DateTime::parse(vals[7] ? vals[7] : "");
                    if (last.isValid()) p->setLastFlightEnd(last);
                }
            }
        });
    db_.query("SELECT id, name, age, phone, base, department FROM groundstaff",
        [&](int argc, char** vals, char**) {
            if (argc >= 6) addGroundStaff(vals[0] ? vals[0] : "", vals[1] ? vals[1] : "",
                                           utils::toInt(vals[2] ? vals[2] : ""),
                                           vals[3] ? vals[3] : "", vals[4] ? vals[4] : "",
                                           vals[5] ? vals[5] : "");
        });
    db_.query("SELECT id, name, age, phone, passport, flight_code, seat, checked_in, boarded, bag_pieces, bag_weight FROM passengers",
        [&](int argc, char** vals, char**) {
            if (argc >= 11) {
                addPassenger(vals[0] ? vals[0] : "", vals[1] ? vals[1] : "",
                              utils::toInt(vals[2] ? vals[2] : ""),
                              vals[3] ? vals[3] : "", vals[4] ? vals[4] : "");
                auto p = findPassenger(vals[0] ? vals[0] : "");
                if (p) {
                    p->setFlightCode(vals[5] ? vals[5] : "");
                    p->setSeat(vals[6] ? vals[6] : "");
                    p->setCheckedIn(utils::toBool(vals[7] ? vals[7] : ""));
                    p->setBoarded(utils::toBool(vals[8] ? vals[8] : ""));
                    p->setBaggage(Baggage(utils::toInt(vals[9] ? vals[9] : ""),
                                          utils::toDouble(vals[10] ? vals[10] : "")));
                }
            }
        });
    db_.query("SELECT id FROM crews",
        [&](int argc, char** vals, char**) {
            if (argc >= 1) createCrew(vals[0] ? vals[0] : "");
        });
    db_.query("SELECT crew_id, pilot_id FROM crew_pilots",
        [&](int argc, char** vals, char**) {
            if (argc >= 2) addPilotToCrew(vals[0] ? vals[0] : "", vals[1] ? vals[1] : "");
        });
    db_.query("SELECT crew_id, ground_id FROM crew_ground",
        [&](int argc, char** vals, char**) {
            if (argc >= 2) addGroundToCrew(vals[0] ? vals[0] : "", vals[1] ? vals[1] : "");
        });
    db_.query("SELECT code, origin, dest, departure, arrival, status, emergency, note, aircraft_reg, crew_id, gate_code FROM flights",
        [&](int argc, char** vals, char**) {
            if (argc >= 11) {
                createFlight(vals[0] ? vals[0] : "", vals[1] ? vals[1] : "",
                              vals[2] ? vals[2] : "", vals[3] ? vals[3] : "",
                              vals[4] ? vals[4] : "");
                auto f = findFlight(vals[0] ? vals[0] : "");
                if (f) {
                    f->setStatus(flightStatusFromString(vals[5] ? vals[5] : ""));
                    f->setEmergency(utils::toBool(vals[6] ? vals[6] : ""));
                    f->setNote(vals[7] ? vals[7] : "");
                    if (vals[8] && *vals[8]) f->setAircraft(findAircraft(vals[8]));
                    if (vals[9] && *vals[9]) f->setCrew(findCrew(vals[9]));
                    f->setGateCode(vals[10] ? vals[10] : "");
                }
            }
        });
    // flight_passengers — nạp sau flights để addPassenger tìm được passengers
    db_.query("SELECT flight_code, passenger_id FROM flight_passengers ORDER BY pos",
        [&](int argc, char** vals, char**) {
            if (argc >= 2) {
                auto f = findFlight(vals[0] ? vals[0] : "");
                auto p = findPassenger(vals[1] ? vals[1] : "");
                if (f && p) f->addPassenger(p);
            }
        });
    // Queues
    {
        std::vector<std::pair<int, std::string>> deps, arrs;  // pos, code
        db_.query("SELECT type, pos, code FROM queues ORDER BY type, pos",
            [&](int argc, char** vals, char**) {
                if (argc >= 3) {
                    std::string t = vals[0] ? vals[0] : "";
                    int pos = utils::toInt(vals[1] ? vals[1] : "0");
                    std::string code = vals[2] ? vals[2] : "";
                    if (t == "DEP") deps.emplace_back(pos, code);
                    else if (t == "ARR") arrs.emplace_back(pos, code);
                }
            });
        std::sort(deps.begin(), deps.end());
        std::sort(arrs.begin(), arrs.end());
        for (auto& d : deps) departureQueue_.push_back(d.second);
        for (auto& a : arrs) arrivalQueue_.push_back(a.second);
    }
    // Users
    db_.query("SELECT role, username, password, full_name FROM users",
        [&](int argc, char** vals, char**) {
            if (argc >= 4)
                users_.push_back(auth::makeUser(vals[0] ? vals[0] : "",
                                                 vals[1] ? vals[1] : "",
                                                 vals[2] ? vals[2] : "",
                                                 vals[3] ? vals[3] : ""));
        });
    // Tickets
    db_.query("SELECT ticket_id, flight_code, passenger_id, passenger_name, owner_username, seat_class FROM tickets",
        [&](int argc, char** vals, char**) {
            if (argc >= 6)
                tickets_.push_back(std::make_shared<Ticket>(
                    vals[0] ? vals[0] : "", vals[1] ? vals[1] : "",
                    vals[2] ? vals[2] : "", vals[3] ? vals[3] : "",
                    vals[4] ? vals[4] : "", vals[5] ? vals[5] : ""));
            else if (argc >= 5)
                tickets_.push_back(std::make_shared<Ticket>(
                    vals[0] ? vals[0] : "", vals[1] ? vals[1] : "",
                    vals[2] ? vals[2] : "", vals[3] ? vals[3] : "",
                    vals[4] ? vals[4] : ""));
        });
    // Metadata: ticket_counter, simulation_time
    {
        auto rows = db_.queryAll("SELECT key, value FROM metadata");
        for (auto& r : rows) {
            if (r.size() >= 2) {
                if (r[0] == "ticket_counter") ticketCounter_ = utils::toInt(r[1]);
                else if (r[0] == "simulation_time" && !r[1].empty()) {
                    DateTime dt = DateTime::parse(r[1]);
                    if (dt.isValid()) simulationTime_ = dt;
                }
            }
        }
    }
    // Event log
    db_.query("SELECT time, text FROM event_log ORDER BY id",
        [&](int argc, char** vals, char**) {
            if (argc >= 2) eventLog_.push_back({vals[0] ? vals[0] : "", vals[1] ? vals[1] : ""});
        });

    rebuildGateReservations();
    return OpResult::success("Đã đọc dữ liệu từ cơ sở dữ liệu '" + dir + "'.");
}

// =================================== migrateFromTxt ========================
OpResult AirportSystem::migrateFromTxt(const std::string& dir) {
    // Đọc toàn bộ dữ liệu từ file .txt cũ bằng logic parseRecord gốc.
    // loadAll cũ đã clear state trước khi gọi; migrateFromTxt được gọi từ loadAll
    // sau khi state đã được clear.

    auto path = [&](const std::string& f) { return dir + "/" + f; };

    for (auto& line : readLines(path("airports.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 3) addAirport(r[0], r[1], r[2]);
    }
    for (auto& line : readLines(path("runways.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 3) addRunway(r[0], r[1], utils::toInt(r[2]));
    }
    for (auto& line : readLines(path("gates.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 3) addGate(r[0], r[1], gateTypeFromString(r[2]));
    }
    for (auto& line : readLines(path("aircraft.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 4)
            addAircraft(aircraftCategoryFromString(r[0]), r[1], r[2], utils::toInt(r[3]));
    }
    for (auto& line : readLines(path("pilots.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 8) {
            addPilot(r[0], r[1], utils::toInt(r[2]), r[3], r[4]);
            auto p = findPilot(r[0]);
            if (p) {
                for (auto& cert : utils::split(r[5], ';')) {
                    std::string t = utils::trim(cert);
                    if (!t.empty()) p->addCertification(aircraftCategoryFromString(t));
                }
                p->setMonthlyFlightHours(utils::toDouble(r[6]));
                DateTime last = DateTime::parse(r[7]);
                if (last.isValid()) p->setLastFlightEnd(last);
            }
        }
    }
    for (auto& line : readLines(path("groundstaff.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 6) addGroundStaff(r[0], r[1], utils::toInt(r[2]), r[3], r[4], r[5]);
    }
    for (auto& line : readLines(path("passengers.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 11) {
            addPassenger(r[0], r[1], utils::toInt(r[2]), r[3], r[4]);
            auto p = findPassenger(r[0]);
            if (p) {
                p->setFlightCode(r[5]);
                p->setSeat(r[6]);
                p->setCheckedIn(utils::toBool(r[7]));
                p->setBoarded(utils::toBool(r[8]));
                p->setBaggage(Baggage(utils::toInt(r[9]), utils::toDouble(r[10])));
            }
        }
    }
    for (auto& line : readLines(path("crews.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 3) {
            createCrew(r[0]);
            for (auto& pid : splitList(r[1])) addPilotToCrew(r[0], pid);
            for (auto& gid : splitList(r[2])) addGroundToCrew(r[0], gid);
        }
    }
    for (auto& line : readLines(path("flights.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 12) {
            createFlight(r[0], r[1], r[2], r[3], r[4]);
            auto f = findFlight(r[0]);
            if (f) {
                f->setStatus(flightStatusFromString(r[5]));
                f->setEmergency(utils::toBool(r[6]));
                f->setNote(r[7]);
                if (!r[8].empty()) f->setAircraft(findAircraft(r[8]));
                if (!r[9].empty()) f->setCrew(findCrew(r[9]));
                f->setGateCode(r[10]);
                for (auto& pid : splitList(r[11])) {
                    auto p = findPassenger(pid);
                    if (p) f->addPassenger(p);
                }
            }
        }
    }
    for (auto& line : readLines(path("queues.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 2) {
            if (r[0] == "DEP") departureQueue_ = splitList(r[1]);
            else if (r[0] == "ARR") arrivalQueue_ = splitList(r[1]);
        }
    }
    for (auto& line : readLines(path("users.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 4) users_.push_back(auth::makeUser(r[0], r[1], r[2], r[3]));
    }
    for (auto& line : readLines(path("tickets.txt"))) {
        auto r = parseRecord(line);
        if (r.size() >= 2 && r[0] == "COUNTER") {
            ticketCounter_ = utils::toInt(r[1]);
        } else if (r.size() >= 6) {
            tickets_.push_back(std::make_shared<Ticket>(r[0], r[1], r[2], r[3], r[4], r[5]));
        } else if (r.size() >= 5) {
            tickets_.push_back(std::make_shared<Ticket>(r[0], r[1], r[2], r[3], r[4]));
        }
    }
    simulationTime_ = DateTime();
    for (auto& line : readLines(path("simulation_time.txt"))) {
        DateTime dt = DateTime::parse(line);
        if (dt.isValid()) simulationTime_ = dt;
    }
    rebuildGateReservations();

    // Lưu vào SQLite.
    OpResult saveResult = saveToDatabase(dir);
    if (!saveResult.ok) return OpResult::failure("Di trú .txt -> SQLite thất bại: " + saveResult.message);

    // Đổi tên file .txt -> .txt.bak.
    const char* txtFiles[] = {
        "airports.txt", "runways.txt", "gates.txt", "aircraft.txt",
        "pilots.txt", "groundstaff.txt", "passengers.txt", "crews.txt",
        "flights.txt", "queues.txt", "users.txt", "tickets.txt",
        "simulation_time.txt"
    };
    for (const char* f : txtFiles) {
        std::string src = dir + "/" + f;
        std::string dst = src + ".bak";
        std::error_code ec;
        if (fs::exists(src, ec)) {
            fs::rename(src, dst, ec);
            // Nếu rename lỗi (file đang mở, permission...), vẫn tiếp tục.
        }
    }

    return OpResult::success("Đã chuyển đổi dữ liệu từ .txt sang SQLite.");
}

// =================================== saveAll ===============================
OpResult AirportSystem::saveAll(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) return OpResult::failure("Không tạo được thư mục dữ liệu: " + dir);
    return saveToDatabase(dir);
}

// =================================== loadAll ================================
OpResult AirportSystem::loadAll(const std::string& dir) {
    std::string dbPath = dir + "/skygate.db";
    bool dbExists = fs::exists(dbPath);
    bool txtExists = fs::exists(dir + "/airports.txt");

    if (!dbExists && !txtExists) {
        return OpResult::failure("Thư mục dữ liệu không tồn tại hoặc trống: " + dir);
    }

    // Di trú một lần: .txt -> .db
    if (!dbExists && txtExists) {
        // Xoá trạng thái trước khi đọc .txt.
        airports_.clear(); aircrafts_.clear(); pilots_.clear(); ground_.clear();
        passengers_.clear(); crews_.clear(); flights_.clear();
        users_.clear(); tickets_.clear(); ticketCounter_ = 0;
        departureQueue_.clear(); arrivalQueue_.clear();
        eventLog_.clear();
        simulationTime_ = DateTime();

        OpResult migrateResult = migrateFromTxt(dir);
        if (!migrateResult.ok) return migrateResult;
        // migrateFromTxt đã gọi saveToDatabase và đổi tên file.
        // Đóng db để loadFromDatabase mở lại sạch.
        db_.close();
    }

    return loadFromDatabase(dir);
}

void AirportSystem::rebuildGateReservations() {
    for (auto& ap : airports_)
        for (auto& g : ap->gates()) g.clearReservations();

    for (auto& f : flights_) {
        if (f->gateCode().empty() || f->isFinished()) continue;
        Airport* origin = findAirport(f->originCode());
        if (!origin) continue;
        Gate* g = origin->findGate(f->gateCode());
        if (!g) continue;
        DateTime s, e;
        gateWindow(*f, s, e);
        g->reserve(f->code(), s, e);  // bỏ qua nếu trùng (dữ liệu đã nhất quán)
    }
}

// ===========================================================================
//  Dữ liệu mẫu & thống kê
// ===========================================================================
void AirportSystem::seedDemoData() {
    // Khởi tạo đồng hồ mô phỏng: 06:00 — trước các chuyến mẫu để có thể tua tới
    // và quan sát toàn bộ vòng đời + hành trình hành khách.
    simulationTime_ = DateTime(2026, 6, 10, 6, 0);

    // --- 1 sân bay nhà (mô hình quản lý 1 sân bay) ---
    addAirport("SKG", "San bay Quoc te SkyGate", "San bay nha — trung tam khai thac");
    addRunway("SKG", "SKG-RW1", 3600);
    addRunway("SKG", "SKG-RW2", 3600);
    // Mô hình 1 sân bay nhà chỉ còn 1 cổng ra máy bay.
    addGate("SKG", "SKG-G1", GateType::DoubleJetBridge);

    // --- Đội máy bay khai thác tại SKG (sức chứa ≤ 100 ghế, bội số 6 cho sơ đồ ghế) ---
    addAircraft(AircraftCategory::WideBody, "VN-SKY900", "SkyCruiser-900", 96);
    addAircraft(AircraftCategory::WideBody, "VN-HRZ350", "Horizon-350", 90);
    addAircraft(AircraftCategory::NarrowBody, "VN-AERO321", "AeroSwift-321", 84);
    addAircraft(AircraftCategory::NarrowBody, "VN-CLD200", "CloudJet-200", 60);
    addAircraft(AircraftCategory::Turboprop, "VN-TER72", "TerraProp-72", 72);

    // --- Hành khách mẫu (đã đặt vé sẵn ở các chuyến bên dưới) ---
    addPassenger("PX001", "Hoang Van Hung", 30, "0980000001", "P0001");
    setPassengerBaggage("PX001", 1, 18.0);
    addPassenger("PX002", "Ngo Thi Lan", 25, "0980000002", "P0002");
    setPassengerBaggage("PX002", 3, 45.0);  // vượt mức -> cảnh báo
    addPassenger("PX003", "Dang Van Minh", 40, "0980000003", "P0003");
    setPassengerBaggage("PX003", 2, 40.0);
    addPassenger("PX004", "Tran Thi Mai", 28, "0980000004", "P0004");
    setPassengerBaggage("PX004", 1, 12.0);

    // --- Chuyến bay mẫu: đều KHỞI HÀNH từ SKG tới các thành phố đích ---
    createFlight("SG100", "SKG", "TP. Ho Chi Minh", "2026-06-10 08:00", "2026-06-10 09:30");
    assignAircraft("SG100", "VN-AERO321");
    assignGate("SG100", "");  // tự động
    bookPassenger("SG100", "PX001");
    bookPassenger("SG100", "PX002");

    createFlight("SG200", "SKG", "Da Nang", "2026-06-10 10:00", "2026-06-10 11:00");
    assignAircraft("SG200", "VN-TER72");
    assignGate("SG200", "");
    bookPassenger("SG200", "PX003");

    createFlight("SG300", "SKG", "Phu Quoc", "2026-06-10 13:00", "2026-06-10 15:00");
    assignAircraft("SG300", "VN-SKY900");
    assignGate("SG300", "");
    bookPassenger("SG300", "PX004");

    // --- Tài khoản đăng nhập mẫu (3 phân hệ) ---
    seedDefaultAccounts();
}

void AirportSystem::seedDefaultAccounts() {
    if (!findUser("admin"))    addUser("Admin", "admin", "admin123", "Quan tri vien");
    if (!findUser("staff"))    addUser("Staff", "staff", "staff123", "Nhan vien quay ve");
    if (!findUser("customer")) addUser("Customer", "customer", "cus123", "Khach hang");
}

std::string AirportSystem::summary() const {
    std::ostringstream o;
    o << "Sân bay: " << airports_.size()
      << " | Máy bay: " << aircrafts_.size()
      << " | Hành khách: " << passengers_.size()
      << " | Chuyến bay: " << flights_.size();
    return o.str();
}

}  // namespace skygate
