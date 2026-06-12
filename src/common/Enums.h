#ifndef SKYGATE_ENUMS_H
#define SKYGATE_ENUMS_H

#include <string>

namespace skygate {

// ---------------------------------------------------------------------------
// Trạng thái chuyến bay theo vòng đời mô tả trong requirements (mục 3.2).
// Scheduled -> Check-in -> Boarding -> Gate Closed -> Ready -> Takeoff
//   -> In Air -> Landed -> Turnaround -> Completed.
// Hai trạng thái sự cố: Delayed (hoãn) và Cancelled (hủy).
// ---------------------------------------------------------------------------
enum class FlightStatus {
    Scheduled,
    CheckIn,
    Boarding,
    GateClosed,
    Ready,
    Takeoff,
    InAir,
    Landed,
    Turnaround,
    Completed,
    Delayed,
    Cancelled
};

// Phân loại máy bay (mục 3.3). Quyết định yêu cầu đường băng / loại gate.
enum class AircraftCategory {
    WideBody,    // Thân rộng
    NarrowBody,  // Thân hẹp
    Turboprop    // Cánh quạt
};

// Loại gate (cổng ra máy bay). Gate đôi phục vụ thân rộng, gate đơn phục vụ
// thân hẹp, gate bãi ngoài (remote) phục vụ máy bay nhỏ / cánh quạt.
enum class GateType {
    DoubleJetBridge,  // Ống lồng đôi
    SingleJetBridge,  // Ống lồng đơn
    RemoteStand       // Bãi ngoài
};

// Vai trò nhân viên dùng cho tổ bay.
enum class StaffRole {
    Pilot,
    GroundStaff
};

// ---------------------------------------------------------------------------
// Chuyển đổi enum <-> chuỗi để hiển thị và lưu file.
// ---------------------------------------------------------------------------
std::string toString(FlightStatus status);
FlightStatus flightStatusFromString(const std::string& s);

std::string toString(AircraftCategory category);
AircraftCategory aircraftCategoryFromString(const std::string& s);

std::string toString(GateType type);
GateType gateTypeFromString(const std::string& s);

std::string toString(StaffRole role);
StaffRole staffRoleFromString(const std::string& s);

}  // namespace skygate

#endif  // SKYGATE_ENUMS_H
