#include "Staff.h"

namespace skygate {

Staff::Staff(std::string id, std::string fullName, int age, std::string phone,
             std::string baseAirport)
    : Person(std::move(id), std::move(fullName), age, std::move(phone)),
      baseAirport_(std::move(baseAirport)) {}

}  // namespace skygate
