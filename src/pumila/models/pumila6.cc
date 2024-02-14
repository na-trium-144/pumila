#include <pumila/models/pumila6.h>
#include <cassert>
#include <stdexcept>
#include <string>

namespace PUMILA_NS {
Pumila6::NNModel::NNModel(int hidden_nodes)
    : hidden_nodes(hidden_nodes),
      matrix_ih(std::make_shared<Eigen::MatrixXd>(
          Eigen::MatrixXd::Random(IN_NODES, hidden_nodes))),
      matrix_hq(std::make_shared<Eigen::VectorXd>(
          Eigen::VectorXd::Random(hidden_nodes))) {}

Pumila6::Pumila6(int hidden_nodes)
    : Pumila(), main(hidden_nodes), target(main) {}

void Pumila6::load(std::istream &is) {
    std::lock_guard lock_target(target_m);
    {
        std::lock_guard lock_matrix(main.matrix_m);
        is.read(reinterpret_cast<char *>(&main.hidden_nodes),
                sizeof(main.hidden_nodes));
        main.matrix_ih = std::make_shared<Eigen::MatrixXd>(NNModel::IN_NODES,
                                                           main.hidden_nodes);
        main.matrix_hq = std::make_shared<Eigen::VectorXd>(main.hidden_nodes);
        is.read(reinterpret_cast<char *>(main.matrix_ih->data()),
                main.matrix_ih->rows() * main.matrix_ih->cols() *
                    sizeof(double));
        is.read(reinterpret_cast<char *>(main.matrix_hq->data()),
                main.matrix_hq->rows() * main.matrix_hq->cols() *
                    sizeof(double));
    }
    target = main;
}
void Pumila6::save(std::ostream &os) {
    std::lock_guard lock_matrix(main.matrix_m);
    os.write(reinterpret_cast<char *>(&main.hidden_nodes),
             sizeof(main.hidden_nodes));
    os.write(reinterpret_cast<char *>(main.matrix_ih->data()),
             main.matrix_ih->rows() * main.matrix_ih->cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main.matrix_hq->data()),
             main.matrix_hq->rows() * main.matrix_hq->cols() * sizeof(double));
}

Pumila6::NNResult Pumila6::NNModel::forward(const Eigen::MatrixXd &in) const {
    if (in.cols() != IN_NODES) {
        throw std::invalid_argument("invalid size in forward: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(IN_NODES));
    }
    NNResult ret;
    {
        std::lock_guard lock(matrix_m);
        ret.matrix_ih = matrix_ih;
        ret.matrix_hq = matrix_hq;
    }
    ret.in = in;
    ret.hidden = in * *ret.matrix_ih;
    ret.hidden.leftCols<1>().array() = 1; // bias
    ret.hidden =
        (1 / (1 + (-ALPHA * ret.hidden.array()).exp())).matrix(); // sigmoid
    assert(ret.hidden.cols() == hidden_nodes);
    ret.q = ret.hidden * *ret.matrix_hq;
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila6::NNModel::backward(const NNResult &result,
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
    auto f1_dif = ALPHA * result.hidden.array() * (1 - result.hidden.array());
    auto delta_1 =
        ((delta_2 * result.matrix_hq->transpose()).array() * f1_dif).matrix();
    assert(delta_1.rows() == result.q.rows());
    assert(delta_1.cols() == hidden_nodes);
    auto diff_hq =
        LEARNING_RATE * result.hidden.transpose() * delta_2 / delta_2.rows();
    auto diff_ih =
        LEARNING_RATE * result.in.transpose() * delta_1 / delta_1.rows();
    {
        std::lock_guard lock(matrix_m);
        matrix_hq = std::make_shared<Eigen::VectorXd>(*matrix_hq + diff_hq);
        matrix_ih = std::make_shared<Eigen::MatrixXd>(*matrix_ih + diff_ih);
    }
}

int Pumila6::getActionRnd(std::shared_ptr<FieldState> field, double rnd_p) {
    NNResult fw_result;
    auto in_feat = Pumila2::getInNodes(field).get();
    fw_result = main.forward(in_feat.in);
    for (int a2 = 0; a2 < ACTIONS_NUM; a2++) {
        if (!in_feat.field_next[a2]->is_valid ||
            in_feat.field_next[a2]->is_over) {
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

void Pumila6::learnStep(std::shared_ptr<FieldState> field) {
    {
        std::unique_lock lock(learning_m);
        if (batch_count >= BATCH_SIZE) {
            learning_cond.wait(lock, [&] { return batch_count < BATCH_SIZE; });
        }
        batch_count++;
    }
    auto pumila = shared_from_this();
    auto next = Pumila2::getInNodes(field).share();
    pool.detach_task([this, pumila, next] {
        std::array<std::shared_future<InFeatures>, ACTIONS_NUM> next2;
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            next2[a] = Pumila2::getInNodes(next, a).share();
        }
        auto in_t = Pumila2::NNModel::truncateInNodes(next.get().in);
        NNResult fw_result;
        {
            std::shared_lock lock(target_m);
            fw_result = target.forward(in_t);
        }
        pool.detach_task([this, pumila, next, next2, fw_result]() {
            Eigen::VectorXd delta_2(fw_result.q.rows());
            for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
                NNResult fw_next;
                {
                    std::shared_lock lock(target_m);
                    fw_next = target.forward(next2[a].get().in);
                }
                for (int a2 = 0; a2 < ACTIONS_NUM; a2++) {
                    if (!next2[a].get().field_next[a2]->is_valid ||
                        next2[a].get().field_next[a2]->is_over) {
                        fw_next.q(a2, 0) = fw_next.q.minCoeff();
                    }
                }
                for (int r = a; r < delta_2.rows(); r += ACTIONS_NUM) {
                    double diff = (calcReward(next.get().field_next[a]) +
                                   GAMMA * fw_next.q.maxCoeff()) -
                                  fw_result.q(r, 0);
                    delta_2(r, 0) = diff;
                }
            }
            {
                std::unique_lock lock(learning_m);
                delta_2.array() /= batch_count;
                // mean_diff = delta_2.sum() / delta_2.rows();
            }
            pool.detach_task([this, pumila, fw_result, delta_2] {
                main.backward(fw_result, delta_2);
                {
                    std::lock_guard lock(target_m);
                    target = main;
                }
                {
                    std::unique_lock lock(learning_m);
                    batch_count--;
                    learning_cond.notify_one();
                }
            });
        });
    });
}

} // namespace PUMILA_NS
