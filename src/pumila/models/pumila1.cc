#include <pumila/pumila.h>
#include <cassert>

namespace pumila {
Pumila1::Pumila1(double alpha, double gamma, double learning_rate)
    : pool(), matrix_ih(Eigen::MatrixXd::Random(IN_NODES, HIDDEN_NODES)),
      matrix_hq(Eigen::VectorXd::Random(HIDDEN_NODES)), alpha(alpha),
      gamma(gamma), learning_rate(learning_rate) {}

std::array<double, ACTIONS_NUM> Pumila1::forward(std::shared_ptr<GameSim> sim) {
    std::array<std::future<Eigen::VectorXd>, ACTIONS_NUM> actions_result;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        actions_result[a] = pool.submit_task([sim, a]() {
            FieldState field = sim->field.copy();
            field.put(sim->getCurrentPair(), actions[a]);
            std::vector<Chain> chains = field.deleteChainRecurse();

            Eigen::VectorXd in(IN_NODES);
            InNodes &in_nodes = *reinterpret_cast<InNodes *>(in.data());
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

    in = Eigen::MatrixXd(ACTIONS_NUM, IN_NODES);
    for (int a = 0; a < ACTIONS_NUM; a++) {
        in.middleRows<1>(a) = actions_result[a].get().transpose();
    }
    hidden = in * matrix_ih;
    hidden.leftCols<1>().array() = 1;                             // bias
    hidden = (1 / (1 + (hidden.array() * alpha).exp())).matrix(); // sigmoid
    assert(hidden.rows() == ACTIONS_NUM);
    assert(hidden.cols() == HIDDEN_NODES);
    q = hidden * matrix_hq;
    assert(q.rows() == ACTIONS_NUM);
    assert(q.cols() == 1);

    std::array<double, ACTIONS_NUM> ret;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        ret[a] = q(a, 0);
    }
    return ret;
}

void Pumila1::backward(int action, double target) {
    Eigen::VectorXd delta_2 = q;
    delta_2(action, 0) = (target + gamma * forwardMax()) - q(action, 0);
    auto delta_1 = ((delta_2 * matrix_hq.transpose()).array() * alpha *
                    hidden.array() * (1 - hidden.array()))
                       .matrix();
    assert(delta_1.rows() == ACTIONS_NUM);
    assert(delta_1.cols() == HIDDEN_NODES);
    matrix_hq -= learning_rate * hidden.transpose() * delta_2;
    matrix_ih -= learning_rate * in.transpose() * delta_1;
}

} // namespace pumila
