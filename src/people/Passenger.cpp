#include "Passenger.h"

namespace skygate {

Passenger::Passenger(std::string id, std::string fullName, int age, std::string phone,
                     std::string passport)
    : Person(std::move(id), std::move(fullName), age, std::move(phone)),
      passport_(std::move(passport)) {}

std::string Passenger::describe() const {
    std::string s = "[Passenger] " + id_ + " - " + fullName_ +
                    " | hộ chiếu " + passport_;
    s += " | chuyến: " + (flightCode_.empty() ? "(chưa đặt)" : flightCode_);
    s += " | ghế: " + (seat_.empty() ? "-" : seat_);
    s += " | check-in: " + std::string(checkedIn_ ? "có" : "chưa");
    s += " | boarded: " + std::string(boarded_ ? "có" : "chưa");
    s += " | hành lý: " + baggage_.describe();
    return s;
}

}  // namespace skygate
