#pragma once
#include "../model_base.h"
#include "./pumila7.h"

namespace PUMILA_NS {
/*!
 * pumila7ベース
 * * 学習時に1手先ではなく2手先まで全通り計算したmaxを報酬として与える
 */
class Pumila8 : public Pumila7 {
  public:
    std::string name() const override { return "pumila8"; }
    PUMILA_DLL explicit Pumila8(int hidden_nodes);
    Pumila8(const std::string &name) : Pumila8(1) { loadFile(name); }
    std::shared_ptr<Pumila8> copy() {
        auto copied = std::make_shared<Pumila8>(main.hidden_nodes);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    using NNModel = Pumila6::NNModel;
    using NNResult = NNModel::NNResult;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    PUMILA_DLL void learnStep(std::shared_ptr<FieldState> field) override;
};
} // namespace PUMILA_NS
