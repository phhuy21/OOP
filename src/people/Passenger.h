#ifndef SKYGATE_PASSENGER_H
#define SKYGATE_PASSENGER_H

#include <string>

#include "Person.h"
#include "../operations/Baggage.h"

namespace skygate {

// ---------------------------------------------------------------------------
// Passenger: hành khách. Kế thừa Person.
// Lưu hộ chiếu, chuyến bay đã đặt, trạng thái check-in / lên máy bay, số ghế
// và hành lý ký gửi (THÀNH PHẦN: Passenger "có một" Baggage).
// ---------------------------------------------------------------------------
class Passenger : public Person {
public:
    Passenger() = default;
    Passenger(std::string id, std::string fullName, int age, std::string phone,
              std::string passport);

    const std::string& passport() const { return passport_; }
    void setPassport(const std::string& p) { passport_ = p; }

    const std::string& flightCode() const { return flightCode_; }
    void setFlightCode(const std::string& code) { flightCode_ = code; }

    const std::string& seat() const { return seat_; }
    void setSeat(const std::string& s) { seat_ = s; }

    bool checkedIn() const { return checkedIn_; }
    void setCheckedIn(bool v) { checkedIn_ = v; }

    bool boarded() const { return boarded_; }
    void setBoarded(bool v) { boarded_ = v; }

    Baggage& baggage() { return baggage_; }
    const Baggage& baggage() const { return baggage_; }
    void setBaggage(const Baggage& b) { baggage_ = b; }

    std::string role() const override { return "Passenger"; }
    std::string describe() const override;

private:
    std::string passport_;
    std::string flightCode_;   // mã chuyến bay đã đặt (rỗng nếu chưa gán)
    std::string seat_;         // số ghế (gán khi check-in)
    bool checkedIn_ = false;
    bool boarded_ = false;
    Baggage baggage_;
};

}  // namespace skygate

#endif  // SKYGATE_PASSENGER_H
