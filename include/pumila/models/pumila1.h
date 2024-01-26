#pragma once
#include "../nn.h"
#include <Eigen/Dense>

namespace pumila {
class Pumila1 : public Pumila {
    struct InNodes {
        double bias;
        double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double field_chains[FieldState::WIDTH * FieldState::HEIGHT];
    };
    static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);
    static constexpr int HIDDEN_NODES = 200;
    static constexpr int Q_NODES = 1;

    Eigen::MatrixXd matrix_ih;
    Eigen::MatrixXd matrix_hq;
    Eigen::VectorXd in;
    Eigen::VectorXd hidden;
    double q;
    
    double alpha; // sigmoid coef
    double learning_rate;

  public:
    explicit Pumila1(double alpha, double learning_rate);
    double forward(const FieldState &fs) override;
    void backward(double target) override;
};
} // namespace Pumila
