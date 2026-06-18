#ifndef SKYGATE_TICKET_H
#define SKYGATE_TICKET_H

#include <string>

namespace skygate {

// ---------------------------------------------------------------------------
// Ticket: vé máy bay — object trung tâm của nghiệp vụ đặt/huỷ vé.
// Mỗi vé liên kết một hành khách (Passenger) đã được tạo với một chuyến bay,
// và thuộc về tài khoản Customer đã mua. Mỗi vé chiếm đúng 1 ghế trên chuyến;
// huỷ vé sẽ trả lại ghế đó.
// ---------------------------------------------------------------------------
class Ticket {
public:
    Ticket() = default;
    Ticket(std::string ticketId, std::string flightCode, std::string passengerId,
           std::string passengerName, std::string ownerUsername)
        : ticketId_(std::move(ticketId)), flightCode_(std::move(flightCode)),
          passengerId_(std::move(passengerId)), passengerName_(std::move(passengerName)),
          ownerUsername_(std::move(ownerUsername)) {}

    const std::string& ticketId() const { return ticketId_; }
    const std::string& flightCode() const { return flightCode_; }
    const std::string& passengerId() const { return passengerId_; }
    const std::string& passengerName() const { return passengerName_; }
    const std::string& ownerUsername() const { return ownerUsername_; }

    std::string describe() const {
        return "[Vé] " + ticketId_ + " — " + passengerName_ + " | chuyến " + flightCode_ +
               " | khách " + ownerUsername_;
    }

private:
    std::string ticketId_;       // mã vé duy nhất
    std::string flightCode_;     // chuyến bay đã đặt
    std::string passengerId_;    // mã hành khách được tạo cho vé này
    std::string passengerName_;  // tên hành khách in trên vé
    std::string ownerUsername_;  // tài khoản Customer đã mua
};

}  // namespace skygate

#endif  // SKYGATE_TICKET_H
