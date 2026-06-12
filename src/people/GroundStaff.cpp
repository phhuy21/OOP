#include "GroundStaff.h"

namespace skygate {

GroundStaff::GroundStaff(std::string id, std::string fullName, int age, std::string phone,
                         std::string baseAirport, std::string department)
    : Staff(std::move(id), std::move(fullName), age, std::move(phone),
            std::move(baseAirport)),
      department_(std::move(department)) {}

std::string GroundStaff::describe() const {
    return "[GroundStaff] " + id_ + " - " + fullName_ +
           " | base " + baseAirport_ +
           " | bộ phận: " + (department_.empty() ? "(chưa rõ)" : department_);
}

}  // namespace skygate
