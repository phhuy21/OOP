#include "Baggage.h"

#include <cstdio>

namespace skygate {

Baggage::Baggage(int pieces, double totalWeightKg) {
    setPieces(pieces);
    setTotalWeightKg(totalWeightKg);
}

void Baggage::setPieces(int pieces) {
    pieces_ = pieces < 0 ? 0 : pieces;
}

void Baggage::setTotalWeightKg(double kg) {
    totalWeightKg_ = kg < 0.0 ? 0.0 : kg;
}

bool Baggage::isOverweight() const {
    return pieces_ > kAllowancePieces || totalWeightKg_ > kAllowanceWeightKg;
}

double Baggage::excessWeightKg() const {
    double excess = totalWeightKg_ - kAllowanceWeightKg;
    return excess > 0.0 ? excess : 0.0;
}

std::string Baggage::describe() const {
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%d kiện, %.1f kg%s",
                  pieces_, totalWeightKg_,
                  isOverweight() ? " (VƯỢT MỨC)" : "");
    return std::string(buf);
}

}  // namespace skygate
