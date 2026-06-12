#ifndef SKYGATE_CREW_H
#define SKYGATE_CREW_H

#include <memory>
#include <string>
#include <vector>

#include "../common/DateTime.h"
#include "../common/Enums.h"
#include "../people/GroundStaff.h"
#include "../people/Pilot.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Crew: tổ bay = phi công + nhân viên phục vụ (mục 3.3).
// THÀNH PHẦN: Crew "có nhiều" Pilot và GroundStaff (giữ qua shared_ptr).
// Cung cấp hàm kiểm tra điều kiện gán tổ bay cho một chuyến bay:
//   - có ít nhất 1 phi công;
//   - mọi phi công có chứng chỉ phù hợp loại máy bay;
//   - không phi công nào vượt 100 giờ bay/tháng khi cộng thêm chuyến này;
//   - mọi phi công đã nghỉ tối thiểu 8 tiếng trước giờ cất cánh.
// ---------------------------------------------------------------------------
class Crew {
public:
    Crew() = default;
    explicit Crew(std::string id) : id_(std::move(id)) {}

    const std::string& id() const { return id_; }
    void setId(const std::string& id) { id_ = id; }

    void addPilot(const std::shared_ptr<Pilot>& p) { if (p) pilots_.push_back(p); }
    void addGroundStaff(const std::shared_ptr<GroundStaff>& g) { if (g) ground_.push_back(g); }

    const std::vector<std::shared_ptr<Pilot>>& pilots() const { return pilots_; }
    const std::vector<std::shared_ptr<GroundStaff>>& groundStaff() const { return ground_; }

    void clear() { pilots_.clear(); ground_.clear(); }

    // Trả về danh sách lỗi (rỗng = hợp lệ) khi gán tổ bay cho một chuyến bay
    // dùng máy bay loại `category`, cất cánh lúc `departure`, kéo dài `flightHours`.
    std::vector<std::string> validateForFlight(AircraftCategory category,
                                               const DateTime& departure,
                                               double flightHours) const;

    // Sau khi chuyến bay hoàn tất: cộng giờ bay và cập nhật mốc nghỉ cho phi công.
    void applyFlightCompletion(double flightHours, const DateTime& arrival);

    std::string describe() const;

private:
    std::string id_;
    std::vector<std::shared_ptr<Pilot>> pilots_;
    std::vector<std::shared_ptr<GroundStaff>> ground_;
};

}  // namespace skygate

#endif  // SKYGATE_CREW_H
