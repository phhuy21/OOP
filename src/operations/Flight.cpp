#include "Flight.h"

#include <cstdio>

namespace skygate {

Flight::Flight(std::string code, std::string originCode, std::string destCode,
               DateTime departure, DateTime arrival)
    : code_(std::move(code)), originCode_(std::move(originCode)),
      destCode_(std::move(destCode)), departure_(departure), arrival_(arrival) {}

void Flight::addPassenger(const std::shared_ptr<Passenger>& p) {
    if (!p) return;
    for (const auto& existing : passengers_) {
        if (existing->id() == p->id()) return;  // tránh trùng
    }
    p->setFlightCode(code_);
    passengers_.push_back(p);
}

void Flight::removePassenger(const std::string& passengerId) {
    for (size_t i = 0; i < passengers_.size(); ++i) {
        if (passengers_[i]->id() == passengerId) {
            passengers_[i]->setFlightCode("");
            passengers_[i]->setCheckedIn(false);
            passengers_[i]->setBoarded(false);
            passengers_.erase(passengers_.begin() + static_cast<long>(i));
            return;
        }
    }
}

Passenger* Flight::findPassenger(const std::string& passengerId) {
    for (auto& p : passengers_) {
        if (p->id() == passengerId) return p.get();
    }
    return nullptr;
}

int Flight::checkedInCount() const {
    int n = 0;
    for (const auto& p : passengers_) if (p->checkedIn()) ++n;
    return n;
}

int Flight::boardedCount() const {
    int n = 0;
    for (const auto& p : passengers_) if (p->boarded()) ++n;
    return n;
}

double Flight::flightHours() const {
    if (!departure_.isValid() || !arrival_.isValid()) return 0.0;
    long long mins = departure_.minutesUntil(arrival_);
    if (mins <= 0) return 0.0;
    return static_cast<double>(mins) / 60.0;
}

FlightStatus Flight::nextStatus(FlightStatus s) {
    switch (s) {
        case FlightStatus::Scheduled:  return FlightStatus::CheckIn;
        case FlightStatus::CheckIn:    return FlightStatus::Boarding;
        case FlightStatus::Boarding:   return FlightStatus::GateClosed;
        case FlightStatus::GateClosed: return FlightStatus::Ready;
        case FlightStatus::Ready:      return FlightStatus::Takeoff;
        case FlightStatus::Takeoff:    return FlightStatus::InAir;
        case FlightStatus::InAir:      return FlightStatus::Landed;
        case FlightStatus::Landed:     return FlightStatus::Turnaround;
        case FlightStatus::Turnaround: return FlightStatus::Completed;
        default:                       return s;  // Completed/Delayed/Cancelled
    }
}

OpResult Flight::advance(const std::vector<std::string>& crewIssues) {
    if (status_ == FlightStatus::Cancelled) {
        return OpResult::failure("Chuyến bay đã bị huỷ, không thể tiếp tục.");
    }
    if (status_ == FlightStatus::Completed) {
        return OpResult::failure("Chuyến bay đã hoàn tất.");
    }
    if (status_ == FlightStatus::Delayed) {
        // Sau khi hoãn, cho phép quay lại tiến trình từ Boarding.
        status_ = FlightStatus::Boarding;
        return OpResult::success("Chuyến bay tiếp tục sau khi hoãn -> Boarding.");
    }

    FlightStatus target = nextStatus(status_);

    // Điều kiện đặc thù khi chuyển sang "Ready" (sẵn sàng cất cánh).
    // Mô hình thu gọn: chỉ cần máy bay + gate; tổ bay là tuỳ chọn.
    // (Nếu CÓ gán tổ bay thì vẫn kiểm tra tính hợp lệ để giữ logic OOP.)
    if (target == FlightStatus::Ready) {
        if (!aircraft_) return OpResult::failure("Chưa gán máy bay, không thể chuyển sang Ready.");
        if (gateCode_.empty()) return OpResult::failure("Chưa gán gate, không thể chuyển sang Ready.");
        if (crew_ && !crewIssues.empty()) {
            std::string m = "Không thể sẵn sàng do tổ bay không hợp lệ: ";
            for (size_t i = 0; i < crewIssues.size(); ++i) {
                if (i) m += " | ";
                m += crewIssues[i];
            }
            return OpResult::failure(m);
        }
    }

    status_ = target;

    // Khi hạ cánh: cộng giờ bay và cập nhật mốc nghỉ cho tổ bay (phục vụ
    // kiểm tra giới hạn 100 giờ/tháng và nghỉ 8 tiếng ở các chuyến sau).
    if (target == FlightStatus::Landed && crew_) {
        crew_->applyFlightCompletion(flightHours(), arrival_);
    }
    return OpResult::success("Trạng thái -> " + toString(status_));
}

OpResult Flight::checkInPassenger(const std::string& passengerId, const std::string& seat) {
    if (status_ != FlightStatus::CheckIn && status_ != FlightStatus::Boarding) {
        return OpResult::failure("Chỉ check-in khi chuyến ở trạng thái Check-in/Boarding (hiện: " +
                                 statusName() + ").");
    }
    Passenger* p = findPassenger(passengerId);
    if (!p) return OpResult::failure("Hành khách không thuộc chuyến này.");
    if (p->checkedIn()) return OpResult::failure("Hành khách đã check-in trước đó.");

    p->setCheckedIn(true);
    if (!seat.empty()) p->setSeat(seat);

    std::string msg = "Check-in thành công cho " + p->fullName() + ".";
    if (p->baggage().isOverweight()) {
        char buf[128];
        std::snprintf(buf, sizeof(buf),
                      " CẢNH BÁO: hành lý vượt mức (%s), phần vượt %.1f kg.",
                      p->baggage().describe().c_str(), p->baggage().excessWeightKg());
        msg += buf;
    }
    return OpResult::success(msg);
}

bool Flight::isGateOpen() const {
    return status_ == FlightStatus::Boarding;
}

OpResult Flight::boardPassenger(const std::string& passengerId) {
    Passenger* p = findPassenger(passengerId);
    if (!p) return OpResult::failure("Hành khách không thuộc chuyến này.");
    if (!p->checkedIn()) return OpResult::failure("Hành khách chưa check-in.");
    if (p->boarded()) return OpResult::failure("Hành khách đã lên máy bay.");

    // Mục 3.4: nếu gate đã đóng, hành khách lên muộn KHÔNG được boarding.
    if (!isGateOpen()) {
        return OpResult::failure("Cửa khởi hành đã đóng (trạng thái " + statusName() +
                                 "). " + p->fullName() + " lên muộn, không được boarding.");
    }
    p->setBoarded(true);
    return OpResult::success(p->fullName() + " đã lên máy bay.");
}

OpResult Flight::delay(long long minutes, const std::string& reason) {
    if (status_ == FlightStatus::Takeoff || status_ == FlightStatus::InAir ||
        status_ == FlightStatus::Landed || status_ == FlightStatus::Turnaround ||
        status_ == FlightStatus::Completed || status_ == FlightStatus::Cancelled) {
        return OpResult::failure("Chuyến bay đang bay hoặc đã kết thúc, không thể hoãn.");
    }
    if (minutes > 0 && departure_.isValid()) {
        departure_ = departure_.addMinutes(minutes);
        if (arrival_.isValid()) arrival_ = arrival_.addMinutes(minutes);
    }
    status_ = FlightStatus::Delayed;
    note_ = "Hoãn: " + reason;
    return OpResult::success("Chuyến " + code_ + " đã hoãn. " + reason);
}

OpResult Flight::cancel(const std::string& reason) {
    if (status_ == FlightStatus::Takeoff || status_ == FlightStatus::InAir ||
        status_ == FlightStatus::Landed || status_ == FlightStatus::Turnaround ||
        status_ == FlightStatus::Completed || status_ == FlightStatus::Cancelled) {
        return OpResult::failure("Chuyến bay đang bay hoặc đã kết thúc, không thể huỷ.");
    }
    status_ = FlightStatus::Cancelled;
    note_ = "Huỷ: " + reason;
    return OpResult::success("Chuyến " + code_ + " đã huỷ. " + reason);
}

bool Flight::isFinished() const {
    return status_ == FlightStatus::Completed || status_ == FlightStatus::Cancelled;
}

std::string Flight::describe() const {
    std::string s = code_ + ": " + originCode_ + " -> " + destCode_ +
                    " | đi " + departure_.toString() +
                    " | đến " + arrival_.toString() +
                    " | " + statusName();
    if (emergency_) s += " | KHẨN CẤP";
    s += "\n      máy bay: " + (aircraft_ ? aircraft_->registration() + " (" + aircraft_->categoryName() + ")" : std::string("(chưa gán)"));
    s += " | tổ bay: " + (crew_ ? crew_->id() : std::string("(chưa gán)"));
    s += " | gate: " + (gateCode_.empty() ? std::string("(chưa gán)") : gateCode_);
    s += "\n      hành khách: " + std::to_string(passengers_.size()) +
         " (check-in " + std::to_string(checkedInCount()) +
         ", boarded " + std::to_string(boardedCount()) + ")";
    if (!note_.empty()) s += " | " + note_;
    return s;
}

}  // namespace skygate
