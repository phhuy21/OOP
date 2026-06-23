#ifndef SKYGATE_WIDEBODYAIRCRAFT_H
#define SKYGATE_WIDEBODYAIRCRAFT_H

#include "Aircraft.h"

namespace skygate {

// Máy bay thân rộng (SkyCruiser-900 / Horizon-350): 300-400 ghế, quay đầu 90'
// cần đường băng dài và gate ống lồng đôi.
class WideBodyAircraft : public Aircraft {
public:
    WideBodyAircraft() = default;
    WideBodyAircraft(std::string registration, std::string model, int capacity)
        : Aircraft(std::move(registration), std::move(model), capacity) {}

    AircraftCategory category() const override { return AircraftCategory::WideBody; }
    int requiredRunwayLength() const override { return 2800; }
    int minTurnaroundMinutes() const override { return 0; }
    GateType preferredGateType() const override { return GateType::DoubleJetBridge; }
    int minGateRank() const override { return 2; }
};

}  // namespace skygate

#endif  // SKYGATE_WIDEBODYAIRCRAFT_H
