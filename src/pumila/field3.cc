#include "pumila/action.h"
#include <pumila/field3.h>
namespace PUMILA_NS {
void FieldState3::set(std::size_t x, std::size_t y, Puyo p) {
    std::lock_guard lock(mtx);
    field.at(y).at(x) = p;
    updated.at(y).at(x) = true;
}

Puyo FieldState3::get(std::size_t x, std::size_t y) const {
    std::lock_guard lock(mtx);
    return field.at(y).at(x);
}
void FieldState3::clearUpdated() {
    std::lock_guard lock(mtx);
    updated = {};
}

Puyo FieldState3::nextColor() {
    std::lock_guard lock(mtx);
    int next_n = static_cast<int>(
        (static_cast<double>(rnd_next()) - rnd_next.min()) /
        (static_cast<double>(rnd_next.max()) - rnd_next.min()) * 4.0);
    switch (next_n) {
    case 0:
        return Puyo::red;
    case 1:
        return Puyo::blue;
    case 2:
        return Puyo::green;
    case 3:
    default:
        return Puyo::yellow;
    }
}

PuyoPair FieldState3::getNext(std::size_t i) const {
    std::lock_guard lock(mtx);
    return next.at(i);
}
void FieldState3::updateNext(const PuyoPair &pp) {
    std::lock_guard lock(mtx);
    next.at(0) = pp;
}
void FieldState3::shiftNext() {
    std::lock_guard lock(mtx);
    for(std::size_t i = 0; i < NextNum - 1; i++){
        next.at(i) = next.at(i + 1);
    }
    next.at(NextNum - 1) = PuyoPair(nextColor(), nextColor());
}


} // namespace PUMILA_NS