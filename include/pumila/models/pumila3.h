#pragma once
#include "./pumila2.h"

namespace pumila {
/*!
 * * learnStepでtaskの建て方を改良(できるだけfuture.get()で待機しないようにした)
 * * 14段目に置こうとしたときの報酬を最小値にした
 */
class Pumila3 : public Pumila2 {
  public:
    std::string name() const override { return "pumila3"; }

    PUMILA_DLL explicit Pumila3(double learning_rate);
    std::shared_ptr<Pumila3> copy() {
        auto copied = std::make_shared<Pumila3>(main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    PUMILA_DLL int getActionRnd(const FieldState &field, double rnd_p) override;
    PUMILA_DLL void learnStep(const FieldState &field) override;
};
} // namespace pumila
