#ifndef SKYGATE_AIRPORTSYSTEM_H
#define SKYGATE_AIRPORTSYSTEM_H

#include <memory>
#include <string>
#include <vector>

#include "../aircraft/Aircraft.h"
#include "../common/DateTime.h"
#include "../common/Enums.h"
#include "../common/OpResult.h"
#include "../operations/Airport.h"
#include "../operations/Crew.h"
#include "../operations/Flight.h"
#include "../people/GroundStaff.h"
#include "../people/Passenger.h"
#include "../people/Pilot.h"

namespace skygate {

// ---------------------------------------------------------------------------
// AirportSystem: lớp ĐIỀU KHIỂN trung tâm (mục "Điều khiển chương trình").
// Quản lý toàn bộ danh sách đối tượng, thực thi các quy tắc nghiệp vụ, điều
// phối hàng đợi cất/hạ cánh, mô phỏng thời tiết và lưu/đọc dữ liệu ra file.
// ---------------------------------------------------------------------------
// Một dòng nhật ký sự kiện (timeline / thông báo mô phỏng cho hành khách).
struct LogEntry {
    std::string time;  // simulationTime_ lúc ghi, dạng "YYYY-MM-DD HH:MM"
    std::string text;  // nội dung sự kiện
};

class AirportSystem {
public:
    AirportSystem() = default;

    // ===================== Sân bay / gate / đường băng =====================
    OpResult addAirport(const std::string& code, const std::string& name,
                        const std::string& note);
    OpResult addRunway(const std::string& airportCode, const std::string& runwayCode,
                       int lengthMeters);
    OpResult addGate(const std::string& airportCode, const std::string& gateCode,
                     GateType type);
    Airport* findAirport(const std::string& code);
    const std::vector<std::shared_ptr<Airport>>& airports() const { return airports_; }

    // =============================== Máy bay ===============================
    OpResult addAircraft(AircraftCategory category, const std::string& registration,
                         const std::string& model, int capacity);
    std::shared_ptr<Aircraft> findAircraft(const std::string& registration);
    const std::vector<std::shared_ptr<Aircraft>>& aircrafts() const { return aircrafts_; }

    // =============================== Con người =============================
    OpResult addPilot(const std::string& id, const std::string& name, int age,
                      const std::string& phone, const std::string& base);
    OpResult addPilotCertification(const std::string& pilotId, AircraftCategory cat);
    OpResult addGroundStaff(const std::string& id, const std::string& name, int age,
                            const std::string& phone, const std::string& base,
                            const std::string& department);
    OpResult addPassenger(const std::string& id, const std::string& name, int age,
                          const std::string& phone, const std::string& passport);
    OpResult setPassengerBaggage(const std::string& passengerId, int pieces, double weightKg);

    std::shared_ptr<Pilot> findPilot(const std::string& id);
    std::shared_ptr<GroundStaff> findGroundStaff(const std::string& id);
    std::shared_ptr<Passenger> findPassenger(const std::string& id);

    const std::vector<std::shared_ptr<Pilot>>& pilots() const { return pilots_; }
    const std::vector<std::shared_ptr<GroundStaff>>& groundStaff() const { return ground_; }
    const std::vector<std::shared_ptr<Passenger>>& passengers() const { return passengers_; }

    // ================================ Tổ bay ===============================
    OpResult createCrew(const std::string& crewId);
    OpResult addPilotToCrew(const std::string& crewId, const std::string& pilotId);
    OpResult addGroundToCrew(const std::string& crewId, const std::string& groundId);
    std::shared_ptr<Crew> findCrew(const std::string& id);
    const std::vector<std::shared_ptr<Crew>>& crews() const { return crews_; }

