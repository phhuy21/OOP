#ifndef SKYGATE_GATE_H
#define SKYGATE_GATE_H

#include <string>
#include <vector>

#include "../common/DateTime.h"
#include "../common/Enums.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Gate: cổng ra máy bay. Có mã, loại gate và DANH SÁCH khung giờ đã đặt.
// Hệ thống dùng các khung giờ này để chống xung đột: không cho hai chuyến bay
// dùng cùng một gate trong cùng thời điểm (mục 3.1).
// ---------------------------------------------------------------------------
class Gate {
public:
    struct Reservation {
        std::string flightCode;
        DateTime start;
        DateTime end;
    };

    Gate() = default;
    Gate(std::string code, GateType type) : code_(std::move(code)), type_(type) {}

    const std::string& code() const { return code_; }
    GateType type() const { return type_; }
    void setCode(const std::string& c) { code_ = c; }
    void setType(GateType t) { type_ = t; }

    int rank() const;  // hạng gate (0 remote, 1 single, 2 double)

    // Còn trống trong khoảng [start, end) không?
    bool isFreeDuring(const DateTime& start, const DateTime& end) const;

    // Đặt chỗ cho một chuyến bay. Trả về false nếu trùng giờ.
    bool reserve(const std::string& flightCode, const DateTime& start, const DateTime& end);

    // Huỷ mọi khung giờ của một chuyến bay (khi huỷ chuyến / đổi gate).
    void releaseFlight(const std::string& flightCode);

    const std::vector<Reservation>& reservations() const { return reservations_; }
    void clearReservations() { reservations_.clear(); }

    std::string describe() const;

private:
    std::string code_;
    GateType type_ = GateType::RemoteStand;
    std::vector<Reservation> reservations_;
};

}  // namespace skygate

#endif  // SKYGATE_GATE_H
