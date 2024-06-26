#pragma once
#include "../model_base.h"
#include "./pumila6.h"

namespace PUMILA_NS {
/*!
 * pumila6ベース
 * * 評価関数を スコア - 前回連鎖からの手数*20 にした
 *
 * 結果
 * * 中間ノードが少ないとあまり連鎖を組めず、単発消しが多くなる
 * * pumila7_5がもっとも連鎖を組む
 * * pumila6は連鎖を組んでも発火しないがその点pumila7は安定(大連鎖は組めなさそうだが)
 *
 */
class Pumila7 : public Pumila6 {
  public:
    std::string name() const override { return "pumila7"; }
    PUMILA_DLL explicit Pumila7(int hidden_nodes);
    Pumila7(const std::string &name) : Pumila7(1) { loadFile(name); }
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

    double calcReward(std::shared_ptr<FieldState> field) const override {
        return Pumila7::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(std::shared_ptr<FieldState> field);
};
} // namespace PUMILA_NS
