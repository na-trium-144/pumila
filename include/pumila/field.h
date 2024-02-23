#pragma once
#include "def.h"
#include "action.h"
#include "chain.h"
#include <utility>
#include <vector>
#include <deque>
#include <shared_mutex>
#include <memory>

namespace PUMILA_NS {
/*!
 * \brief 1プレイヤーの盤面の情報
 *
 * nextやスコアなどは含まない
 *
 */
struct FieldState {
    static constexpr std::size_t WIDTH = 6;
    static constexpr std::size_t HEIGHT = 13;

    /*!
     * \brief x, yがフィールドの範囲内かどうか判定
     *
     */
    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }

    std::array<std::array<Puyo, WIDTH>, HEIGHT> field;

    /*!
     * \brief 盤面が変化したかどうか
     * * put時にtrue (←fall時にもtrue, deleteConnection時にもtrue)
     * * deleteChainでchanged=trueのみチェック
     * * 次のput時にclearUpdateFlagする
     * * FieldStateのコピーでコピーされない
     */
    std::array<std::array<bool, WIDTH>, HEIGHT> updated;

    /*!
     * \brief nextのぷよ
     *
     * freePhase中は操作中のぷよ+next2つで合計3組になり、それ以外の場合2組
     *
     */
    std::array<PuyoPair, 3> next;
    std::size_t next_num = 0;
    /*!
     * \brief 試合開始からの手数
     */
    int step_num = 0;
    /*!
     * \brief 最後の連鎖の連鎖数と得点
     */
    int prev_chain_num = 0, prev_chain_score = 0;
    /*!
     * \brief 最後の連鎖とその前の連鎖の間のステップ数
     * pumila7で使用
     */
    int last_chain_step_num = 0;
    /*!
     * \brief 現在スコア
     * 表示用
     */
    int total_score = 0;

    /*!
     * \brief 確定したおじゃま (自分の盤面に降る量)
     */
    int garbage_ready = 0;
    /*!
     * \brief 現在の連鎖のおじゃま (相殺する量 + 相手に送る量)
     */
    int garbage_current = 0;
    /*!
     * \brief おじゃまの計算に使うスコア
     */
    int garbage_score = 0;

    /*!
     * \brief 直前に操作した手が無効な場合false
     */
    bool is_valid = true;
    /*!
     * \brief fall()後に(2,12)が埋まったらtrue
     */
    bool is_over = false;

    FieldState() : field(), updated(), next() {}
    std::shared_ptr<FieldState> copy() const {
        return std::make_shared<FieldState>(*this);
    }

    void clearUpdateFlag() {
        for (std::size_t y = 0; y < HEIGHT; y++) {
            updated.at(y).fill(false);
        }
    }

    PUMILA_DLL void addCurrentGarbage(int score_add, int rate = 70);
    PUMILA_DLL int calcGarbageSend();

    PUMILA_DLL void put(std::size_t x, std::size_t y, Puyo p);

    struct PuyoConnection {
        std::vector<std::pair<std::size_t, std::size_t>> colored, garbage;
        PuyoConnection() : colored(), garbage() {}
    };

    /*!
     * \brief x, y とつながっているぷよの数を数え、
     * すでに数えたものをフィールドから消す
     * \return 削除したぷよ
     */
    PUMILA_DLL PuyoConnection deleteConnection(std::size_t x, std::size_t y);

    /*!
     * \brief x, y とつながっているぷよの数を数え、
     * すでに数えたものをフィールドから消す
     */
    PUMILA_DLL void deleteConnection(std::size_t x, std::size_t y,
                                     PuyoConnection &deleted);

    /*!
     * \brief ぷよを取得
     * 範囲外の場合例外
     */
    PUMILA_DLL Puyo get(std::size_t x, std::size_t y) const;

    PUMILA_DLL PuyoPair getNext() const;
    PUMILA_DLL void updateNext(const PuyoPair &pp);
    PUMILA_DLL void popNext();
    PUMILA_DLL void pushNext(const PuyoPair &pp);

    /*!
     * \brief puyopairを落とす
     */
    PUMILA_DLL void put(const PuyoPair &pp);
    /*!
     * \brief next[0]を落とす
     */
    void put() { put(getNext()); }
    /*!
     * \brief puyopairを指定した位置に落とす
     */
    void put(const PuyoPair &pp, const Action &action) { put({pp, action}); }
    /*!
     * \brief next[0]を指定した位置に落とす
     */
    void put(const Action &action) { put(getNext(), action); }

    /*!
     * \brief 落下中のぷよが既存のぷよに重なっているまたは画面外か調べる
     * \return フィールド上のぷよと重なるor画面外ならtrue
     */
    PUMILA_DLL bool checkCollision(const PuyoPair &pp);

    /*!
     * \brief puyopairを落とした場合のy座標を調べる
     * \param fall false->ちぎり前、true->ちぎり後の座標
     * \return bottom, topのそれぞれのy座標
     *
     */
    PUMILA_DLL std::pair<std::size_t, std::size_t> getHeight(const PuyoPair &pp,
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

    /*!
     * \brief 12,2を調べis_overにセットするとともに返す
     */
    PUMILA_DLL bool checkGameOver();
};
} // namespace PUMILA_NS
