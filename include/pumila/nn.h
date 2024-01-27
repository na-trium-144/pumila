#pragma once
#include "game.h"

namespace pumila {
class Pumila {
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
     */
    virtual void backward(int action, double target) = 0;
};
} // namespace pumila
