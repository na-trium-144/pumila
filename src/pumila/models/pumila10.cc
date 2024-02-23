#include <pumila/models/pumila10.h>

namespace PUMILA_NS {
Pumila10::Pumila10(int hidden_nodes, double gamma)
    : Pumila(), gamma(gamma), main(hidden_nodes), target(main) {}

void Pumila10::load(std::istream &is) {
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
void Pumila10::save(std::ostream &os) {
    std::shared_lock lock_main(main_m);
    os.write(reinterpret_cast<char *>(&gamma), sizeof(gamma));
    os.write(reinterpret_cast<char *>(&main.hidden_nodes),
             sizeof(main.hidden_nodes));
    os.write(reinterpret_cast<char *>(main.matrix_ih.data()),
             main.matrix_ih.rows() * main.matrix_ih.cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main.matrix_hq.data()),
             main.matrix_hq.rows() * main.matrix_hq.cols() * sizeof(double));
}

Pumila10::NNResult Pumila10::forward(const Eigen::MatrixXd &in) const {
    if (in.cols() != NNModel::IN_NODES) {
        throw std::invalid_argument("invalid size in forward: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(NNModel::IN_NODES));
    }
    std::shared_lock lock(main_m);
    NNResult ret;
    ret.in = in;
    ret.hidden = in * main.matrix_ih;
    ret.hidden.leftCols<1>().array() = 1; // bias
    ret.hidden = (1 / (1 + (-NNModel::ALPHA * ret.hidden.array()).exp()))
                     .matrix(); // sigmoid
    assert(ret.hidden.cols() == main.hidden_nodes);
    ret.q = ret.hidden * main.matrix_hq;
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila10::backward(const NNResult &result, const Eigen::VectorXd &diff) {
    if (diff.rows() * 24 != result.q.rows() && diff.rows() != result.q.rows()) {
        throw std::invalid_argument("invalid diff size in backward: q -> " +
                                    std::to_string(result.q.rows()) +
                                    ", diff -> " + std::to_string(diff.rows()));
    }
    std::shared_lock lock(main_m);
    // f2(u) = u -> delta_2 = diff * f2'(u) = diff;
    // f1(u) = 1 / (1 + exp(-au))
    Eigen::VectorXd delta_2(result.q.rows());
    for (int r = 0; r < delta_2.rows(); r += diff.rows()) {
        delta_2.middleRows(r, diff.rows()) = diff;
    }
    auto f1_dif =
        NNModel::ALPHA * result.hidden.array() * (1 - result.hidden.array());
    auto delta_1 =
        ((delta_2 * main.matrix_hq.transpose()).array() * f1_dif).matrix();
    assert(delta_1.rows() == result.q.rows());
    assert(delta_1.cols() == main.hidden_nodes);
    auto diff_hq = NNModel::LEARNING_RATE * result.hidden.transpose() *
                   delta_2 / delta_2.rows();
    auto diff_ih = NNModel::LEARNING_RATE * result.in.transpose() * delta_1 /
                   delta_1.rows();
    {
        std::lock_guard lock_target(target_m);
        target.matrix_hq += diff_hq;
        target.matrix_ih += diff_ih;
    }
}

double Pumila10::getActionCoeff(std::shared_ptr<FieldState> field) {
    NNResult fw_result;
    auto in_feat = Pumila8s::getInNodes(*field).get();
    fw_result = forward(in_feat.in);
    for (int a2 = 0; a2 < ACTIONS_NUM; a2++) {
        if (!in_feat.each[a2].get().field_next.is_valid ||
            in_feat.each[a2].get().field_next.is_over) {
            fw_result.q(a2, 0) = fw_result.q.minCoeff();
        }
    }
    return fw_result.q.maxCoeff();
}
int Pumila10::getActionRnd(std::shared_ptr<FieldState> field, double rnd_p) {
    NNResult fw_result;
    auto in_feat = Pumila8s::getInNodes(*field).get();
    fw_result = forward(in_feat.in);
    for (int a2 = 0; a2 < ACTIONS_NUM; a2++) {
        if (!in_feat.each[a2].get().field_next.is_valid ||
            in_feat.each[a2].get().field_next.is_over) {
            fw_result.q(a2, 0) = fw_result.q.minCoeff();
        }
    }
    if (getRndD() >= rnd_p) {
        int action = 0;
        fw_result.q.maxCoeff(&action);
        return action;
    } else {
        fw_result.q.array() -= fw_result.q.minCoeff();
        fw_result.q.array() /= fw_result.q.sum();
        double r = getRndD();
        for (int a = 0; a < ACTIONS_NUM; a++) {
            r -= fw_result.q(a, 0);
            if (r <= 0) {
                return a;
            }
        }
        return ACTIONS_NUM - 1;
    }
}

void Pumila10::learnStep(std::shared_ptr<FieldState> field) {
    {
        std::unique_lock lock(learning_m);
        learning_cond.wait(lock, [&] { return step_started < BATCH_SIZE; });
        step_started++;
    }
    auto pumila = shared_from_this();
    auto next = Pumila8s::getInNodes(*field).share();
    std::array<std::shared_future<InFeatures>, ACTIONS_NUM> next2;
    std::array<std::array<std::shared_future<InFeatures>, ACTIONS_NUM>,
               ACTIONS_NUM>
        next3;
    for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
        next2[a] = Pumila8s::getInNodes(next, a).share();
    }
    for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
        for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
            next3[a][a2] = Pumila8s::getInNodes(next2[a], a2).share();
        }
    }
    pool.detach_task([this, pumila, next, next2 = std::move(next2),
                      next3 = std::move(next3)] {
        auto in_t = Pumila2::NNModel::truncateInNodes(next.get().in);
        NNResult fw_result;
        fw_result = forward(in_t);
        Eigen::VectorXd delta_2(fw_result.q.rows());
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            // q(a3, a2)
            Eigen::MatrixXd fw_next3_q(ACTIONS_NUM, ACTIONS_NUM);
            for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
                fw_next3_q.middleCols<1>(a2) =
                    gamma * gamma * forward(next3[a][a2].get().in).q;
                fw_next3_q.middleCols<1>(a2).array() +=
                    gamma *
                    calcReward(next2[a].get().each[a2].get().field_next);
            }
            for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
                for (std::size_t a3 = 0; a3 < ACTIONS_NUM; a3++) {
                    if (!next3[a][a2]
                             .get()
                             .each[a3]
                             .get()
                             .field_next.is_valid ||
                        next3[a][a2].get().each[a3].get().field_next.is_over) {
                        fw_next3_q(a3, a2) = fw_next3_q.minCoeff();
                    }
                }
            }
            for (int r = a; r < delta_2.rows(); r += ACTIONS_NUM) {
                double diff = (calcReward(next.get().each[a].get().field_next) +
                               fw_next3_q.maxCoeff()) -
                              fw_result.q(r, 0);
                delta_2(r, 0) = diff;
            }
        }
        delta_2.array() /= BATCH_SIZE;
        backward(fw_result, delta_2);
        {
            std::unique_lock lock(learning_m);
            step_finished++;
            if (step_finished >= BATCH_SIZE) {
                pool.detach_task([this, pumila] {
                    {
                        std::lock_guard lock(main_m);
                        main = target;
                    }
                    {
                        std::unique_lock lock(learning_m);
                        step_started = 0;
                        step_finished = 0;
                        learning_cond.notify_all();
                    }
                });
            }
        }
    });
}

} // namespace PUMILA_NS
