#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace skygate {
namespace utils {

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delimiter) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

std::string join(const std::vector<std::string>& parts, char delimiter) {
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out.push_back(delimiter);
        out += parts[i];
    }
    return out;
}

std::string trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string escapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|':  out += "\\p"; break;
            case '\n': out += "\\n"; break;
            case '\r': break;  // bỏ ký tự CR
            default:   out.push_back(c); break;
        }
    }
    return out;
}

std::string unescapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == '\\')      { out.push_back('\\'); ++i; }
            else if (n == 'p')  { out.push_back('|');  ++i; }
            else if (n == 'n')  { out.push_back('\n'); ++i; }
            else                { out.push_back(s[i]); }
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::vector<std::string> parseRecord(const std::string& line) {
    std::vector<std::string> raw = split(line, '|');
    std::vector<std::string> out;
    out.reserve(raw.size());
    for (const auto& f : raw) out.push_back(unescapeField(f));
    return out;
}

std::string buildRecord(const std::vector<std::string>& fields) {
    std::vector<std::string> escaped;
    escaped.reserve(fields.size());
    for (const auto& f : fields) escaped.push_back(escapeField(f));
    return join(escaped, '|');
}

int toInt(const std::string& s, int fallback) {
    try {
        return std::stoi(trim(s));
    } catch (...) {
        return fallback;
    }
}

double toDouble(const std::string& s, double fallback) {
    try {
        return std::stod(trim(s));
    } catch (...) {
        return fallback;
    }
}

bool toBool(const std::string& s) {
    std::string t = toLower(trim(s));
    return t == "1" || t == "true" || t == "yes" || t == "y";
}

}  // namespace utils
}  // namespace skygate
