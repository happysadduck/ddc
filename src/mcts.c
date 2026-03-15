#include <string.h>
#include <math.h>
#include "mcts.h"
#include "pool.h"

/**
 * 计算UCB（Upper Confidence Bound）值。
 *
 * @param parent_visits  父节点被访问的次数
 * @param child_visits   子节点被访问的次数
 * @param child_value    子节点的累计价值（总和）
 * @param exploration    探索常数，控制探索与利用的平衡
 * @return               子节点的UCB值，若 child_visits == 0 则返回 INFINITY
 */
float ucb_value(int parent_visits, int child_visits, float child_value, float exploration)
{
    if (child_visits == 0)
        return INFINITY;
    if (parent_visits <= 0)
        parent_visits = 1;

    float exploitation = child_value / child_visits;
    float exploration_term = exploration * sqrtf(logf((float)parent_visits) / child_visits);

    return exploitation + exploration_term;
}

/**
 * 根据动作数组构建随机顺序的未尝试动作链表。
 * @param bg           MCTS 背景（含 action_size、untried_pool、index_buffer）
 * @param actions      动作数组（只读，每个元素大小为 bg->action_size）
 * @param num_actions  动作数量
 * @param head         输出参数：链表头指针
 * @return 成功返回 num_actions，失败返回 -1（例如 pool_alloc 失败）
 */
int untried_list_create(MCTSBackground *bg, const void *actions, int num_actions, UntriedAction **head);

/**
 * 创建新节点，从 node_arena 分配 MCTSNode，从未尝试动作池构建链表。
 * @param node_arena    用于分配 MCTSNode 的 Arena
 * @param bg            MCTS 背景（含 untried_pool、index_buffer 等）
 * @param state         状态指针（已分配）
 * @param parent        父节点
 * @param action        从父节点到本节点的动作（指针，指向大小为 bg->action_size 的数据）
 * @param legal_actions 合法动作数组（只读，每个元素大小为 bg->action_size）
 * @param num_actions   合法动作数量
 * @return 新节点指针，失败返回 NULL
 */
MCTSNode *node_create(Arena *node_arena, MCTSBackground *bg,
                      void *state, MCTSNode *parent, void *action,
                      const void *legal_actions, int num_actions);

/**
 * 将子节点添加到父节点的子节点链表（头插法）。
 * @param parent 父节点
 * @param child  子节点
 */
void node_add_child(MCTSNode *parent, MCTSNode *child);

/**
 * 从节点的未尝试动作链表中弹出一个动作，将动作内容拷贝到 action_out。
 * @param node          目标节点
 * @param action_out    输出缓冲区，大小至少为 bg->action_size
 * @param bg            MCTS 背景（含 untried_pool）
 *
 * 前置条件：node->num_untried > 0
 */
void node_pop_random_untried(MCTSNode *node, void *action_out, MCTSBackground *bg);

/**
 * 遍历子节点链表，选择 UCB 值最大的子节点。
 * @param node        当前节点
 * @param exploration 探索常数
 * @return 最佳子节点，若无子节点返回 NULL
 */
MCTSNode *node_select_best_child(MCTSNode *node, float exploration);

void node_update(MCTSNode *node, float value);

/**
 * 选择阶段：从起始节点开始，沿着最佳子节点下降，直到遇到终止节点或还有未尝试动作的节点。
 * @param start       起始节点
 * @param exploration 探索常数
 * @param is_terminal 外部终止判断函数
 * @return 选中的叶子节点
 */
MCTSNode *mcts_select(MCTSNode *start, float exploration, int (*is_terminal)(void *));

/**
 * 从给定状态开始进行随机模拟（或使用策略），返回最终价值。
 * @param state 起始状态
 * @param bg    MCTS 背景
 * @return 模拟得到的价值（从当前玩家视角，通常为 1/0/-1）
 */
static float rollout(void *state, MCTSBackground *bg);

/**
 * 评估阶段：如果 use_policy 则调用外部 evaluate，否则进行 rollout。
 * @param node 要评估的节点
 * @param bg   MCTS 背景
 * @return 评估价值
 */
float mcts_evaluate(MCTSNode *node, MCTSBackground *bg);

/**
 * 扩展阶段：为叶子节点添加一个子节点。
 * @param leaf         叶子节点（num_untried > 0）
 * @param node_arena   用于分配 MCTSNode 的 Arena
 * @param bg           MCTS 背景
 * @return 新子节点指针，失败返回 NULL
 */
MCTSNode *mcts_expand(MCTSNode *leaf, Arena *node_arena, MCTSBackground *bg);

/**
 * 回溯阶段：从叶子节点向上更新所有祖先节点的统计信息。
 * @param leaf  本次模拟的叶子节点
 * @param value 本次模拟获得的原始价值（从叶子节点视角）
 */
void mcts_backup(MCTSNode *leaf, float value);

/**
 * 从根节点获取当前最优动作（访问次数最多的子节点对应的动作），拷贝到 action_out。
 * @param root        根节点
 * @param action_out  输出缓冲区，大小至少为 bg->action_size
 * @param bg          MCTS 背景（用于获取动作大小）
 *
 * 前置条件：root 至少有一个子节点。
 */
void mcts_best_action(MCTSNode *root, void *action_out, MCTSBackground *bg);