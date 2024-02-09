#pragma once
#include "../model_base.h"
#include "./pumila6.h"

namespace PUMILA_NS {
/*!
 * pumila6ベース
 * * 評価関数を
 */
class Pumila7 : public Pumila6 {
  public:
    std::string name() const override { return "pumila7"; }
    PUMILA_DLL explicit Pumila7(int hidden_nodes);
    std::shared_ptr<Pumila7> copy() {
        auto copied = std::make_shared<Pumila7>(main.hidden_nodes);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    using NNModel = Pumila6::NNModel;
    using NNResult = NNModel::NNResult;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    double calcReward(const FieldState &field) const override {
        return Pumila7::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(const FieldState &field);
};
} // namespace PUMILA_NS
