#ifndef SKYGATE_AIRCRAFT_H
#define SKYGATE_AIRCRAFT_H

#include <string>

#include "../common/Enums.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Aircraft: lớp cha trừu tượng cho máy bay (mục 3.3).
//   Aircraft -> WideBodyAircraft, NarrowBodyAircraft, TurbopropAircraft
//
// ĐA HÌNH: mỗi loại máy bay tự định nghĩa yêu cầu đường băng, loại gate phù
// hợp và thời gian quay đầu tối thiểu thông qua các hàm ảo thuần.
//
// Quy tắc "máy bay lớn không dùng gate nhỏ" được mô hình hoá bằng "hạng gate":
//   RemoteStand = 0, SingleJetBridge = 1, DoubleJetBridge = 2.
// Máy bay chỉ dùng được gate có hạng >= hạng tối thiểu của nó.
// ---------------------------------------------------------------------------
class Aircraft {
public:
    Aircraft() = default;
    Aircraft(std::string registration, std::string model, int capacity);
    virtual ~Aircraft() = default;

    // --- Thuộc tính chung ---
    const std::string& registration() const { return registration_; }
    const std::string& model() const { return model_; }
    int capacity() const { return capacity_; }

    void setRegistration(const std::string& r) { registration_ = r; }
    void setModel(const std::string& m) { model_ = m; }
    void setCapacity(int c) { if (c > 0) capacity_ = c; }

    // --- Giao diện đa hình (mỗi lớp con override) ---
    virtual AircraftCategory category() const = 0;
    virtual int requiredRunwayLength() const = 0;   // mét
    virtual int minTurnaroundMinutes() const = 0;   // phút quay đầu tối thiểu
    virtual GateType preferredGateType() const = 0; // loại gate phù hợp nhất

    // Hạng gate tối thiểu mà loại máy bay này cần.
    virtual int minGateRank() const = 0;

    // --- Kiểm tra hạ tầng (dùng chung, dựa trên hàm ảo) ---
    bool canUseRunwayLength(int runwayLength) const {
        return runwayLength >= requiredRunwayLength();
    }
    bool canUseGateType(GateType type) const {
        return gateRank(type) >= minGateRank();
    }

    // Hạng của một loại gate (tiện ích tĩnh).
    static int gateRank(GateType type);

    std::string categoryName() const { return toString(category()); }
    virtual std::string describe() const;

protected:
    std::string registration_;  // số hiệu đăng ký (tail number) - định danh
    std::string model_;         // model thương mại, ví dụ "SkyCruiser-900"
    int capacity_ = 0;          // số ghế
};

}  // namespace skygate

#endif  // SKYGATE_AIRCRAFT_H
