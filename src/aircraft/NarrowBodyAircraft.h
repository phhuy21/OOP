#ifndef SKYGATE_NARROWBODYAIRCRAFT_H
#define SKYGATE_NARROWBODYAIRCRAFT_H

#include "Aircraft.h"

namespace skygate {

// Máy bay thân hẹp (AeroSwift-321 / CloudJet-200): 150-220 ghế, quay đầu 45'
// cần đường băng trung bình hoặc dài và gate ống lồng đơn.
class NarrowBodyAircraft : public Aircraft {
public:
    NarrowBodyAircraft() = default;
    NarrowBodyAircraft(std::string registration, std::string model, int capacity)
        : Aircraft(std::move(registration), std::move(model), capacity) {}

    AircraftCategory category() const override { return AircraftCategory::NarrowBody; }
    int requiredRunwayLength() const override { return 2500; }
    int minTurnaroundMinutes() const override { return 45; }
    GateType preferredGateType() const override { return GateType::SingleJetBridge; }
    int minGateRank() const override { return 1; }
};

}  // namespace skygate

#endif  // SKYGATE_NARROWBODYAIRCRAFT_H
