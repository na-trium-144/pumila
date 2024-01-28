#pragma once
#include "game.h"
#include <BS_thread_pool.hpp>

namespace pumila {
class Pumila {
  protected:
    inline static BS::thread_pool pool;

  public:
    virtual ~Pumila() {}
    /*!
     * \brief 現在の状態s(t)を入力として受け取り、
     * 22個の操作に対する評価値Q(s(t), a(t))を返す
     *
     */
    virtual std::array<double, ACTIONS_NUM>
    forward(std::shared_ptr<GameSim> sim) = 0;

    /*!
     * \brief 教師信号r(t)を受け取り逆伝播を計算、モデルに反映
     *
     * \param sim_after アクション実行後の状態
     * \param action_prev 最後に選択したアクション (0 <= a < 22)
     * \param target 教師信号
     *
     */
    virtual void backward(std::shared_ptr<GameSim> sim_after, int action_prev,
                          double target) = 0;
};
} // namespace pumila
