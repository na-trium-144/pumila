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
    FieldState3 field_before;
    std::optional<FieldState3> field_after;
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
     * 送られてきたおじゃま
     * beforeの前→afterの前
     * beforeの前→afterの時点で未完
     * beforeの後→afterの前: 複数存在する可能性がある
     */
    std::vector<std::vector<Chain>> op_chains;
    std::vector<std::shared_ptr<GarbageGroup>> garbage_recv;

    /*!
     * \brief このターンに降ったおじゃま
     */
    std::vector<std::pair<std::size_t, std::size_t>> garbage_fell_pos;

    StepResult(const FieldState3 &field_before)
        : field_before(field_before), field_after(), chains(), garbage_send(),
          op_field_before(), op_field_after(), op_chains(), garbage_recv(),
          garbage_fell_pos() {}
    StepResult(const StepResult &) = delete;
    StepResult &operator=(const StepResult &) = delete;
    ~StepResult() = default;
};
} // namespace PUMILA_NS
