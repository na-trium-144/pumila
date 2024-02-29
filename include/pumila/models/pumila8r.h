#pragma once
#include "../model_base.h"
#include "./pumila7r.h"

namespace PUMILA_NS {
/*!
 * pumila7rベース
 * * 学習時に1手先ではなく2手先まで全通り計算したmaxを報酬として与える
 */
class Pumila8r : public Pumila7r {
  public:
    std::string name() const override { return "pumila8r"; }
    PUMILA_DLL explicit Pumila8r(int hidden_nodes);
    Pumila8r(const std::string &name) : Pumila8r(1) { loadFile(name); }
    std::shared_ptr<Pumila8r> copy() {
        return std::make_shared<Pumila8r>(*this);
    }

    using NNModel = Pumila6r::NNModel;
    using InFeatures = Pumila2::InFeatures;
    using InFeatureSingle = Pumila2::InFeatureSingle;

    PUMILA_DLL void learnStep(std::shared_ptr<FieldState> field) override;
};
} // namespace PUMILA_NS
