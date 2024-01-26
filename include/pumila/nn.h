#pragma once
#include "field.h"

namespace pumila {
class Pumila {
  public:
    virtual ~Pumila() {}
    /*!
     * \brief 状態を入力として受け取り評価値を返す関数
     *
     */
    virtual double forward(const FieldState &fs) = 0;
    /*!
     * \brief 誤差を受け取り逆伝播を計算、モデルに反映
     * 
     */
    virtual void backward(double target) = 0;
};
} // namespace pumila
