#ifndef SKYGATE_GROUNDSTAFF_H
#define SKYGATE_GROUNDSTAFF_H

#include <string>

#include "Staff.h"

namespace skygate {

// ---------------------------------------------------------------------------
// GroundStaff: nhân viên mặt đất / phục vụ. Kế thừa Staff.
// Có thêm bộ phận công tác (ví dụ: Check-in, Boarding, Baggage, Cabin).
// ---------------------------------------------------------------------------
class GroundStaff : public Staff {
public:
    GroundStaff() = default;
    GroundStaff(std::string id, std::string fullName, int age, std::string phone,
                std::string baseAirport, std::string department);

    StaffRole staffRole() const override { return StaffRole::GroundStaff; }

    const std::string& department() const { return department_; }
    void setDepartment(const std::string& d) { department_ = d; }

    std::string describe() const override;

private:
    std::string department_;
};

}  // namespace skygate

#endif  // SKYGATE_GROUNDSTAFF_H
