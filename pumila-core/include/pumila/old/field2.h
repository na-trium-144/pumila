#pragma once
#include "def.h"
#include "action.h"
#include "chain.h"
#include <array>
#include <utility>
#include <random>

namespace PUMILA_NS {
/*!
 * \brief 1プレイヤーの盤面の情報
 */
class FieldState2 {
    inline static std::mt19937 rnd_general{std::random_device()()};

  public:
    static constexpr std::size_t WIDTH = 6;
    static constexpr std::size_t HEIGHT = 13;
    /*!
     * \brief x, yがフィールドの範囲内かどうか判定
     */
    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }

    //! 盤面
    class Field {
        std::array<std::array<Puyo, WIDTH>, HEIGHT> field = {};
        /*!
         * \brief 盤面が変化したかどうか
         * * set時にtrue
         * * deleteChainでchanged=trueのみチェック
         * * putNext時にクリア
         */
        std::array<std::array<bool, WIDTH>, HEIGHT> updated = {};
        /*!
         * \brief ぷよ総数
         */
        int puyo_num = 0;

      public:
        friend struct FieldState;

        PUMILA_DLL void set(std::size_t x, std::size_t y, Puyo p);
        PUMILA_DLL Puyo get(std::size_t x, std::size_t y) const;
        PUMILA_DLL bool getUpdated(std::size_t x, std::size_t y) const;
        void clearUpdated() { updated = {}; }
        int puyoNum() const { return puyo_num; }
        bool operator==(const Field &other) const {
            return field == other.field;
        }
    };
    class NextList {
        /*!
         * \brief nextのぷよ(操作中の位置情報を含む)
         * * freePhase中は操作中のぷよ+next2つで合計3組になり、それ以外の場合2組
         */
        std::array<PuyoPair, 3> next = {};
        std::mt19937 rnd_next;
        PUMILA_DLL Puyo nextColor();

      public:
        friend struct FieldState;
        PUMILA_DLL explicit NextList(typename std::mt19937::result_type seed);

        PUMILA_DLL PuyoPair get() const;
        PUMILA_DLL void update(const PuyoPair &pp);
        PUMILA_DLL void pop();
        const PuyoPair &operator[](int i) const { return next.at(i); }
        bool operator==(const NextList &other) const {
            return next == other.next && rnd_next == other.rnd_next;
        }
    };
    class StepInfo {
        int num_;
        std::vector<Chain> chains_;
        int chain_score_;

      public:
        explicit StepInfo(int num) : num_(num), chains_(), chain_score_(0) {}
        PUMILA_DLL void pushChain(const Chain &chain);
        std::size_t chainNum() const { return chains_.size(); }
        int num() const { return num_; }
        const std::vector<Chain> &chains() const { return chains_; }
        int chainScore() const { return chain_score_; }
        /*!
         * \brief wait_timeを1減らし、現在waitしている連鎖の情報を返す
         */
        PUMILA_DLL const Chain *step();

        bool operator==(const StepInfo &other) const {
            return num_ == other.num_ && chains_ == other.chains_ &&
                   chain_score_ == other.chain_score_;
        }
    };
    class GarbageInfo {
        static constexpr int rate = 70;
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

      public:
        friend struct FieldState;

        PUMILA_DLL void pushScore(int score_add);
        void addGarbage(int garbage_add) { garbage_ready += garbage_add; }
        /*!
         * \brief 盤面に降る量をreadyにセットし相手に送る量を返す
         */
        PUMILA_DLL int send();
        int getReady() const { return garbage_ready; }
        int getCurrent() const { return garbage_current; }
        bool operator==(const GarbageInfo &other) const {
            return garbage_current == other.garbage_current &&
                   garbage_score == other.garbage_score;
        }
    };

  private:
    Field field_;
    NextList next_;
    /*!
     * \brief 現在、1手前、最後に連鎖したときの手数と連鎖の情報
     *
     * * putNextでcurrentをprevに移動し初期化、
     * * deleteChainでcurrentを更新
     */
    StepInfo current_step_, prev_step_, last_chain_step_;
    /*!
     * \brief 1手前のぷよ総数
     */
    int prev_puyo_num_ = 0;
    /*!
     * \brief 現在スコア
     * * deleteChainで更新
     */
    int total_score_ = 0;

