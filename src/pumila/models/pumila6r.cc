#include <pumila/models/pumila6r.h>
#include <cassert>
#include <stdexcept>
#include <string>

namespace PUMILA_NS {
Pumila6r::NNModel::NNModel(int hidden_nodes)
    : std::enable_shared_from_this<Pumila6r::NNModel>(),
      hidden_nodes(hidden_nodes),
      matrix_ih(Eigen::MatrixXd::Random(IN_NODES, hidden_nodes)),
      matrix_hq(Eigen::VectorXd::Random(hidden_nodes)) {}

Pumila6r::Pumila6r(int hidden_nodes)
    : Pumila(), main(std::make_shared<NNModel>(hidden_nodes)),
      target(main->copy()) {}

void Pumila6r::load(std::istream &is) {
    std::lock_guard lock_main(main_m);
    {
        std::lock_guard lock_matrix(main->matrix_m);
        is.read(reinterpret_cast<char *>(&main->hidden_nodes),
                sizeof(main->hidden_nodes));
        main->matrix_ih =
            Eigen::MatrixXd(NNModel::IN_NODES, main->hidden_nodes);
        main->matrix_hq = Eigen::VectorXd(main->hidden_nodes);
        is.read(reinterpret_cast<char *>(main->matrix_ih.data()),
                main->matrix_ih.rows() * main->matrix_ih.cols() *
                    sizeof(double));
        is.read(reinterpret_cast<char *>(main->matrix_hq.data()),
                main->matrix_hq.rows() * main->matrix_hq.cols() *
                    sizeof(double));
    }
    target = main->copy();
}
void Pumila6r::save(std::ostream &os) {
    std::shared_lock lock_main(main_m);
    std::shared_lock lock_matrix(main->matrix_m);
    os.write(reinterpret_cast<char *>(&main->hidden_nodes),
             sizeof(main->hidden_nodes));
    os.write(reinterpret_cast<char *>(main->matrix_ih.data()),
             main->matrix_ih.rows() * main->matrix_ih.cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main->matrix_hq.data()),
             main->matrix_hq.rows() * main->matrix_hq.cols() * sizeof(double));
}

Pumila6r::NNResult Pumila6r::NNModel::forward(const Eigen::MatrixXd &in) const {
    if (in.cols() != IN_NODES) {
        throw std::invalid_argument("invalid size in forward: in -> " +
                                    std::to_string(in.cols()) + ", expected " +
                                    std::to_string(IN_NODES));
    }
    std::shared_lock lock_matrix(matrix_m);
    NNResult ret;
    ret.model = shared_from_this();
    ret.in = in;
    ret.hidden = in * matrix_ih;
    ret.hidden.leftCols<1>().array() = 1; // bias
    ret.hidden =
        (1 / (1 + (-ALPHA * ret.hidden.array()).exp())).matrix(); // sigmoid
    assert(ret.hidden.cols() == hidden_nodes);
    ret.q = ret.hidden * matrix_hq;
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila6r::NNModel::backward(const NNResult &result,
                                 const Eigen::VectorXd &diff) {
    if (diff.rows() * 24 != result.q.rows() && diff.rows() != result.q.rows()) {
        throw std::invalid_argument("invalid diff size in backward: q -> " +
                                    std::to_string(result.q.rows()) +
                                    ", diff -> " + std::to_string(diff.rows()));
    }
    std::shared_lock lock_matrix(result.model->matrix_m);
    // f2(u) = u -> delta_2 = diff * f2'(u) = diff;
    // f1(u) = 1 / (1 + exp(-au))
    Eigen::VectorXd delta_2(result.q.rows());
    for (int r = 0; r < delta_2.rows(); r += diff.rows()) {
        delta_2.middleRows(r, diff.rows()) = diff;
    }
    auto f1_dif = ALPHA * result.hidden.array() * (1 - result.hidden.array());
    auto delta_1 =
        ((delta_2 * result.model->matrix_hq.transpose()).array() * f1_dif)
            .matrix();
    assert(delta_1.rows() == result.q.rows());
    assert(delta_1.cols() == hidden_nodes);
    auto diff_hq =
        LEARNING_RATE * result.hidden.transpose() * delta_2 / delta_2.rows();
    auto diff_ih =
        LEARNING_RATE * result.in.transpose() * delta_1 / delta_1.rows();
    {
        std::lock_guard lock(matrix_m);
        matrix_hq += diff_hq;
        matrix_ih += diff_ih;
    }
}

int Pumila6r::getActionRnd(std::shared_ptr<FieldState> field, double rnd_p) {
    NNResult fw_result;
    auto in_feat = Pumila2::getInNodes(field).get();
    std::shared_ptr<NNModel> mmain;
    {
        std::shared_lock lock(main_m);
        mmain = main;
        // mainが途中で置き換えられても問題ない
    }
    fw_result = mmain->forward(in_feat.in);
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

void Pumila6r::learnStep(std::shared_ptr<FieldState> field) {
    {
        std::unique_lock lock(learning_m);
        learning_cond.wait(lock, [&] { return step_started < BATCH_SIZE; });
        step_started++;
    }
    auto pumila = shared_from_this();
    auto next = Pumila2::getInNodes(field).share();
    std::array<std::shared_future<InFeatures>, ACTIONS_NUM> next2;
    for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
        next2[a] = Pumila2::getInNodes(next, a).share();
    }
    pool.detach_task([this, pumila, next, next2 = std::move(next2)] {
        auto in_t = Pumila2::NNModel::truncateInNodes(next.get().in);
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
            NNResult fw_next;
            fw_next = mmain->forward(next2[a].get().in);
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
