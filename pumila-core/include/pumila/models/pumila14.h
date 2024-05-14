#pragma once
#include "../def.h"
#include "../field3.h"
#include "../step.h"
#include "../matrix.h"

namespace PUMILA_NS {
/*!
 * Pumila11と同じ
 * 報酬は自分のスコア
 */
struct Pumila14 {
    struct InFeature {
        double bias;
        double field_colors[FieldState3::WIDTH * FieldState3::HEIGHT * 4];
        double field_chains[FieldState3::WIDTH * FieldState3::HEIGHT * 4];
        double score_diff[20];
    };

    PUMILA_DLL static Matrix
    calcAction(const FieldState3 &field,
               const std::optional<FieldState3> &op_field);

    PUMILA_DLL static Matrix transpose(const Matrix &in);

    PUMILA_DLL static double reward(const StepResult &result);
};
} // namespace PUMILA_NS
