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
    PuyoPair next_pair, next2_pair;
    PuyoPair getCurrentPair() const;
    int score = 0;
    std::optional<Chain> current_chain = std::nullopt;

    GameSim();
    GameSim(const GameSim &sim) = delete;
    GameSim(GameSim &&sim) = delete;

    void movePair(int dx);
    void rotPair(int r);
    void quickDrop();
    void softDrop();

    void step();

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
