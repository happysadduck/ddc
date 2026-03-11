import os
import pickle
from pathlib import Path

import rules
import mcts
import policy
import numpy as np


class SelfPlay:
    def __init__(self, net: policy.Net, num_simulations=800, temperature=1.0):
        self.net = net
        self.policy = policy.Policy(net)
        self.config = mcts.MCTSConfig(
            num_simulations=num_simulations,
            temperature=temperature,
            use_value_network=True,
        )

    def generate_game(self):
        state = rules.GameState()
        tree = mcts.MCTS(self.config, self.policy)
        game_history = []

        while not state.is_terminal():
            print(state.__repr__())
            current_player = state.get_current_player()
            state_tensor = self.policy._state_to_tensor(state)

            action, probs = tree.search(state)

            policy_target = np.zeros(19 * 15, dtype=np.float32)
            for (r, c), p in probs.items():
                idx = r * 15 + c
                policy_target[idx] = p

            game_history.append(
                (
                    state_tensor.cpu().numpy(),
                    policy_target,
                    current_player,
                )
            )

            state = state.apply_action(action)
            tree.update_root(action)

        self.policy.clear_cache()
        winner = state.get_winner()

        training_data = []
        for state_tensor, policy_target, player in game_history:
            value = 0.0 if winner is None else (1.0 if player == winner else -1.0)
            training_data.append((state_tensor, policy_target, value))

        return training_data


def save_batch(data, save_dir, batch_id):
    Path(save_dir).mkdir(parents=True, exist_ok=True)
    filepath = os.path.join(save_dir, f"batch_{batch_id:06d}.pkl")
    with open(filepath, "wb") as f:
        pickle.dump(data, f)
    return filepath


def generate_games(
    net, num_games=100, num_simulations=800, save_dir="./selfplay_data", batch_size=10
):
    selfplay = SelfPlay(net, num_simulations)

    batch_data = []
    batch_id = 0
    total_samples = 0

    existing = (
        sorted(Path(save_dir).glob("batch_*.pkl")) if os.path.exists(save_dir) else []
    )
    if existing:
        last_id = int(existing[-1].stem.split("_")[1])
        batch_id = last_id + 1
        print(f"Found existing data, continue from batch {batch_id}")

    for i in range(num_games):
        game_data = selfplay.generate_game()
        batch_data.extend(game_data)
        total_samples += len(game_data)
        print(f"Completed {i + 1}/{num_games}, total samples: {total_samples}")

        if (i + 1) % batch_size == 0:
            filepath = save_batch(batch_data, save_dir, batch_id)
            print(f"  Saved {len(batch_data)} samples to {filepath}")
            batch_data = []
            batch_id += 1

    if batch_data:
        filepath = save_batch(batch_data, save_dir, batch_id)
        print(f"  Saved remaining {len(batch_data)} samples to {filepath}")

    return total_samples


if __name__ == "__main__":
    net = policy.Net(
        board_channels=2, board_size=(19, 15), num_res_blocks=2, channels=64
    )

    total = generate_games(
        net,
        num_games=10,
        num_simulations=1000,
        save_dir="./selfplay_data",
        batch_size=5,
    )
    print(f"Total samples collected: {total}")
