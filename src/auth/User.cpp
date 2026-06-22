#include "User.h"

namespace skygate {
namespace auth {

User::User(std::string username, std::string password, std::string fullName)
    : username_(std::move(username)), password_(std::move(password)),
      fullName_(std::move(fullName)) {}

std::string User::describe() const {
    return "[" + role() + "] " + username_ + " - " + fullName_;
}

// Các "khoá tab" tương ứng với data-tab trên giao diện web.
//   flights   : danh sách & theo dõi chuyến bay (ai cũng xem được)
//   monitor   : giám sát hành khách theo cổng/chuyến + cảnh báo
//   passengers: tra cứu hành khách & hành trình từng khách
//   airports  : sân bay nhà / đường băng / gate
//   aircrafts : đội máy bay
//   create    : tạo chuyến bay
//   log       : nhật ký sự kiện
//   users     : quản lý tài khoản (chỉ Admin)
//   booking   : đặt / huỷ vé

std::vector<std::string> Admin::menu() const {
    return {"flights", "monitor", "passengers", "aircrafts", "create",
            "booking", "log", "users"};
}

std::vector<std::string> Staff::menu() const {
    return {"flights", "monitor", "passengers", "aircrafts",
            "booking", "log"};
}

std::vector<std::string> Customer::menu() const {
    return {"flights", "booking"};
}

std::shared_ptr<User> makeUser(const std::string& role, const std::string& username,
                               const std::string& password, const std::string& fullName) {
    if (role == "Admin") return std::make_shared<Admin>(username, password, fullName);
    if (role == "Customer") return std::make_shared<Customer>(username, password, fullName);
    return std::make_shared<Staff>(username, password, fullName);
}

}  // namespace auth
}  // namespace skygate
