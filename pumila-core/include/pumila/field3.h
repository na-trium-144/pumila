#pragma once
#include "def.h"
#include <cstddef>
#include <array>
#include <memory>
#include <random>
#include <vector>
#include <mutex>
#include "action.h"
#include "garbage.h"
#include "chain.h"

namespace PUMILA_NS {
/*!
 * \brief ある瞬間のフィールドの状態。
 * 履歴は含まなくていい
 */
class FieldState3 {
    mutable std::recursive_mutex mtx;

  public:
    static constexpr std::size_t WIDTH = 6;
    static constexpr std::size_t HEIGHT = 13;

  private:
    std::array<std::array<Puyo, WIDTH>, HEIGHT> field;

    /*!
     * \brief x, y とつながっているぷよの数を数え、フィールドから消す
     */
    PUMILA_DLL void deleteConnectionImpl(std::size_t x, std::size_t y,
                                         PuyoConnection &deleted);
    /*!
     * \brief x, y とつながっているぷよの数を数え、フィールドから消す
     * \return 削除したぷよ
     */
    PUMILA_DLL PuyoConnection deleteConnection(std::size_t x, std::size_t y);

    /*!
     * \brief 盤面が変化したかどうか
     * * set時にtrue
     * * deleteChainでchanged=trueのみチェック
     * * putNext時にクリア
     */
    std::array<std::array<bool, WIDTH>, HEIGHT> updated;
    /*!
     * \brief updatedをクリア
     */
    PUMILA_DLL void clearUpdated();

    static constexpr std::size_t NextNum = 3;
    std::mt19937 rnd_next;
    /*!
     * \brief 乱数を1進める
     */
    PUMILA_DLL Puyo nextColor();

    /*!
     * \brief nextのぷよ(操作中の位置情報を含む)
     */
    std::array<PuyoPair, NextNum> next;
    /*!
     * \brief nextを1つすすめて補充
     */
    PUMILA_DLL void shiftNext();

    /*!
     * \brief 自フィールドに降るおじゃま
     */
    std::vector<std::shared_ptr<GarbageGroup>> garbage_ready;
    /*!
     * \brief おじゃま計算用スコア
     */
    int garbage_score;

    int total_score;

  public:
    FieldState3()
        : field(), updated(), rnd_next(), next(), garbage_ready(),
          garbage_score(0), total_score(0) {}
    explicit FieldState3(std::uint_fast32_t seed) : FieldState3() {
        rnd_next.seed(seed);
        for (std::size_t i = 0; i < NextNum; i++) {
            shiftNext();
        }
    }

    FieldState3(const FieldState3 &other) : FieldState3() { *this = other; }
    FieldState3(FieldState3 &&other) : FieldState3() {
        *this = std::move(other);
    }
    FieldState3 &operator=(const FieldState3 &other) {
        std::lock_guard lock(other.mtx);
        field = other.field;
        updated = other.updated;
        rnd_next = other.rnd_next;
        next = other.next;
        garbage_ready = other.garbage_ready;
        garbage_score = other.garbage_score;
        total_score = other.total_score;
        return *this;
    }
    FieldState3 &operator=(FieldState3 &&other) {
        std::lock_guard lock(other.mtx);
        field = std::move(other.field);
        updated = std::move(other.updated);
        rnd_next = std::move(other.rnd_next);
        next = std::move(other.next);
        garbage_ready = std::move(other.garbage_ready);
        garbage_score = other.garbage_score;
        total_score = other.total_score;
        return *this;
    }

    /*!
     * \brief x, yがフィールドの範囲内かどうか判定
     *
     */
    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }
    /*!
     * \brief フィールドを取得
     */
    PUMILA_DLL Puyo get(std::size_t x, std::size_t y) const;
    /*!
     * \brief フィールドを上書き
     */
    PUMILA_DLL void set(std::size_t x, std::size_t y, Puyo p);

    /*!
     * \brief i番目のnextを取得
     */
    PUMILA_DLL PuyoPair getNext(std::size_t i) const;
    /*!
     * \brief next[0]を上書き
     */
    PUMILA_DLL void updateNext(const PuyoPair &pp);

    /*!
     * \brief 相手からのおじゃまを追加
     * \param garbage 送られてきたおじゃま nullptrも可
     */
    PUMILA_DLL void addGarbage(const std::shared_ptr<GarbageGroup> &garbage);
    /*!
     * \brief 現在のおじゃまを取得
     */
    PUMILA_DLL std::size_t getGarbageNumTotal() const;
    /*!
     * \brief garbageを降らせる:
     * garbageReadyを0にする or 30減らす
     * \param garbage_list おじゃまが降った位置が返る
     */
    PUMILA_DLL void putGarbage(std::vector<std::pair<std::size_t, std::size_t>>
                                   *garbage_list = nullptr);
    static constexpr int GARBAGE_RATE = 70;
    /*!
     * \brief 生成されるおじゃま数を計算, garbage_scoreに加算
     * \return おじゃま数
     */
    PUMILA_DLL std::size_t calcGarbage(int score_add);
    /*!
     * \brief おじゃまを相殺する
     * \return 相殺した数
     */
    PUMILA_DLL std::size_t cancelGarbage(std::size_t garbage_num);

    /*!
     * \brief nextを落とした場合のy座標を調べる
     * \return bottom, topのそれぞれのy座標
     */
    PUMILA_DLL std::pair<std::size_t, std::size_t>
    getNextHeight(const Action &action) const;
    std::pair<std::size_t, std::size_t> getNextHeight() const {
        return getNextHeight(getNext(0));
    }
    PUMILA_DLL std::size_t getHeight(std::size_t x) const;
    /*!
     * \brief 落下中のぷよが既存のぷよに重なっているまたは画面外か調べる
     * \return フィールド上のぷよと重なるor画面外ならtrue
     */
    PUMILA_DLL bool checkNextCollision(const Action &action) const;
    bool checkNextCollision() const { return checkNextCollision(getNext(0)); }

    /*!
     * \brief nextを指定した位置に落とす
     * * field.updatedをクリア
     * * nextを削除する
     */
    PUMILA_DLL void putNext();

    /*!
     * \brief 4連結を探し、消す
     * * total_scoreに追加
     * * 盤面に4連結が無かった場合何もせず消したぷよの数は0として返る
     * \param chain_num 連鎖数(1連鎖目→1)
     * \return 消したぷよの情報
     */
    PUMILA_DLL Chain deleteChain(std::size_t chain_num);
    /*!
     * \brief 空中に浮いているぷよを落とす
     * \return 落ちたぷよがあったらtrue
     */
    PUMILA_DLL bool fall();

    /*!
     * \brief 連鎖が止まるまでdeleteChain,fallをする
     */
    PUMILA_DLL std::vector<Chain> deleteChainRecurse();

    /*!
     * \brief 盤面の各マスについて消したら何連鎖が起きるかを計算する
     */
    PUMILA_DLL std::array<std::array<std::size_t, WIDTH>, HEIGHT>
    calcChainAll() const;

    PUMILA_DLL int totalScore() const;
    /*!
     * \brief 11,2を調べ埋まっているかどうか返す
     */
    bool isGameOver() const { return get(2, 11) != Puyo::none; }
};
} // namespace PUMILA_NS
