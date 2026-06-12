#include "Gate.h"

#include "../aircraft/Aircraft.h"

namespace skygate {

int Gate::rank() const {
    return Aircraft::gateRank(type_);
}

bool Gate::isFreeDuring(const DateTime& start, const DateTime& end) const {
    if (!start.isValid() || !end.isValid()) return false;
    for (const auto& r : reservations_) {
        if (DateTime::overlaps(start, end, r.start, r.end)) {
            return false;
        }
    }
    return true;
}

bool Gate::reserve(const std::string& flightCode, const DateTime& start, const DateTime& end) {
    if (!isFreeDuring(start, end)) return false;
    reservations_.push_back(Reservation{flightCode, start, end});
    return true;
}

void Gate::releaseFlight(const std::string& flightCode) {
    for (size_t i = 0; i < reservations_.size();) {
        if (reservations_[i].flightCode == flightCode) {
            reservations_.erase(reservations_.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
}

std::string Gate::describe() const {
    return code_ + " [" + toString(type_) + "] - " +
           std::to_string(reservations_.size()) + " lượt đặt";
}

}  // namespace skygate
