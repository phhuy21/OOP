#ifndef SKYGATE_TURBOPROPAIRCRAFT_H
#define SKYGATE_TURBOPROPAIRCRAFT_H

#include "Aircraft.h"

namespace skygate {

// Máy bay cánh quạt (TerraProp-72): 60-90 ghế, quay đầu 30', dùng được đường
// băng ngắn/trung bình/dài và gate bãi ngoài (remote stand).
class TurbopropAircraft : public Aircraft {
public:
    TurbopropAircraft() = default;
    TurbopropAircraft(std::string registration, std::string model, int capacity)
        : Aircraft(std::move(registration), std::move(model), capacity) {}

    AircraftCategory category() const override { return AircraftCategory::Turboprop; }
    int requiredRunwayLength() const override { return 1500; }
    int minTurnaroundMinutes() const override { return 0; }
    GateType preferredGateType() const override { return GateType::RemoteStand; }
    int minGateRank() const override { return 0; }
};

}  // namespace skygate

#endif  // SKYGATE_TURBOPROPAIRCRAFT_H
