#include <pumila/models/pumila5.h>

namespace pumila {
Pumila5::Pumila5(double learning_rate) : Pumila3(learning_rate) {
    gamma = 0.99;
}

double Pumila5::calcRewardS(const FieldState &field) {
    return field.prev_chain_score;
}
} // namespace pumila
