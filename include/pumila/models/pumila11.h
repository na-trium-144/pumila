#pragma once
#include "./pumila10.h"

namespace PUMILA_NS {
struct NNModel11 {
    static constexpr double ALPHA = 0.01; // sigmoid coef
    static constexpr double LEARNING_RATE = 0.01;
    int hidden_nodes;

    struct InNodes {
        double bias;
        double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
        double score_diff[20];
    };
    static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);

    Eigen::MatrixXd matrix_ih;
    Eigen::VectorXd matrix_hq;

    PUMILA_DLL explicit NNModel11(int hidden_nodes);
};
/*!
 * pumila10ベース
 * * 入力にスコアをそのままではなく連鎖1回ごとにわけて与えるようにする
 */
class Pumila11 : public Pumila10Base<NNModel11> {
    void load(std::istream &is) override;
    void save(std::ostream &os) override;

  public:
    std::string name() const override { return "pumila11"; }
    explicit Pumila11(int hidden_nodes, double gamma)
        : Pumila10Base(hidden_nodes, gamma) {}
    std::shared_ptr<Pumila11> copy() {
        return std::make_shared<Pumila11>(*this);
    }

    using NNModel = NNModel11;

    double calcReward(const FieldState2 &field) const override {
        return Pumila10::calcRewardS(field);
    }

    InFeatureSingle getInNodeSingle(const FieldState2 &field,
                                    int a) const override {
        return Pumila11::getInNodeSingleS(field, a);
    }
    PUMILA_DLL static InFeatureSingle getInNodeSingleS(const FieldState2 &field,
                                                       int a);
    Eigen::MatrixXd truncateInNodes(const Eigen::MatrixXd &in) const override {
        return truncateInNodesS(in);
    }
    PUMILA_DLL static Eigen::MatrixXd
    truncateInNodesS(const Eigen::MatrixXd &in);
};
} // namespace PUMILA_NS
