#include <pumila/models/pumila1.h>
#include <cassert>
#include <stdexcept>
#include <string>

namespace pumila {
Pumila1::NNModel::NNModel(double alpha, double learning_rate)
    : alpha(alpha), learning_rate(learning_rate),
      matrix_ih(Eigen::MatrixXd::Random(IN_NODES, HIDDEN_NODES)),
      matrix_hq(Eigen::VectorXd::Random(HIDDEN_NODES)) {}

Pumila1::Pumila1(double alpha, double gamma, double learning_rate)
    : Pumila(), gamma(gamma), back_count(0), main(alpha, learning_rate),
      target(main) {}

Eigen::MatrixXd Pumila1::getInNodes(std::shared_ptr<GameSim> sim) const {
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
                        in_nodes
                            .field_chains[(y * FieldState::WIDTH + x) * 4 + p] =
                            chain_all[y][x];
                    }
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
Pumila1::NNResult Pumila1::NNModel::forward(const Eigen::MatrixXd &in) const {
    if (in.cols() != IN_NODES) {
        throw std::invalid_argument("invalid size in forward: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(IN_NODES));
    }
    NNResult ret;
    ret.in = in;
    ret.hidden = in * matrix_ih;
    ret.hidden.leftCols<1>().array() = 1; // bias
    ret.hidden =
        (1 / (1 + (ret.hidden.array() * alpha).exp())).matrix(); // sigmoid
    assert(ret.hidden.cols() == HIDDEN_NODES);
    ret.q = ret.hidden * matrix_hq;
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila1::NNModel::backward(const NNResult &result,
                                const Eigen::VectorXd &diff) {
    if (diff.rows() != result.q.rows()) {
        throw std::invalid_argument("invalid diff size in backward: q -> " +
                                    std::to_string(result.q.rows()) +
                                    ", diff -> " + std::to_string(diff.rows()));
    }
    // f2(u) = u -> delta_2 = diff * f2'(u) = diff;
    // f1(u) = 1 / (1 + exp(-au))
    auto f1_dif = alpha * result.hidden.array() * (1 - result.hidden.array());
    auto delta_1 = ((diff * matrix_hq.transpose()).array() * f1_dif).matrix();
    assert(delta_1.rows() == result.q.rows());
    assert(delta_1.cols() == NNModel::HIDDEN_NODES);
    matrix_hq += learning_rate * result.hidden.transpose() * diff;
    matrix_ih += learning_rate * result.in.transpose() * delta_1;
}

double Pumila1::calcReward(std::shared_ptr<GameSim> sim_after) const {
    double r = -1;
    std::size_t max_y = 0;
    for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
        auto y = sim_after->field.putTargetY(x);
        max_y = y > max_y ? y : max_y;
    }
    if (max_y >= 4) {
        /*
        4段 -> 1 (1連鎖)
        5段 -> 9 (2連鎖)
        8段 -> 57 (4連鎖)
        に相当するとし、適当に2次で近似

        */
        r -= 10 * (2 * max_y * max_y - 10 * max_y + 9);
    }
    r += sim_after->prev_chain_score / 4;
    return r;
}

int Pumila1::getAction(std::shared_ptr<GameSim> sim) {
    NNResult fw_result = main.forward(getInNodes(sim));
    int action = 0;
    fw_result.q.maxCoeff(&action);
    return action;
}
std::pair<int, int> Pumila1::getLearnAction(std::shared_ptr<GameSim> sim) {
    int id = ++learn_results_index;
    learn_results.emplace(id, main.forward(getInNodes(sim)));
    int action = 0;
    learn_results[id].q.maxCoeff(&action);
    if (rnd() - rnd.min() < 0.3 * (rnd.max() - rnd.min())) {
        action = static_cast<int>(22.0 * (rnd() - rnd.min()) /
                                  (rnd.max() - rnd.min()));
    }
    learn_actions.emplace(id, action);
    return std::make_pair(action, id);
}
double Pumila1::learnResult(int id, std::shared_ptr<GameSim> sim_after) {
    auto action = learn_actions.find(id);
    auto fw_result = learn_results.find(id);
    if (action == learn_actions.end() && fw_result == learn_results.end()) {
        throw std::invalid_argument("invalid id " + std::to_string(id) +
                                    " for learnResult");
    }
    Eigen::VectorXd delta_2 = Eigen::VectorXd::Zero(fw_result->second.q.rows());
    double diff = (calcReward(sim_after) +
                   gamma * target.forward(getInNodes(sim_after)).q.maxCoeff()) -
                  fw_result->second.q(action->second, 0);
    delta_2(action->second, 0) = diff;
    main.backward(fw_result->second, delta_2);
    learn_actions.erase(action);
    learn_results.erase(fw_result);
    if (++back_count > 10) {
        back_count = 0;
        target = main;
    }
    return diff;
}

} // namespace pumila
