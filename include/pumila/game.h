#pragma once
#include <utility>
#include <array>
#include <cstdint>

namespace pumila {
enum class Puyo {
    none = 0,
    red,
    blue,
    green,
    yellow,
    purple,
    garbage,
};
struct PuyoPair {
    Puyo bottom, top;
};
enum class Rotation {
    vertical,
    horizontal_right,
    vertical_inverse,
    horizontal_left,
};
constexpr std::size_t WIDTH = 6, HEIGHT = 13;
struct Chain;
class GameState {
    std::array<std::array<Puyo, WIDTH>, HEIGHT> field = {};

    void put(std::size_t x, std::size_t y, Puyo p) {
        if (y < HEIGHT && x < WIDTH) {
            field.at(y).at(x) = p;
        }
    }

    /*!
     * \brief x, y とつながっているぷよの数を数える
     *
     * すでに数えたものはフィールドから消してしまう
     *
     */
    int findConnect(std::size_t x, std::size_t y);

  public:
    Puyo get(std::size_t x, std::size_t y) const { return field.at(y).at(x); }

    /*!
     * \brief ツモを置く
     *
     */
    GameState put(const PuyoPair &pp, Rotation rot, std::size_t x) const;

    /*!
     * \brief 4連結を探す
     *
     */
    Chain findChain() const;
    /*!
     * \brief 空中に浮いているぷよを落とす
     *
     */
    void fall();
};
} // namespace pumila
