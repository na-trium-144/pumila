#pragma once
#include "../model_base.h"
#include "./pumila8s.h"
#include "./pumila10.h"
#include <memory>
#include <shared_mutex>

namespace PUMILA_NS {
struct NNModel12 {
    static constexpr double ALPHA = 0.01; // sigmoid coef
    static constexpr double LEARNING_RATE = 0.01;
    int hidden_nodes;

    struct InNodes {
        double bias;
        double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double chains[20];
        double op_garbage_ready[12]; // 6ずつ
        double op_field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
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
 */
class Pumila12 : public Pumila10Base<NNModel12> {
  public:
    std::string name() const override { return "pumila12"; }
    explicit Pumila12(int hidden_nodes, double gamma)
        : Pumila12Base(hidden_nodes, gamma) {}
    std::shared_ptr<Pumila12> copy() {
        return std::make_shared<Pumila12>(*this);
    }

    using NNModel = NNModel12;

    double calcReward(const FieldState2 &field,
                      const FieldState2 &op_field_after) const override {
        return Pumila12::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(const FieldState2 &field,
                                         const FieldState2 &op_field_after);
    InFeatureSingle
    getInNodeSingle(const FieldState2 &field, int a,
                    FieldState2 &op_field) const override {
        return Pumila12::getInNodeSingleS(field, a);
    }
    PUMILA_DLL static InFeatureSingle
    getInNodeSingleS(const FieldState2 &field, int a,
                     FieldState2 &op_field);
    Eigen::MatrixXd truncateInNodes(const Eigen::MatrixXd &in) const override {
        return Pumila12::truncateInNodesS(in);
    }
    PUMILA_DLL static Eigen::MatrixXd
    truncateInNodesS(const Eigen::MatrixXd &in);
};
} // namespace PUMILA_NS
