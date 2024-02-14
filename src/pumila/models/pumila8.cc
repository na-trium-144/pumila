#include <pumila/models/pumila8.h>

namespace PUMILA_NS {
Pumila8::Pumila8(int hidden_nodes) : Pumila7(hidden_nodes) {}

void Pumila8::learnStep(std::shared_ptr<FieldState> field) {
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
        std::array<std::array<std::shared_future<InFeatures>, ACTIONS_NUM>,
                   ACTIONS_NUM>
            next3;
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            next2[a] = Pumila2::getInNodes(next, a).share();
        }
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
                next3[a][a2] = Pumila2::getInNodes(next2[a], a2).share();
            }
        }
        auto in_t = Pumila2::NNModel::truncateInNodes(next.get().in);
        NNResult fw_result;
        {
            std::shared_lock lock(target_m);
            fw_result = target.forward(in_t);
        }
        pool.detach_task([this, pumila, next, next2, next3, fw_result]() {
            Eigen::VectorXd delta_2(fw_result.q.rows());
            for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
                // q(a3, a2)
                Eigen::MatrixXd fw_next3_q(ACTIONS_NUM, ACTIONS_NUM);
                for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
                    std::shared_lock lock(target_m);
                    fw_next3_q.middleCols<1>(a2) =
                        target.forward(next3[a][a2].get().in).q;
                }
                for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
                    for (std::size_t a3 = 0; a3 < ACTIONS_NUM; a3++) {
                        if (!next3[a][a2].get().field_next[a3]->is_valid ||
                            next3[a][a2].get().field_next[a3]->is_over) {
                            fw_next3_q(a3, a2) = fw_next3_q.minCoeff();
                        }
                    }
                }
                for (int r = a; r < delta_2.rows(); r += ACTIONS_NUM) {
                    double diff = (calcReward(next.get().field_next[a]) +
                                   GAMMA * fw_next3_q.maxCoeff()) -
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
