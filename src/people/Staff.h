#ifndef SKYGATE_STAFF_H
#define SKYGATE_STAFF_H

#include <string>

#include "Person.h"
#include "../common/Enums.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Staff: lớp cha trừu tượng cho nhân viên sân bay. Kế thừa Person.
//   Staff -> Pilot, GroundStaff
// Thêm thuộc tính sân bay cơ sở (base) và vai trò nhân viên.
// ---------------------------------------------------------------------------
class Staff : public Person {
public:
    Staff() = default;
    Staff(std::string id, std::string fullName, int age, std::string phone,
          std::string baseAirport);

    const std::string& baseAirport() const { return baseAirport_; }
    void setBaseAirport(const std::string& code) { baseAirport_ = code; }

    // Vai trò nhân viên cụ thể (Pilot / GroundStaff). Hàm ảo thuần.
    virtual StaffRole staffRole() const = 0;

    std::string role() const override { return toString(staffRole()); }

protected:
    std::string baseAirport_;  // mã sân bay cơ sở
};

}  // namespace skygate

#endif  // SKYGATE_STAFF_H
