#include <pumila/models/pumila11.h>

namespace PUMILA_NS {

NNModel11::NNModel11(int hidden_nodes)
    : hidden_nodes(hidden_nodes),
      matrix_ih(Eigen::MatrixXd::Random(IN_NODES, hidden_nodes)),
      matrix_hq(Eigen::VectorXd::Random(hidden_nodes)) {}

void Pumila11::load(std::istream &is) {
    std::lock_guard lock_main(main_m);
    std::lock_guard lock_target(target_m);
    {
        is.read(reinterpret_cast<char *>(&gamma), sizeof(gamma));
        is.read(reinterpret_cast<char *>(&main.hidden_nodes),
                sizeof(main.hidden_nodes));
        main.matrix_ih = Eigen::MatrixXd(NNModel::IN_NODES, main.hidden_nodes);
        main.matrix_hq = Eigen::VectorXd(main.hidden_nodes);
        is.read(reinterpret_cast<char *>(main.matrix_ih.data()),
                main.matrix_ih.rows() * main.matrix_ih.cols() * sizeof(double));
        is.read(reinterpret_cast<char *>(main.matrix_hq.data()),
                main.matrix_hq.rows() * main.matrix_hq.cols() * sizeof(double));
        target = main;
    }
}
void Pumila11::save(std::ostream &os) {
    std::shared_lock lock_main(main_m);
    os.write(reinterpret_cast<char *>(&gamma), sizeof(gamma));
    os.write(reinterpret_cast<char *>(&main.hidden_nodes),
             sizeof(main.hidden_nodes));
    os.write(reinterpret_cast<char *>(main.matrix_ih.data()),
             main.matrix_ih.rows() * main.matrix_ih.cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main.matrix_hq.data()),
             main.matrix_hq.rows() * main.matrix_hq.cols() * sizeof(double));
}

Pumila11::InFeatureSingle
Pumila11::getInNodeSingleS(const FieldState2 &field_copy, int a) {
    InFeatureSingle feat;
    feat.field_next = field_copy;
    feat.field_next.putNext(actions[a]);
    auto chains = feat.field_next.deleteChainRecurse();

    feat.in = Eigen::VectorXd::Zero(NNModel::IN_NODES);
    NNModel::InNodes &in_nodes =
        *reinterpret_cast<NNModel::InNodes *>(feat.in.data());
    in_nodes.bias = 1;
    auto chain_all = feat.field_next.calcChainAll();
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
            default:
                p = -1;
                break;
            }
            if (p >= 0) {
                in_nodes.field_colors[(y * FieldState::WIDTH + x) * 4 + p] = 1;
                in_nodes.field_chains[(y * FieldState::WIDTH + x) * 4 + p] =
                    chain_all[y][x];
            }
        }
    }
    for (std::size_t i = 0;
         i < chains.size() && i < sizeof(in_nodes.score_diff) / sizeof(double);
         i++) {
        double score_base = 40.0 * Chain::chainBonus(i + 1);
        in_nodes.score_diff[i] =
            chains[i].score() / (score_base ? score_base : 40.0);
    }
    return feat;
}
Eigen::MatrixXd Pumila11::truncateInNodesS(const Eigen::MatrixXd &in) {
    if (in.cols() != NNModel::IN_NODES) {
        throw std::invalid_argument("invalid size in truncate: in -> " +
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
        for (std::size_t p = 0;
             p < FieldState::WIDTH * FieldState::HEIGHT * 4 * 2; p += 4) {
            for (int i = 0; i < 4; i++) {
                transform(1 + p + i, 1 + p + i) = 0;
                transform(1 + p + i, 1 + p + index[i]) = 1;
            }
        }
        ret.middleRows(in.rows() * r, in.rows()) = in * transform;
        r++;
    } while (std::next_permutation(index.begin(), index.end()));
    return ret;
}

} // namespace PUMILA_NS