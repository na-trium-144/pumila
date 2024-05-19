import pypumila
from collections import deque
import random
from typing import List, Optional, NamedTuple
import numpy as np
import threading


class ReplayData(NamedTuple):
    step: pypumila.StepResult
    feat: np.ndarray
    action: int


class ReplayMemory:
    memory_unchecked: deque
    memory_done: deque
    capacity: int
    m: threading.Lock

    def __init__(self, capacity: int):
        self.memory_done = deque(maxlen=capacity)
        self.memory_unchecked = deque(maxlen=capacity)
        self.capacity = capacity
        self.m = threading.Lock()

    def push(self, data: ReplayData):
        """Save a transition"""
        with self.m:
            self.memory_unchecked.append(data)

    def sample(self, batch_size: int) -> Optional[List[ReplayData]]:
        with self.m:
            while len(self.memory_unchecked) and self.memory_unchecked[0].step.done():
                self.memory_done.append(self.memory_unchecked.popleft())
        if len(self.memory_done) < batch_size:
            return None
        return random.sample(self.memory_done, batch_size)
