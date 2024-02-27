#pragma once
#include "../model_base.h"
#include "../field.h"
#include <array>
#include <memory>

namespace PUMILA_NS {
/*!
 * \brief Pumila1と同じ条件、ニューラルネットワーク無しで連鎖をねらう
 *
 */
class Pumila1N : public Pumila {
    int target_chain;

  public:
    explicit Pumila1N(int target_chain)
        : Pumila(), target_chain(target_chain) {}

    PUMILA_DLL int getAction(std::shared_ptr<FieldState> field);
    int getAction(const FieldState2 &field) override {
        return getAction(std::make_shared<FieldState>(field));
    }
};
} // namespace PUMILA_NS
