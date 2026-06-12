#ifndef SKYGATE_AIRCRAFTFACTORY_H
#define SKYGATE_AIRCRAFTFACTORY_H

#include <memory>
#include <string>

#include "Aircraft.h"
#include "../common/Enums.h"

namespace skygate {

// Tạo đối tượng máy bay con phù hợp theo phân loại. Tập trung logic khởi tạo
// đa hình tại một nơi để UI và phần lưu trữ dùng lại.
class AircraftFactory {
public:
    static std::shared_ptr<Aircraft> create(AircraftCategory category,
                                            const std::string& registration,
                                            const std::string& model,
                                            int capacity);
};

}  // namespace skygate

#endif  // SKYGATE_AIRCRAFTFACTORY_H
