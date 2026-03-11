import math
import random
from typing import Optional, Dict, List, Tuple
from dataclasses import dataclass
import policy
import rules


@dataclass
class MCTSConfig:
    num_simulations: int = 800
    c_puct: float = 1.414
    temperature: float = 1.0
    dirichlet_epsilon: float = 0.0
    dirichlet_alpha: float = 0.3
    use_value_network: bool = False


class MCTSNode:
    __slots__ = [
        "state",
        "parent",
        "action",
        "children",
        "untried_actions",
        "visit_count",
        "total_value",
        "prior_prob",
        "player_id",
    ]

    def __init__(self, state, parent=None, action=None, prior_prob=1.0):
        self.state = state
        self.parent = parent
        self.action = action
        self.children: Dict[any, MCTSNode] = {}
        self.untried_actions: Optional[List] = None

        self.visit_count = 0
        self.total_value = 0.0
        self.prior_prob = prior_prob
        self.player_id = state.get_current_player()

    def is_fully_expanded(self) -> bool:
        return self.untried_actions is not None and len(self.untried_actions) == 0

    def best_child(self, c_puct: float) -> "MCTSNode":
        parent_visits = self.visit_count

        def uct_score(child: MCTSNode) -> float:
            if child.visit_count == 0:
                q_value = 0.0
            else:
                q_value = -child.total_value / child.visit_count

            u_value = (
                c_puct
                * child.prior_prob
                * math.sqrt(parent_visits)
                / (1 + child.visit_count)
            )

            return q_value + u_value

        return max(self.children.values(), key=uct_score)

    def expand(self, action, new_state, prior_prob=1.0) -> "MCTSNode":
        child = MCTSNode(new_state, parent=self, action=action, prior_prob=prior_prob)
        self.children[action] = child
        self.untried_actions.remove(action)
        return child

    def update(self, value: float):
        self.visit_count += 1
        self.total_value += value

    def mean_value(self) -> float:
        if self.visit_count == 0:
            return 0.0
        return self.total_value / self.visit_count


class MCTS:
    def __init__(
        self, config: MCTSConfig = None, policy: Optional[policy.Policy] = None
    ):
        self.config = config or MCTSConfig()
        self.policy = policy
        self.root: Optional[MCTSNode] = None

    def search(self, root_state) -> Tuple[any, Dict[any, float]]:
        self.root = MCTSNode(root_state)

        if self.policy and self.config.dirichlet_epsilon > 0:
            self._add_dirichlet_noise(self.root)

        for i in range(self.config.num_simulations):
            node = self._select(self.root)
            value = self._evaluate(node)
            self._backup(node, value)
            if i % 100 == 0:
                visits = {a: n.visit_count for a, n in self.root.children.items()}
                print(f"Sim {i}: {visits}")

        action_visits = {a: n.visit_count for a, n in self.root.children.items()}
        print(f"Final visits: {action_visits}")
        total_visits = sum(action_visits.values())

        if self.config.temperature == 0:
            best_action = max(action_visits.items(), key=lambda x: x[1])[0]
            probs = {a: 1.0 if a == best_action else 0.0 for a in action_visits}
        else:
            visits_temp = {
                a: (v / total_visits) ** (1.0 / self.config.temperature)
                for a, v in action_visits.items()
            }
            sum_temp = sum(visits_temp.values())
            probs = {a: v / sum_temp for a, v in visits_temp.items()}
            best_action = random.choices(
                list(probs.keys()), weights=list(probs.values())
            )[0]

        return best_action, probs

    def _select(self, node: MCTSNode) -> MCTSNode:
        while not node.state.is_terminal():
            if not node.is_fully_expanded():
                return self._expand(node)
            else:
                node = node.best_child(self.config.c_puct)
        return node

    def _expand(self, node: MCTSNode) -> MCTSNode:
        if node.untried_actions is None:
            node.untried_actions = node.state.get_legal_actions()
            if self.policy:
                actions, probs = self.policy.get_action_probs(node.state)
                prior_dict = dict(zip(actions, probs))
                for action in node.untried_actions:
                    if action not in prior_dict:
                        prior_dict[action] = 1.0 / len(node.untried_actions)
                node.prior_dict = prior_dict

        action = random.choice(node.untried_actions)
        new_state = node.state.apply_action(action)

        prior = getattr(node, "prior_dict", {}).get(action, 1.0)
        return node.expand(action, new_state, prior)

    def _evaluate(self, node: MCTSNode) -> float:
        if node.state.is_terminal():
            return node.state.get_reward(node.player_id)

        if self.config.use_value_network and self.policy:
            return self.policy.get_value(node.state)
        else:
            return self._rollout(node.state, node.player_id)

    def _rollout(self, state, perspective_player: int) -> float:
        current_state = state
        while not current_state.is_terminal():
            actions = current_state.get_legal_actions()
            action = random.choice(actions)
            current_state = current_state.apply_action(action)

        return current_state.get_reward(perspective_player)

    def _backup(self, node: MCTSNode, value: float):
        current = node
        while current is not None:
            if current.player_id == node.player_id:
                current.update(value)
            else:
                current.update(-value)
            current = current.parent

    def _add_dirichlet_noise(self, node: MCTSNode):
        if node.untried_actions is None:
            node.untried_actions = node.state.get_legal_actions()

        actions = node.untried_actions
        for _ in actions:
            noise = random.gammavariate(self.config.dirichlet_alpha, 1.0)
        noise = [n / sum(noise) for n in noise]

        node.dirichlet_noise = dict(zip(actions, noise))

    def update_root(self, action):
        if self.root and action in self.root.children:
            self.root = self.root.children[action]
            self.root.parent = None
        else:
            self.root = None

    def get_stats(self) -> Dict:
        if not self.root:
            return {}

        return {
            "root_visits": self.root.visit_count,
            "children": {
                a: {"visits": n.visit_count, "value": n.mean_value()}
                for a, n in self.root.children.items()
            },
        }
