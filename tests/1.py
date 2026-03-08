import math
import random
from typing import Optional, Dict, List, Tuple
from dataclasses import dataclass
from collections import defaultdict


@dataclass
class MCTSConfig:
    """MCTS配置参数"""
    num_simulations: int = 800          # 模拟次数
    c_puct: float = 1.414               # PUCT探索常数（sqrt(2)约等于1.414）
    temperature: float = 1.0            # 动作选择温度（0=argmax, 1=按visit比例）
    dirichlet_epsilon: float = 0.0      # Dirichlet噪声比例（0=关闭）
    dirichlet_alpha: float = 0.3        # Dirichlet分布参数
    use_value_network: bool = False     # 是否使用价值网络替代rollout

class Policy:
    def __init__(self):
        pass

class MCTSNode:
    """MCTS树节点"""
    __slots__ = ['state', 'parent', 'action', 'children', 'untried_actions',
                 'visit_count', 'total_value', 'prior_prob', 'player_id']
    
    def __init__(self, state, parent=None, action=None, prior_prob=1.0):
        self.state = state
        self.parent = parent              # 父节点
        self.action = action              # 从父节点到本节点的动作
        self.children: Dict[any, MCTSNode] = {}  # action -> node
        self.untried_actions: Optional[List] = None  # 未尝试的动作
        
        self.visit_count = 0
        self.total_value = 0.0            # 从当前玩家视角的累计价值
        self.prior_prob = prior_prob      # 先验概率（来自策略网络）
        self.player_id = state.get_current_player()  # 当前节点是哪个玩家的回合
    
    def is_fully_expanded(self) -> bool:
        """是否所有动作都已扩展"""
        return self.untried_actions is not None and len(self.untried_actions) == 0
    
    def best_child(self, c_puct: float) -> 'MCTSNode':
        """
        使用PUCT公式选择最佳子节点
        Q(s,a) + c_puct * P(s,a) * sqrt(N(s)) / (1 + N(s,a))
        """
        parent_visits = self.visit_count
        
        def uct_score(child: MCTSNode) -> float:
            # Q值：平均价值（从父节点视角，需要转换）
            # 子节点的价值是从子节点player视角的，对父节点来说是负的
            if child.visit_count == 0:
                q_value = 0.0
            else:
                # 价值转换：子节点存储的是从子节点player视角的价值
                # 对父节点来说，子节点的价值是 -child.mean_value()
                q_value = -child.total_value / child.visit_count
            
            # U值：探索奖励
            u_value = (c_puct * child.prior_prob * 
                      math.sqrt(parent_visits) / (1 + child.visit_count))
            
            return q_value + u_value
        
        return max(self.children.values(), key=uct_score)
    
    def expand(self, action, new_state, prior_prob=1.0) -> 'MCTSNode':
        """扩展新节点"""
        child = MCTSNode(new_state, parent=self, action=action, prior_prob=prior_prob)
        self.children[action] = child
        self.untried_actions.remove(action)
        return child
    
    def update(self, value: float):
        """反向传播更新节点"""
        self.visit_count += 1
        # value是从当前节点player视角的价值
        self.total_value += value
    
    def mean_value(self) -> float:
        """平均价值"""
        if self.visit_count == 0:
            return 0.0
        return self.total_value / self.visit_count


