import rules
import mcts
import policy

if __name__ == "__main__":
    config = mcts.MCTSConfig(num_simulations=1000, temperature=1)
    tree = mcts.MCTS(config)

    state = rules.GameState()
    print("初始状态:")
    state.__repr__()

    while not state.is_terminal():
        action, probs = tree.search(state)
        print(f"\n动作概率: {probs}")
        print(f"选择动作: {action}")

        state = state.apply_action(action)
        state.__repr__()

        tree.update_root(action)

    winner = state.state[1]
    print(f"游戏结束，获胜者: {'X' if winner == 0 else 'O' if winner == 1 else '平局'}")
