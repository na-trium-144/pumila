#pragma once
#include "../model_base.h"
#include "./pumila8s.h"
#include <memory>
#include <shared_mutex>

namespace PUMILA_NS {
/*!
 * pumila8rベース
 * * gammaを変更可能にした
 * * FieldStateを改良→FieldState2
 */
template <typename NNModel>
class Pumila10Base : public Pumila {
    void load(std::istream &is) override;
    void save(std::ostream &os) override;

  public:
    using NNResult = Pumila8s::NNResult;

  protected:
    double gamma;

    static constexpr int BATCH_SIZE = 10;
    std::vector<std::pair<NNResult, Eigen::VectorXd>> step_data;
    int step_started = 0, step_finished = 0;
    /*!
     * \brief step_count, step_dataを編集するときのロック
     */
    std::mutex learning_m;
    std::condition_variable learning_cond;

  public:
    PUMILA_DLL explicit Pumila10Base(int hidden_nodes, double gamma);
    Pumila10Base(const Pumila10Base &other) : Pumila10Base(1, 1) {
        std::lock_guard lock_main(other.main_m);
        gamma = other.gamma;
        main = target = other.main;
    }
    auto &operator=(const Pumila10Base &) = delete;

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

    struct InFeatureSingle {
        FieldState2 field_next;
        Eigen::VectorXd in;
    };
    struct InFeatures {
        std::array<std::shared_future<InFeatureSingle>, ACTIONS_NUM> each;
        Eigen::MatrixXd in;
    };
    /*!
     * \brief 現在のフィールドから次の22通りのフィールドと特徴量を計算
     * (Pumila::poolで実行され完了するまで待機)
     * \return 22 * IN_NODES の行列
     */
    PUMILA_DLL std::future<InFeatures>
    getInNodes(const FieldState2 &field) const;
    PUMILA_DLL std::future<InFeatures>
    getInNodes(std::shared_future<InFeatures> feature, int feat_a) const;

    virtual InFeatureSingle getInNodeSingle(const FieldState2 &field,
                                            int a) const = 0;
    virtual Eigen::MatrixXd
    transposeInNodes(const Eigen::MatrixXd &in) const = 0;

    double calcReward(std::shared_ptr<FieldState2> field) const {
        return calcReward(*field);
    }
    virtual double calcReward(const FieldState2 &field) const = 0;

    int getAction(const FieldState2 &field,
                  const std::optional<FieldState2> &) override {
        return getActionRnd(field, 0);
    }
    PUMILA_DLL int getActionRnd(const FieldState2 &field, double rnd_p);

    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        return getActionRnd(*sim->field, rnd_p);
    }

    PUMILA_DLL void learnStep(const FieldState2 &field);

    std::vector<double> diff_history = {};
};

class Pumila10 : public Pumila10Base<Pumila8s::NNModel> {
  public:
    std::string name() const override { return "pumila10"; }
    explicit Pumila10(int hidden_nodes, double gamma)
        : Pumila10Base(hidden_nodes, gamma) {}
    Pumila10(const std::string &name) : Pumila10(1, 1) { loadFile(name); }
    std::shared_ptr<Pumila10> copy() {
        return std::make_shared<Pumila10>(*this);
    }

    using NNModel = Pumila8s::NNModel;
    InFeatureSingle getInNodeSingle(const FieldState2 &field,
                                    int a) const override {
        return Pumila10::getInNodeSingleS(field, a);
    }
    PUMILA_DLL static InFeatureSingle getInNodeSingleS(const FieldState2 &field,
                                                       int a);
    Eigen::MatrixXd transposeInNodes(const Eigen::MatrixXd &in) const override {
        return Pumila2::NNModel::transposeInNodes(in);
    }
    double calcReward(const FieldState2 &field) const override {
        return Pumila10::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(const FieldState2 &field);
};
} // namespace PUMILA_NS
