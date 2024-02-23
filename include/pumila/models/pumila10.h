#pragma once
#include "../model_base.h"
#include "./pumila8s.h"
#include <memory>
#include <shared_mutex>

namespace PUMILA_NS {
/*!
 * pumila8rベース
 * * gammaを変更可能にした
 */
class Pumila10 : public Pumila {
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

    void load(std::istream &is) override;
    void save(std::ostream &os) override;

  public:
    std::string name() const override { return "pumila10"; }
    PUMILA_DLL explicit Pumila10(int hidden_nodes, double gamma);
    Pumila10(const Pumila10 &other) : Pumila10(1, 1) {
        std::lock_guard lock_main(other.main_m);
        gamma = other.gamma;
        main = target = other.main;
    }
    auto &operator=(const Pumila10 &) = delete;
    std::shared_ptr<Pumila10> copy() {
        return std::make_shared<Pumila10>(*this);
    }

    using NNModel = Pumila8s::NNModel;
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

    using InFeatureSingle = Pumila8s::InFeatureSingle;
    using InFeatures = Pumila8s::InFeatures;

    double calcReward(std::shared_ptr<FieldState> field) const {
        return calcReward(*field);
    }
    virtual double calcReward(const FieldState &field) const {
        return Pumila8s::calcRewardS(field);
    }

    int getAction(std::shared_ptr<FieldState2> field) override {
        return getAction(std::make_shared<FieldState>(*field));
    }
    int getAction(std::shared_ptr<FieldState> field) {
        return getActionRnd(field, 0);
    }
    PUMILA_DLL int getActionRnd(std::shared_ptr<FieldState> field,
                                double rnd_p);
    PUMILA_DLL double getActionCoeff(std::shared_ptr<FieldState> field);

    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        return getActionRnd(sim->field1(), rnd_p);
    }

    PUMILA_DLL virtual void learnStep(std::shared_ptr<FieldState> field);
};
} // namespace PUMILA_NS
