#pragma once
#include "field.h"
#include <vector>
#include <utility>

namespace pumila {
/*!
 * \brief 1連鎖で消えるぷよの情報
 *
 */
struct Chain {
    /*!
     * \brief 消去後の盤面
     *
     */
    FieldState state_after;
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
    int scoreA() const;
    int scoreB(int chain_num) const;
    int score(int chain_num) const { return scoreA() * scoreB(chain_num); };
};

} // namespace pumila
