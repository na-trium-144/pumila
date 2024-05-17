#pragma once
#include "def.h"
#include "field3.h"
#include "chain.h"
#include "pumila/step.h"
#include <random>
#include <memory>
#include <optional>
#include <vector>

namespace PUMILA_NS {
// class Pumila;
// struct FieldState;

/*!
 * \brief fieldに加えてネクスト、落下時間、スコアなども管理する(1プレイヤー分)
 *
 * 時間の単位はフレーム (1/60s)
 *
 */
class GameSim : public std::enable_shared_from_this<GameSim> {
    // std::optional<std::thread> model_action_thread;
    // std::atomic<bool> running;
    std::optional<Action> soft_put_target = std::nullopt;

    /*!
     * \brief rotPairして失敗した場合その回転方向が入る
     * 回転できた場合0
     */
    int rot_fail = 0;
    int rot_fail_count = 0;
    static constexpr int ROT_FAIL_COUNT = 10;

  public:
    bool enable_garbage;

    /*!
     * \brief おじゃまぷよを送る相手をセットしてね
     */
    std::weak_ptr<GameSim> opponent;

    std::optional<FieldState3> field;
    // PUMILA_DLL std::optional<FieldState2> field2();
    // PUMILA_DLL std::shared_ptr<FieldState> field1();

    // std::shared_ptr<Pumila> model;
    // std::string name;

    int soft_put_interval = 6;
    int soft_put_cnt = 0;

    bool is_over = false;

    struct Phase {
        GameSim *sim;
        explicit Phase(GameSim *sim) : sim(sim) {}
        virtual ~Phase() {}
        /*!
         * \brief 処理を1周期進める
         *
         * \return 別のフェーズに移行する場合新しいフェーズ、
         * そうでなければnullptr
         *
         */
        virtual std::unique_ptr<Phase> step() = 0;
        enum PhaseEnum {
            none,
            free,
            fall,
            garbage,
        };
        virtual PhaseEnum get() const = 0;
    };
    std::unique_ptr<Phase> phase;

    /*!
     * \brief FreePhaseを抜ける時に作られ、
     * Fall, GarbagePhaseで情報が更新される
     */
    std::shared_ptr<StepResult> current_step;

  private:
    // PUMILA_DLL explicit GameSim(
    //     std::shared_ptr<Pumila> model, const std::string &name = "",
    //     typename std::mt19937::result_type seed = std::random_device()(),
    //     bool enable_garbage = true);
    PUMILA_DLL explicit GameSim(
        typename std::mt19937::result_type seed = std::random_device()(),
        bool enable_garbage = true);

  public:
    static std::shared_ptr<GameSim> makeNew() {
        return std::shared_ptr<GameSim>(new GameSim());
    }
    static std::shared_ptr<GameSim>
    makeNew(typename std::mt19937::result_type seed, bool enable_garbage) {
        return std::shared_ptr<GameSim>(new GameSim(seed, enable_garbage));
    }

    GameSim(const GameSim &sim) = delete;
    GameSim &operator=(const GameSim &) = delete;
    GameSim(GameSim &&sim) = delete;
    GameSim &operator=(GameSim &&sim) = delete;

    // ~GameSim() { stopAction(); }
    // PUMILA_DLL void stopAction();

    // bool hasModel() { return model != nullptr; }

    PUMILA_DLL void
    reset(typename std::mt19937::result_type seed = std::random_device()());

    /*!
     * \brief 相手simを相互にセットする
     */
    PUMILA_DLL void setOpponentSim(const std::shared_ptr<GameSim> &opponent_s);

    /*!
     * \brief freePhase時のみぷよを操作する
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    PUMILA_DLL void movePair(int dx);
    PUMILA_DLL void rotPair(int r);
    PUMILA_DLL void quickDrop();
    PUMILA_DLL void softDrop();

    /*!
     * \brief 1フレーム時間を進める
     *
     */
    PUMILA_DLL void step();

    /*!
     * \brief ぷよを瞬間移動で置く
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    PUMILA_DLL void put(const Action &action);
    /*!
     * \brief ぷよを目標位置まで動かして置くときはここにセットしてstep()を呼ぶ
     */
    PUMILA_DLL void softPut(const Action &action);

    struct GarbagePhase final : Phase {
        PUMILA_DLL explicit GarbagePhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::garbage; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 20;
        int wait_t;
    };
    struct FreePhase final : Phase {
        PUMILA_DLL explicit FreePhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::free; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int PUT_T = 100;
        static constexpr double FALL_SPEED = 1.0;
        static constexpr double SOFT_SPEED = 25.0;
        int put_t;
    };
    struct FallPhase final : Phase {
        PUMILA_DLL explicit FallPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::fall; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int FALL_T = 20;
        static constexpr int CHAIN_T = 30;
        std::vector<int> chain_t;
        std::size_t current_chain;
        int fall_wait_t;
        FieldState3 display_field;
    };
};

} // namespace PUMILA_NS
