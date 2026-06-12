#ifndef SKYGATE_FLIGHT_H
#define SKYGATE_FLIGHT_H

#include <memory>
#include <string>
#include <vector>

#include "Crew.h"
#include "../aircraft/Aircraft.h"
#include "../common/DateTime.h"
#include "../common/Enums.h"
#include "../common/OpResult.h"
#include "../people/Passenger.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Flight: chuyến bay (mục 3.2). Đây là lớp KẾT HỢP/THÀNH PHẦN trung tâm:
// một Flight tham chiếu Aircraft, Crew, sân bay đi/đến, Gate và danh sách
// Passenger. Lớp này cũng quản lý MÁY TRẠNG THÁI vòng đời chuyến bay.
// ---------------------------------------------------------------------------
class Flight {
public:
    Flight() = default;
    Flight(std::string code, std::string originCode, std::string destCode,
           DateTime departure, DateTime arrival);

    // --- Thuộc tính cơ bản ---
    const std::string& code() const { return code_; }
    const std::string& originCode() const { return originCode_; }
    const std::string& destCode() const { return destCode_; }
    const DateTime& departure() const { return departure_; }
    const DateTime& arrival() const { return arrival_; }
    void setCode(const std::string& c) { code_ = c; }
    void setOriginCode(const std::string& c) { originCode_ = c; }
    void setDestCode(const std::string& c) { destCode_ = c; }
    void setDeparture(const DateTime& d) { departure_ = d; }
    void setArrival(const DateTime& a) { arrival_ = a; }

    FlightStatus status() const { return status_; }
    void setStatus(FlightStatus s) { status_ = s; }

    bool emergency() const { return emergency_; }
    void setEmergency(bool v) { emergency_ = v; }

    const std::string& note() const { return note_; }
    void setNote(const std::string& n) { note_ = n; }

    // --- Tài nguyên gán cho chuyến (composition) ---
    void setAircraft(const std::shared_ptr<Aircraft>& a) { aircraft_ = a; }
    std::shared_ptr<Aircraft> aircraft() const { return aircraft_; }

    void setCrew(const std::shared_ptr<Crew>& c) { crew_ = c; }
    std::shared_ptr<Crew> crew() const { return crew_; }

    const std::string& gateCode() const { return gateCode_; }
    void setGateCode(const std::string& g) { gateCode_ = g; }

    // --- Hành khách ---
    void addPassenger(const std::shared_ptr<Passenger>& p);
    void removePassenger(const std::string& passengerId);
    const std::vector<std::shared_ptr<Passenger>>& passengers() const { return passengers_; }
    int checkedInCount() const;
    int boardedCount() const;

    // --- Tính toán ---
    double flightHours() const;  // thời lượng bay (giờ), 0 nếu thời gian không hợp lệ

    // --- Máy trạng thái ---
    // Chuyển sang trạng thái kế tiếp trong vòng đời bình thường, có kiểm tra
    // điều kiện (ví dụ muốn tới Ready phải có máy bay + tổ bay hợp lệ + gate).
    // `crewIssues` (nếu truyền) là kết quả kiểm tra tổ bay đã thực hiện ở ngoài.
    OpResult advance(const std::vector<std::string>& crewIssues);

    OpResult checkInPassenger(const std::string& passengerId, const std::string& seat);
    OpResult boardPassenger(const std::string& passengerId);

    OpResult delay(long long minutes, const std::string& reason);
    OpResult cancel(const std::string& reason);

    bool isGateOpen() const;  // hành khách còn được lên máy bay không?
    bool isFinished() const;  // Completed hoặc Cancelled

    std::string describe() const;
    std::string statusName() const { return toString(status_); }

private:
    Passenger* findPassenger(const std::string& passengerId);
    static FlightStatus nextStatus(FlightStatus s);

    std::string code_;
    std::string originCode_;
    std::string destCode_;
    DateTime departure_;
    DateTime arrival_;
    FlightStatus status_ = FlightStatus::Scheduled;
    bool emergency_ = false;
    std::string note_;

    std::shared_ptr<Aircraft> aircraft_;
    std::shared_ptr<Crew> crew_;
    std::string gateCode_;  // gate tại sân bay đi

    std::vector<std::shared_ptr<Passenger>> passengers_;
};

}  // namespace skygate

#endif  // SKYGATE_FLIGHT_H
