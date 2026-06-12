#ifndef SKYGATE_RUNWAY_H
#define SKYGATE_RUNWAY_H

#include <string>

namespace skygate {

// ---------------------------------------------------------------------------
// Runway: đường băng của một sân bay. Đặc trưng bởi mã và chiều dài (mét).
// Chiều dài quyết định loại máy bay được phép cất/hạ cánh.
// ---------------------------------------------------------------------------
class Runway {
public:
    Runway() = default;
    Runway(std::string code, int lengthMeters)
        : code_(std::move(code)), lengthMeters_(lengthMeters) {}

    const std::string& code() const { return code_; }
    int lengthMeters() const { return lengthMeters_; }

    void setCode(const std::string& c) { code_ = c; }
    void setLengthMeters(int m) { if (m > 0) lengthMeters_ = m; }

    std::string describe() const {
        return code_ + " (" + std::to_string(lengthMeters_) + "m)";
    }

private:
    std::string code_;
    int lengthMeters_ = 0;
};

}  // namespace skygate

#endif  // SKYGATE_RUNWAY_H
