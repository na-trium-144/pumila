#pragma once
#include "../nn.h"
#include <Eigen/Dense>
#include <BS_thread_pool.hpp>
#include <array>
#include <memory>
#include <limits>

namespace pumila {
class Pumila1 : public Pumila {
    struct InNodes {
        double bias;
        double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double field_chains[FieldState::WIDTH * FieldState::HEIGHT];
        double score_diff;
    };
    static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);
    static constexpr int HIDDEN_NODES = 200;

    BS::thread_pool pool;

    Eigen::MatrixXd matrix_ih;
    Eigen::VectorXd matrix_hq;
    Eigen::MatrixXd in;
    Eigen::MatrixXd hidden;
    Eigen::VectorXd q;

    double alpha; // sigmoid coef
    double gamma;
    double learning_rate;

  public:
    explicit Pumila1(double alpha, double gamma, double learning_rate);
    std::array<double, ACTIONS_NUM>
    forward(std::shared_ptr<GameSim> sim) override;
    void backward(int action, double target) override;

    double forwardMax(std::shared_ptr<GameSim> sim) const {
        Pumila1 model_copy = *this;
        auto fw = model_copy.forward(sim);
        double max_v = std::numeric_limits<double>::lowest();
        for (double v : fw) {
            max_v = max_v > v ? max_v : v;
        }
        return max_v;
    }
};
} // namespace pumila
