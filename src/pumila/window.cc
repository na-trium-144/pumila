#include <pumila/window.h>
#include <iostream>

#ifdef PUMILA_SDL2
#include <SDL2/SDL.h>
#endif

namespace pumila {
Window::Window() : seed(), rnd(seed()) {
#ifdef PUMILA_SDL2
    SDL_Window *sdl_window = nullptr;
    SDL_Renderer *sdl_renderer = nullptr;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
    } else {
        sdl_window =
            SDL_CreateWindow("Pumila", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
        sdl_window_p = static_cast<void *>(sdl_window);
        if (!sdl_window) {
            std::cerr << "Couldn't create window: " << SDL_GetError()
                      << std::endl;
        } else {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            sdl_renderer =
                SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
            sdl_renderer_p = static_cast<void *>(sdl_renderer);
            if (!sdl_renderer) {
                std::cerr << "Couldn't create renderer: " << SDL_GetError()
                          << std::endl;
            }
        }
    }
#else
    std::cerr << "Pumila not built with SDL2" << std::endl;
#endif
}

void Window::loop() {
#ifdef PUMILA_SDL2
    if (sdl_window_p && sdl_renderer_p) {
        SDL_Window *sdl_window = static_cast<SDL_Window *>(sdl_window_p);
        SDL_Renderer *sdl_renderer =
            static_cast<SDL_Renderer *>(sdl_renderer_p);
        while (true) {
            SDL_SetRenderDrawColor(sdl_renderer, 96, 128, 255, 255);
            SDL_RenderClear(sdl_renderer);
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                    SDL_Quit();
                    return;
                default:
                    break;
                }
            }
            SDL_RenderPresent(sdl_renderer);
            SDL_Delay(16);
        }
    }
#endif
}

Puyo Window::randomPuyo() {
    switch (static_cast<int>(static_cast<double>(rnd() - rnd.min()) /
                             (rnd.max() - rnd.min()) * 4)) {
    case 0:
        return Puyo::red;
    case 1:
        return Puyo::blue;
    case 2:
        return Puyo::green;
    default:
        return Puyo::yellow;
    }
}
void Window::init() {
    // todo: 最初のツモは完全ランダムではなかった気がする
    current_pair.bottom = randomPuyo();
    current_pair.top = randomPuyo();
    next_pair.bottom = randomPuyo();
    next_pair.top = randomPuyo();
    next2_pair.bottom = randomPuyo();
    next2_pair.top = randomPuyo();
}

} // namespace pumila
