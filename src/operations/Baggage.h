#ifndef SKYGATE_BAGGAGE_H
#define SKYGATE_BAGGAGE_H

#include <string>

namespace skygate {

// ---------------------------------------------------------------------------
// Baggage: hành lý ký gửi của một hành khách.
// Theo requirements (mục 3.4): nếu vượt mức tiêu chuẩn chỉ CẢNH BÁO, không
// xử lý thanh toán thật. Mức miễn cước mặc định: 23 kg / kiện, tối đa 2 kiện.
// ---------------------------------------------------------------------------
class Baggage {
public:
    Baggage() = default;
    Baggage(int pieces, double totalWeightKg);

    int pieces() const { return pieces_; }
    double totalWeightKg() const { return totalWeightKg_; }

    void setPieces(int pieces);
    void setTotalWeightKg(double kg);

    // Mức tiêu chuẩn cho phép (cấu hình tĩnh dùng chung).
    static int allowancePieces() { return kAllowancePieces; }
    static double allowanceWeightKg() { return kAllowanceWeightKg; }

    // Có vượt mức tiêu chuẩn không (theo số kiện hoặc theo khối lượng)?
    bool isOverweight() const;

    // Phần vượt mức (kg). Bằng 0 nếu không vượt.
    double excessWeightKg() const;

    std::string describe() const;

private:
    static constexpr int kAllowancePieces = 2;
    static constexpr double kAllowanceWeightKg = 46.0;  // 2 kiện x 23 kg

    int pieces_ = 0;
    double totalWeightKg_ = 0.0;
};

}  // namespace skygate

#endif  // SKYGATE_BAGGAGE_H
