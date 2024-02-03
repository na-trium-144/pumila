#pragma once
#include "../model_base.h"
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

    PUMILA_DLL int getAction(const FieldState &field) override;
};
} // namespace PUMILA_NS
