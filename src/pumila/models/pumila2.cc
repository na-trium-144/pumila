#include <pumila/models/pumila2.h>
#include <cassert>
#include <stdexcept>
#include <string>

namespace PUMILA_NS {
Pumila2::NNModel::NNModel(double alpha, double learning_rate)
    : alpha(alpha), learning_rate(learning_rate),
      matrix_ih(std::make_shared<Eigen::MatrixXd>(
          Eigen::MatrixXd::Random(IN_NODES, HIDDEN_NODES))),
      matrix_hq(std::make_shared<Eigen::VectorXd>(
          Eigen::VectorXd::Random(HIDDEN_NODES))) {}

Pumila2::Pumila2(double alpha, double gamma, double learning_rate)
    : Pumila(), gamma(gamma), main(alpha, learning_rate), target(main) {}

void Pumila2::load(std::istream &is) {
    std::lock_guard lock_target(target_m);
    {
        std::lock_guard lock_matrix(main.matrix_m);
        is.read(reinterpret_cast<char *>(&main.alpha), sizeof(main.alpha));
        is.read(reinterpret_cast<char *>(main.matrix_ih->data()),
                main.matrix_ih->rows() * main.matrix_ih->cols() *
                    sizeof(double));
        is.read(reinterpret_cast<char *>(main.matrix_hq->data()),
                main.matrix_hq->rows() * main.matrix_hq->cols() *
                    sizeof(double));
    }
    target = main;
}
void Pumila2::save(std::ostream &os) {
    std::lock_guard lock_matrix(main.matrix_m);
    os.write(reinterpret_cast<char *>(&main.alpha), sizeof(main.alpha));
    os.write(reinterpret_cast<char *>(main.matrix_ih->data()),
             main.matrix_ih->rows() * main.matrix_ih->cols() * sizeof(double));
    os.write(reinterpret_cast<char *>(main.matrix_hq->data()),
             main.matrix_hq->rows() * main.matrix_hq->cols() * sizeof(double));
}

Pumila2::InFeatureSingle Pumila2::getInNodeSingle(std::shared_ptr<FieldState> field,
                                                  int a) {
    InFeatureSingle feat;
    feat.field_next = field->copy();
    feat.field_next->put(actions[a]);
    feat.field_next->next.pop_front();
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
        in_nodes.score_diff += c.score();
    }
    return feat;
}

std::future<Pumila2::InFeatures> Pumila2::getInNodes(std::shared_ptr<FieldState> field) {
    std::array<std::shared_future<InFeatureSingle>, ACTIONS_NUM> feat;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        feat[a] =
            pool.submit_task([field, a] { return getInNodeSingle(field, a); })
                .share();
    }
    return pool.submit_task([feat]() {
        InFeatures feats;
        feats.in = Eigen::MatrixXd(ACTIONS_NUM, NNModel::IN_NODES);
        int a = 0;
        for (auto &res : feat) {
            auto r = res.get();
            feats.field_next[a] = r.field_next;
            feats.in.middleRows<1>(a) = r.in.transpose();
            a++;
        }
        return feats;
    });
}
std::future<Pumila2::InFeatures>
Pumila2::getInNodes(std::shared_future<InFeatures> feature, int feat_a) {
    std::array<std::shared_future<InFeatureSingle>, ACTIONS_NUM> feat;
    for (int a = 0; a < ACTIONS_NUM; a++) {
        feat[a] =
            pool.submit_task([feature, feat_a, a] {
                    return getInNodeSingle(feature.get().field_next[feat_a], a);
                })
                .share();
    }
    return pool.submit_task([feat]() {
        InFeatures feats;
        feats.in = Eigen::MatrixXd(ACTIONS_NUM, NNModel::IN_NODES);
        int a = 0;
        for (auto &res : feat) {
            auto r = res.get();
            feats.field_next[a] = r.field_next;
            feats.in.middleRows<1>(a) = r.in.transpose();
            a++;
        }
        return feats;
    });
}

Eigen::MatrixXd Pumila2::NNModel::truncateInNodes(const Eigen::MatrixXd &in) {
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
        for (std::size_t p = 0;
             p < FieldState::WIDTH * FieldState::HEIGHT * 4 * 2; p += 4) {
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

Pumila2::NNResult Pumila2::NNModel::forward(const Eigen::MatrixXd &in) const {
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
        (1 / (1 + (-alpha * ret.hidden.array()).exp())).matrix(); // sigmoid
    assert(ret.hidden.cols() == HIDDEN_NODES);
    ret.q = ret.hidden * *ret.matrix_hq;
    assert(ret.q.cols() == 1);
    return ret;
}
void Pumila2::NNModel::backward(const NNResult &result,
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

double Pumila2::calcRewardS(std::shared_ptr<FieldState> field) {
    double r = -1;
    std::size_t max_y = 0;
    for (std::size_t x = 0; x < FieldState::WIDTH; x++) {
        auto y = field->getHeight(x);
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
    r += field->prev_chain_score / 4;
    return r;
}

int Pumila2::getActionRnd(std::shared_ptr<FieldState> field, double rnd_p) {
    NNResult fw_result;
    fw_result = main.forward(getInNodes(field).get().in);
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
void Pumila2::learnStep(std::shared_ptr<FieldState> field) {
    {
        std::unique_lock lock(learning_m);
        if (batch_count >= BATCH_SIZE) {
            learning_cond.wait(lock, [&] { return batch_count < BATCH_SIZE; });
        }
        batch_count++;
    }
    auto pumila = shared_from_this();
    auto next = getInNodes(field).share();
    auto fw_result = pool.submit_task([this, pumila, next] {
                             auto in_t =
                                 NNModel::truncateInNodes(next.get().in);
                             {
                                 std::shared_lock lock(target_m);
                                 return target.forward(in_t);
                             }
                         })
                         .share();
    std::array<std::shared_future<InFeatures>, ACTIONS_NUM> next2;
    for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
        next2[a] = getInNodes(next, a).share();
    }
    auto delta_2 =
        pool.submit_task([this, pumila, next, next2, fw_result]() {
                Eigen::VectorXd delta_2(fw_result.get().q.rows());
                for (std::size_t a = 0; a < ACTIONS_NUM; a++) {
                    NNResult fw_next;
                    {
                        std::shared_lock lock(target_m);
                        fw_next = target.forward(next2[a].get().in);
                    }
                    for (int r = a; r < delta_2.rows(); r += ACTIONS_NUM) {
                        double diff = (calcReward(next.get().field_next[a]) +
                                       gamma * fw_next.q.maxCoeff()) -
                                      fw_result.get().q(r, 0);
                        delta_2(r, 0) = diff;
                    }
                }
                {
                    std::unique_lock lock(learning_m);
                    delta_2.array() /= batch_count;
                    mean_diff = delta_2.sum() / delta_2.rows();
                }
                return delta_2;
            })
            .share();
    pool.detach_task([this, pumila, fw_result, delta_2] {
        main.backward(fw_result.get(), delta_2.get());
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
}

} // namespace PUMILA_NS
