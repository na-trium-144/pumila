#pragma once
#include "def.h"
#include <cassert>
#include <mutex>

namespace PUMILA_NS {
/*!
 * \brief 1回の連鎖で送るおじゃまのまとまり
 */
class GarbageGroup {
    mutable std::recursive_mutex mtx;
    std::size_t garbage_num;
    std::size_t cancelled_num;
    std::size_t fell_num;

  public:
    explicit GarbageGroup(std::size_t garbage_num = 0)
        : garbage_num(garbage_num), cancelled_num(0), fell_num(0) {}
    GarbageGroup(const GarbageGroup &) = delete;
    GarbageGroup &operator=(const GarbageGroup &) = delete;
    ~GarbageGroup() = default;

    /*!
     * \brief もとのおじゃまの数
     */
    std::size_t garbageNum() const {
        std::lock_guard lock(mtx);
        return garbage_num;
    }
    /*!
     * \brief まだ未確定のおじゃまの数
     */
    std::size_t restGarbageNum() const {
        std::lock_guard lock(mtx);
        assert(garbageNum() >= (cancelledNum() + fellNum()));
        return garbageNum() - cancelledNum() - fellNum();
    }
    /*!
     * \brief 相殺する
     * \return 相殺量
     */
    std::size_t cancel(std::size_t cancel) {
        std::lock_guard lock(mtx);
        std::size_t cancel_actual =
            cancel > restGarbageNum() ? restGarbageNum() : cancel;
        cancelled_num += cancel_actual;
        return cancel_actual;
    }
    /*!
     * \brief ぷよを降らせる
     * \return 降らせるぷよの量
     */
    std::size_t fall(std::size_t fall_max) {
        std::lock_guard lock(mtx);
        std::size_t fall_actual =
            fall_max > restGarbageNum() ? restGarbageNum() : fall_max;
        fell_num += fall_actual;
        return fall_actual;
    }
    /*!
     * \brief ぷよをすべて降らせる
     */
    std::size_t fallAll() { return fall(restGarbageNum()); }

    bool done() const {
        std::lock_guard lock(mtx);
        return restGarbageNum() == 0;
    }
    std::size_t cancelledNum() const {
        std::lock_guard lock(mtx);
        return cancelled_num;
    }
    std::size_t fellNum() const {
        std::lock_guard lock(mtx);
        return fell_num;
    }
};

} // namespace PUMILA_NS
