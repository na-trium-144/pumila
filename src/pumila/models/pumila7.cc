#include <pumila/models/pumila7.h>

namespace PUMILA_NS {
Pumila7::Pumila7(int hidden_nodes) : Pumila6(hidden_nodes) {}

double Pumila7::calcRewardS(const FieldState &field) {
    if (field.prev_chain_score > 0) {
        double rw = field.prev_chain_score -
                    (field.step_num - field.last_chain_step_num) * 20;
        return rw > 40 ? rw : 40;
    }
    return 0;
}

} // namespace PUMILA_NS
