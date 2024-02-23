#include <pumila/models/pumila9.h>

namespace PUMILA_NS {
Pumila9::Pumila9(int hidden_nodes) : Pumila8s(hidden_nodes) {}

double Pumila9::calcRewardS(const FieldState &field) {
    /*
    1連鎖 4->100, 8->50, 12->0
    2連鎖 8->200, 16->100, 24->0
    3連鎖 12->300, 24->150, 36->0
    7連鎖 28->700, 56->350,
    */
    int chain = field.prev_chain_num;
    int puyo = field.prev_puyo_num;
    double reward = chain * 100.0 - (puyo / (chain * 4.0) - 1) * (chain * 50.0);
    return reward > 0 ? reward : 0;
}
} // namespace PUMILA_NS
