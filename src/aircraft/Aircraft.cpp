#include "Aircraft.h"

namespace skygate {

Aircraft::Aircraft(std::string registration, std::string model, int capacity)
    : registration_(std::move(registration)), model_(std::move(model)),
      capacity_(capacity > 0 ? capacity : 0) {}

int Aircraft::gateRank(GateType type) {
    switch (type) {
        case GateType::RemoteStand:     return 0;
        case GateType::SingleJetBridge: return 1;
        case GateType::DoubleJetBridge: return 2;
    }
    return 0;
}

std::string Aircraft::describe() const {
    return "[" + categoryName() + "] " + registration_ + " (" + model_ + ")" +
           " | sức chứa " + std::to_string(capacity_) +
           " | đường băng >= " + std::to_string(requiredRunwayLength()) + "m" +
           " | gate >= " + toString(preferredGateType()) +
           " | quay đầu " + std::to_string(minTurnaroundMinutes()) + " phút";
}

}  // namespace skygate
