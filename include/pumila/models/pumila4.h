#pragma once
#include "./pumila2.h"

namespace pumila {
/*!
 * * 中間層の活性化関数をsigmoidからreluに変更
 * → 中間層がすべて0になって学習しなくなった
 * * scoreの入力を40で割って小さくした
 */
class Pumila4 : public Pumila {
    double gamma = 0.9;

    int batch_count = 0;
    std::mutex learning_m;
    std::condition_variable learning_cond;
    static constexpr int BATCH_SIZE = 10;

    void load(std::istream &is) override;
    void save(std::ostream &os) override;
    std::string name() const override { return "pumila4"; }

  public:
    struct NNModel {
        double learning_rate;

        struct InNodes {
            double bias;
            double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double score_diff;
        };
        static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);
        static constexpr int HIDDEN_NODES = 200;

        std::shared_ptr<Eigen::MatrixXd> matrix_ih;
        std::shared_ptr<Eigen::VectorXd> matrix_hq;
        mutable std::mutex matrix_m;

        using NNResult = Pumila2::NNModel::NNResult;

        NNModel(double learning_rate);
        NNModel(const NNModel &rhs) { *this = rhs; }
        NNModel &operator=(const NNModel &rhs) {
            learning_rate = rhs.learning_rate;
            std::lock_guard lock(matrix_m);
            std::lock_guard lock2(rhs.matrix_m);
            matrix_ih = rhs.matrix_ih;
            matrix_hq = rhs.matrix_hq;
            return *this;
        }

        /*!
         * \brief 順伝播
         * \return NNResult型で, hidden, q の行数は in.rows()
         *
         */
        NNResult forward(const Eigen::MatrixXd &in) const;

        /*!
         * \brief 逆伝播
         * \param result 順伝播の結果
         * \param delta それぞれqの誤差 (行数はq.rows()と同じでなければならない)
         *
         */
        void backward(const NNResult &result, const Eigen::VectorXd &diff);

        inline static Eigen::MatrixXd
        truncateInNodes(const Eigen::MatrixXd &in) {
            return Pumila2::NNModel::truncateInNodes(in);
        }
    };
    NNModel main, target;
    std::shared_mutex target_m;
    using NNResult = NNModel::NNResult;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    inline static std::future<InFeatures> getInNodes(const FieldState &field) {
        return Pumila2::getInNodes(field);
    }
    inline static std::future<InFeatures>
    getInNodes(std::shared_future<InFeatures> feature, int feat_a) {
        return Pumila2::getInNodes(feature, feat_a);
    }
    inline static InFeatureSingle getInNodeSingle(const FieldState &field,
                                                  int a);


    Pumila4(double learning_rate);
    std::shared_ptr<Pumila4> copy() {
        auto copied = std::make_shared<Pumila4>(main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    inline static double calcReward(const FieldState &field) {
        return Pumila2::calcReward(field);
    }

    int getAction(const FieldState &field) override {
        return getActionRnd(field, 0);
    }
    virtual int getActionRnd(const FieldState &field, double rnd_p);
    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        return getActionRnd(sim->field, rnd_p);
    }
    virtual void learnStep(const FieldState &field);
};
} // namespace pumila
