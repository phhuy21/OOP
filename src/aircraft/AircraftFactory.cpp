#include "AircraftFactory.h"

#include "NarrowBodyAircraft.h"
#include "TurbopropAircraft.h"
#include "WideBodyAircraft.h"

namespace skygate {

std::shared_ptr<Aircraft> AircraftFactory::create(AircraftCategory category,
                                                  const std::string& registration,
                                                  const std::string& model,
                                                  int capacity) {
    switch (category) {
        case AircraftCategory::WideBody:
            return std::make_shared<WideBodyAircraft>(registration, model, capacity);
        case AircraftCategory::NarrowBody:
            return std::make_shared<NarrowBodyAircraft>(registration, model, capacity);
        case AircraftCategory::Turboprop:
            return std::make_shared<TurbopropAircraft>(registration, model, capacity);
    }
    return std::make_shared<NarrowBodyAircraft>(registration, model, capacity);
}

}  // namespace skygate
