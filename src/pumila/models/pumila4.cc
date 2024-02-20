#include <pumila/models/pumila4.h>

namespace PUMILA_NS {
Pumila4::NNModel::NNModel(double learning_rate)
    : learning_rate(learning_rate),
      matrix_ih(std::make_shared<Eigen::MatrixXd>(
          Eigen::MatrixXd::Random(IN_NODES, HIDDEN_NODES))),
      matrix_hq(std::make_shared<Eigen::VectorXd>(
          Eigen::VectorXd::Random(HIDDEN_NODES))) {
    matrix_ih->array() += 1;
    matrix_hq->array() += 1;
}

Pumila4::Pumila4(double learning_rate)
    : Pumila(), main(learning_rate), target(main) {}

void Pumila4::load(std::istream &is) {
    std::lock_guard lock_target(target_m);
    {
        std::lock_guard lock_matrix(main.matrix_m);
        is.read(reinterpret_cast<char *>(main.matrix_ih->data()),
                main.matrix_ih->rows() * main.matrix_ih->cols() *
                    sizeof(double));
        is.read(reinterpret_cast<char *>(main.matrix_hq->data()),
                main.matrix_hq->rows() * main.matrix_hq->cols() *
                    sizeof(double));
    }
    target = main;
}
void Pumila4::save(std::ostream &os) {
    std::lock_guard lock_matrix(main.matrix_m);
    os.write(reinterpret_cast<char *>(main.matrix_ih->data()),
             main.matrix_ih->rows() * main.matrix_ih->cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main.matrix_hq->data()),
             main.matrix_hq->rows() * main.matrix_hq->cols() * sizeof(double));
}
Pumila4::InFeatureSingle
Pumila4::getInNodeSingle(std::shared_ptr<FieldState> field, int a) {
    InFeatureSingle feat;
    feat.field_next = field->copy();
    feat.field_next->put(actions[a]);
    feat.field_next->popNext();
    std::vector<Chain> chains = feat.field_next->deleteChainRecurse();

    feat.in = Eigen::VectorXd::Zero(NNModel::IN_NODES);
    NNModel::InNodes &in_nodes =
        *reinterpret_cast<NNModel::InNodes *>(feat.in.data());
    in_nodes.bias = 1;
    auto chain_all = feat.field_next->calcChainAll();
    for (std::size_t y = 0; y < FieldState::HEIGHT; y++) {
        for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
            int p;
            switch (feat.field_next->get(x, y)) {
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
                in_nodes.field_colors[(y * FieldState::WIDTH + x) * 4 + p] = 1;
                in_nodes.field_chains[(y * FieldState::WIDTH + x) * 4 + p] =
                    chain_all[y][x];
            }
        }
    }
    in_nodes.score_diff = 0;
    for (const auto &c : chains) {
        in_nodes.score_diff += c.score() / 40.0;
    }
    return feat;
}


Pumila4::NNResult Pumila4::NNModel::forward(const Eigen::MatrixXd &in) const {
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
    ret.hidden = (ret.hidden.array() * (ret.hidden.array() > 0).cast<double>())
                     .matrix(); // relu
    assert(ret.hidden.cols() == HIDDEN_NODES);
    ret.q = ret.hidden * *ret.matrix_hq;
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila4::NNModel::backward(const NNResult &result,
                                const Eigen::VectorXd &diff) {
    if (diff.rows() * 24 != result.q.rows() && diff.rows() != result.q.rows()) {
        throw std::invalid_argument("invalid diff size in backward: q -> " +
                                    std::to_string(result.q.rows()) +
                                    ", diff -> " + std::to_string(diff.rows()));
    }
    Eigen::VectorXd delta_2(result.q.rows());
    for (int r = 0; r < delta_2.rows(); r += diff.rows()) {
        delta_2.middleRows(r, diff.rows()) = diff;
    }
    auto f1_dif = (result.hidden.array() > 0).cast<double>() * 1;
    auto delta_1 =
        ((delta_2 * result.matrix_hq->transpose()).array() * f1_dif).matrix();
    assert(delta_1.rows() == result.q.rows());
    assert(delta_1.cols() == NNModel::HIDDEN_NODES);
    auto diff_hq =
        learning_rate * result.hidden.transpose() * delta_2 / delta_2.rows();
    auto diff_ih =
        learning_rate * result.in.transpose() * delta_1 / delta_1.rows();
    {
        std::lock_guard lock(matrix_m);
        matrix_hq = std::make_shared<Eigen::VectorXd>(*matrix_hq + diff_hq);
        matrix_ih = std::make_shared<Eigen::MatrixXd>(*matrix_ih + diff_ih);
    }
}

int Pumila4::getActionRnd(std::shared_ptr<FieldState> field, double rnd_p) {
    NNResult fw_result;
    auto in_feat = getInNodes(field).get();
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

void Pumila4::learnStep(std::shared_ptr<FieldState> field) {
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
