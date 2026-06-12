#include "Enums.h"

namespace skygate {

std::string toString(FlightStatus status) {
    switch (status) {
        case FlightStatus::Scheduled:  return "Scheduled";
        case FlightStatus::CheckIn:    return "Check-in";
        case FlightStatus::Boarding:   return "Boarding";
        case FlightStatus::GateClosed: return "GateClosed";
        case FlightStatus::Ready:      return "Ready";
        case FlightStatus::Takeoff:    return "Takeoff";
        case FlightStatus::InAir:      return "InAir";
        case FlightStatus::Landed:     return "Landed";
        case FlightStatus::Turnaround: return "Turnaround";
        case FlightStatus::Completed:  return "Completed";
        case FlightStatus::Delayed:    return "Delayed";
        case FlightStatus::Cancelled:  return "Cancelled";
    }
    return "Unknown";
}

FlightStatus flightStatusFromString(const std::string& s) {
    if (s == "Scheduled")  return FlightStatus::Scheduled;
    if (s == "Check-in")   return FlightStatus::CheckIn;
    if (s == "Boarding")   return FlightStatus::Boarding;
    if (s == "GateClosed") return FlightStatus::GateClosed;
    if (s == "Ready")      return FlightStatus::Ready;
    if (s == "Takeoff")    return FlightStatus::Takeoff;
    if (s == "InAir")      return FlightStatus::InAir;
    if (s == "Landed")     return FlightStatus::Landed;
    if (s == "Turnaround") return FlightStatus::Turnaround;
    if (s == "Completed")  return FlightStatus::Completed;
    if (s == "Delayed")    return FlightStatus::Delayed;
    if (s == "Cancelled")  return FlightStatus::Cancelled;
    return FlightStatus::Scheduled;
}

std::string toString(AircraftCategory category) {
    switch (category) {
        case AircraftCategory::WideBody:   return "WideBody";
        case AircraftCategory::NarrowBody: return "NarrowBody";
        case AircraftCategory::Turboprop:  return "Turboprop";
    }
    return "Unknown";
}

AircraftCategory aircraftCategoryFromString(const std::string& s) {
    if (s == "WideBody")   return AircraftCategory::WideBody;
    if (s == "NarrowBody") return AircraftCategory::NarrowBody;
    if (s == "Turboprop")  return AircraftCategory::Turboprop;
    return AircraftCategory::NarrowBody;
}

std::string toString(GateType type) {
    switch (type) {
        case GateType::DoubleJetBridge: return "DoubleJetBridge";
        case GateType::SingleJetBridge: return "SingleJetBridge";
        case GateType::RemoteStand:     return "RemoteStand";
    }
    return "Unknown";
}

GateType gateTypeFromString(const std::string& s) {
    if (s == "DoubleJetBridge") return GateType::DoubleJetBridge;
    if (s == "SingleJetBridge") return GateType::SingleJetBridge;
    if (s == "RemoteStand")     return GateType::RemoteStand;
    return GateType::RemoteStand;
}

std::string toString(StaffRole role) {
    switch (role) {
        case StaffRole::Pilot:       return "Pilot";
        case StaffRole::GroundStaff: return "GroundStaff";
    }
    return "Unknown";
}

StaffRole staffRoleFromString(const std::string& s) {
    if (s == "Pilot") return StaffRole::Pilot;
    return StaffRole::GroundStaff;
}

}  // namespace skygate
