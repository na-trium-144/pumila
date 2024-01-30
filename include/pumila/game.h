#pragma once
#include "field.h"
#include "chain.h"
#include <random>
#include <memory>
#include <optional>

namespace pumila {
/*!
 * \brief fieldに加えてネクスト、落下時間、スコアなども管理する(1プレイヤー分)
 *
 * 時間の単位はフレーム (1/60s)
 *
 */
class GameSim {
    std::random_device seed;
    std::mt19937 rnd;
    Puyo randomPuyo();

  public:
    FieldState field;
    /*!
     * \brief 現在の連鎖情報
     *
     * ChainPhaseを抜けるとリセット
     *
     */
    std::optional<Chain> current_chain = std::nullopt;
    
    GameSim();
    GameSim(const GameSim &sim) = delete;
    GameSim(GameSim &&sim) = delete;

    /*!
     * \brief freePhase時のみぷよを操作する
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    void movePair(int dx);
    void rotPair(int r);
    void quickDrop();
    void softDrop();

    /*!
     * \brief 1フレーム時間を進める
     *
     */
    void step();

    /*!
     * \brief ぷよを置く
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    void put(const Action &action);

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
            free,
            fall,
            chain,
        };
        virtual PhaseEnum get() const = 0;
    };
    std::unique_ptr<Phase> phase;

    bool isFreePhase() const { return phase->get() == Phase::free; }

    struct FreePhase final : Phase {
        explicit FreePhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::free; }
        std::unique_ptr<Phase> step() override;
        static constexpr int PUT_T = 100;
        PuyoPair current_pair;
        int put_t;
    };
    struct FallPhase final : Phase {
        explicit FallPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::fall; }
        std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 30;
        int wait_t;
    };
    struct ChainPhase final : Phase {
        explicit ChainPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::chain; }
        std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 30;
        int wait_t;
    };
};

} // namespace pumila
