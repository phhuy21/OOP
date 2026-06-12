#ifndef SKYGATE_OPRESULT_H
#define SKYGATE_OPRESULT_H

#include <string>

namespace skygate {

// Kết quả một thao tác nghiệp vụ: thành công/thất bại kèm thông điệp.
struct OpResult {
    bool ok = false;
    std::string message;

    static OpResult success(const std::string& msg = "Thành công.") {
        return OpResult{true, msg};
    }
    static OpResult failure(const std::string& msg) {
        return OpResult{false, msg};
    }

    explicit operator bool() const { return ok; }
};

}  // namespace skygate

#endif  // SKYGATE_OPRESULT_H
