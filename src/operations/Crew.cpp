#include "Crew.h"

#include <cstdio>

namespace skygate {

std::vector<std::string> Crew::validateForFlight(AircraftCategory category,
                                                 const DateTime& departure,
                                                 double flightHours) const {
    std::vector<std::string> issues;

    if (pilots_.empty()) {
        issues.push_back("Tổ bay không có phi công nào.");
    }

    for (const auto& p : pilots_) {
        const std::string who = p->id() + " (" + p->fullName() + ")";

        if (!p->isCertifiedFor(category)) {
            issues.push_back("Phi công " + who + " không có chứng chỉ cho loại máy bay " +
                             toString(category) + ".");
        }
        if (!p->canTakeAdditionalHours(flightHours)) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          "Phi công %s vượt giới hạn giờ bay tháng (%.1f + %.1f > 100).",
                          who.c_str(), p->monthlyFlightHours(), flightHours);
            issues.push_back(buf);
        }
        if (!p->hasEnoughRestBefore(departure)) {
            issues.push_back("Phi công " + who + " chưa nghỉ đủ 8 tiếng trước giờ cất cánh.");
        }
    }
    return issues;
}

void Crew::applyFlightCompletion(double flightHours, const DateTime& arrival) {
    for (auto& p : pilots_) {
        p->addFlightHours(flightHours);
        p->setLastFlightEnd(arrival);
    }
}

std::string Crew::describe() const {
    std::string s = "Tổ bay " + id_ + ": " + std::to_string(pilots_.size()) +
                    " phi công, " + std::to_string(ground_.size()) + " NV mặt đất";
    if (!pilots_.empty()) {
        s += " [";
        for (size_t i = 0; i < pilots_.size(); ++i) {
            if (i) s += ", ";
            s += pilots_[i]->id();
        }
        s += "]";
    }
    return s;
}

}  // namespace skygate
