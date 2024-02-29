#pragma once
#include "../model_base.h"
#include "./pumila8s.h"
#include "./pumila10.h"
#include <memory>
#include <shared_mutex>

namespace PUMILA_NS {
template <typename NNModel>
class Pumila12Base : public Pumila {
    void load(std::istream &is) override;
    void save(std::ostream &os) override;

  public:
    using NNResult = Pumila8s::NNResult;

  protected:
    double gamma;

    static constexpr int BATCH_SIZE = 50;
    std::vector<std::pair<NNResult, Eigen::VectorXd>> step_data;
    int step_started = 0, step_finished = 0;
    /*!
     * \brief step_count, step_dataを編集するときのロック
     */
    std::mutex learning_m;
    std::condition_variable learning_cond;

  public:
    PUMILA_DLL explicit Pumila12Base(int hidden_nodes, double gamma);
    Pumila12Base(const Pumila12Base &other) : Pumila12Base(1, 1) {
        std::lock_guard lock_main(other.main_m);
        gamma = other.gamma;
        main = target = other.main;
    }
    auto &operator=(const Pumila12Base &) = delete;

    /*!
     * getAction時はmainを使う
     * learn時はmainで計算し、targetに反映し、mainをtargetで上書き
     */
    NNModel main, target;
    /*!
     * \brief main, targetにアクセスするときのロック
     */
    mutable std::shared_mutex main_m, target_m;
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

    using InFeatureSingle = Pumila10::InFeatureSingle;
    using InFeatures = Pumila10::InFeatures;

    /*!
     * \brief 現在のフィールドから次の22通りのフィールドと特徴量を計算
     * (Pumila::poolで実行され完了するまで待機)
     * \return 22 * IN_NODES の行列
     */
    PUMILA_DLL std::future<InFeatures>
    getInNodes(const FieldState2 &field,
               const std::optional<FieldState2> &op_field) const;
    PUMILA_DLL std::future<InFeatures>
    getInNodes(std::shared_future<InFeatures> feature, int feat_a,
               const std::optional<FieldState2> &op_field) const;

    virtual InFeatureSingle
    getInNodeSingle(const FieldState2 &field, int a,
                    const std::optional<FieldState2> &op_field) const = 0;
    virtual Eigen::MatrixXd
    truncateInNodes(const Eigen::MatrixXd &in) const = 0;

    virtual double
    calcReward(const FieldState2 &field,
               const std::optional<FieldState2> &op_field) const = 0;

    int getAction(const FieldState2 &field,
                  const std::optional<FieldState2> &op_field) override {
        return getActionRnd(field, op_field, 0);
    }
    PUMILA_DLL int getActionRnd(const FieldState2 &field,
                                const std::optional<FieldState2> &op_field,
                                double rnd_p);

    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        if (!sim->field) {
            return 0;
        }
        auto op = sim->opponent.lock();
        if (op) {
            return getActionRnd(*sim->field, op->field, rnd_p);
        } else {
            return getActionRnd(*sim->field, std::nullopt, rnd_p);
        }
    }

    PUMILA_DLL void learnStep(const FieldState2 &field, int a,
                              const std::optional<FieldState2> &op_field_before,
                              const std::optional<FieldState2> &op_field_after);

    std::vector<double> diff_history = {};
};

struct NNModel12 {
    static constexpr double ALPHA = 0.01; // sigmoid coef
    static constexpr double LEARNING_RATE = 0.01;
    int hidden_nodes;

    struct InNodes {
        double bias;
        double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 5];
        double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double chains[20];
        double op_garbage_ready[12]; // 6ずつ
        double op_field_colors[FieldState::WIDTH * FieldState::HEIGHT * 5];
        double op_field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double op_chains[20];
    };
    static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);

    Eigen::MatrixXd matrix_ih;
    Eigen::VectorXd matrix_hq;

    PUMILA_DLL explicit NNModel12(int hidden_nodes);
};

/*!
 * pumila11ベース
 *
 * * 敵フィールド追加
 *   * q(sim.field + action, op.field) <- reward(sim.field + action, op.field +
 * action)
 * * rewardは自分ゲームオーバーで-1000、敵ゲームオーバーで+1000
 */
class Pumila12 : public Pumila12Base<NNModel12> {
  public:
    std::string name() const override { return "pumila12"; }
    explicit Pumila12(int hidden_nodes, double gamma)
        : Pumila12Base(hidden_nodes, gamma) {}
    std::shared_ptr<Pumila12> copy() {
        return std::make_shared<Pumila12>(*this);
    }

    using NNModel = NNModel12;

    double calcReward(
        const FieldState2 &field,
        const std::optional<FieldState2> &op_field_after) const override {
        return Pumila12::calcRewardS(field, op_field_after);
    }
    PUMILA_DLL static double
    calcRewardS(const FieldState2 &field,
                const std::optional<FieldState2> &op_field_after);
    InFeatureSingle
    getInNodeSingle(const FieldState2 &field, int a,
                    const std::optional<FieldState2> &op_field) const override {
        return Pumila12::getInNodeSingleS(field, a, op_field);
    }
    PUMILA_DLL static InFeatureSingle
    getInNodeSingleS(const FieldState2 &field, int a,
                     const std::optional<FieldState2> &op_field);
    Eigen::MatrixXd truncateInNodes(const Eigen::MatrixXd &in) const override {
        return Pumila12::truncateInNodesS(in);
    }
    PUMILA_DLL static Eigen::MatrixXd
    truncateInNodesS(const Eigen::MatrixXd &in);
};
} // namespace PUMILA_NS
