import pypumila
from collections import deque
import random
import numpy as np
from typing import List, Tuple


class ReplayMemory:
    memory: deque
    capacity: int

    def __init__(self, capacity: int):
        self.memory = deque([], maxlen=capacity)
        self.capacity = capacity

    def push(self, step: pypumila.StepResult, feat: np.ndarray):
        """Save a transition"""
        self.memory.append((step, feat))

    def sample(self, batch_size: int) -> List[Tuple[pypumila.StepResult, np.ndarray]]:
        sample = []
        i = 0
        while len(sample) < batch_size and i < self.capacity:
            i += 1
            step = random.choice(self.memory)
            if step[0].done():
                sample.append(step)
        return sample

    def __len__(self):
        return len(self.memory)
