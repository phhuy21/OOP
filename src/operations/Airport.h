#ifndef SKYGATE_AIRPORT_H
#define SKYGATE_AIRPORT_H

#include <string>
#include <vector>

#include "Gate.h"
#include "Runway.h"
#include "../common/DateTime.h"

namespace skygate {

class Aircraft;  // forward declaration

// ---------------------------------------------------------------------------
// Airport: sân bay giả lập trong mạng SkyGate (mục 3.1).
// THÀNH PHẦN: một Airport "có nhiều" Runway và Gate.
// ---------------------------------------------------------------------------
class Airport {
public:
    Airport() = default;
    Airport(std::string code, std::string name, std::string note = "");

    const std::string& code() const { return code_; }
    const std::string& name() const { return name_; }
    const std::string& note() const { return note_; }
    void setCode(const std::string& c) { code_ = c; }
    void setName(const std::string& n) { name_ = n; }
    void setNote(const std::string& n) { note_ = n; }

    // --- Đường băng ---
    void addRunway(const Runway& r) { runways_.push_back(r); }
    // Xoá đường băng theo mã; trả về true nếu có xoá.
    bool removeRunway(const std::string& runwayCode) {
        for (size_t i = 0; i < runways_.size(); ++i) {
            if (runways_[i].code() == runwayCode) {
                runways_.erase(runways_.begin() + static_cast<long>(i));
                return true;
            }
        }
        return false;
    }
    std::vector<Runway>& runways() { return runways_; }
    const std::vector<Runway>& runways() const { return runways_; }
    int longestRunway() const;  // chiều dài đường băng dài nhất (mét)
    // Có đường băng nào đủ dài cho máy bay này không?
    bool hasRunwayFor(const Aircraft& aircraft) const;

    // --- Gate ---
    void addGate(const Gate& g) { gates_.push_back(g); }
    std::vector<Gate>& gates() { return gates_; }
    const std::vector<Gate>& gates() const { return gates_; }
    Gate* findGate(const std::string& gateCode);

    // Tìm một gate trống & phù hợp loại máy bay trong khoảng thời gian cho trước.
    // Trả về con trỏ tới gate (nullptr nếu không có). Chưa đặt chỗ.
    Gate* findAvailableGate(const Aircraft& aircraft,
                            const DateTime& start, const DateTime& end);

    std::string describe() const;

private:
    std::string code_;
    std::string name_;
    std::string note_;
    std::vector<Runway> runways_;
    std::vector<Gate> gates_;
};

}  // namespace skygate

#endif  // SKYGATE_AIRPORT_H
