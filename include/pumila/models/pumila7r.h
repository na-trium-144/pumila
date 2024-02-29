#pragma once
#include "../model_base.h"
#include "./pumila6r.h"
#include "./pumila7.h"

namespace PUMILA_NS {
/*!
 * pumila6rベース
 * * 評価関数を スコア - 前回連鎖からの手数*20 にした
 *
 */
class Pumila7r : public Pumila6r {
  public:
    std::string name() const override { return "pumila7r"; }
    PUMILA_DLL explicit Pumila7r(int hidden_nodes);
    Pumila7r(const std::string &name) : Pumila7r(1) { loadFile(name); }
    std::shared_ptr<Pumila7r> copy() {
        return std::make_shared<Pumila7r>(*this);
    }

    using NNModel = Pumila6r::NNModel;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    double calcReward(std::shared_ptr<FieldState> field) const override {
        return Pumila7::calcRewardS(field);
    }
};
} // namespace PUMILA_NS
