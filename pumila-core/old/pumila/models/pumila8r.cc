#include <pumila/models/pumila8r.h>

namespace PUMILA_NS {
Pumila8r::Pumila8r(int hidden_nodes) : Pumila7r(hidden_nodes) {}


void Pumila8r::learnStep(std::shared_ptr<FieldState> field) {
    {
        std::unique_lock lock(learning_m);
        learning_cond.wait(lock, [&] { return step_started < BATCH_SIZE; });
        step_started++;
    }
    auto pumila = shared_from_this();
    auto next = Pumila2::getInNodes(field).share();
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
    pool.detach_task([this, pumila, next, next2 = std::move(next2),
                      next3 = std::move(next3)] {
        auto in_t = Pumila2::NNModel::transposeInNodes(next.get().in);
        NNResult fw_result;
        std::shared_ptr<NNModel> mmain;
        {
            std::shared_lock lock(main_m);
            mmain = main;
            // mainが途中で置き換えられても問題ない
        }
        fw_result = mmain->forward(in_t);
        Eigen::VectorXd delta_2(fw_result.q.rows());
        for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
            // q(a3, a2)
            Eigen::MatrixXd fw_next3_q(ACTIONS_NUM, ACTIONS_NUM);
            for (std::size_t a2 = 0; a2 < ACTIONS_NUM; a2++) {
                fw_next3_q.middleCols<1>(a2) =
                    GAMMA * GAMMA * mmain->forward(next3[a][a2].get().in).q;
                fw_next3_q.middleCols<1>(a2).array() +=
                    GAMMA * calcReward(next2[a].get().field_next[a2]);
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
                               fw_next3_q.maxCoeff()) -
                              fw_result.q(r, 0);
                delta_2(r, 0) = diff;
            }
        }
        delta_2.array() /= BATCH_SIZE;
        target->backward(fw_result, delta_2);
        {
            std::unique_lock lock(learning_m);
            step_finished++;
            if (step_finished >= BATCH_SIZE) {
                pool.detach_task([this, pumila] {
                    {
                        std::lock_guard lock(main_m);
                        main = target->copy();
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
