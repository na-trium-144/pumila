from pypumila import *
import torch.nn as nn
import torch.nn.functional as F
import numpy as np


class Net14(nn.Module):
    def __init__(self) -> None:
        super(Net14, self).__init__()
        self.layer1 = nn.Linear(Pumila14.feature_num(), 300)
        self.layer2 = nn.Linear(300, 1)

    def forward(self, x):
        x = self.layer1(x)
        x = F.sigmoid(x)
        return self.layer2(x)

    @staticmethod
    def calc_action(state: StepResult) -> np.ndarray:
        return np.array(Pumila14.calc_action(state), copy=False)

    @staticmethod
    def rotate_color(feat: np.ndarray) -> np.ndarray:
        return np.array(Pumila14.rotate_color(feat), copy=False)

    @staticmethod
    def reward(state: StepResult) -> float:
        return Pumila14.reward(state)
