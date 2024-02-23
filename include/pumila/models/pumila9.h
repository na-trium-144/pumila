#pragma once
#include "../model_base.h"
#include "./pumila8s.h"

namespace PUMILA_NS {
/*!
 * pumila8sベース
 * * 報酬をスコアではなく連鎖数に比例するようにし、盤面のぷよ量に応じてマイナスするようにした
 * しかし8sと同じような動きしかしない
 */
class Pumila9 : public Pumila8s {
  public:
    std::string name() const override { return "pumila9"; }
    PUMILA_DLL explicit Pumila9(int hidden_nodes);
    Pumila9(const Pumila9 &other) : Pumila9(1) {
        std::lock_guard lock_main(other.main_m);
        main = target = other.main;
    }
    auto &operator=(const Pumila9 &) = delete;
    std::shared_ptr<Pumila9> copy() {
        return std::make_shared<Pumila9>(*this);
    }

    double calcReward(const FieldState &field) const override {
        return Pumila9::calcRewardS(field);
    }
    PUMILA_DLL static double calcRewardS(const FieldState &field);
};
} // namespace PUMILA_NS
