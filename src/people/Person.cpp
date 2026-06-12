#include "Person.h"

namespace skygate {

Person::Person(std::string id, std::string fullName, int age, std::string phone)
    : id_(std::move(id)), fullName_(std::move(fullName)), age_(age), phone_(std::move(phone)) {}

void Person::setAge(int age) {
    if (age >= 0 && age < 130) {
        age_ = age;
    }
}

std::string Person::describe() const {
    return "[" + role() + "] " + id_ + " - " + fullName_ +
           " (tuổi " + std::to_string(age_) + ", SĐT " + phone_ + ")";
}

}  // namespace skygate
