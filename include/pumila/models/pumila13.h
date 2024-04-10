#pragma once
#include "../model_base.h"
#include "./pumila12.h"

namespace PUMILA_NS {

struct NNModel13 {
    static constexpr double ALPHA = 0.01; // sigmoid coef
    static constexpr double LEARNING_RATE = 0.01;
    int hidden_nodes;

    struct InNodes {
        double bias;
        double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 5];
        double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double chains[20];
        double op_garbage_ready[12]; // 6ずつ
        double op_height[6];
        double op_field_chains[20];
        double op_chains[20];
    };
    static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);

    Eigen::MatrixXd matrix_ih;
    Eigen::VectorXd matrix_hq;

    PUMILA_DLL explicit NNModel13(int hidden_nodes);
};

/*!
 * pumila12ベース
 *
 * * rewardは自分の連鎖スコア - 相手の連鎖スコア
 */
class Pumila13 : public Pumila12Base<NNModel13> {
    mutable int prev_op_score = 0;

  public:
    std::string name() const override { return "pumila13"; }
    explicit Pumila13(int hidden_nodes, double gamma)
        : Pumila12Base(hidden_nodes, gamma) {}
    Pumila13(const std::string &name) : Pumila13(1, 1) { loadFile(name); }
    std::shared_ptr<Pumila13> copy() {
        return std::make_shared<Pumila13>(*this);
    }

    using NNModel = NNModel13;

    PUMILA_DLL std::pair<double, bool> calcReward(
        const FieldState2 &field,
        const std::optional<FieldState2> &op_field_after) const override;

    InFeatureSingle
    getInNodeSingle(const FieldState2 &field, int a,
                    const std::optional<FieldState2> &op_field) const override {
        return Pumila13::getInNodeSingleS(field, a, op_field);
    }
    PUMILA_DLL static InFeatureSingle
    getInNodeSingleS(const FieldState2 &field, int a,
                     const std::optional<FieldState2> &op_field);
    Eigen::MatrixXd transposeInNodes(const Eigen::MatrixXd &in) const override {
        return Pumila13::transposeInNodesS(in);
    }
    PUMILA_DLL static Eigen::MatrixXd
    transposeInNodesS(const Eigen::MatrixXd &in);
};
} // namespace PUMILA_NS
