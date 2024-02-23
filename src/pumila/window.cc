#include <pumila/window.h>
#include <pumila/model_base.h>
#include "drawing.h"
#include <iostream>
#include <sstream>

#ifdef PUMILA_SDL2
#include <SDL.h>
#endif

namespace PUMILA_NS {
Window::Window(const std::vector<std::shared_ptr<GameSim>> &sim)
    : sim(sim), key_state(this) {
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
    TTF_Font *ttf_font_sm = TTF_OpenFont("Roboto-Regular.ttf", 12);
    ttf_font_sm_p = static_cast<void *>(ttf_font_sm);
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

void Window::step(bool sim_step) {
#ifdef PUMILA_SDL2
    if (isRunning()) {
        handleEvent();
    }
    key_state.keyFrame();
    if (sim_step && state == WindowState::game) {
        for (const auto &s : sim) {
            s->step();
        }
    }
    if (isRunning()) {
        draw();
        SDL_Delay(16);
    }
    phase_t++;
    switch (state) {
    case WindowState::ready:
        if (phase_t >= READY_T) {
            state = WindowState::game;
            phase_t = 0;
        }
        break;
    case WindowState::game:
        for (std::size_t i = 0; i < sim.size(); i++) {
            std::shared_lock lock(sim[i]->field_m);
            if (sim[i]->is_over) {
                state = WindowState::finish;
                phase_t = 0;
            }
        }
        break;
    default:
        break;
    }
#endif
}

void Window::draw() {
#ifdef PUMILA_SDL2
    using namespace PUMILA_NS::drawing;
    SDL_Renderer *sdl_renderer = static_cast<SDL_Renderer *>(sdl_renderer_p);
    TTF_Font *ttf_font = static_cast<TTF_Font *>(ttf_font_p);
    TTF_Font *ttf_font_sm = static_cast<TTF_Font *>(ttf_font_sm_p);

    SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

    // 背景
    SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
    SDL_RenderClear(sdl_renderer);

    for (std::size_t i = 0; i < sim.size(); i++) {
        std::shared_lock lock(sim[i]->field_m);
        // 枠
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
        SDL_Rect field_rect = {FIELD_X[i], FIELD_Y - PUYO_SIZE * 12,
                               PUYO_SIZE * 6, PUYO_SIZE * 12};
        SDL_RenderDrawRect(sdl_renderer, &field_rect);

        if (sim[i]->isFreePhase()) {
            auto current_pair = sim[i]->field->next().get();
            drawPuyo(current_pair.bottom, current_pair.bottomX(),
                     sim[i]->field->getNextHeight(current_pair).first, i,
                     false);
            drawPuyo(current_pair.top, current_pair.topX(),
                     sim[i]->field->getNextHeight(current_pair).second, i,
                     false);
            drawPuyo(current_pair.bottom, current_pair.bottomX(),
                     current_pair.bottomY(), i, true);
            drawPuyo(current_pair.top, current_pair.topX(), current_pair.topY(),
                     i, true);
        }
        std::size_t next_p = sim[i]->isFreePhase() ? 1 : 0;
        if (sim[i]->field->next().size() > next_p) {
            drawPuyo(sim[i]->field->next()[next_p].bottom, 6.5, 10.5, i, true);
            drawPuyo(sim[i]->field->next()[next_p].top, 6.5, 11.5, i, true);
        }
        if (sim[i]->field->next().size() > next_p + 1) {
            drawPuyo(sim[i]->field->next()[next_p + 1].bottom, 8, 9.5, i, true);
            drawPuyo(sim[i]->field->next()[next_p + 1].top, 8, 10.5, i, true);
        }
        for (std::size_t y = 0; y < FieldState2::HEIGHT; y++) {
            for (std::size_t x = 0; x < FieldState2::WIDTH; x++) {
                drawPuyo(sim[i]->field->field().get(x, y), x, y, i, true);
            }
        }

        if (ttf_font) {
            std::ostringstream ss;
            int text_w, text_h;
            SDL_Texture *text;
            SDL_Rect rect;

            text = drawText(sdl_renderer,
                            sim[i]->name.empty() ? "player" : sim[i]->name,
                            ttf_font, {0, 0, 0, 255}, &text_w, &text_h);
            rect = {FIELD_X[i] + PUYO_SIZE * 3 - text_w / 2, FIELD_Y + 5,
                    text_w, text_h};
            SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
            SDL_DestroyTexture(text);

            ss << sim[i]->field->totalScore();
            text = drawText(sdl_renderer, ss.str(), ttf_font, {0, 0, 0, 255},
                            &text_w, &text_h);
            rect = {FIELD_X[i] + PUYO_SIZE * 6 - text_w - 10, FIELD_Y + 35,
                    text_w, text_h};
            SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
            SDL_DestroyTexture(text);
            ss.str("");

            if (sim[i]->field->garbage().getReady() > 0) {
                drawPuyo(Puyo::garbage, 0.5, 13.5, i, true);
                ss << "x " << sim[i]->field->garbage().getReady();
                text = drawText(sdl_renderer, ss.str(), ttf_font,
                                sim[i]->field->garbage().getReady() >= 30
                                    ? SDL_Color{255, 0, 0, 255}
                                    : SDL_Color{0, 0, 0, 255},
                                &text_w, &text_h);
                rect = {FIELD_X[i] + static_cast<int>(PUYO_SIZE * 1.5) + 5,
                        FIELD_Y - PUYO_SIZE * 14 - 15, text_w, text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);
                ss.str("");
            }

            if (sim[i]->field->garbage().getCurrent() > 0) {
                ss << "- " << sim[i]->field->garbage().getCurrent();
                text = drawText(sdl_renderer, ss.str(), ttf_font,
                                SDL_Color{0, 190, 0, 255}, &text_w, &text_h);
                rect = {FIELD_X[i] + PUYO_SIZE * 6 - text_w - 10,
                        FIELD_Y - PUYO_SIZE * 14 - 15, text_w, text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);
                ss.str("");
            }

            if (sim[i]->current_chain) {
                ss << sim[i]->current_chain->scoreA() << " x "
                   << sim[i]->current_chain->scoreB();
                text = drawText(sdl_renderer, ss.str(), ttf_font,
                                {0, 0, 0, 255}, &text_w, &text_h);
                rect = {FIELD_X[i] + PUYO_SIZE * 6 - text_w - 10, FIELD_Y + 60,
                        text_w, text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);
                ss.str("");

                ss << sim[i]->field->currentStep().chain_num;
                text = drawText(sdl_renderer, ss.str(), ttf_font,
                                {0, 0, 0, 255}, &text_w, &text_h);
                rect = {FIELD_X[i] + PUYO_SIZE * 7 - text_w / 2, FIELD_Y - 60,
                        text_w, text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);
                ss.str("");

                ss << "chain";
                if (sim[i]->field->currentStep().chain_num >= 2) {
                    ss << "s";
                }
                ss << "!";
                text = drawText(sdl_renderer, ss.str(), ttf_font_sm,
                                {0, 0, 0, 255}, &text_w, &text_h);
                rect = {FIELD_X[i] + PUYO_SIZE * 7 - text_w / 2, FIELD_Y - 30,
                        text_w, text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);

            } else {
                // ss << "(" << sim->field->prev_chain_num << ", "
                //    << sim->field->prev_chain_score << ")";
                // score_t = drawText(sdl_renderer, ss.str(), ttf_font, {0, 0,
                // 0, 255},
                //                    &text_w, &text_h);
                // rect = {FIELD_X + 10, FIELD_Y + 40, text_w, text_h};
                // SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
                // SDL_DestroyTexture(score_t);
                // ss.str("");
            }

            switch (state) {
            case WindowState::ready:
                text = drawText(sdl_renderer, "Ready?", ttf_font,
                                {0, 0, 0, 255}, &text_w, &text_h);
                rect = {WIDTH / 2 - text_w / 2, HEIGHT / 2 - text_h / 2, text_w,
                        text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);
                break;
            case WindowState::game:
                if (phase_t < GO_T) {
                    text = drawText(sdl_renderer, "Go!", ttf_font,
                                    {0, 0, 0, 255}, &text_w, &text_h);
                    rect = {WIDTH / 2 - text_w / 2, HEIGHT / 2 - text_h / 2,
                            text_w, text_h};
                    SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                    SDL_DestroyTexture(text);
                }
                break;
            case WindowState::finish:
                text = drawText(sdl_renderer, "Finish", ttf_font,
                                {0, 0, 0, 255}, &text_w, &text_h);
                rect = {WIDTH / 2 - text_w / 2, HEIGHT / 2 - text_h / 2, text_w,
                        text_h};
                SDL_RenderCopy(sdl_renderer, text, NULL, &rect);
                SDL_DestroyTexture(text);
            default:
                break;
            }
        }
    }

    SDL_RenderPresent(sdl_renderer);
#endif
}
void Window::drawPuyo(Puyo p, double x, double y, int i, bool not_ghost) {
#ifdef PUMILA_SDL2
    if (isRunning() && p != Puyo::none) {
        using namespace PUMILA_NS::drawing;
        SDL_Renderer *sdl_renderer =
            static_cast<SDL_Renderer *>(sdl_renderer_p);
        Points points;
        SDL_SetRenderDrawColor(sdl_renderer, col(p).r, col(p).g, col(p).b,
                               not_ghost ? 255 : 100);
        points = drawCircle(puyoX(x, i), puyoY(y),
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
                case SDL_SCANCODE_P:
                    key_state.reset = is_down;
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
    if (reset) {
        for (const auto &s : win->sim) {
            s->reset();
        }
        win->state = WindowState::ready;
        win->phase_t = 0;
    }
    reset = false;
    if (quick_drop && win->state == WindowState::game) {
        for (const auto &s : win->sim) {
            if (!s->hasModel()) {
                s->quickDrop();
            }
        }
    }
    quick_drop = false;
    if (soft_drop && win->state == WindowState::game) {
        for (const auto &s : win->sim) {
            if (!s->hasModel()) {
                s->softDrop();
            }
        }
    }
    if (left && win->state == WindowState::game) {
        if (!key_repeat_wait) {
            for (const auto &s : win->sim) {
                if (!s->hasModel()) {
                    s->movePair(-1);
                }
            }
        }
    }
    if (right && win->state == WindowState::game) {
        if (!key_repeat_wait) {
            for (const auto &s : win->sim) {
                if (!s->hasModel()) {
                    s->movePair(1);
                }
            }
        }
    }
    if (rot_left && win->state == WindowState::game) {
        for (const auto &s : win->sim) {
            if (!s->hasModel()) {
                s->rotPair(-1);
            }
        }
        rot_left = false;
    }
    if (rot_right && win->state == WindowState::game) {
        for (const auto &s : win->sim) {
            if (!s->hasModel()) {
                s->rotPair(1);
            }
        }
        rot_right = false;
    }
    if (key_repeat_wait == 0) {
        key_repeat_wait = KEY_REPEAT;
    } else {
        key_repeat_wait--;
    }
}

} // namespace PUMILA_NS
