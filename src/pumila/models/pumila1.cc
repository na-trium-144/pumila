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

std::pair<std::array<FieldState, ACTIONS_NUM>, Eigen::MatrixXd>
Pumila1::getInNodes(const FieldState &field) {
    auto actions_result = pool.submit_blocks(
        0, ACTIONS_NUM,
        [&field](int a, int) {
            FieldState field_next = field.copy();
            field_next.put(actions[a]);
            field_next.next.pop_front();
            std::vector<Chain> chains = field_next.deleteChainRecurse();

            Eigen::VectorXd in = Eigen::VectorXd::Zero(NNModel::IN_NODES);
            NNModel::InNodes &in_nodes =
                *reinterpret_cast<NNModel::InNodes *>(in.data());
            in_nodes.bias = 1;
            auto chain_all = field_next.calcChainAll();
            for (int y = 0; y < FieldState::HEIGHT; y++) {
                for (int x = 0; x < FieldState::WIDTH; x++) {
                    int p;
                    switch (field_next.get(x, y)) {
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
            return std::make_pair(field_next, in);
        },
        ACTIONS_NUM);

    std::array<FieldState, ACTIONS_NUM> field_next;
    Eigen::MatrixXd in(ACTIONS_NUM, NNModel::IN_NODES);
    int a = 0;
    for (auto &res : actions_result) {
        auto r = res.get();
        field_next[a] = r.first;
        in.middleRows<1>(a) = r.second.transpose();
        a++;
    }
    return std::make_pair(field_next, in);
}
Eigen::MatrixXd Pumila1::NNModel::truncateInNodes(const Eigen::MatrixXd &in) {
    if (in.cols() != IN_NODES) {
        throw std::invalid_argument("invalid size in truncate: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(IN_NODES));
    }
    Eigen::MatrixXd ret(in.rows() * 24, IN_NODES);
    std::array<int, 4> index = {1, 2, 3, 4};
    int r = 0;
    do {
        assert(r < 24);
        Eigen::MatrixXd transform =
            Eigen::MatrixXd::Identity(IN_NODES, IN_NODES);
        for (int p = 0; p < FieldState::WIDTH * FieldState::HEIGHT * 4 * 2;
             p += 4) {
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

Pumila1::NNResult Pumila1::NNModel::forward(const Eigen::MatrixXd &in) const {
    if (in.cols() != IN_NODES) {
        throw std::invalid_argument("invalid size in forward: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(IN_NODES));
    }
    NNResult ret;
    ret.in = in;
    ret.hidden = in * getMatrixIH();
    ret.hidden.leftCols<1>().array() = 1; // bias
    ret.hidden =
        (1 / (1 + (ret.hidden.array() * alpha).exp())).matrix(); // sigmoid
    assert(ret.hidden.cols() == HIDDEN_NODES);
    ret.q = ret.hidden * getMatrixHQ();
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila1::NNModel::backward(const NNResult &result,
                                const Eigen::VectorXd &diff) {
    if (diff.rows() * 24 != result.q.rows() && diff.rows() != result.q.rows()) {
        throw std::invalid_argument("invalid diff size in backward: q -> " +
                                    std::to_string(result.q.rows()) +
                                    ", diff -> " + std::to_string(diff.rows()));
    }
    // f2(u) = u -> delta_2 = diff * f2'(u) = diff;
    // f1(u) = 1 / (1 + exp(-au))
    Eigen::VectorXd delta_2(result.q.rows());
    for (int r = 0; r < delta_2.rows(); r += diff.rows()) {
        delta_2.middleRows(r, diff.rows()) = diff;
    }
    auto f1_dif = alpha * result.hidden.array() * (1 - result.hidden.array());
    auto delta_1 =
        ((delta_2 * matrix_hq.transpose()).array() * f1_dif).matrix();
    assert(delta_1.rows() == result.q.rows());
    assert(delta_1.cols() == NNModel::HIDDEN_NODES);
    auto diff_hq =
        learning_rate * result.hidden.transpose() * delta_2 / delta_2.rows();
    auto diff_ih =
        learning_rate * result.in.transpose() * delta_1 / delta_1.rows();
    {
        std::lock_guard lock(matrix_m);
        matrix_hq += diff_hq;
        matrix_ih += diff_ih;
    }
}

double Pumila1::calcReward(const FieldState &field) {
    double r = -1;
    std::size_t max_y = 0;
    for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
        auto y = field.getHeight(x);
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
    r += field.prev_chain_score / 4;
    return r;
}

int Pumila1::getAction(const FieldState &field) {
    NNResult fw_result = main.forward(getInNodes(field).second);
    int action = 0;
    fw_result.q.maxCoeff(&action);
    return action;
}
int Pumila1::getActionRnd(const FieldState &field) {
    NNResult fw_result = main.forward(getInNodes(field).second);
    fw_result.q.array() -= fw_result.q.minCoeff();
    fw_result.q.array() /= fw_result.q.sum();
    double r = static_cast<double>(rnd() - rnd.min()) / (rnd.max() - rnd.min());
    for (int a = 0; a < ACTIONS_NUM; a++) {
        r -= fw_result.q(a, 0);
        if (r <= 0) {
            return a;
        }
    }
    return ACTIONS_NUM - 1;
}
void Pumila1::learnStep(const FieldState &field) {
    std::thread([this, pumila = shared_from_this(), field] {
        auto [field_next, in] = getInNodes(field);
        auto fw_result =
            pool.submit_task(
                    [&] { return main.forward(NNModel::truncateInNodes(in)); })
                .get();

        Eigen::VectorXd delta_2(fw_result.q.rows());
        double diff_sum = 0;
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            double diff =
                (calcReward(field_next[a]) +
                 gamma * target.forward(getInNodes(field_next[a]).second)
                             .q.maxCoeff()) -
                fw_result.q(a, 0);
            diff_sum += diff;
            delta_2(a, 0) = diff;
        }
        pool.submit_task([&] {
                for (int r = ACTIONS_NUM; r < delta_2.rows();
                     r += ACTIONS_NUM) {
                    delta_2.middleRows(r, ACTIONS_NUM) =
                        delta_2.topRows(ACTIONS_NUM);
                }
                main.backward(fw_result, delta_2);
            })
            .get();
        if (++back_count > 10) {
            back_count = 0;
            target = main;
        }
        mean_diff = diff_sum / ACTIONS_NUM;
    }).detach();
}

} // namespace pumila
