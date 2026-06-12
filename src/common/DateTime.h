#ifndef SKYGATE_DATETIME_H
#define SKYGATE_DATETIME_H

#include <string>

namespace skygate {

// ---------------------------------------------------------------------------
// DateTime: thời điểm đơn giản theo lịch (không xét múi giờ, không xét giây).
// Định dạng chuẩn dùng trong toàn hệ thống: "YYYY-MM-DD HH:MM".
// Nội bộ quy đổi về tổng số phút kể từ mốc 0000-01-01 00:00 để dễ so sánh,
// cộng/trừ và tính khoảng cách giữa hai mốc thời gian.
// ---------------------------------------------------------------------------
class DateTime {
public:
    DateTime();  // Mốc rỗng / không hợp lệ (totalMinutes = -1).
    DateTime(int year, int month, int day, int hour, int minute);

    // Phân tích chuỗi "YYYY-MM-DD HH:MM". Trả về DateTime rỗng nếu sai định dạng.
    static DateTime parse(const std::string& text);

    bool isValid() const { return totalMinutes_ >= 0; }

    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }

    // Tổng số phút tuyệt đối (để so sánh / tính toán).
    long long totalMinutes() const { return totalMinutes_; }

    // Khoảng cách (phút) từ this đến other = other - this. Dương nếu other muộn hơn.
    long long minutesUntil(const DateTime& other) const;

    // Trả về một DateTime mới = this + số phút cho trước.
    DateTime addMinutes(long long minutes) const;

    // Cùng tháng/năm? (dùng cho giới hạn giờ bay theo tháng).
    bool sameMonth(const DateTime& other) const;

    std::string toString() const;  // "YYYY-MM-DD HH:MM"

    bool operator<(const DateTime& other) const { return totalMinutes_ < other.totalMinutes_; }
    bool operator<=(const DateTime& other) const { return totalMinutes_ <= other.totalMinutes_; }
    bool operator>(const DateTime& other) const { return totalMinutes_ > other.totalMinutes_; }
    bool operator>=(const DateTime& other) const { return totalMinutes_ >= other.totalMinutes_; }
    bool operator==(const DateTime& other) const { return totalMinutes_ == other.totalMinutes_; }

    // Hai khoảng [aStart,aEnd) và [bStart,bEnd) có giao nhau không?
    static bool overlaps(const DateTime& aStart, const DateTime& aEnd,
                         const DateTime& bStart, const DateTime& bEnd);

private:
    void recompute();  // Tính lại totalMinutes_ từ các trường lịch.

    int year_ = 0;
    int month_ = 0;
    int day_ = 0;
    int hour_ = 0;
    int minute_ = 0;
    long long totalMinutes_ = -1;
};

}  // namespace skygate

#endif  // SKYGATE_DATETIME_H
