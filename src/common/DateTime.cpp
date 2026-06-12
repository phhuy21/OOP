#include "DateTime.h"

#include <cstdio>
#include <sstream>

namespace skygate {

namespace {

// Số ngày kể từ lịch dân dụng (thuật toán Howard Hinnant). Chính xác cho mọi
// năm/tháng/ngày hợp lệ, xử lý đúng năm nhuận.
long long daysFromCivil(int y, int m, int d) {
    y -= (m <= 2);
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = static_cast<unsigned>((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

bool isValidCalendar(int y, int mo, int d, int h, int mi) {
    if (mo < 1 || mo > 12) return false;
    if (d < 1 || d > 31) return false;
    if (h < 0 || h > 23) return false;
    if (mi < 0 || mi > 59) return false;
    if (y < 1) return false;
    static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int dim = mdays[mo - 1];
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    if (mo == 2 && leap) dim = 29;
    return d <= dim;
}

}  // namespace

DateTime::DateTime() = default;

DateTime::DateTime(int year, int month, int day, int hour, int minute)
    : year_(year), month_(month), day_(day), hour_(hour), minute_(minute) {
    if (isValidCalendar(year, month, day, hour, minute)) {
        recompute();
    } else {
        totalMinutes_ = -1;
    }
}

void DateTime::recompute() {
    long long days = daysFromCivil(year_, month_, day_);
    totalMinutes_ = days * 1440LL + static_cast<long long>(hour_) * 60 + minute_;
}

DateTime DateTime::parse(const std::string& text) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    // Chấp nhận "YYYY-MM-DD HH:MM" và "YYYY-MM-DDTHH:MM".
    if (std::sscanf(text.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) == 5 ||
        std::sscanf(text.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) == 5) {
        if (isValidCalendar(y, mo, d, h, mi)) {
            return DateTime(y, mo, d, h, mi);
        }
    }
    return DateTime();  // không hợp lệ
}

long long DateTime::minutesUntil(const DateTime& other) const {
    return other.totalMinutes_ - totalMinutes_;
}

DateTime DateTime::addMinutes(long long minutes) const {
    if (!isValid()) return DateTime();
    long long total = totalMinutes_ + minutes;
    long long days = total / 1440;
    long long rem = total % 1440;
    if (rem < 0) { rem += 1440; days -= 1; }
    int h = static_cast<int>(rem / 60);
    int mi = static_cast<int>(rem % 60);

    // Quy đổi ngược số ngày -> y/m/d (đảo thuật toán civil).
    long long z = days + 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long y = static_cast<long long>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned dd = doy - (153 * mp + 2) / 5 + 1;
    const unsigned mm = mp < 10 ? mp + 3 : mp - 9;
    int yy = static_cast<int>(y + (mm <= 2));
    return DateTime(yy, static_cast<int>(mm), static_cast<int>(dd), h, mi);
}

bool DateTime::sameMonth(const DateTime& other) const {
    return year_ == other.year_ && month_ == other.month_;
}

std::string DateTime::toString() const {
    if (!isValid()) return "(chưa đặt)";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
                  year_, month_, day_, hour_, minute_);
    return std::string(buf);
}

bool DateTime::overlaps(const DateTime& aStart, const DateTime& aEnd,
                        const DateTime& bStart, const DateTime& bEnd) {
    if (!aStart.isValid() || !aEnd.isValid() || !bStart.isValid() || !bEnd.isValid()) {
        return false;
    }
    return aStart.totalMinutes() < bEnd.totalMinutes() &&
           bStart.totalMinutes() < aEnd.totalMinutes();
}

}  // namespace skygate
