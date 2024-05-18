import pypumila
from collections import deque
import random
from typing import List, Tuple, Optional
import numpy as np


class ReplayMemory:
    memory_unchecked: deque
    memory_done: deque
    capacity: int

    def __init__(self, capacity: int):
        self.memory_done = deque([], maxlen=capacity)
        self.memory_unchecked = deque([], maxlen=capacity)
        self.capacity = capacity

    def push(self, step: pypumila.StepResult, feat: np.ndarray, a: int):
        """Save a transition"""
        self.memory_unchecked.append((step, feat, a))

    def sample(
        self, batch_size: int
    ) -> Optional[List[Tuple[pypumila.StepResult, np.ndarray, int]]]:
        memory_yet = deque([], maxlen=self.capacity)
        while len(self.memory_unchecked):
            step = self.memory_unchecked.pop()
            if step[0].done():
                self.memory_done.append(step)
            else:
                memory_yet.append(step)
        self.memory_unchecked = memory_yet
        if len(self.memory_done) < batch_size:
            return None
        else:
            return random.sample(self.memory_done, batch_size)
