#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mcts_helper.h"
#include "pool.h"

/**
 * 随机打乱数组
 *
 * @param indices 数组
 * @param n       数组大小
 * @return        None
 */
static void shuffle_indices(int *indices, int n)
{
    for (int i = n - 1; i > 0; --i)
    {
        int j = rand() % (i + 1);
        int tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }
}

/**
 * 计算UCB(Upper Confidence Bound)值.
 *
 * @param parent_visits  父节点被访问的次数
 * @param child_visits   子节点被访问的次数
 * @param child_value    子节点的累计价值(总和)
 * @param exploration    探索常数, 控制探索与利用的平衡
 * @return               子节点的UCB值, 若 child_visits == 0 则返回 INFINITY
 */
static float ucb_value(int parent_visits, int child_visits, float child_value, float exploration)
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
 * 创建新节点, 从 bg->total_arena 分配 MCTSNode, 从未尝试动作池构建链表.
 * @param bg            MCTS 背景
 * @param state         状态指针(已分配)
 * @param parent        父节点
 * @param action        从父节点到本节点的动作(指针, 指向大小为 bg->action_size 的数据)
 * @param legal_actions 合法动作数组(只读, 每个元素大小为 bg->action_size)
 * @param num_actions   合法动作数量
 * @return 新节点指针, 失败返回 NULL
 */
MCTSNode *node_create(const MCTSBackground *bg,
                      void *state, MCTSNode *parent,
                      const void *legal_actions, int num_actions)
{
    /* 分配节点内存*/
    MCTSNode *node = (MCTSNode *)arena_alloc(bg->total_arena, sizeof(MCTSNode));

    node->state = state;
    node->parent = parent;
    node->visits = 0;
    node->total_value = 0.0f;
    node->children = NULL; /* 子节点链表初始为空 */
    node->next = NULL;     /* 侵入式指针初始为空(用于父节点的链表) */
    node->num_untried = num_actions;
    memcmp(node->untried_actions, legal_actions, bg->action_size * num_actions);

    return node;
}

/**
 * 将子节点添加到父节点的子节点链表(头插法).
 * @param parent 父节点
 * @param child  子节点
 */
static void node_add_child(MCTSNode *parent, MCTSNode *child)
{
    /* 头插法将子节点插入父节点的 children 链表 */
    child->next = parent->children;
    parent->children = child;
}

/**
 * 从节点的未尝试动作链表中弹出一个动作, 将动作内容拷贝到 action_out.
 * @param node          目标节点
 * @param action_out    输出缓冲区, 大小至少为 bg->action_size
 * @param bg            MCTS 背景(含 untried_pool)
 *
 * 前置条件：node->num_untried > 0
 */

static void *node_pop_random_untried(MCTSNode *node, const MCTSBackground *bg)
{
    int n = node->num_untried;
    int pick_idx = rand() % n;
    int last_idx = n - 1;
    char *base = (char *)node->untried_actions;
    char *pick_ptr = base + pick_idx * bg->action_size;
    char *last_ptr = base + last_idx * bg->action_size;
    if (pick_idx != last_idx)
    {
        for (int i = 0; i < bg->action_size; i++)
        {
            char tmp = pick_ptr[i];
            pick_ptr[i] = last_ptr[i];
            last_ptr[i] = tmp;
        }
        pick_ptr = last_ptr;
    }
    node->num_untried--;
    return pick_ptr;
}

/**
 * 遍历子节点链表, 选择 UCB 值最大的子节点.
 * @param node        当前节点
 * @param exploration 探索常数
 * @return 最佳子节点, 若无子节点返回 NULL
 */
static MCTSNode *node_select_best_child(MCTSNode *node, float exploration)
{
    if (!node->children)
        return NULL;

    MCTSNode *best = NULL;
    float best_ucb = -INFINITY;

    /* 遍历子节点链表 */
    for (MCTSNode *child = node->children; child != NULL; child = child->next)
    {
        float ucb = ucb_value(node->visits, child->visits, child->total_value, exploration);
        if (ucb > best_ucb)
        {
            best_ucb = ucb;
            best = child;
        }
    }
    return best;
}

