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
        double field_colors[FieldState3::WIDTH * FieldState3::HEIGHT * 4];
        double field_chains[FieldState3::WIDTH * FieldState3::HEIGHT * 4];
        double score_diff[20];
    };
    static constexpr std::size_t FEATURE_NUM =
        sizeof(InFeature) / sizeof(double);
    PUMILA_DLL static Matrix calcAction(const StepResult &result);
    PUMILA_DLL static Matrix rotateColor(const Matrix &in);
    PUMILA_DLL static double reward(const StepResult &result);
};
} // namespace PUMILA_NS
