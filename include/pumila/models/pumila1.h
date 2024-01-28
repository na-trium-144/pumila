#pragma once
#include "../model_base.h"
#include <Eigen/Dense>
#include <array>
#include <memory>

namespace pumila {
class Pumila1 : public Pumila {
  public:
    struct NNModel {
        struct InNodes {
            double bias;
            double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double field_chains[FieldState::WIDTH * FieldState::HEIGHT];
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

        double alpha; // sigmoid coef
        double learning_rate;

        NNModel(double alpha, double learning_rate);

        NNResult forward(const Eigen::MatrixXd &in) const;
        double forwardMax(const Eigen::MatrixXd &in) const;
    } main, target;
    using NNResult = NNModel::NNResult;

    double gamma;
    NNResult last_result;
    int back_count = 0;

    explicit Pumila1(double alpha, double gamma, double learning_rate);
    Pumila1 copy() const { return *this; }
    
    Eigen::MatrixXd getInNodes(std::shared_ptr<GameSim> sim);
    std::array<double, ACTIONS_NUM>
    forward(std::shared_ptr<GameSim> sim) override;
    void backward(std::shared_ptr<GameSim> sim_after, int action_prev,
                  double target) override;
};
} // namespace pumila
