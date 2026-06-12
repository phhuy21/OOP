#ifndef SKYGATE_UTILS_H
#define SKYGATE_UTILS_H

#include <string>
#include <vector>

namespace skygate {
namespace utils {

// ---------------------------------------------------------------------------
// Tiện ích chuỗi dùng chung cho nhập liệu và đọc/ghi file.
// Định dạng lưu trữ của hệ thống là text theo dòng, các trường ngăn bởi '|'.
// Để tránh xung đột khi dữ liệu chứa ký tự '|' hoặc xuống dòng, ta escape.
// ---------------------------------------------------------------------------

// Tách chuỗi theo ký tự phân cách (không bỏ trường rỗng).
std::vector<std::string> split(const std::string& s, char delimiter);

// Nối các chuỗi bằng ký tự phân cách.
std::string join(const std::vector<std::string>& parts, char delimiter);

// Bỏ khoảng trắng đầu/cuối.
std::string trim(const std::string& s);

// Chuyển về chữ thường (ASCII).
std::string toLower(const std::string& s);

// Escape / unescape cho một trường khi ghi/đọc file (xử lý '|', '\\', newline).
std::string escapeField(const std::string& s);
std::string unescapeField(const std::string& s);

// Tách một dòng record thành các trường đã được unescape.
std::vector<std::string> parseRecord(const std::string& line);

// Gộp các trường (tự escape) thành một dòng record.
std::string buildRecord(const std::vector<std::string>& fields);

// Chuyển đổi số an toàn (trả về giá trị mặc định nếu lỗi).
int toInt(const std::string& s, int fallback = 0);
double toDouble(const std::string& s, double fallback = 0.0);
bool toBool(const std::string& s);

}  // namespace utils
}  // namespace skygate

#endif  // SKYGATE_UTILS_H
