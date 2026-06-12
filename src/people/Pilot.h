#ifndef SKYGATE_PILOT_H
#define SKYGATE_PILOT_H

#include <set>
#include <string>

#include "Staff.h"
#include "../common/DateTime.h"
#include "../common/Enums.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Pilot: phi công. Kế thừa Staff.
// Lưu các quy tắc kiểm tra (mục requirements):
//   - Chứng chỉ phải phù hợp loại máy bay được gán.
//   - Tổng giờ bay trong tháng không vượt quá 100 giờ.
//   - Thời gian nghỉ giữa hai chuyến tối thiểu 8 tiếng (480 phút).
// ---------------------------------------------------------------------------
class Pilot : public Staff {
public:
    static constexpr double kMaxMonthlyHours = 100.0;
    static constexpr long long kMinRestMinutes = 8 * 60;  // 8 tiếng

    Pilot() = default;
    Pilot(std::string id, std::string fullName, int age, std::string phone,
          std::string baseAirport);

    StaffRole staffRole() const override { return StaffRole::Pilot; }

    // --- Chứng chỉ máy bay ---
    void addCertification(AircraftCategory cat) { certifications_.insert(cat); }
    void clearCertifications() { certifications_.clear(); }
    bool isCertifiedFor(AircraftCategory cat) const {
        return certifications_.count(cat) > 0;
    }
    const std::set<AircraftCategory>& certifications() const { return certifications_; }

    // --- Giờ bay trong tháng ---
    double monthlyFlightHours() const { return monthlyFlightHours_; }
    void setMonthlyFlightHours(double h) { monthlyFlightHours_ = h < 0 ? 0 : h; }
    void addFlightHours(double h) { if (h > 0) monthlyFlightHours_ += h; }
    // Có thể nhận thêm số giờ này mà vẫn <= 100 giờ?
    bool canTakeAdditionalHours(double hours) const {
        return monthlyFlightHours_ + hours <= kMaxMonthlyHours;
    }

    // --- Thời gian nghỉ ---
    const DateTime& lastFlightEnd() const { return lastFlightEnd_; }
    void setLastFlightEnd(const DateTime& dt) { lastFlightEnd_ = dt; }
    // Đã nghỉ đủ 8 tiếng trước thời điểm cất cánh kế tiếp chưa?
    bool hasEnoughRestBefore(const DateTime& nextDeparture) const;

    std::string certificationList() const;  // ví dụ "WideBody;NarrowBody"
    std::string describe() const override;

private:
    std::set<AircraftCategory> certifications_;
    double monthlyFlightHours_ = 0.0;
    DateTime lastFlightEnd_;  // thời điểm kết thúc chuyến gần nhất
};

}  // namespace skygate

#endif  // SKYGATE_PILOT_H