    // =============================== Chuyến bay ============================
    OpResult createFlight(const std::string& code, const std::string& origin,
                          const std::string& dest, const std::string& departure,
                          const std::string& arrival);
    OpResult assignAircraft(const std::string& flightCode, const std::string& registration);
    OpResult assignCrew(const std::string& flightCode, const std::string& crewId);
    // gateCode rỗng = tự động tìm gate phù hợp & còn trống.
    OpResult assignGate(const std::string& flightCode, const std::string& gateCode);
    OpResult bookPassenger(const std::string& flightCode, const std::string& passengerId);
    OpResult checkIn(const std::string& flightCode, const std::string& passengerId,
                     const std::string& seat);
    OpResult board(const std::string& flightCode, const std::string& passengerId);
    OpResult advanceFlight(const std::string& flightCode);
    OpResult delayFlight(const std::string& flightCode, long long minutes,
                         const std::string& reason);
    OpResult cancelFlight(const std::string& flightCode, const std::string& reason);
    std::shared_ptr<Flight> findFlight(const std::string& code);
    const std::vector<std::shared_ptr<Flight>>& flights() const { return flights_; }

    // Kiểm tra tổ bay cho một chuyến bay (rỗng = hợp lệ).
    std::vector<std::string> validateCrewForFlight(const Flight& flight) const;
    void setMarkEmergency(const std::string& flightCode, bool value);

    // ===================== Đồng hồ mô phỏng & nhật ký (mở rộng) ============
    const DateTime& currentTime() const { return simulationTime_; }
    const std::vector<LogEntry>& eventLog() const { return eventLog_; }
    void logEvent(const std::string& text);
    // Tua thời gian ảo `minutes` phút; tự động đẩy trạng thái chuyến theo lịch.
    OpResult tickSimulation(long long minutes);

    // ===================== Hàng đợi cất/hạ cánh (mở rộng) ==================
    OpResult enqueueDeparture(const std::string& flightCode);
    OpResult enqueueArrival(const std::string& flightCode);
    OpResult processNextDeparture();  // ưu tiên chuyến khẩn cấp
    OpResult processNextArrival();
    const std::vector<std::string>& departureQueue() const { return departureQueue_; }
    const std::vector<std::string>& arrivalQueue() const { return arrivalQueue_; }

    // ===================== Mô phỏng thời tiết xấu (mở rộng) ================
    // Hoãn (hoặc huỷ nếu cancel=true) các chuyến đi/đến sân bay trong khung giờ.
    // Trả về số chuyến bị ảnh hưởng.
    int simulateBadWeather(const std::string& airportCode, const std::string& start,
                           const std::string& end, bool cancel);

    // ============================ Lưu / đọc file ===========================
    OpResult saveAll(const std::string& dir);
    OpResult loadAll(const std::string& dir);

    // Nạp dữ liệu mẫu (4 sân bay, máy bay, phi công... theo requirements).
    void seedDemoData();

    // Thống kê nhanh.
    std::string summary() const;

private:
    // Cửa sổ chiếm gate của một chuyến (dựa trên thời gian quay đầu của máy bay).
    static void gateWindow(const Flight& flight, DateTime& outStart, DateTime& outEnd);
    int pickPriorityIndex(const std::vector<std::string>& queue) const;
    void rebuildGateReservations();

    // Trạng thái mục tiêu của chuyến theo thời gian ảo hiện tại (so với dep/arr).
    FlightStatus targetStatusFor(const Flight& f) const;
    static int lifecycleOrdinal(FlightStatus s);

    std::vector<std::shared_ptr<Airport>> airports_;
    std::vector<std::shared_ptr<Aircraft>> aircrafts_;
    std::vector<std::shared_ptr<Pilot>> pilots_;
    std::vector<std::shared_ptr<GroundStaff>> ground_;
    std::vector<std::shared_ptr<Passenger>> passengers_;
    std::vector<std::shared_ptr<Crew>> crews_;
    std::vector<std::shared_ptr<Flight>> flights_;

    std::vector<std::string> departureQueue_;  // mã chuyến chờ cất cánh
    std::vector<std::string> arrivalQueue_;     // mã chuyến chờ hạ cánh

    DateTime simulationTime_;          // "now" ảo; khởi tạo trong seedDemoData()
    std::vector<LogEntry> eventLog_;   // nhật ký; mới nhất ở cuối, render đảo ngược
};

}  // namespace skygate

#endif  // SKYGATE_AIRPORTSYSTEM_H
