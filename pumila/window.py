import pygame
from pygame.locals import *
from enum import Enum
import pypumila
from typing import List, Tuple
import math

WIDTH = 700
HEIGHT = 560
PUYO_SIZE = 30
FIELD_X = [50, 400]
FIELD_Y = HEIGHT - 100


def puyo_x(x: float, i: int) -> int:
    return round(FIELD_X[i] + PUYO_SIZE * (2 * x + 1) / 2)


def puyo_y(y: float) -> int:
    return round(FIELD_Y - PUYO_SIZE * (2 * y + 1) / 2)


class KeyState:
    soft_drop: bool
    quick_drop: bool
    left: bool
    right: bool
    rot_left: bool
    rot_right: bool
    reset: bool
    key_repeat_wait: int

    def __init__(self) -> None:
        self.soft_drop = False
        self.quick_drop = False
        self.left = False
        self.right = False
        self.rot_left = False
        self.rot_right = False
        self.reset = False
        self.key_repeat_wait = 0


class WindowPhase(Enum):
    READY = 0
    GAME = 1
    FINISH = 2


class Window:
    window_init: bool = False
    screen: pygame.Surface
    font: pygame.font.Font
    key_state: KeyState
    phase_t: int
    phase: WindowPhase
    sim: List[pypumila.GameSim]
    is_player: List[bool]

    def __init__(self) -> None:
        if Window.window_init:
            raise RuntimeError("pygame already initialized")
        pygame.init()
        pygame.key.set_repeat()
        self.screen = pygame.display.set_mode((700, 560))
        pygame.font.init()
        self.font = pygame.font.Font("Roboto-Regular.ttf", 24)
        self.font_sm = pygame.font.Font("Roboto-Regular.ttf", 16)
        Window.window_init = True
        self.key_state = KeyState()
        self.phase_t = 0
        self.phase = WindowPhase.READY
        self.sim = []
        self.is_player = []

    def set_sim(self, sim: List[Tuple[pypumila.GameSim, bool]]) -> None:
        assert len(sim) <= 2
        self.sim = [s[0] for s in sim]
        self.is_player = [s[1] for s in sim]

    def step(self) -> None:
        self.handle_event()
        self.key_frame()
        self.draw()

    def handle_event(self) -> None:
        for event in pygame.event.get():
            if event.type in [KEYDOWN, KEYUP]:
                self.key_state.key_repeat_wait = 0
                is_down = event.type == KEYDOWN
                if event.key in [K_w, K_UP]:
                    self.key_state.quick_drop = is_down
                elif event.key in [K_s, K_DOWN]:
                    self.key_state.soft_drop = is_down
                elif event.key in [K_a, K_LEFT]:
                    self.key_state.left = is_down
                elif event.key in [K_d, K_RIGHT]:
                    self.key_state.right = is_down
                elif event.key in [K_n, K_KP_DIVIDE]:
                    self.key_state.rot_left = is_down
                elif event.key in [K_m, K_KP_MULTIPLY]:
                    self.key_state.rot_right = is_down
                elif event.key in [K_p]:
                    self.key_state.reset = is_down

    def key_frame(self) -> None:
        if self.key_state.reset:
            for s in self.sim:
                s.reset()
            self.phase = WindowPhase.READY
            self.phase_t = 0
        self.key_state.reset = False

        if self.key_state.quick_drop and self.phase == WindowPhase.GAME:
            for s, p in zip(self.sim, self.is_player):
                if p:
                    s.quick_drop()
        self.key_state.quick_drop = False

        if self.key_state.soft_drop and self.phase == WindowPhase.GAME:
            for s, p in zip(self.sim, self.is_player):
                if p:
                    s.soft_drop()
        self.key_state.soft_drop = False

        if (
            self.key_state.left
            and self.phase == WindowPhase.GAME
            and not self.key_state.key_repeat_wait
        ):
            for s, p in zip(self.sim, self.is_player):
                if p:
                    s.move_pair(-1)

        if (
            self.key_state.right
            and self.phase == WindowPhase.GAME
            and not self.key_state.key_repeat_wait
        ):
            for s, p in zip(self.sim, self.is_player):
                if p:
                    s.move_pair(1)

        if self.key_state.rot_left and self.phase == WindowPhase.GAME:
            for s, p in zip(self.sim, self.is_player):
                if p:
                    s.rot_pair(-1)
        self.key_state.rot_left = False

        if self.key_state.rot_right and self.phase == WindowPhase.GAME:
            for s, p in zip(self.sim, self.is_player):
                if p:
                    s.rot_pair(1)
        self.key_state.rot_right = False

        if self.key_state.key_repeat_wait == 0:
            self.key_state.key_repeat_wait = 10
        else:
            self.key_state.key_repeat_wait -= 1

    def draw(self) -> None:
        self.screen.fill(pygame.Color(255, 255, 255))

        black = pygame.Color(0, 0, 0)
        for i, s in enumerate(self.sim):
            pygame.draw.rect(
                self.screen,
                black,
                pygame.Rect(
                    FIELD_X[i], FIELD_Y - PUYO_SIZE * 12, PUYO_SIZE * 6, PUYO_SIZE * 12
                ),
                width=1,
            )

            field = s.fall_phase_display_field()
            if field is None:
                field = s.field_copy()
            next_p = 0
            if s.phase_get() == pypumila.GameSim.PhaseEnum.free:
                next_p = 1
                pair = field.get_next(0)
                self.draw_puyo(
                    pair.bottom,
                    pair.bottom_x(),
                    field.get_next_height(pair)[0],
                    i,
                    False,
                )
                self.draw_puyo(
                    pair.top, pair.top_x(), field.get_next_height(pair)[1], i, False
                )
                self.draw_puyo(pair.bottom, pair.bottom_x(), pair.bottom_y(), i, True)
                self.draw_puyo(pair.top, pair.top_x(), pair.top_y(), i, True)
            next1 = field.get_next(next_p)
            self.draw_puyo(next1.bottom, 6.5, 10.5, i, True)
            self.draw_puyo(next1.top, 6.5, 11.5, i, True)
            next2 = field.get_next(next_p + 1)
            self.draw_puyo(next2.bottom, 8, 9.5, i, True)
            self.draw_puyo(next2.top, 8, 10.5, i, True)

            for y in range(13):
                for x in range(6):
                    self.draw_puyo(field.get(x, y), x, y, i, True)

            score_text = self.font.render(str(field.total_score()), True, black)
            self.screen.blit(
                score_text,
                (
                    FIELD_X[i] + PUYO_SIZE * 6 - score_text.get_width() - 10,
                    FIELD_Y + 35,
                ),
            )

            def garbage_gauge(garbage: int) -> int:
                if garbage > 0:
                    return round(PUYO_SIZE * 3 * math.log(1 + garbage) / math.log(30))
                return 0

            ready = field.get_garbage_num_total()
            red = ready >= 30
            if ready > 0:
                self.draw_puyo(pypumila.Puyo.garbage, 0.5, 13.5, i, True)
                gb_color = black
                if red:
                    gb_color = pygame.Color(255, 0, 0)
                gb_text = self.font.render(f"x {ready}", True, gb_color)
                self.screen.blit(
                    gb_text,
                    (
                        FIELD_X[i] + round(PUYO_SIZE * 1.5) + 5,
                        FIELD_Y - PUYO_SIZE * 14 - 15,
                    ),
                )
                pygame.draw.rect(
                    self.screen,
                    gb_color,
                    pygame.Rect(
                        FIELD_X[i],
                        FIELD_Y - PUYO_SIZE * 13 - 10,
                        garbage_gauge(ready),
                        3,
                    ),
                )

            if (
                s.phase_get() == pypumila.GameSim.PhaseEnum.fall
                and s.fall_phase_current_chain() < len(s.current_step().chains)
            ):
                current_chain = s.current_step().chains[s.fall_phase_current_chain()]
                current_sc_text = self.font.render(
                    f"{current_chain.score_a()} x {current_chain.score_b()}",
                    True,
                    black,
                )
                self.screen.blit(
                    current_sc_text,
                    (
                        FIELD_X[i] + PUYO_SIZE * 6 - current_sc_text.get_width() - 10,
                        FIELD_Y + 60,
                    ),
                )

                current_chain_text = self.font.render(
                    str(current_chain.chain_num),
                    True,
                    black,
                )
                self.screen.blit(
                    current_chain_text,
                    (
                        round(
                            FIELD_X[i]
                            + PUYO_SIZE * 7.5
                            - current_chain_text.get_width() / 2
                        ),
                        FIELD_Y - PUYO_SIZE * 3 - 30,
                    ),
                )

                chain_text = self.font_sm.render(
                    "chains!" if current_chain.chain_num >= 2 else "chain!",
                    True,
                    black,
                )
                self.screen.blit(
                    chain_text,
                    (
                        round(
                            FIELD_X[i] + PUYO_SIZE * 7.5 - chain_text.get_width() / 2
                        ),
                        FIELD_Y - PUYO_SIZE * 3,
                    ),
                )

        if self.phase == WindowPhase.READY:
            text = self.font.render("Ready?", True, black)
            if self.phase_t >= 60:
                self.phase_t = 0
                self.phase = WindowPhase.GAME
        elif self.phase == WindowPhase.GAME:
            if self.phase_t < 60:
                text = self.font.render("Go!", True, black)
            else:
                text = None
            for s in self.sim:
                if s.is_over:
                    self.phase_t = 0
                    self.phase = WindowPhase.FINISH
        elif self.phase == WindowPhase.FINISH:
            text = self.font.render("Finish", True, black)
        if text is not None:
            self.screen.blit(
                text,
                (
                    round(WIDTH / 2 - text.get_width() / 2),
                    round(HEIGHT / 2 - text.get_height() / 2),
                ),
            )

        pygame.display.flip()

        self.phase_t += 1

    def draw_puyo(self, p: pypumila.Puyo, x: float, y: float, i: int, not_ghost: bool):
        if p == pypumila.Puyo.none:
            return
        elif p == pypumila.Puyo.red:
            if not_ghost:
                color = pygame.Color(255, 120, 120)
            else:
                color = pygame.Color(255, 200, 200)
        elif p == pypumila.Puyo.blue:
            if not_ghost:
                color = pygame.Color(120, 120, 255)
            else:
                color = pygame.Color(200, 200, 255)
        elif p == pypumila.Puyo.green:
            if not_ghost:
                color = pygame.Color(120, 255, 120)
            else:
                color = pygame.Color(200, 255, 200)
        elif p == pypumila.Puyo.yellow:
            if not_ghost:
                color = pygame.Color(220, 220, 0)
            else:
                color = pygame.Color(255, 255, 120)
        elif p == pypumila.Puyo.purple:
            if not_ghost:
                color = pygame.Color(255, 0, 255)
            else:
                color = pygame.Color(255, 120, 255)
        elif p == pypumila.Puyo.garbage:
            color = pygame.Color(190, 190, 190)

        if not_ghost:
            radius = PUYO_SIZE * 0.9 / 2
        else:
            radius = PUYO_SIZE * 0.5 / 2

        pygame.draw.circle(self.screen, color, (puyo_x(x, i), puyo_y(y)), round(radius))
