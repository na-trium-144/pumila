#pragma once
#include "../model_base.h"
#include "./pumila2.h"
#include "./pumila5.h"

namespace PUMILA_NS {
/*!
 * pumila5ベース
 * * 中間ノードの数を変更可能にした
 */
class Pumila6 : public Pumila {
  protected:
    static constexpr double GAMMA = 0.99;

    int batch_count = 0;
    std::mutex learning_m;
    std::condition_variable learning_cond;
    static constexpr int BATCH_SIZE = 10;

    void load(std::istream &is) override;
    void save(std::ostream &os) override;

  public:
    std::string name() const override { return "pumila6"; }
    PUMILA_DLL explicit Pumila6(int hidden_nodes);
    std::shared_ptr<Pumila6> copy() {
        auto copied = std::make_shared<Pumila6>(main.hidden_nodes);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    struct NNModel {
        static constexpr double ALPHA = 0.01; // sigmoid coef
        static constexpr double LEARNING_RATE = 0.01;
        int hidden_nodes;

        struct InNodes {
            double bias;
            double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double score_diff;
        };
        static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);

        std::shared_ptr<Eigen::MatrixXd> matrix_ih;
        std::shared_ptr<Eigen::VectorXd> matrix_hq;
        mutable std::mutex matrix_m;

        using NNResult = Pumila2::NNModel::NNResult;

        PUMILA_DLL NNModel(int hidden_nodes);
        NNModel(const NNModel &rhs) { *this = rhs; }
        NNModel &operator=(const NNModel &rhs) {
            hidden_nodes = rhs.hidden_nodes;
            std::lock_guard lock(matrix_m);
            std::lock_guard lock2(rhs.matrix_m);
            matrix_ih = rhs.matrix_ih;
            matrix_hq = rhs.matrix_hq;
            return *this;
        }

        /*!
         * \brief 順伝播
         * \return NNResult型で, hidden, q の行数は in.rows()
         */
        PUMILA_DLL NNResult forward(const Eigen::MatrixXd &in) const;

        /*!
         * \brief 逆伝播
         * \param result 順伝播の結果
         * \param delta それぞれqの誤差 (行数はq.rows()と同じでなければならない)
         */
        PUMILA_DLL void backward(const NNResult &result,
                                 const Eigen::VectorXd &diff);
    };
    /*!
     * getAction時はmainを使う
     * learn時はtargetで計算し、mainに反映し、targetをmainで上書き
     */
    NNModel main, target;
    /*!
     * \brief targetを上書きするときのロック
     */
    std::shared_mutex target_m;

    using NNResult = NNModel::NNResult;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    virtual double calcReward(std::shared_ptr<FieldState> field) const {
        return Pumila5::calcRewardS(field);
    }
    int getAction(const FieldState2 &field,
                  const std::optional<FieldState2> &) override {
        return getAction(std::make_shared<FieldState>(field));
    }
    int getAction(std::shared_ptr<FieldState> field) {
        return getActionRnd(field, 0);
    }
    PUMILA_DLL int getActionRnd(std::shared_ptr<FieldState> field,
                                double rnd_p);
    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        return getActionRnd(sim->field1(), rnd_p);
    }

    PUMILA_DLL virtual void learnStep(std::shared_ptr<FieldState> field);
};
} // namespace PUMILA_NS
