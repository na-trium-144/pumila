#pragma once
#include "action.h"
#include "chain.h"
#include <utility>
#include <vector>

namespace pumila {
/*!
 * \brief 1プレイヤーの盤面の情報
 *
 * nextやスコアなどは含まない
 *
 */
class FieldState {
  public:
    static constexpr std::size_t WIDTH = 6, HEIGHT = 13;

    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }

  private:
    std::array<std::array<Puyo, WIDTH>, HEIGHT> field = {};

    void put(std::size_t x, std::size_t y, Puyo p) {
        if (y < HEIGHT && x < WIDTH) {
            field.at(y).at(x) = p;
        }
    }

    /*!
     * \brief x, y とつながっているぷよの数を数え、
     * すでに数えたものをフィールドから消す
     * \return 削除したぷよ
     */
    std::vector<std::pair<std::size_t, std::size_t>>
    deleteConnection(std::size_t x, std::size_t y);

    void
    deleteConnection(std::size_t x, std::size_t y,
                     std::vector<std::pair<std::size_t, std::size_t>> &deleted);

  public:
    FieldState copy() const { return *this; }

    Puyo get(std::size_t x, std::size_t y) const { return field.at(y).at(x); }

    /*!
     * \brief puyopairを落とす
     *
     */
    void put(const PuyoPair &pp);
    void put(const PuyoPair &pp, const Action &action) { put({pp, action}); }

    /*!
     * \brief puyopairを落とした場合のy座標を調べる
     * \param fall false->ちぎり前、true->ちぎり後の座標
     * \return bottom, topのそれぞれのy座標
     *
     */
    std::pair<std::size_t, std::size_t> putTargetY(const PuyoPair &pp,
                                                   bool fall) const;
    /*!
     * \brief ぷよを1つ落とした場合のy座標を調べる
     *
     */
    std::size_t putTargetY(std::size_t x) const;

    /*!
     * \brief 4連結を探し、消す
     *
     * 盤面に4連結が無かった場合何もせず消したぷよの数は0として返る
     * \param chain_num 今消すぷよが何連鎖目か (chain_num >= 1)
     * \return 消したぷよの情報
     *
     */
    Chain deleteChain(int chain_num);

    /*!
     * \brief 連鎖が止まるまでdeleteChainをする
     *
     */
    std::vector<Chain> deleteChainRecurse();

    /*!
     * \brief 盤面の各マスについて消したら何連鎖が起きるかを計算する
     *
     */
    std::array<std::array<int, WIDTH>, HEIGHT> calcChainAll() const;

    /*!
     * \brief 空中に浮いているぷよを落とす
     *
     * \return 落ちたぷよがあったらtrue
     *
     */
    bool fall();
};
} // namespace pumila
