#include <pumila/window.h>
#include "drawing.h"
#include <iostream>
#include <sstream>

#ifdef PUMILA_SDL2
#include <SDL2/SDL.h>
#endif

namespace pumila {
Window::Window(const std::shared_ptr<GameSim> &sim) : sim(sim), key_state(sim) {
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
    TTF_Init();
    TTF_Font *ttf_font = TTF_OpenFont("Roboto-Regular.ttf", 24);
    ttf_font_p = static_cast<void *>(ttf_font);
    if (!ttf_font) {
        std::cerr << "Font not found" << std::endl;
    }
#else
    std::cerr << "Pumila not built with SDL2" << std::endl;
#endif
}

Window::~Window() { quit(); }
void Window::quit() {
#ifdef PUMILA_SDL2
    if (isRunning()) {
        SDL_Quit();
        sdl_window_p = nullptr;
        sdl_renderer_p = nullptr;
    }
#endif
}
bool Window::isRunning() const { return sdl_window_p && sdl_renderer_p; }

void Window::step(bool sim_step, bool player) {
#ifdef PUMILA_SDL2
    if (isRunning()) {
        handleEvent();
    }
    if (player) {
        key_state.keyFrame();
    }
    if (sim_step) {
        sim->step();
    }
    if (isRunning()) {
        draw();
        SDL_Delay(16);
    }
#endif
}

void Window::draw() {
#ifdef PUMILA_SDL2
    using namespace pumila::drawing;
    SDL_Renderer *sdl_renderer = static_cast<SDL_Renderer *>(sdl_renderer_p);
    TTF_Font *ttf_font = static_cast<TTF_Font *>(ttf_font_p);

    SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

    // 背景
    SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
    SDL_RenderClear(sdl_renderer);

    // 枠
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
    SDL_Rect field_rect = {FIELD_X, FIELD_Y - PUYO_SIZE * 12, PUYO_SIZE * 6,
                           PUYO_SIZE * 12};
    SDL_RenderDrawRect(sdl_renderer, &field_rect);

    if (sim->phase->get() == GameSim::Phase::free) {
        auto f_phase = dynamic_cast<GameSim::FreePhase *>(sim->phase.get());
        drawPuyo(f_phase->current_pair.bottom, f_phase->current_pair.bottomX(),
                 sim->field.putTargetY(f_phase->current_pair, true).first,
                 false);
        drawPuyo(f_phase->current_pair.top, f_phase->current_pair.topX(),
                 sim->field.putTargetY(f_phase->current_pair, true).second,
                 false);
        drawPuyo(f_phase->current_pair.bottom, f_phase->current_pair.bottomX(),
                 f_phase->current_pair.bottomY(), true);
        drawPuyo(f_phase->current_pair.top, f_phase->current_pair.topX(),
                 f_phase->current_pair.topY(), true);
    }
    for (int y = 0; y < FieldState::HEIGHT; y++) {
        for (int x = 0; x < FieldState::WIDTH; x++) {
            drawPuyo(sim->field.get(x, y), x, y, true);
        }
    }

    if (ttf_font) {
        int text_w, text_h;
        SDL_Texture *score_t =
            drawText(sdl_renderer, std::to_string(sim->score), ttf_font,
                     {0, 0, 0, 255}, &text_w, &text_h);
        SDL_Rect rect = {FIELD_X + PUYO_SIZE * 6 - text_w - 10, FIELD_Y + 10,
                         text_w, text_h};
        SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
        SDL_DestroyTexture(score_t);

        std::ostringstream ss;
        if (sim->current_chain) {
            ss << sim->current_chain->scoreA() << " x "
               << sim->current_chain->scoreB();
            score_t = drawText(sdl_renderer, ss.str(), ttf_font, {0, 0, 0, 255},
                               &text_w, &text_h);
            rect = {FIELD_X + PUYO_SIZE * 6 - text_w - 10, FIELD_Y + 40, text_w,
                    text_h};
            SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
            SDL_DestroyTexture(score_t);
            ss.str("");
        } else {
            ss << "(" << sim->prev_chain_num << ", " << sim->prev_chain_score
               << ")";
            score_t = drawText(sdl_renderer, ss.str(), ttf_font, {0, 0, 0, 255},
                               &text_w, &text_h);
            rect = {FIELD_X + 10, FIELD_Y + 40, text_w, text_h};
            SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
            SDL_DestroyTexture(score_t);
            ss.str("");
        }
    }

    SDL_RenderPresent(sdl_renderer);
#endif
}
void Window::drawPuyo(Puyo p, double x, double y, bool not_ghost) {
#ifdef PUMILA_SDL2
    if (isRunning() && p != Puyo::none) {
        using namespace pumila::drawing;
        SDL_Renderer *sdl_renderer =
            static_cast<SDL_Renderer *>(sdl_renderer_p);
        Points points;
        SDL_SetRenderDrawColor(sdl_renderer, col(p).r, col(p).g, col(p).b,
                               not_ghost ? 255 : 100);
        points = drawCircle(puyoX(x), puyoY(y),
                            PUYO_SIZE * (not_ghost ? 0.9 : 0.5) / 2);
        SDL_RenderDrawPoints(sdl_renderer, points.data(), points.size());
    }
#endif
}

void Window::handleEvent() {
#ifdef PUMILA_SDL2
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            SDL_Quit();
            sdl_window_p = nullptr;
            sdl_renderer_p = nullptr;
            return;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            if (!event.key.repeat) {
                bool is_down = event.type == SDL_KEYDOWN;
                key_state.key_repeat_wait = 0;
                switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_UP:
                case SDL_SCANCODE_W:
                    key_state.quick_drop = is_down;
                    break;
                case SDL_SCANCODE_DOWN:
                case SDL_SCANCODE_S:
                    key_state.soft_drop = is_down;
                    break;
                case SDL_SCANCODE_LEFT:
                case SDL_SCANCODE_A:
                    key_state.left = is_down;
                    break;
                case SDL_SCANCODE_RIGHT:
                case SDL_SCANCODE_D:
                    key_state.right = is_down;
                    break;
                case SDL_SCANCODE_KP_DIVIDE:
                case SDL_SCANCODE_N:
                    key_state.rot_left = is_down;
                    break;
                case SDL_SCANCODE_KP_MULTIPLY:
                case SDL_SCANCODE_M:
                    key_state.rot_right = is_down;
                    break;
                default:
                    break;
                }
            }
            break;
        }
        default:
            break;
        }
    }
#endif
}
void Window::KeyState::keyFrame() {
    if (quick_drop) {
        sim->quickDrop();
        quick_drop = false;
    }
    if (soft_drop) {
        sim->softDrop();
    }
    if (left) {
        if (!key_repeat_wait) {
            sim->movePair(-1);
        }
    }
    if (right) {
        if (!key_repeat_wait) {
            sim->movePair(1);
        }
    }
    if (rot_left) {
        sim->rotPair(-1);
        rot_left = false;
    }
    if (rot_right) {
        sim->rotPair(1);
        rot_right = false;
    }
    if (key_repeat_wait == 0) {
        key_repeat_wait = KEY_REPEAT;
    } else {
        key_repeat_wait--;
    }
}

} // namespace pumila
