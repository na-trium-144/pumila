#pragma once
#include "def.h"
#include "action.h"
#include "chain.h"
#include <utility>
#include <vector>
#include <deque>

namespace pumila {
/*!
 * \brief 1プレイヤーの盤面の情報
 *
 * nextやスコアなどは含まない
 *
 */
struct FieldState {
    static constexpr std::size_t WIDTH = 6;
    static constexpr std::size_t HEIGHT = 13;

    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }

    std::array<std::array<Puyo, WIDTH>, HEIGHT> field = {};

    /*!
     * \brief nextのぷよ
     *
     * freePhase中は操作中のぷよ+next2つで合計3組になり、それ以外の場合2組
     *
     */
    std::deque<PuyoPair> next = {};
    int prev_chain_num = 0;
    int prev_chain_score = 0;
    int total_score = 0;
    /*!
     * \brief 直前に操作した手が無効な場合false
     */
    bool is_valid = true;
    /*!
     * \brief fall()後に(2,12)が埋まったらtrue
     */
    bool is_over = false;

    PUMILA_DLL void put(std::size_t x, std::size_t y, Puyo p);

    /*!
     * \brief x, y とつながっているぷよの数を数え、
     * すでに数えたものをフィールドから消す
     * \return 削除したぷよ
     */
    PUMILA_DLL std::vector<std::pair<std::size_t, std::size_t>>
    deleteConnection(std::size_t x, std::size_t y);

    PUMILA_DLL void
    deleteConnection(std::size_t x, std::size_t y,
                     std::vector<std::pair<std::size_t, std::size_t>> &deleted);

    FieldState copy() const { return *this; }

    PUMILA_DLL Puyo get(std::size_t x, std::size_t y) const;

    /*!
     * \brief puyopairを落とす
     *
     */
    PUMILA_DLL void put(const PuyoPair &pp);
    void put() {
        if (!next.empty()) {
            put(next[0]);
        }
    }
    void put(const PuyoPair &pp, const Action &action) { put({pp, action}); }
    void put(const Action &action) {
        if (!next.empty()) {
            put({next[0], action});
        }
    }

    /*!
     * \brief puyopairを落とした場合のy座標を調べる
     * \param fall false->ちぎり前、true->ちぎり後の座標
     * \return bottom, topのそれぞれのy座標
     *
     */
   PUMILA_DLL  std::pair<std::size_t, std::size_t> getHeight(const PuyoPair &pp,
                                                  bool fall) const;
    /*!
     * \brief ぷよを1つ落とした場合のy座標を調べる
     *
     */
    PUMILA_DLL std::size_t getHeight(std::size_t x) const;

    /*!
     * \brief 4連結を探し、消す
     *
     * 盤面に4連結が無かった場合何もせず消したぷよの数は0として返る
     * \param chain_num 今消すぷよが何連鎖目か (chain_num >= 1)
     * \return 消したぷよの情報
     *
     */
    PUMILA_DLL Chain deleteChain(int chain_num);

    /*!
     * \brief 連鎖が止まるまでdeleteChainをする
     *
     */
    PUMILA_DLL std::vector<Chain> deleteChainRecurse();

    /*!
     * \brief 盤面の各マスについて消したら何連鎖が起きるかを計算する
     *
     */
    PUMILA_DLL std::array<std::array<int, WIDTH>, HEIGHT> calcChainAll() const;

    /*!
     * \brief 空中に浮いているぷよを落とす
     *
     * \return 落ちたぷよがあったらtrue
     *
     */
    PUMILA_DLL bool fall();
};
} // namespace pumila