class MCTS:
    """MCTS主类"""
    
    def __init__(self, config: MCTSConfig = None, policy: Optional[Policy] = None):
        self.config = config or MCTSConfig()
        self.policy = policy
        self.root: Optional[MCTSNode] = None
    
    def search(self, root_state) -> Tuple[any, Dict[any, float]]:
        """
        执行MCTS搜索
        Args: root_state - GameState实例
        Returns: (best_action, action_probs_dict)
            best_action: 最佳动作
            action_probs: 每个动作的概率分布（可用于训练）
        """
        self.root = MCTSNode(root_state)
        
        # 如果有策略网络，添加Dirichlet噪声（训练时增加探索）
        if self.policy and self.config.dirichlet_epsilon > 0:
            self._add_dirichlet_noise(self.root)
        
        # 执行多次模拟
        for _ in range(self.config.num_simulations):
            node = self._select(self.root)
            value = self._evaluate(node)
            self._backup(node, value)
        
        # 计算动作概率分布
        action_visits = {a: n.visit_count for a, n in self.root.children.items()}
        total_visits = sum(action_visits.values())
        
        if self.config.temperature == 0:
            # 选择访问次数最多的
            best_action = max(action_visits.items(), key=lambda x: x[1])[0]
            probs = {a: 1.0 if a == best_action else 0.0 for a in action_visits}
        else:
            # 按访问次数的temperature次方比例选择
            visits_temp = {a: (v / total_visits) ** (1.0 / self.config.temperature) 
                          for a, v in action_visits.items()}
            sum_temp = sum(visits_temp.values())
            probs = {a: v / sum_temp for a, v in visits_temp.items()}
            best_action = random.choices(list(probs.keys()), weights=list(probs.values()))[0]
        
        return best_action, probs
    
    def _select(self, node: MCTSNode) -> MCTSNode:
        """选择阶段：从根节点选择到叶子节点"""
        while not node.state.is_terminal():
            if not node.is_fully_expanded():
                return self._expand(node)
            else:
                node = node.best_child(self.config.c_puct)
        return node
    
    def _expand(self, node: MCTSNode) -> MCTSNode:
        """扩展阶段：从untried_actions中选择一个动作扩展"""
        if node.untried_actions is None:
            node.untried_actions = node.state.get_legal_actions()
            # 如果有策略网络，获取先验概率
            if self.policy:
                actions, probs = self.policy.get_action_probs(node.state)
                prior_dict = dict(zip(actions, probs))
                # 为untried_actions设置先验概率
                for action in node.untried_actions:
                    if action not in prior_dict:
                        prior_dict[action] = 1.0 / len(node.untried_actions)
                node.prior_dict = prior_dict
        
        action = random.choice(node.untried_actions)
        new_state = node.state.apply_action(action)
        
        prior = getattr(node, 'prior_dict', {}).get(action, 1.0)
        return node.expand(action, new_state, prior)
    
    def _evaluate(self, node: MCTSNode) -> float:
        """评估阶段：返回叶子节点的价值"""
        if node.state.is_terminal():
            # 游戏结束，返回实际奖励（从当前节点player视角）
            return node.state.get_reward(node.player_id)
        
        if self.config.use_value_network and self.policy:
            # 使用价值网络评估
            return self.policy.get_value(node.state)
        else:
            # 随机rollout（快速走子直到结束）
            return self._rollout(node.state, node.player_id)
    
    def _rollout(self, state, perspective_player: int) -> float:
        """随机模拟直到游戏结束"""
        current_state = state
        while not current_state.is_terminal():
            actions = current_state.get_legal_actions()
            action = random.choice(actions)
            current_state = current_state.apply_action(action)
        
        # 返回从perspective_player视角的奖励
        return current_state.get_reward(perspective_player)
    
    def _backup(self, node: MCTSNode, value: float):
        """反向传播：将价值更新到路径上所有节点"""
        current = node
        while current is not None:
            # value是从原始叶子节点player视角的
            # 如果当前节点player与原始相同，加value；否则加-value
            if current.player_id == node.player_id:
                current.update(value)
            else:
                current.update(-value)
            current = current.parent
    
    def _add_dirichlet_noise(self, node: MCTSNode):
        """为根节点添加Dirichlet噪声增加探索"""
        if node.untried_actions is None:
            node.untried_actions = node.state.get_legal_actions()
        
        actions = node.untried_actions
        for _ in actions:
            noise = random.gammavariate(self.config.dirichlet_alpha, 1.0)
        noise = [n / sum(noise) for n in noise]
        
        # 这里我们只记录噪声，实际在expand时应用
        node.dirichlet_noise = dict(zip(actions, noise))
    
    def update_root(self, action):
        """重用子树：执行动作后更新根节点"""
        if self.root and action in self.root.children:
            self.root = self.root.children[action]
            self.root.parent = None  # 断开旧连接
        else:
            self.root = None
    
    def get_stats(self) -> Dict:
        """获取搜索统计信息"""
        if not self.root:
            return {}
        
        return {
            'root_visits': self.root.visit_count,
            'children': {
                a: {'visits': n.visit_count, 'value': n.mean_value()}
                for a, n in self.root.children.items()
            }
        }


# ==================== 使用示例：井字棋 ====================

class TicTacToeState:
    """井字棋状态示例"""
    
    def __init__(self, board=None, current_player=0):
        self.board = board if board is not None else [None] * 9
        self.current_player = current_player
    
    def get_legal_actions(self) -> list:
        return [i for i in range(9) if self.board[i] is None]
    
    def apply_action(self, action) -> 'TicTacToeState':
        new_board = self.board.copy()
        new_board[action] = self.current_player
        return TicTacToeState(new_board, 1 - self.current_player)
    
    def is_terminal(self) -> bool:
        return self.get_winner() is not None or None not in self.board
    
    def get_winner(self):
        # 检查胜负
        wins = [(0,1,2), (3,4,5), (6,7,8), (0,3,6), (1,4,7), (2,5,8), (0,4,8), (2,4,6)]
        for a, b, c in wins:
            if self.board[a] is not None and self.board[a] == self.board[b] == self.board[c]:
                return self.board[a]
        return None if None in self.board else -1  # -1表示平局
    
    def get_reward(self, player_id: int) -> float:
        winner = self.get_winner()
        if winner == -1:
            return 0.0  # 平局
        return 1.0 if winner == player_id else -1.0
    
    def get_current_player(self) -> int:
        return self.current_player
    
    def __hash__(self):
        return hash(tuple(self.board))
    
    def __eq__(self, other):
        return (isinstance(other, TicTacToeState) and 
                self.board == other.board and 
                self.current_player == other.current_player)
    
    def __repr__(self):
        symbols = {None: '.', 0: 'X', 1: 'O'}
        rows = [''.join(symbols[self.board[i]] for i in range(j, j+3)) 
                for j in range(0, 9, 3)]
        return '\n'.join(rows) + f'\nPlayer: {self.current_player}\n'

# 测试
if __name__ == "__main__":
    # 基础MCTS（无神经网络）
    config = MCTSConfig(num_simulations=1000, temperature=0)
    mcts = MCTS(config)
    
    state = TicTacToeState()
    print("初始状态:")
    print(state)
    
    # MCTS自我对弈
    while not state.is_terminal():
        action, probs = mcts.search(state)
        print(f"\n动作概率: {probs}")
        print(f"选择动作: {action}")
        
        state = state.apply_action(action)
        print(state)
        
        mcts.update_root(action)  # 重用搜索树
    
    winner = state.get_winner()
    print(f"游戏结束，获胜者: {'X' if winner == 0 else 'O' if winner == 1 else '平局'}")