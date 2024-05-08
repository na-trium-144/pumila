#pragma once
#include "def.h"
#include <cstddef>
#include <array>
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

    static constexpr std::size_t WIDTH = 6;
    static constexpr std::size_t HEIGHT = 13;
    std::array<std::array<Puyo, WIDTH>, HEIGHT> field;

    /*!
     * \brief 盤面が変化したかどうか
     * * set時にtrue
     * * deleteChainでchanged=trueのみチェック
     * * putNext時にクリア
     */
    std::array<std::array<bool, WIDTH>, HEIGHT> updated;

    static constexpr std::size_t NextNum = 3;
    std::mt19937 rnd_next;
    PUMILA_DLL Puyo nextColor();

    /*!
     * \brief nextのぷよ(操作中の位置情報を含む)
     */
    std::array<PuyoPair, NextNum> next;
    PUMILA_DLL void shiftNext();

    /*!
     * \brief 自フィールドに降るおじゃま
     */
    std::vector<GarbageGroup> garbage_ready;
    /*!
     * \brief スコア
     */
    int total_score;
    /*!
     * \brief 現在進行中の連鎖
     */
    std::vector<Chain> current_chain;
    /*!
     * \brief current_chainの開始時刻, 終了時刻
     */
    int chain_begin_t, chain_end_t;

  public:
    FieldState3() = default;
    explicit FieldState3(std::uint_fast32_t seed)
        : field(), updated(), rnd_next(seed), garbage_ready(), total_score(),
          current_chain(), chain_begin_t(), chain_end_t() {
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
        // updated
        rnd_next = other.rnd_next;
        garbage_ready = other.garbage_ready;
        total_score = other.total_score;
        current_chain = other.current_chain;
        chain_begin_t = other.chain_begin_t;
        chain_end_t = other.chain_end_t;
        return *this;
    }
    FieldState3 &operator=(FieldState3 &&other) {
        std::lock_guard lock(other.mtx);
        field = std::move(other.field);
        // updated
        rnd_next = std::move(other.rnd_next);
        garbage_ready = std::move(other.garbage_ready);
        total_score = std::move(other.total_score);
        current_chain = std::move(other.current_chain);
        chain_begin_t = std::move(other.chain_begin_t);
        chain_end_t = std::move(other.chain_end_t);
        return *this;
    }

    /*!
     * \brief x, yがフィールドの範囲内かどうか判定
     *
     */
    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }
    PUMILA_DLL Puyo get(std::size_t x, std::size_t y) const;
    PUMILA_DLL void set(std::size_t x, std::size_t y, Puyo p);
    PUMILA_DLL void clearUpdated();

    PUMILA_DLL PuyoPair getNext(std::size_t i = 0) const;
    PUMILA_DLL void updateNext(const PuyoPair &pp);

};
} // namespace PUMILA_NS