    GarbageInfo garbage_;

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

  public:
    bool operator==(const FieldState2 &other) const {
        return field_ == other.field_ && next_ == other.next_ &&
               current_step_ == other.current_step_ &&
               prev_step_ == other.prev_step_ &&
               last_chain_step_ == other.last_chain_step_ &&
               prev_puyo_num_ == other.prev_puyo_num_ &&
               total_score_ == other.total_score_ && garbage_ == other.garbage_;
    }
    bool operator!=(const FieldState2 &other) const {
        return !(*this == other);
    }

    FieldState2(
        typename std::mt19937::result_type seed = std::random_device()())
        : field_(), next_(seed), current_step_(0), prev_step_(0),
          last_chain_step_(0), garbage_() {}

    const Field &field() const { return field_; }
    NextList &next() { return next_; }
    const NextList &next() const { return next_; }
    const StepInfo &currentStep() const { return current_step_; }
    StepInfo &currentStep() { return current_step_; }
    const StepInfo &prevStep() const { return prev_step_; }
    const StepInfo &lastChainStep() const { return last_chain_step_; }
    int prevPuyoNum() const { return prev_puyo_num_; }
    int totalScore() const { return total_score_; }
    GarbageInfo &garbage() { return garbage_; }
    const GarbageInfo &garbage() const { return garbage_; }

    /*!
     * \brief nextを指定した位置に落とす
     * * field.updatedをクリア
     * * nextは削除する
     * * currentstep, prevstep, lastchainstepを更新
     * * prevpuyonumを更新
     */
    PUMILA_DLL void putNext(const Action &action);
    void putNext() { putNext(next_.get()); }
    /*!
     * \brief garbageを降らせる
     *
     * garbage.readyを0にする or 30減らす
     *
     * \param garbage_list おじゃまが降った位置が返る
     */
    PUMILA_DLL void putGarbage(std::array<std::pair<std::size_t, std::size_t>,
                                          30> *garbage_list = nullptr,
                               std::size_t *garbage_num = nullptr);

    /*!
     * \brief 落下中のぷよが既存のぷよに重なっているまたは画面外か調べる
     * \return フィールド上のぷよと重なるor画面外ならtrue
     */
    PUMILA_DLL bool checkNextCollision(const Action &action) const;
    bool checkNextCollision() const { return checkNextCollision(next_.get()); }

    /*!
     * \brief nextを落とした場合のy座標を調べる
     * \return bottom, topのそれぞれのy座標
     */
    PUMILA_DLL std::pair<std::size_t, std::size_t>
    getNextHeight(const Action &action) const;
    std::pair<std::size_t, std::size_t> getNextHeight() const {
        return getNextHeight(next_.get());
    }
    PUMILA_DLL std::size_t getHeight(std::size_t x) const;

    /*!
     * \brief 4連結を探し、消す
     * * garbage.pushScoreに追加
     * * total_scoreに追加
     * * current_stepを更新
     * * 盤面に4連結が無かった場合何もせず消したぷよの数は0として返る
     * \return 消したぷよの情報
     */
    PUMILA_DLL Chain deleteChain();
    /*!
     * \brief 空中に浮いているぷよを落とす
     * \return 落ちたぷよがあったらtrue
     */
    PUMILA_DLL bool fall();
    /*!
     * \brief 連鎖が止まるまでdeleteChainとfallをする
     */
    PUMILA_DLL std::vector<Chain> deleteChainRecurse();

    /*!
     * \brief 盤面の各マスについて消したら何連鎖が起きるかを計算する
     */
    PUMILA_DLL std::array<std::array<int, WIDTH>, HEIGHT> calcChainAll() const;

    /*!
     * \brief 11,2を調べ埋まっているかどうか返す
     */
    bool isGameOver() const { return field_.get(2, 11) != Puyo::none; }
};
} // namespace PUMILA_NS