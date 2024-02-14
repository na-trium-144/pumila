#pragma once
#include "./pumila3.h"

namespace PUMILA_NS {
/*!
 * pumila3ベース
 * * 報酬をスコアのみにした
 * * 教師信号のgammaを0.99にした
 * * pumila3より高く組むようになった。スコアはあまり変わらん
 *
 */
class Pumila5 : public Pumila3 {

  public:
    std::string name() const override { return "pumila5"; }
    PUMILA_DLL explicit Pumila5(double learning_rate);
    std::shared_ptr<Pumila5> copy() {
        auto copied = std::make_shared<Pumila5>(main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    double calcReward(std::shared_ptr<FieldState> field) const override {
        return Pumila5::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(std::shared_ptr<FieldState> field);
};
} // namespace PUMILA_NS
