#pragma once
#include "game.h"
#include "chain.h"
#include <random>

namespace pumila {
class Window {
    GameState state = {};
    PuyoPair current_pair = {}, next_pair = {}, next2_pair = {};
    int score = 0;
    Chain last_chain = {};

    std::random_device seed;
    std::mt19937 rnd;

    void *sdl_window_p = nullptr, *sdl_renderer_p = nullptr;
    static constexpr int WIDTH = 640, HEIGHT = 480;

    Puyo randomPuyo();

  public:
    Window();
    void loop();

    void init();

};
} // namespace pumila
