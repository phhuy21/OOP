#include "Airport.h"

#include "../aircraft/Aircraft.h"

namespace skygate {

Airport::Airport(std::string code, std::string name, std::string note)
    : code_(std::move(code)), name_(std::move(name)), note_(std::move(note)) {}

int Airport::longestRunway() const {
    int best = 0;
    for (const auto& r : runways_) {
        if (r.lengthMeters() > best) best = r.lengthMeters();
    }
    return best;
}

bool Airport::hasRunwayFor(const Aircraft& aircraft) const {
    for (const auto& r : runways_) {
        if (aircraft.canUseRunwayLength(r.lengthMeters())) return true;
    }
    return false;
}

Gate* Airport::findGate(const std::string& gateCode) {
    for (auto& g : gates_) {
        if (g.code() == gateCode) return &g;
    }
    return nullptr;
}

Gate* Airport::findAvailableGate(const Aircraft& aircraft,
                                 const DateTime& start, const DateTime& end) {
    for (auto& g : gates_) {
        if (aircraft.canUseGateType(g.type()) && g.isFreeDuring(start, end)) {
            return &g;
        }
    }
    return nullptr;
}

std::string Airport::describe() const {
    std::string s = code_ + " - " + name_;
    s += " | đường băng: " + std::to_string(runways_.size()) +
         " (dài nhất " + std::to_string(longestRunway()) + "m)";
    s += " | gate: " + std::to_string(gates_.size());
    if (!note_.empty()) s += " | " + note_;
    return s;
}

}  // namespace skygate
