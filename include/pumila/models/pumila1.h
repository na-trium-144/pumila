#pragma once
#include "../model_base.h"
#include "../field.h"
#include <Eigen/Dense>
#include <array>
#include <memory>

namespace PUMILA_NS {
class Pumila1 : public Pumila {
    double gamma;
    int back_count = 0;

    void load(std::istream &is) override;
    void save(std::ostream &os) override;

  public:
    std::string name() const override { return "pumila1"; }
    struct NNModel {
        double alpha; // sigmoid coef
        double learning_rate;

        struct InNodes {
            double bias;
            double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double score_diff;
        };
        static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);
        static constexpr int HIDDEN_NODES = 200;


        Eigen::MatrixXd matrix_ih;
        Eigen::VectorXd matrix_hq;
        struct NNResult {
            Eigen::MatrixXd in;
            Eigen::MatrixXd hidden;
            Eigen::VectorXd q;
        };

        PUMILA_DLL NNModel(double alpha, double learning_rate);

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

    } main, target;
    using NNResult = NNModel::NNResult;

    std::unordered_map<int, NNResult> learn_results;
    std::unordered_map<int, int> learn_actions;
    int learn_results_index = 0;

    PUMILA_DLL explicit Pumila1(double alpha, double gamma,
                                double learning_rate);
    std::shared_ptr<Pumila1> copy() {
        auto copied =
            std::make_shared<Pumila1>(main.alpha, gamma, main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    /*!
     * \brief フィールドから特徴量を計算
     * \return 22 * IN_NODES の行列
     */
    Eigen::MatrixXd getInNodes(std::shared_ptr<GameSim> sim) const {
        return getInNodes(sim->field1());
    }
    PUMILA_DLL Eigen::MatrixXd
    getInNodes(std::shared_ptr<FieldState> field) const;

    /*!
     * \brief 報酬を計算
     *
     */
    PUMILA_DLL double calcReward(std::shared_ptr<GameSim> sim_after) const;

    PUMILA_DLL int getAction(std::shared_ptr<FieldState> field);
    int getAction(std::shared_ptr<FieldState2> field) override {
        return getAction(std::make_shared<FieldState>(*field));
    }
    PUMILA_DLL std::pair<int, int> getLearnAction(std::shared_ptr<GameSim> sim);
    PUMILA_DLL double learnResult(int id, std::shared_ptr<GameSim> sim_after);
};
} // namespace PUMILA_NS
