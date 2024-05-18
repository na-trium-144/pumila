import pypumila
from collections import deque
import random
from typing import List, Optional, NamedTuple
import numpy as np


class ReplayData(NamedTuple):
    step: pypumila.StepResult
    feat: np.ndarray
    action: int


class ReplayMemory:
    memory_unchecked: deque
    memory_done: deque
    capacity: int

    def __init__(self, capacity: int):
        self.memory_done = deque([], maxlen=capacity)
        self.memory_unchecked = deque([], maxlen=capacity)
        self.capacity = capacity

    def push(self, data: ReplayData):
        """Save a transition"""
        self.memory_unchecked.append(data)

    def sample(self, batch_size: int) -> Optional[List[ReplayData]]:
        memory_yet: deque = deque([], maxlen=self.capacity)
        while len(self.memory_unchecked):
            data = self.memory_unchecked.pop()
            if data[0].done():
                self.memory_done.append(data)
            else:
                memory_yet.append(data)
        self.memory_unchecked = memory_yet
        if len(self.memory_done) < batch_size:
            return None
        return random.sample(self.memory_done, batch_size)
