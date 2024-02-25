#include <pumila/models/pumila3.h>

namespace PUMILA_NS {
Pumila3::Pumila3(double learning_rate) : Pumila2(0.01, 0.9, learning_rate) {}


int Pumila3::getActionRnd(std::shared_ptr<FieldState> field, double rnd_p) {
    NNResult fw_result;
    auto in_feat = getInNodes(field).get();
    fw_result = main.forward(in_feat.in);
    for (int a2 = 0; a2 < ACTIONS_NUM; a2++) {
        if (!in_feat.field_next[a2]->is_valid ||
            in_feat.field_next[a2]->is_over) {
            fw_result.q(a2, 0) = fw_result.q.minCoeff();
        }
    }
    int action = 0;
    setActionCoeff(fw_result.q.maxCoeff(&action));
    if (getRndD() >= rnd_p) {
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

void Pumila3::learnStep(std::shared_ptr<FieldState> field) {
    {
        std::unique_lock lock(learning_m);
        if (batch_count >= BATCH_SIZE) {
            learning_cond.wait(lock, [&] { return batch_count < BATCH_SIZE; });
        }
        batch_count++;
    }
    auto pumila = shared_from_this();
    auto next = getInNodes(field).share();
    pool.detach_task([this, pumila, next] {
        std::array<std::shared_future<InFeatures>, ACTIONS_NUM> next2;
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            next2[a] = getInNodes(next, a).share();
        }
        auto in_t = NNModel::truncateInNodes(next.get().in);
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
                                   gamma * fw_next.q.maxCoeff()) -
                                  fw_result.q(r, 0);
                    delta_2(r, 0) = diff;
                }
            }
            {
                std::unique_lock lock(learning_m);
                delta_2.array() /= batch_count;
                mean_diff = delta_2.sum() / delta_2.rows();
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
