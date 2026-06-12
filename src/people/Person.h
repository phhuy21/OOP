#ifndef SKYGATE_PERSON_H
#define SKYGATE_PERSON_H

#include <string>

namespace skygate {

// ---------------------------------------------------------------------------
// Person: lớp cha trừu tượng cho mọi con người trong hệ thống.
// Minh hoạ ĐÓNG GÓI (thuộc tính protected, truy cập qua getter/setter) và
// ĐA HÌNH (role() / describe() là hàm ảo thuần / hàm ảo).
//   Person -> Passenger, Staff
//   Staff  -> Pilot, GroundStaff
// ---------------------------------------------------------------------------
class Person {
public:
    Person() = default;
    Person(std::string id, std::string fullName, int age, std::string phone);
    virtual ~Person() = default;  // hàm huỷ ảo cho lớp cha đa hình

    // --- Getter ---
    const std::string& id() const { return id_; }
    const std::string& fullName() const { return fullName_; }
    int age() const { return age_; }
    const std::string& phone() const { return phone_; }

    // --- Setter (có kiểm tra cơ bản) ---
    void setId(const std::string& id) { id_ = id; }
    void setFullName(const std::string& name) { fullName_ = name; }
    void setAge(int age);
    void setPhone(const std::string& phone) { phone_ = phone; }

    // Vai trò trong hệ thống ("Passenger", "Pilot", ...). Hàm ảo thuần.
    virtual std::string role() const = 0;

    // Mô tả ngắn gọn để in ra màn hình (có thể override để bổ sung thông tin).
    virtual std::string describe() const;

protected:
    std::string id_;        // mã định danh duy nhất (CMND/CCCD nội bộ, mã NV...)
    std::string fullName_;
    int age_ = 0;
    std::string phone_;
};

}  // namespace skygate

#endif  // SKYGATE_PERSON_H
