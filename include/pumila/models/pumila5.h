#pragma once
#include "./pumila3.h"

namespace pumila {
/*!
 * pumila3ベース
 * * 報酬をスコアのみにした
 * * 教師信号のgammaを0.99にした
 */
class Pumila5 : public Pumila3 {

  public:
    std::string name() const override { return "pumila5"; }
    explicit Pumila5(double learning_rate);
    std::shared_ptr<Pumila5> copy() {
        auto copied = std::make_shared<Pumila5>(main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    double calcReward(const FieldState &field) const override {
        return Pumila5::calcRewardS(field);
    }
    static double calcRewardS(const FieldState &field);
};
} // namespace pumila
