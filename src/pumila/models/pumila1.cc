#include <pumila/models/pumila1.h>
#include <cassert>

namespace pumila {
Pumila1::NNModel::NNModel(double alpha, double learning_rate)
    : matrix_ih(Eigen::MatrixXd::Random(IN_NODES, HIDDEN_NODES)),
      matrix_hq(Eigen::VectorXd::Random(HIDDEN_NODES)), alpha(alpha),
      learning_rate(learning_rate) {}

Pumila1::Pumila1(double alpha, double gamma, double learning_rate)
    : main(alpha, learning_rate), target(main), gamma(gamma), last_result(),
      back_count(0) {}

Eigen::MatrixXd Pumila1::getInNodes(std::shared_ptr<GameSim> sim) {
    std::array<std::future<Eigen::VectorXd>, ACTIONS_NUM> actions_result;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        actions_result[a] = pool.submit_task([sim, a]() {
            FieldState field = sim->field.copy();
            field.put(sim->getCurrentPair(), actions[a]);
            std::vector<Chain> chains = field.deleteChainRecurse();

            Eigen::VectorXd in = Eigen::VectorXd::Zero(NNModel::IN_NODES);
            NNModel::InNodes &in_nodes =
                *reinterpret_cast<NNModel::InNodes *>(in.data());
            in_nodes.bias = 1;
            auto chain_all = field.calcChainAll();
            for (int y = 0; y < FieldState::HEIGHT; y++) {
                for (int x = 0; x < FieldState::WIDTH; x++) {
                    int p;
                    switch (field.get(x, y)) {
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
                        in_nodes
                            .field_colors[(y * FieldState::WIDTH + x) * 4 + p] =
                            1;
                    }
                    in_nodes.field_chains[y * FieldState::WIDTH + x] =
                        chain_all[y][x];
                }
            }
            in_nodes.score_diff = 0;
            for (const auto &c : chains) {
                in_nodes.score_diff += c.score();
            }

            return in;
        });
    }

    Eigen::MatrixXd in(ACTIONS_NUM, NNModel::IN_NODES);
    for (int a = 0; a < ACTIONS_NUM; a++) {
        in.middleRows<1>(a) = actions_result[a].get().transpose();
    }
    return in;
}
std::array<double, ACTIONS_NUM> Pumila1::forward(std::shared_ptr<GameSim> sim) {
    last_result = main.forward(getInNodes(sim));
    std::array<double, ACTIONS_NUM> ret;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        ret[a] = last_result.q(a, 0);
    }
    return ret;
}
Pumila1::NNResult Pumila1::NNModel::forward(const Eigen::MatrixXd &in) const {
    NNResult ret;
    ret.in = in;
    assert(in.rows() == ACTIONS_NUM);
    assert(in.cols() == IN_NODES);
    ret.hidden = in * matrix_ih;
    ret.hidden.leftCols<1>().array() = 1; // bias
    ret.hidden =
        (1 / (1 + (ret.hidden.array() * alpha).exp())).matrix(); // sigmoid
    assert(ret.hidden.rows() == ACTIONS_NUM);
    assert(ret.hidden.cols() == HIDDEN_NODES);
    ret.q = ret.hidden * matrix_hq;
    assert(ret.q.rows() == ACTIONS_NUM);
    assert(ret.q.cols() == 1);
    return ret;
}
double Pumila1::NNModel::forwardMax(const Eigen::MatrixXd &in) const {
    NNResult ret = forward(in);
    double max_q = std::numeric_limits<double>::lowest();
    for (double q : ret.q) {
        max_q = max_q > q ? max_q : q;
    }
    return max_q;
}

void Pumila1::backward(std::shared_ptr<GameSim> sim_after, int action_prev,
                       double target_r) {
    Eigen::VectorXd delta_2 = Eigen::VectorXd::Zero(last_result.q.rows());
    delta_2(action_prev, 0) =
        (target_r + gamma * target.forwardMax(getInNodes(sim_after))) -
        last_result.q(action_prev, 0);
    auto delta_1 =
        ((delta_2 * main.matrix_hq.transpose()).array() * main.alpha *
         last_result.hidden.array() * (1 - last_result.hidden.array()))
            .matrix();
    assert(delta_1.rows() == ACTIONS_NUM);
    assert(delta_1.cols() == NNModel::HIDDEN_NODES);
    main.matrix_hq +=
        main.learning_rate * last_result.hidden.transpose() * delta_2;
    main.matrix_ih += main.learning_rate * last_result.in.transpose() * delta_1;

    if (++back_count > 10) {
        back_count = 0;
        target = main;
    }
}

} // namespace pumila
