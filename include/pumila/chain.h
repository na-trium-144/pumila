#pragma once
#include "game.h"
#include <vector>
#include <utility>

namespace pumila {
struct Chain {
  /*!
   * \brief 消去後の盤面
   *
   */
  GameState state_after;
  /*!
   * \brief ぷよ連結数の情報
   *
   * 例えば赤4連結+青5連結の9個なら{{red, 4}, {blue, 5}}
   */
  std::vector<std::pair<Puyo, int>> connections;

  int connectionNum() const;
  static int chainBonus(int chain_num);
  int connectionBonus() const;
  int colorBonus() const;
  int score(int chain_num) const;
};

} // namespace pumila
