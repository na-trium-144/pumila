#include <pumila/models/pumila13.h>

namespace PUMILA_NS {

NNModel13::NNModel13(int hidden_nodes)
    : hidden_nodes(hidden_nodes),
      matrix_ih(Eigen::MatrixXd::Random(IN_NODES, hidden_nodes)),
      matrix_hq(Eigen::VectorXd::Random(hidden_nodes)) {}

Pumila13::InFeatureSingle
Pumila13::getInNodeSingleS(const FieldState2 &field_copy, int a,
                           const std::optional<FieldState2> &op_field_copy) {
    if (!op_field_copy) {
        throw std::invalid_argument("op_field is nullopt");
    }
    InFeatureSingle feat;
    feat.field_next = field_copy;
    feat.field_next.putNext(actions[a]);
    auto chains = feat.field_next.deleteChainRecurse();

    feat.in = Eigen::VectorXd::Zero(NNModel::IN_NODES);
    NNModel::InNodes &in_nodes =
        *reinterpret_cast<NNModel::InNodes *>(feat.in.data());
    in_nodes.bias = 1;
    auto chain_all = feat.field_next.calcChainAll();
    auto op_chain_all = op_field_copy->calcChainAll();
    for (std::size_t y = 0; y < FieldState::HEIGHT; y++) {
        for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
            int p;
            switch (feat.field_next.field().get(x, y)) {
            case Puyo::red:
                p = 0;
                break;
            case Puyo::blue:
                p = 1;
                break;
            case Puyo::green:
                p = 2;
                break;
            case Puyo::yellow:
                p = 3;
                break;
            case Puyo::garbage:
                p = 4;
                break;
            default:
                p = -1;
                break;
            }
            if (p >= 0) {
                in_nodes.field_colors[(y * FieldState::WIDTH + x) * 5 + p] = 1;
                in_nodes.field_chains[(y * FieldState::WIDTH + x) * 4 + p] =
                    chain_all[y][x];
            }
            if (op_chain_all[y][x] > 0 && op_chain_all[y][x] <= 20) {
                for (int c = 0; c < op_chain_all[y][x]; c++) {
                    in_nodes.op_field_chains[c]++;
                }
            }
        }
    }
    for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
        in_nodes.op_height[x] = op_field_copy->getHeight(x);
    }
    for (std::size_t i = 0;
         i < chains.size() && i < sizeof(in_nodes.chains) / sizeof(double);
         i++) {
        double score_base = 40.0 * Chain::chainBonus(i + 1);
        in_nodes.chains[i] =
            chains[i].score() / (score_base ? score_base : 40.0);
    }
    for (std::size_t i = 0; i < op_field_copy->currentStep().chainNum() &&
                            i < sizeof(in_nodes.op_chains) / sizeof(double);
         i++) {
        double score_base = 40.0 * Chain::chainBonus(i + 1);
        in_nodes.op_chains[i] =
            op_field_copy->currentStep().chains()[i].score() /
            (score_base ? score_base : 40.0);
    }
    auto op_garbage = op_field_copy->garbage().getReady();
    for (int i = 0; i * 6 < op_garbage &&
                    i < static_cast<int>(sizeof(in_nodes.op_garbage_ready) /
                                         sizeof(double));
         i++) {
        if ((i + 1) * 6 < op_garbage) {
            in_nodes.op_garbage_ready[i] = 1;
        } else {
            in_nodes.op_garbage_ready[i] = (op_garbage - i * 6) / 6.0;
        }
    }

    return feat;
}
Eigen::MatrixXd Pumila13::transposeInNodesS(const Eigen::MatrixXd &in) {
    if (in.cols() != NNModel::IN_NODES) {
        throw std::invalid_argument("invalid size in transpose: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(NNModel::IN_NODES));
    }
    Eigen::MatrixXd ret(in.rows() * 24, NNModel::IN_NODES);
    std::array<int, 4> index = {1, 2, 3, 4};
    int r = 0;
    do {
        assert(r < 24);
        Eigen::MatrixXd transform =
            Eigen::MatrixXd::Identity(NNModel::IN_NODES, NNModel::IN_NODES);
        for (std::size_t a = 0; a < FieldState::WIDTH * FieldState::HEIGHT;
             a++) {
            for (int i = 0; i < 4; i++) {
                int p = sizeof(NNModel::InNodes::bias) / sizeof(double);
                assert(p == 1);
                transform(p + a * 5 + i, p + a * 5 + i) = 0;
                transform(p + a * 5 + i, p + a * 5 + index[i]) = 1;
                p += sizeof(NNModel::InNodes::field_colors) / sizeof(double);
                transform(p + a * 4 + i, p + a * 4 + i) = 0;
                transform(p + a * 4 + i, p + a * 4 + index[i]) = 1;
            }
        }
        ret.middleRows(in.rows() * r, in.rows()) = in * transform;
        r++;
    } while (std::next_permutation(index.begin(), index.end()));
    return ret;
}

std::pair<double, bool>
Pumila13::calcReward(const FieldState2 &field,
                     const std::optional<FieldState2> &op_field_after) const {
    if (!op_field_after) {
        throw std::invalid_argument("op_field is nullopt");
    }
    int op_score = op_field_after->totalScore() - this->prev_op_score;
    if (op_score < 0) {
        op_score = 0;
    }
    this->prev_op_score = op_field_after->totalScore();
    if (field.isGameOver()) {
        return {-op_score, true};
    }
    return {field.currentStep().chainScore() - op_score, false};
}
} // namespace PUMILA_NS