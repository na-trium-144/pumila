#include <pumila/action.h>

namespace PUMILA_NS {
const std::array<Action, 22> actions = {{
    {0, Action::Rotation::vertical},
    {1, Action::Rotation::vertical},
    {2, Action::Rotation::vertical},
    {3, Action::Rotation::vertical},
    {4, Action::Rotation::vertical},
    {5, Action::Rotation::vertical},
    {0, Action::Rotation::vertical_inverse},
    {1, Action::Rotation::vertical_inverse},
    {2, Action::Rotation::vertical_inverse},
    {3, Action::Rotation::vertical_inverse},
    {4, Action::Rotation::vertical_inverse},
    {5, Action::Rotation::vertical_inverse},
    {0, Action::Rotation::horizontal_right},
    {1, Action::Rotation::horizontal_right},
    {2, Action::Rotation::horizontal_right},
    {3, Action::Rotation::horizontal_right},
    {4, Action::Rotation::horizontal_right},
    {1, Action::Rotation::horizontal_left},
    {2, Action::Rotation::horizontal_left},
    {3, Action::Rotation::horizontal_left},
    {4, Action::Rotation::horizontal_left},
    {5, Action::Rotation::horizontal_left},
}};

int PuyoPair::topX() const {
    switch (rot) {
    case Rotation::vertical:
    case Rotation::vertical_inverse:
        return x;
    case Rotation::horizontal_left:
        return x - 1;
    case Rotation::horizontal_right:
        return x + 1;
    }
    return 0;
}
double PuyoPair::topY() const {
    switch (rot) {
    case Rotation::vertical:
        return y + 1;
    case Rotation::vertical_inverse:
        return y - 1;
    case Rotation::horizontal_left:
    case Rotation::horizontal_right:
        return y;
    }
    return 0;
}
void PuyoPair::rotate(int right) {
    rot = static_cast<Rotation>(((static_cast<int>(rot) + right) % 4 + 4) % 4);
}

} // namespace PUMILA_NS