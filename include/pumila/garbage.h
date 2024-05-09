#pragma once
#include "def.h"
#include <chrono>
#include <memory>
#include <cassert>

namespace PUMILA_NS {
/*!
 * \brief 1回の連鎖で送るおじゃまのまとまり
 */
class GarbageGroup {
    std::size_t garbage_num;
    std::shared_ptr<std::size_t> cancelled_num;
    std::shared_ptr<std::size_t> fell_num;

  public:
    GarbageGroup() : garbage_num(0), cancelled_num() {}
    explicit GarbageGroup(std::size_t garbage_num)
        : garbage_num(garbage_num),
          cancelled_num(std::make_shared<std::size_t>(0)),
          fell_num(std::make_shared<std::size_t>(0)) {}

    /*!
     * \brief もとのおじゃまの数
     */
    std::size_t garbageNum() const { return garbage_num; }
    /*!
     * \brief まだ未確定のおじゃまの数
     */
    std::size_t restGarbageNum() const {
        assert(garbageNum() >= (cancelledNum() + fellNum()));
        return garbageNum() - cancelledNum() - fellNum();
    }
    /*!
     * \brief 相殺する
     * \return 相殺量
     */
    std::size_t cancel(std::size_t cancel) {
        std::size_t cancel_actual =
            cancel > restGarbageNum() ? restGarbageNum() : cancel;
        assert(cancelled_num);
        *cancelled_num += cancel_actual;
        return cancel_actual;
    }
    /*!
     * \brief ぷよを降らせる
     * \return 降らせるぷよの量
     */
    std::size_t fall(std::size_t fall_max) {
        std::size_t fall_actual =
            fall_max > restGarbageNum() ? restGarbageNum() : fall_max;
        assert(fell_num);
        *fell_num += fall_actual;
        return fall_actual;
    }

    bool done() const { return restGarbageNum() == 0; }
    std::size_t cancelledNum() const {
        return cancelled_num ? *cancelled_num : 0;
    }
    std::size_t fellNum() const { return fell_num ? *fell_num : 0; }
};

} // namespace PUMILA_NS
