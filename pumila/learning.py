import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
from .replay import ReplayMemory
import math
import random
import pypumila
from typing import Tuple, Optional


class Params:
    # BATCH_SIZE is the number of transitions sampled from the replay buffer
    # GAMMA is the discount factor as mentioned in the previous section
    # EPS_START is the starting value of epsilon
    # EPS_END is the final value of epsilon
    # EPS_DECAY controls the rate of exponential decay of epsilon, higher means a slower decay
    # TAU is the update rate of the target network
    # LR is the learning rate of the ``AdamW`` optimizer

    def __init__(
        self,
        batch_size=128,
        gamma=0.99,
        eps_start=0.9,
        eps_end=0.05,
        eps_decay=1000,
        tau=0.005,
        lr=1e-4,
        memory_size=10000,
    ) -> None:
        self.batch_size = batch_size
        self.gamma = gamma
        self.eps_start = eps_start
        self.eps_end = eps_end
        self.eps_decay = eps_decay
        self.tau = tau
        self.lr = lr
        self.memory_size = memory_size


class Learning:
    device: torch.device
    dtype: torch.dtype
    params: Params
    policy_net: nn.Module
    target_net: nn.Module
    optimizer: optim.AdamW
    memory: ReplayMemory
    steps_done: int

    def __init__(self, Net=None, file=None, **kwargs) -> None:
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
        # デバイスによってfloat64が使えなかったりするので変換先を設定
        self.device = None
        self.dtype = torch.float64
        if torch.cuda.is_available():
            self.device = torch.device("cuda")
        if self.device is None and torch.backends.mps.is_built():
            if torch.backends.mps.is_available():
                self.device = torch.device("mps")
                self.dtype = torch.float32
            else:
                print(
                    "MPS not available because the current PyTorch install was not built with MPS enabled."
                )
        if self.device is None and torch.backends.mkldnn.is_available():
            self.device = torch.device("mkldnn")
        if self.device is None:
            self.device = torch.device("cpu")
        return self.device

    def random_eps(self) -> float:
        return self.params.eps_end + (
            self.params.eps_start - self.params.eps_end
        ) * math.exp(-1.0 * self.steps_done / self.params.eps_decay)

    def select_action(
        self, feat: np.ndarray, random_eps: Optional[float] = None
    ) -> torch.Tensor:
        if random_eps is None:
            self.steps_done += 1
            random_eps = self.random_eps()
        if random.random() > random_eps:
            feat = torch.from_numpy(feat).to(self.dtype).to(self.device)
            with torch.no_grad():
                return self.policy_net(feat).max(0).indices.view(1, 1)
        else:
            return torch.tensor(
                [[random.randint(0, 21)]], device=self.device, dtype=torch.long
            )

    def optimize_model(self) -> None:
        # Perform one step of the optimization (on the policy network)
        batch = self.memory.sample(self.params.batch_size)
        if len(batch) < self.params.batch_size:
            return

        # batch = (step, feat, action)
        step_batch = [s[0] for s in batch]
        feat_batch_np = self.policy_net.rotate_color(
            pypumila.Matrix(np.vstack([s[1][s[2], :] for s in batch]))
        )
        feat_batch = torch.from_numpy(feat_batch_np).to(self.dtype).to(self.device)

        state_action_values = self.policy_net(feat_batch)

        reward_batch = torch.tensor(
            [self.policy_net.reward(step) for step in step_batch] * 24,
            dtype=self.dtype,
            device=self.device,
        )

        next_feat_batch_np = np.array(
            [self.policy_net.calc_action(step.next()) for step in step_batch]
        )
        next_feat_batch = (
            torch.from_numpy(next_feat_batch_np).to(self.dtype).to(self.device)
        )
        next_state_values = self.target_net(next_feat_batch).max(1).indices.squeeze()
        next_state_values = torch.tensor(
            next_state_values.tolist() * 24, dtype=self.dtype, device=self.device
        )

        # Compute the expected Q values
        expected_state_action_values = (
            next_state_values * self.params.gamma
        ) + reward_batch

        # Compute Huber loss
        criterion = nn.SmoothL1Loss()
        loss = criterion(state_action_values, expected_state_action_values.unsqueeze(1))

        # Optimize the model
        self.optimizer.zero_grad()
        loss.backward()
        # In-place gradient clipping
        torch.nn.utils.clip_grad_value_(self.policy_net.parameters(), 100)
        self.optimizer.step()

    def get_step(self, sim: pypumila.GameSim) -> Tuple[pypumila.StepResult, np.ndarray]:
        step = sim.current_step()
        feat = self.policy_net.calc_action(step)
        return (step, feat)

    def update_target(self) -> None:
        # Soft update of the target network's weights
        # θ′ ← τ θ + (1 −τ )θ′
        target_net_state_dict = self.target_net.state_dict()
        policy_net_state_dict = self.policy_net.state_dict()
        for key in policy_net_state_dict:
            target_net_state_dict[key] = policy_net_state_dict[
                key
            ] * self.params.tau + target_net_state_dict[key] * (1 - self.params.tau)
        self.target_net.load_state_dict(target_net_state_dict)
