#pragma once
#include "../model_base.h"
#include "./pumila6.h"
#include <memory>
#include <shared_mutex>

namespace PUMILA_NS {
/*!
 * pumila8rベース
 * * shared_ptrを極力使わないようにする
 * * 報酬がpumila5,6のものになってる
 */
class Pumila8s : public Pumila {
  public:
    struct NNResult;

  protected:
    static constexpr double GAMMA = 0.99;

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
    std::string name() const override { return "pumila8s"; }
    PUMILA_DLL explicit Pumila8s(int hidden_nodes);
    Pumila8s(const Pumila8s &other) : Pumila(), main(1), target(1) {
        std::lock_guard lock_main(other.main_m);
        main = target = other.main;
    }
    auto &operator=(const Pumila8s &) = delete;
    std::shared_ptr<Pumila8s> copy() {
        return std::make_shared<Pumila8s>(*this);
    }

    struct NNModel {
        static constexpr double ALPHA = 0.01; // sigmoid coef
        static constexpr double LEARNING_RATE = 0.01;
        int hidden_nodes;

        using InNodes = Pumila6::NNModel::InNodes;
        static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);

        Eigen::MatrixXd matrix_ih;
        Eigen::VectorXd matrix_hq;

        PUMILA_DLL explicit NNModel(int hidden_nodes);
    };
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

    struct NNResult {
        Eigen::MatrixXd in;
        Eigen::MatrixXd hidden;
        Eigen::VectorXd q;
    };
    struct InFeatureSingle {
        FieldState field_next;
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
    PUMILA_DLL static std::future<InFeatures>
    getInNodes(const FieldState &field);
    PUMILA_DLL static std::future<InFeatures>
    getInNodes(std::shared_future<InFeatures> feature, int feat_a);
    /*!
     * \brief フィールドにaのアクションをしたあとの特徴量を計算
     */
    PUMILA_DLL static InFeatureSingle getInNodeSingle(const FieldState &field,
                                                      int a);

    double calcReward(std::shared_ptr<FieldState> field) const {
        return calcReward(*field);
    }
    virtual double calcReward(const FieldState &field) const {
        return Pumila8s::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(const FieldState &field);

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
