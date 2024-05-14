#pragma once
#include "def.h"
#include "field3.h"
#include "chain.h"
#include "pumila/garbage.h"
#include <memory>
#include <optional>
#include <vector>

namespace PUMILA_NS {
struct StepResult {
    FieldState3 field_before, field_after;
    std::vector<Chain> chains;
    /*!
     * 自分のgarbageと相殺した場合無
     * 相手に送られた場合は相殺されても落ちてもここに含む
     */
    std::shared_ptr<GarbageGroup> garbage_send;
    /*!
     * 連鎖中の場合は終了後のfield
     */
    std::optional<FieldState3> op_field_before, op_field_after;
    /*!
     * beforeの前→afterの前
     * beforeの前→afterの時点で未完
     * beforeの後→afterの前: 複数存在する可能性がある
     */
    std::vector<std::vector<Chain>> op_chains;
    std::vector<std::shared_ptr<GarbageGroup>> garbage_recv;
};
}
