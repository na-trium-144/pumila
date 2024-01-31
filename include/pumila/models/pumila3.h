#pragma once
#include "./pumila2.h"
#include <Eigen/Dense>
#include <array>
#include <memory>
#include <utility>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <future>

namespace pumila {
/*!
 * * 14段目に置こうとしたら-100000の報酬を与えるようにした
 */
class Pumila3 : public Pumila2 {
    std::string name() const override { return "pumila3"; }

  public:
    explicit Pumila3(double learning_rate);
    std::shared_ptr<Pumila3> copy() {
        auto copied = std::make_shared<Pumila3>(main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    static constexpr double REWARD_INVALID = -100000;
    int getActionRnd(const FieldState &field, double rnd_p) override;
    void learnStep(const FieldState &field) override;
};
} // namespace pumila
