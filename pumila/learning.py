import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
from .replay import ReplayMemory, ReplayData
import math
import random
import pypumila
from typing import Tuple, Optional, NamedTuple, List


class Params(NamedTuple):
    # BATCH_SIZE is the number of transitions sampled from the replay buffer
    # GAMMA is the discount factor as mentioned in the previous section
    # EPS_START is the starting value of epsilon
    # EPS_END is the final value of epsilon
    # EPS_DECAY controls the rate of exponential decay of epsilon, higher means a slower decay
    # TAU is the update rate of the target network
    # LR is the learning rate of the ``AdamW`` optimizer
    batch_size: int = 128
    gamma: float = 0.99
    eps_start: float = 0.9
    eps_end: float = 0.05
    eps_decay: float = 1000
    tau: float = 0.005
    lr: float = 1e-4
    memory_size: int = 10000


class Learning:
    device: torch.device
    dtype: torch.dtype
    params: Params
    policy_net: nn.Module
    target_net: nn.Module
    optimizer: optim.AdamW
    memory: ReplayMemory
    steps_done: int

    def __init__(
        self, Net: Optional[type] = None, file: Optional[str] = None, **kwargs
    ) -> None:
        self.init_device()
        self.params = Params(**kwargs)
        if Net is not None:
            self.policy_net = Net().to(self.device)
            self.target_net = Net().to(self.device)
            self.target_net.load_state_dict(self.policy_net.state_dict())
        if file is not None:
            self.policy_net = torch.load(file).to(self.device)
            self.target_net = torch.load(file).to(self.device)
        self.optimizer = optim.AdamW(
            self.policy_net.parameters(), lr=self.params.lr, amsgrad=True
        )
        self.memory = ReplayMemory(self.params.memory_size)
        self.steps_done = 0

    def init_device(self) -> torch.device:
        self.device = None
        self.dtype = torch.float32
        if torch.cuda.is_available():
            self.device = torch.device("cuda")
        if self.device is None and torch.backends.mps.is_built():
            if torch.backends.mps.is_available():
                self.device = torch.device("mps")
            else:
                print(
                    "MPS not available because the current PyTorch install was not built with MPS enabled."
                )
        # if self.device is None and torch.backends.mkldnn.is_available():
        #     self.device = torch.device("mkldnn")
        if self.device is None:
            self.device = torch.device("cpu")
        return self.device

    def get_batch(self) -> Optional[List[ReplayData]]:
        return self.memory.sample(self.params.batch_size)

    def calc_q_batch(self, batch: List[ReplayData]) -> torch.Tensor:
        feat_batch_np = self.policy_net.rotate_color(
            pypumila.Matrix(np.vstack([s.feat[s.action, :] for s in batch]))
        )
        feat_batch = torch.from_numpy(feat_batch_np).to(self.dtype).to(self.device)
        return self.policy_net(feat_batch)

    def calc_expected_q_batch(self, batch: List[ReplayData]) -> torch.Tensor:
        reward_batch = torch.tensor(
            [self.policy_net.reward(data.step) for data in batch] * 24,
            dtype=self.dtype,
            device=self.device,
        )
        next_feat_batch_np = np.array(
            [self.policy_net.calc_action(data.step.next()) for data in batch]
        )
        next_feat_batch = (
            torch.from_numpy(next_feat_batch_np).to(self.dtype).to(self.device)
        )
        next_state_values = self.target_net(next_feat_batch).max(1).indices.squeeze()
        next_state_values = torch.tensor(
            next_state_values.tolist() * 24, dtype=self.dtype, device=self.device
        )
        # Compute the expected Q values
        return (next_state_values * self.params.gamma + reward_batch).unsqueeze(1)

    def optimize_batch(self, q: torch.Tensor, expected_q: torch.Tensor) -> None:
        # Perform one step of the optimization (on the policy network)
        # Compute Huber loss
        criterion = nn.SmoothL1Loss()
        loss = criterion(q, expected_q)
        # Optimize the model
        self.optimizer.zero_grad()
        loss.backward()
        # In-place gradient clipping
        torch.nn.utils.clip_grad_value_(self.policy_net.parameters(), 100)
        self.optimizer.step()

    def update_target_net(self) -> None:
        # Soft update of the target network's weights
        # θ′ ← τ θ + (1 −τ )θ′
        target_net_state_dict = self.target_net.state_dict()
        policy_net_state_dict = self.policy_net.state_dict()
        for key in policy_net_state_dict:
            target_net_state_dict[key] = policy_net_state_dict[
                key
            ] * self.params.tau + target_net_state_dict[key] * (1 - self.params.tau)
        self.target_net.load_state_dict(target_net_state_dict)

    def get_step(self, sim: pypumila.GameSim) -> ReplayData:
        step = sim.current_step()
        feat = self.policy_net.calc_action(step)
        return ReplayData(step=step, feat=feat, action=0)

    def random_eps(self) -> float:
        return self.params.eps_end + (
            self.params.eps_start - self.params.eps_end
        ) * math.exp(-1.0 * self.steps_done / self.params.eps_decay)

    def select_action(
        self, data: ReplayData, random_eps: Optional[float] = None
    ) -> torch.Tensor:
        if random_eps is None:
            self.steps_done += 1
            random_eps = self.random_eps()
        if random.random() > random_eps:
            feat_t = torch.from_numpy(data.feat).to(self.dtype).to(self.device)
            with torch.no_grad():
                return self.policy_net(feat_t).max(0).indices.view(1, 1)
        else:
            return torch.tensor(
                [[random.randint(0, 21)]], device=self.device, dtype=torch.long
            )

    def push_step(self, data: ReplayData, action: int) -> None:
        self.memory.push(data._replace(action=action))
