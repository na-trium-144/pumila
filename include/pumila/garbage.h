#pragma once
#include "def.h"
#include <chrono>
#include <future>
#include <memory>

namespace PUMILA_NS {
/*!
 * \brief 1回の連鎖で送るおじゃまのまとまり
 */
class GarbageGroup {
    int garbage_num;
    std::shared_ptr<std::promise<int>> cancelled_p;
    std::shared_future<int> cancelled_num;

  public:
    GarbageGroup() : garbage_num(0), cancelled_num() {}
    explicit GarbageGroup(int garbage_num)
        : garbage_num(garbage_num),
          cancelled_p(std::make_shared<std::promise<int>>()),
          cancelled_num(cancelled_p->get_future().share()) {}

    /*!
     * \brief もとのおじゃまの数
     */
    int garbageNum() const { return garbage_num; }
    /*!
     * \brief 相殺する
     * \return 相殺量がgarbageNumより多い場合余りをreturnする
     *
     * 足りない場合fellNum()で取得
     */
    int setCancelled(int cancel) {
        if (cancelled_p) {
            cancelled_p->set_value(cancel > garbageNum() ? garbageNum()
                                                         : cancel);
        }
        return cancel - cancelledNum();
    }
    /*!
     * \brief 落下する量を返す
     * setCancelledを呼んでいない場合自動的に0をセットする
     */
    int fall() {
        setCancelled(0);
        return fellNum();
    }

    bool ready() const {
        return cancelled_num.valid() &&
               cancelled_num.wait_for(std::chrono::milliseconds(0)) ==
                   std::future_status::ready;
    }
    int cancelledNum() const {
        return cancelled_num.valid() ? cancelled_num.get() : 0;
    }
    int fellNum() const { return garbageNum() - cancelledNum(); }
};

} // namespace PUMILA_NS
