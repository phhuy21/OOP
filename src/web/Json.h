#ifndef SKYGATE_WEB_JSON_H
#define SKYGATE_WEB_JSON_H

#include <sstream>
#include <string>
#include <vector>

namespace skygate {
namespace json {

// Escape một chuỗi cho đúng chuẩn JSON (xử lý dấu ", \, xuống dòng, ký tự điều khiển).
inline std::string esc(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

// Builder cho một JSON OBJECT: { "k": v, ... }
// Dùng kiểu chuỗi hoá thủ công để không phải kéo thêm thư viện ngoài.
class Obj {
public:
    Obj& str(const std::string& key, const std::string& value) {
        sep();
        body_ << '"' << esc(key) << "\":\"" << esc(value) << '"';
        return *this;
    }
    Obj& num(const std::string& key, long long value) {
        sep();
        body_ << '"' << esc(key) << "\":" << value;
        return *this;
    }
    Obj& num(const std::string& key, double value) {
        sep();
        body_ << '"' << esc(key) << "\":" << value;
        return *this;
    }
    Obj& boolean(const std::string& key, bool value) {
        sep();
        body_ << '"' << esc(key) << "\":" << (value ? "true" : "false");
        return *this;
    }
    // Nhúng raw JSON đã dựng sẵn (object/array con).
    Obj& raw(const std::string& key, const std::string& rawJson) {
        sep();
        body_ << '"' << esc(key) << "\":" << rawJson;
        return *this;
    }
    std::string dump() const { return "{" + body_.str() + "}"; }

private:
    void sep() { if (first_) first_ = false; else body_ << ','; }
    std::ostringstream body_;
    bool first_ = true;
};

// Builder cho một JSON ARRAY các object/giá trị raw.
class Arr {
public:
    Arr& push(const std::string& rawJson) {
        if (first_) first_ = false; else body_ << ',';
        body_ << rawJson;
        return *this;
    }
    Arr& pushStr(const std::string& value) {
        if (first_) first_ = false; else body_ << ',';
        body_ << '"' << esc(value) << '"';
        return *this;
    }
    std::string dump() const { return "[" + body_.str() + "]"; }

private:
    std::ostringstream body_;
    bool first_ = true;
};

}  // namespace json
}  // namespace skygate

#endif  // SKYGATE_WEB_JSON_H
