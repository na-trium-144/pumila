#pragma once
#include "def.h"
#include "field3.h"
#include "chain.h"
#include "pumila/garbage.h"
#include <memory>
#include <optional>
#include <vector>
#include <cassert>
#include <iostream>

namespace PUMILA_NS {
struct StepResult {
    /*!
     * putするまえ (Fallの直前)
     */
    FieldState3 field_before;
    /*!
     * おじゃまが降った後 (Free)
     */
    std::optional<FieldState3> field_after;
    /*!
     * 自分の連鎖 (Fall)
     */
    std::vector<Chain> chains;
    /*!
     * 自分が送ったおじゃま (Garbage)
     * 自分のgarbageと相殺した場合無
     * 相手に送られた場合は相殺されても落ちてもここに含む
     */
    std::shared_ptr<GarbageGroup> garbage_send;
    /*!
     * 自分のfield_before時点(Free)の相手のfield
     * 相手連鎖中の場合は相手連鎖終了後のfieldになる
     */
    std::optional<FieldState3> op_field_before;
    /*!
     * 自分のfield_after時点(Free = 自フィールドに降った直後)の相手のfield
     * 相手連鎖中の場合は相手連鎖終了後のfieldになる
     */
    std::optional<FieldState3> op_field_after;
    /*!
     * 自分のbefore→after間(Free→Free)に送られてきたおじゃま
     */
    std::vector<std::shared_ptr<GarbageGroup>> garbage_recv;
    /*!
     * このターンに降ったおじゃま (Garbage)
     */
    std::vector<std::pair<std::size_t, std::size_t>> garbage_fell_pos;

    explicit StepResult(const FieldState3 &field_before)
        : field_before(field_before), field_after(), chains(), garbage_send(),
          op_field_before(), op_field_after(), garbage_recv(),
          garbage_fell_pos() {}
    StepResult(const StepResult &) = delete;
    StepResult &operator=(const StepResult &) = delete;
    ~StepResult() = default;

    bool done() const {
        if (!field_after) {
            return false;
        }
        if (garbage_send && !garbage_send->done()) {
            return false;
        }
        if (!garbage_recv.empty()) {
            if (!std::all_of(garbage_recv.cbegin(), garbage_recv.cend(),
                             [](const auto &gr) { return gr->done(); })) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<StepResult> next() const {
        assert(field_after);
        auto n = std::make_shared<StepResult>(*field_after);
        if (op_field_after) {
            n->op_field_before = op_field_after;
        }
        return n;
    }
};
} // namespace PUMILA_NS
