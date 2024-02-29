#pragma once
#include "./pumila2.h"

namespace PUMILA_NS {
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

    PUMILA_DLL void load(std::istream &is) override;
    PUMILA_DLL void save(std::ostream &os) override;

  public:
    std::string name() const override { return "pumila4"; }
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

        PUMILA_DLL NNModel(double learning_rate);
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
        PUMILA_DLL NNResult forward(const Eigen::MatrixXd &in) const;

        /*!
         * \brief 逆伝播
         * \param result 順伝播の結果
         * \param delta それぞれqの誤差 (行数はq.rows()と同じでなければならない)
         *
         */
        PUMILA_DLL void backward(const NNResult &result,
                                 const Eigen::VectorXd &diff);

        inline static Eigen::MatrixXd
        transposeInNodes(const Eigen::MatrixXd &in) {
            return Pumila2::NNModel::transposeInNodes(in);
        }
    };
    NNModel main, target;
    std::shared_mutex target_m;
    using NNResult = NNModel::NNResult;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    inline static std::future<InFeatures>
    getInNodes(std::shared_ptr<FieldState> field) {
        return Pumila2::getInNodes(field);
    }
    inline static std::future<InFeatures>
    getInNodes(std::shared_future<InFeatures> feature, int feat_a) {
        return Pumila2::getInNodes(feature, feat_a);
    }
    inline static InFeatureSingle
    getInNodeSingle(std::shared_ptr<FieldState> field, int a);


    PUMILA_DLL Pumila4(double learning_rate);
    std::shared_ptr<Pumila4> copy() {
        auto copied = std::make_shared<Pumila4>(main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    inline static double calcReward(std::shared_ptr<FieldState> field) {
        return Pumila2::calcRewardS(field);
    }

    int getAction(const FieldState2 &field,
                  const std::optional<FieldState2> &) override {
        return getAction(std::make_shared<FieldState>(field));
    }
    int getAction(std::shared_ptr<FieldState> field) {
        return getActionRnd(field, 0);
    }
    PUMILA_DLL virtual int getActionRnd(std::shared_ptr<FieldState> field,
                                        double rnd_p);
    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        return getActionRnd(sim->field1(), rnd_p);
    }
    PUMILA_DLL virtual void learnStep(std::shared_ptr<FieldState> field);
};
} // namespace PUMILA_NS
