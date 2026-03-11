import rules
import mcts
import policy

if __name__ == "__main__":
    # 基础MCTS（无神经网络）
    config = mcts.MCTSConfig(num_simulations=1000, temperature=1)
    tree = mcts.MCTS(config)

    state = rules.GameState()
    print("初始状态:")
    state.__repr__()

    # MCTS自我对弈
    while not state.is_terminal():
        action, probs = tree.search(state)
        print(f"\n动作概率: {probs}")
        print(f"选择动作: {action}")

        state = state.apply_action(action)
        state.__repr__()

        tree.update_root(action)  # 重用搜索树

    winner = state.state[1]
    print(f"游戏结束，获胜者: {'X' if winner == 0 else 'O' if winner == 1 else '平局'}")
