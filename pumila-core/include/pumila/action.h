#pragma once
#include "def.h"
#include <array>

namespace PUMILA_NS {
enum class Puyo {
    none = 0,
    red,
    blue,
    green,
    yellow,
    purple,
    garbage,
};

/*!
 * \brief ぷよの置き方
 *
 */
struct Action {
    static constexpr int START_X = 2;
    static constexpr int START_Y = 13;
    int x = START_X;
    enum class Rotation {
        vertical = 0,
        horizontal_right = 1,
        vertical_inverse = 2,
        horizontal_left = 3,
    } rot = Rotation::vertical;
    int bottomX() const { return x; }
    PUMILA_DLL int topX() const;
    /*!
     * \brief 右にn回回転する
     *
     */
    PUMILA_DLL void rotate(int right);

    bool operator==(const Action &other) const {
        return x == other.x && rot == other.rot;
    }
};

constexpr int ACTIONS_NUM = 22;
/*!
 * \brief 可能な置き方の組み合わせ全て
 *
 */
extern PUMILA_DLL const std::array<Action, ACTIONS_NUM> actions;

/*!
 * \brief 降ってくる2個組のぷよ
 *
 * bottomが回転中心
 *
 */
struct PuyoPair : Action {
    using Rotation = Action::Rotation;
    /*!
     * \brief bottomの座標
     *
     */
    double y = START_Y;
    /*!
     * \brief ぷよの色
     *
     */
    Puyo bottom = Puyo::none, top = Puyo::none;
    double bottomY() const { return y; }
    /*!
     * \brief x, y, rotからtopの座標を計算
     *
     */
    PUMILA_DLL double topY() const;

    PuyoPair() = default;
    PuyoPair(Puyo bottom, Puyo top, const Action &action = Action{})
        : Action(action), bottom(bottom), top(top) {}

    PuyoPair(const PuyoPair &pp, const Action &action)
        : Action(action), y(pp.y), bottom(pp.bottom), top(pp.top) {}

    bool operator==(const PuyoPair &other) const {
        return x == other.x && rot == other.rot && bottom == other.bottom &&
               top == other.top;
    }
};

} // namespace PUMILA_NS