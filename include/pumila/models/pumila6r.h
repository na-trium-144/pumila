#pragma once
#include "../model_base.h"
#include "./pumila6.h"
#include <memory>
#include <shared_mutex>

namespace PUMILA_NS {
/*!
 * pumila6ベース
 * * learnStepの並列処理を改良
 * (forwardからdelta2計算まで同じモデルを使うようにした &
 * バッチ処理数を一定にした)
 */
class Pumila6r : public Pumila {
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
    std::string name() const override { return "pumila6"; }
    PUMILA_DLL explicit Pumila6r(int hidden_nodes);
    Pumila6r(const Pumila6r &other) : Pumila() {
        std::lock_guard lock_main(other.main_m);
        main = other.main;
        target = main->copy();
    }
    auto &operator=(const Pumila6r &) = delete;
    std::shared_ptr<Pumila6r> copy() {
        return std::make_shared<Pumila6r>(*this);
    }

    struct NNModel : std::enable_shared_from_this<NNModel> {
        static constexpr double ALPHA = 0.01; // sigmoid coef
        static constexpr double LEARNING_RATE = 0.01;
        int hidden_nodes;

        using InNodes = Pumila6::NNModel::InNodes;
        static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);

        Eigen::MatrixXd matrix_ih;
        Eigen::VectorXd matrix_hq;
        mutable std::shared_mutex matrix_m;

        PUMILA_DLL explicit NNModel(int hidden_nodes);
        NNModel(const NNModel &other)
            : std::enable_shared_from_this<NNModel>() {
            std::shared_lock lock(other.matrix_m);
            hidden_nodes = other.hidden_nodes;
            matrix_ih = other.matrix_ih;
            matrix_hq = other.matrix_hq;
        }
        auto &operator=(const NNModel &) = delete;
        std::shared_ptr<NNModel> copy() const {
            return std::make_shared<NNModel>(*this);
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
     * learn時はmainで計算し、targetに反映し、mainをtargetで上書き
     */
    std::shared_ptr<NNModel> main, target;
    /*!
     * \brief modelのポインタ自体を書き換えるときのロック
     *
     * modelの中身のアクセスに関してはforward, backwardの中で排他制御される
     */
    mutable std::shared_mutex main_m;

    struct NNResult {
        Eigen::MatrixXd in;
        Eigen::MatrixXd hidden;
        Eigen::VectorXd q;
        std::shared_ptr<const NNModel> model;
    };
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    virtual double calcReward(std::shared_ptr<FieldState> field) const {
        return Pumila5::calcRewardS(field);
    }
    int getAction(const FieldState2 &field) override {
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
