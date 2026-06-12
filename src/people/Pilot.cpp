#include "Pilot.h"

#include <cstdio>

namespace skygate {

Pilot::Pilot(std::string id, std::string fullName, int age, std::string phone,
             std::string baseAirport)
    : Staff(std::move(id), std::move(fullName), age, std::move(phone),
            std::move(baseAirport)) {}

bool Pilot::hasEnoughRestBefore(const DateTime& nextDeparture) const {
    if (!lastFlightEnd_.isValid()) return true;   // chưa bay chuyến nào
    if (!nextDeparture.isValid()) return false;
    long long rest = lastFlightEnd_.minutesUntil(nextDeparture);
    return rest >= kMinRestMinutes;
}

std::string Pilot::certificationList() const {
    std::string out;
    bool first = true;
    for (AircraftCategory c : certifications_) {
        if (!first) out += ";";
        out += toString(c);
        first = false;
    }
    return out;
}

std::string Pilot::describe() const {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.1f", monthlyFlightHours_);
    std::string s = "[Pilot] " + id_ + " - " + fullName_ +
                    " | base " + baseAirport_ +
                    " | chứng chỉ: " + (certificationList().empty() ? "(không)" : certificationList()) +
                    " | giờ bay tháng: " + buf + "/100";
    s += " | nghỉ từ: " + lastFlightEnd_.toString();
    return s;
}

}  // namespace skygate