/**
 * 更新节点
 *
 * @param node 节点
 * @param value 反向传播上来的或者直接获取的价值数
 * @return None
 */
static void node_update(MCTSNode *node, float value)
{
    node->visits++;
    node->total_value += value;
}

/**
 * 选择阶段：从起始节点开始, 沿着最佳子节点下降, 直到遇到终止节点或还有未尝试动作的节点.
 * @param start       起始节点
 * @param exploration 探索常数
 * @param is_terminal 外部终止判断函数
 * @return 选中的叶子节点
 */
MCTSNode *mcts_select(MCTSNode *start, float exploration, int (*is_terminal)(void *))
{
    MCTSNode *node = start;
    while (!is_terminal(node->state) && node->num_untried == 0)
    {
        node = node_select_best_child(node, exploration);
    }
    return node;
}

/**
 * 评估阶段：如果 use_policy 则调用外部 evaluate, 否则进行 rollout.
 * @param node 要评估的节点
 * @param bg   MCTS 背景
 * @return 评估价值
 */
float mcts_evaluate(MCTSNode *node, const MCTSBackground *bg)
{
    if (bg->use_policy)
    {
        return bg->evaluate(node->state);
    }
    else
    {
        return rollout(node->state, bg);
    }
}

/**
 * 扩展阶段：为叶子节点添加一个子节点.
 * @param leaf         叶子节点(num_untried > 0)
 * @param bg           MCTS 背景
 * @return 新子节点指针, 失败返回 NULL
 */
MCTSNode *mcts_expand(MCTSNode *leaf, const MCTSBackground *bg)
{
    /* 弹出随机未尝试动作 */
    void *new_action = node_pop_random_untried(leaf, bg);

    /* 应用动作得到新状态 */
    void *new_state = arena_alloc(bg->total_arena, bg->state_size);
    memcpy(new_state, leaf->state, bg->state_size);
    bg->apply_action(leaf->state, new_action);
    if (!new_state)
        return NULL;

    /* 获取新状态的合法动作 */
    int num_actions = bg->get_legal_actions(new_state, bg->action_buffer, bg->max_actions);
    if (num_actions < 0)
        return NULL;

    /* 创建子节点 */
    MCTSNode *child = node_create(bg, new_state, leaf, bg->action_buffer, num_actions);

    /* 将子节点加入父节点 */
    node_add_child(leaf, child);
    return child;
}

/**
 * 回溯阶段：从叶子节点向上更新所有祖先节点的统计信息.
 * @param leaf  本次模拟的叶子节点
 * @param value 本次模拟获得的原始价值(从叶子节点视角)
 */
void mcts_backup(MCTSNode *leaf, float value)
{
    MCTSNode *node = leaf;
    float v = value;
    while (node)
    {
        node_update(node, v);
        v = -v; /* 翻转价值, 用于父节点(零和博弈) */
        node = node->parent;
    }
}

/**
 * 从根节点获取当前最优动作(访问次数最多的子节点对应的动作), 拷贝到 action_out.
 * @param root        根节点
 * @param action_out  输出缓冲区, 大小至少为 bg->action_size
 * @param bg          MCTS 背景(用于获取动作大小)
 *
 * 前置条件：root 至少有一个子节点.
 */
void mcts_best_action(MCTSNode *root, void *action_out, const MCTSBackground *bg)
{
    MCTSNode *best_child = NULL;
    int max_visits = -1;

    for (MCTSNode *child = root->children; child != NULL; child = child->next)
    {
        if (child->visits > max_visits)
        {
            max_visits = child->visits;
            best_child = child;
        }
    }

    if (best_child)
    {
        void *action = (char *)best_child + sizeof(MCTSNode);
        memcpy(action_out, action, bg->action_size);
    }
}