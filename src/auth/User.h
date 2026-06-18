#ifndef SKYGATE_USER_H
#define SKYGATE_USER_H

#include <memory>
#include <string>
#include <vector>

namespace skygate {
namespace auth {

// ---------------------------------------------------------------------------
// User: lớp cha trừu tượng cho 3 phân hệ tài khoản (Admin / Staff / Customer).
// Minh hoạ ĐA HÌNH: role() và menu() là hàm ảo thuần — mỗi vai trò tự khai báo
// tên vai trò và danh sách tab/chức năng mà giao diện web được phép hiển thị.
// (Ở phiên bản console, đây chính là vai trò của hàm ảo showMenu().)
//
// Đặt trong namespace skygate::auth để không trùng với lớp skygate::Staff
// (lớp cha của Pilot/GroundStaff trong mô hình nhân sự sân bay).
// ---------------------------------------------------------------------------
class User {
public:
    User() = default;
    User(std::string username, std::string password, std::string fullName);
    virtual ~User() = default;  // huỷ ảo cho lớp cha đa hình

    const std::string& username() const { return username_; }
    const std::string& password() const { return password_; }
    const std::string& fullName() const { return fullName_; }

    void setPassword(const std::string& p) { password_ = p; }
    void setFullName(const std::string& n) { fullName_ = n; }

    bool checkPassword(const std::string& p) const { return p == password_; }

    // Tên vai trò ("Admin" / "Staff" / "Customer") — dùng cho hiển thị,
    // phân quyền và lưu file.
    virtual std::string role() const = 0;

    // Danh sách "khoá tab" mà vai trò này được phép thấy trên giao diện web.
    virtual std::vector<std::string> menu() const = 0;

    virtual std::string describe() const;

protected:
    std::string username_;
    std::string password_;
    std::string fullName_;
};

// --- Admin: toàn quyền, quản lý hạ tầng (máy bay/đường băng) & tài khoản ---
class Admin : public User {
public:
    Admin() = default;
    Admin(std::string username, std::string password, std::string fullName)
        : User(std::move(username), std::move(password), std::move(fullName)) {}
    std::string role() const override { return "Admin"; }
    std::vector<std::string> menu() const override;
};

// --- Staff: vận hành (tạo chuyến, đổi trạng thái, đặt/huỷ vé) ---
class Staff : public User {
public:
    Staff() = default;
    Staff(std::string username, std::string password, std::string fullName)
        : User(std::move(username), std::move(password), std::move(fullName)) {}
    std::string role() const override { return "Staff"; }
    std::vector<std::string> menu() const override;
};

// --- Customer: tra cứu chuyến & mua/huỷ vé của chính mình ---
class Customer : public User {
public:
    Customer() = default;
    Customer(std::string username, std::string password, std::string fullName)
        : User(std::move(username), std::move(password), std::move(fullName)) {}
    std::string role() const override { return "Customer"; }
    std::vector<std::string> menu() const override;
};

// Tạo đối tượng User đúng lớp con theo tên vai trò (Factory).
std::shared_ptr<User> makeUser(const std::string& role, const std::string& username,
                               const std::string& password, const std::string& fullName);

}  // namespace auth
}  // namespace skygate

#endif  // SKYGATE_USER_H
