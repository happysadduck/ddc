import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np


class ResBlock(nn.Module):
    def __init__(self, channels):
        super().__init__()
        self.conv1 = nn.Conv2d(channels, channels, 3, padding=1)
        self.bn1 = nn.BatchNorm2d(channels)
        self.conv2 = nn.Conv2d(channels, channels, 3, padding=1)
        self.bn2 = nn.BatchNorm2d(channels)

    def forward(self, x):
        residual = x
        x = F.relu(self.bn1(self.conv1(x)))
        x = self.bn2(self.conv2(x))
        x += residual
        return F.relu(x)


class Net(nn.Module):
    def __init__(
        self, board_channels=2, board_size=(19, 15), num_res_blocks=3, channels=64
    ):
        super().__init__()
        self.board_size = board_size
        self.action_size = board_size[0] * board_size[1]

        self.conv_input = nn.Conv2d(board_channels, channels, 3, padding=1)
        self.bn_input = nn.BatchNorm2d(channels)

        self.res_blocks = nn.ModuleList(
            [ResBlock(channels) for _ in range(num_res_blocks)]
        )

        self.policy_conv = nn.Conv2d(channels, 2, 1)
        self.policy_bn = nn.BatchNorm2d(2)
        self.policy_fc = nn.Linear(2 * board_size[0] * board_size[1], self.action_size)

        self.value_conv = nn.Conv2d(channels, 1, 1)
        self.value_bn = nn.BatchNorm2d(1)
        self.value_fc1 = nn.Linear(1 * board_size[0] * board_size[1], 64)
        self.value_fc2 = nn.Linear(64, 1)

    def forward(self, x):
        x = F.relu(self.bn_input(self.conv_input(x)))
        for res in self.res_blocks:
            x = res(x)

        pol = F.relu(self.policy_bn(self.policy_conv(x)))
        pol = pol.view(pol.size(0), -1)
        policy_logits = self.policy_fc(pol)

        val = F.relu(self.value_bn(self.value_conv(x)))
        val = val.view(val.size(0), -1)
        val = F.relu(self.value_fc1(val))
        value = torch.tanh(self.value_fc2(val))

        return policy_logits, value

    def predict(self, state_tensor):
        self.eval()
        with torch.no_grad():
            state_tensor = state_tensor.unsqueeze(0)
            policy_logits, value = self.forward(state_tensor)

            policy = F.softmax(policy_logits, dim=1).squeeze(0).numpy()
            value = value.item()

        return policy, value


class Policy:
    def __init__(self, network: Net, device="cpu"):
        self.net = network.to(device)
        self.device = device
        self.board_h = 19
        self.board_w = 15
        self.cache = {}

    def _state_to_tensor(self, state) -> torch.Tensor:
        board_list = state.get_board()
        current_player = state.get_current_player()

        board_arr = np.array(board_list, dtype=object)
        channels = np.zeros((2, self.board_h * self.board_w), dtype=np.float32)

        channels[0] = (board_arr == current_player).astype(np.float32)
        channels[1] = (board_arr == (1 - current_player)).astype(np.float32)

        tensor = torch.tensor(
            channels.reshape(2, self.board_h, self.board_w), dtype=torch.float32
        )
        return tensor.to(self.device)

    def _action_to_index(self, action):
        r, c = action
        return r * self.board_w + c

    def _get_state_key(self, state):
        return (tuple(state.get_board()), state.get_current_player())

    def _get_network_output(self, state):
        key = self._get_state_key(state)
        if key not in self.cache:
            state_tensor = self._state_to_tensor(state)
            policy, value = self.net.predict(state_tensor)
            self.cache[key] = (policy, value)
        return self.cache[key]

    def get_action_probs(self, state) -> tuple[list, list]:
        legal_actions = state.get_legal_actions()
        if not legal_actions:
            return [], []

        policy, _ = self._get_network_output(state)

        legal_indices = [self._action_to_index(a) for a in legal_actions]
        legal_probs = policy[legal_indices]

        if legal_probs.sum() < 1e-8:
            legal_probs = np.ones(len(legal_actions)) / len(legal_actions)
        else:
            legal_probs = legal_probs / legal_probs.sum()

        return legal_actions, legal_probs.tolist()

    def get_value(self, state) -> float:
        _, value = self._get_network_output(state)
        return value

    def clear_cache(self):
        self.cache = {}
