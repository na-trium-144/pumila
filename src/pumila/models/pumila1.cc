#include <pumila/models/pumila1.h>
#include <pumila/models/pumila2.h>
#include <cassert>
#include <stdexcept>
#include <string>

namespace PUMILA_NS {
Pumila1::NNModel::NNModel(double alpha, double learning_rate)
    : alpha(alpha), learning_rate(learning_rate),
      matrix_ih(Eigen::MatrixXd::Random(IN_NODES, HIDDEN_NODES)),
      matrix_hq(Eigen::VectorXd::Random(HIDDEN_NODES)) {}

Pumila1::Pumila1(double alpha, double gamma, double learning_rate)
    : Pumila(), gamma(gamma), back_count(0), main(alpha, learning_rate),
      target(main) {}

void Pumila1::load(std::istream &is) {
    is.read(reinterpret_cast<char *>(&main.alpha), sizeof(main.alpha));
    is.read(reinterpret_cast<char *>(main.matrix_ih.data()),
            main.matrix_ih.rows() * main.matrix_ih.cols() * sizeof(double));
    is.read(reinterpret_cast<char *>(main.matrix_hq.data()),
            main.matrix_hq.rows() * main.matrix_hq.cols() * sizeof(double));
    target = main;
}
void Pumila1::save(std::ostream &os) {
    os.write(reinterpret_cast<char *>(&main.alpha), sizeof(main.alpha));
    os.write(reinterpret_cast<char *>(main.matrix_ih.data()),
             main.matrix_ih.rows() * main.matrix_ih.cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main.matrix_hq.data()),
             main.matrix_hq.rows() * main.matrix_hq.cols() * sizeof(double));
}

Eigen::MatrixXd Pumila1::getInNodes(const FieldState &field) const {
    return Pumila2::getInNodes(field).get().in;
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
        (1 / (1 + (-alpha * ret.hidden.array()).exp())).matrix(); // sigmoid
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
    return Pumila2::calcRewardS(sim_after->field);
}

int Pumila1::getAction(const FieldState &field) {
    NNResult fw_result = main.forward(getInNodes(field));
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

} // namespace PUMILA_NS
