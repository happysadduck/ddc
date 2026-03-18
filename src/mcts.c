#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mcts.h"
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
int untried_list_create(MCTSBackground *bg, const void *actions, int num_actions, UntriedAction **head)
{
    if (num_actions == 0)
    {
        *head = NULL;
        return 0;
    }

    /* 使用bg->index_buffer存储随机顺序的索引 */
    int *indices = bg->index_buffer;
    for (int i = 0; i < num_actions; ++i)
        indices[i] = i;
    shuffle_indices(indices, num_actions);

    UntriedAction *prev = NULL;
    for (int i = 0; i < num_actions; ++i)
    {
        UntriedAction *node = (UntriedAction *)pool_alloc(bg->untried_pool);
        /* 动作数据紧随UntriedAction之后 */
        void *action_storage = (char *)node + sizeof(UntriedAction);
        const void *src_action = (const char *)actions + indices[i] * bg->action_size;
        memcpy(action_storage, src_action, bg->action_size);

        node->next = prev;
        prev = node;
    }
    *head = prev;
    return num_actions;
}

/**
 * 创建新节点，从 bg->node_arena 分配 MCTSNode，从未尝试动作池构建链表。
 * @param bg            MCTS 背景（含 untried_pool、index_buffer 等）
 * @param state         状态指针（已分配）
 * @param parent        父节点
 * @param action        从父节点到本节点的动作（指针，指向大小为 bg->action_size 的数据）
 * @param legal_actions 合法动作数组（只读，每个元素大小为 bg->action_size）
 * @param num_actions   合法动作数量
 * @return 新节点指针，失败返回 NULL
 */
MCTSNode *node_create(MCTSBackground *bg,
                      void *state, MCTSNode *parent, void *action,
                      const void *legal_actions, int num_actions)
{
    /* 创建未尝试动作链表 */
    UntriedAction *untried_head;
    if (untried_list_create(bg, legal_actions, num_actions, &untried_head) < 0)
        return NULL;

    /* 分配节点内存（额外预留动作存储空间） */
    size_t node_size = sizeof(MCTSNode) + bg->action_size;
    MCTSNode *node = (MCTSNode *)arena_alloc(bg->node_arena, (int)node_size);

    node->state = state;
    node->parent = parent;
    node->visits = 0;
    node->total_value = 0.0f;
    node->children = NULL; /* 子节点链表初始为空 */
    node->next = NULL;     /* 侵入式指针初始为空（用于父节点的链表） */
    node->num_untried = num_actions;
    node->untried_actions = untried_head;

    /* 存储从父节点到本节点的动作（紧跟在 MCTSNode 之后） */
    if (action)
    {
        void *node_action = (char *)node + sizeof(MCTSNode);
        memcpy(node_action, action, bg->action_size);
    }

    return node;
}

/**
 * 将子节点添加到父节点的子节点链表（头插法）。
 * @param parent 父节点
 * @param child  子节点
 */
void node_add_child(MCTSNode *parent, MCTSNode *child)
{
    /* 头插法将子节点插入父节点的 children 链表 */
    child->next = parent->children;
    parent->children = child;
    child->parent = parent; /* 确保父节点正确 */
}

/**
 * 从节点的未尝试动作链表中弹出一个动作，将动作内容拷贝到 action_out。
 * @param node          目标节点
 * @param action_out    输出缓冲区，大小至少为 bg->action_size
 * @param bg            MCTS 背景（含 untried_pool）
 *
 * 前置条件：node->num_untried > 0
 */
void node_pop_random_untried(MCTSNode *node, void *action_out, MCTSBackground *bg)
{
    /* 从未尝试动作链表中弹出头部*/
    UntriedAction *head = (UntriedAction *)node->untried_actions;
    node->untried_actions = head->next;
    node->num_untried--;

    void *action_data = (char *)head + sizeof(UntriedAction);
    memcpy(action_out, action_data, bg->action_size);

    pool_return(bg->untried_pool, head);
}

/**
 * 遍历子节点链表，选择 UCB 值最大的子节点。
 * @param node        当前节点
 * @param exploration 探索常数
 * @return 最佳子节点，若无子节点返回 NULL
 */
MCTSNode *node_select_best_child(MCTSNode *node, float exploration)
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
void node_update(MCTSNode *node, float value)
{
    node->visits++;
    node->total_value += value;
}

/**
 * 选择阶段：从起始节点开始，沿着最佳子节点下降，直到遇到终止节点或还有未尝试动作的节点。
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
 * 从给定状态开始进行随机模拟（或使用策略），返回最终价值。
 * @param state 起始状态
 * @param bg    MCTS 背景
 * @return 模拟得到的价值（从当前玩家视角，通常为 1/0/-1）
 */
static float rollout(void *state, MCTSBackground *bg)
{
    void *cur = state;
    int is_first = 1; /* 标记当前状态是否为传入的原始状态（不释放） */

    while (!bg->is_terminal(cur))
    {
        /* 获取合法动作 */
        int num_actions = bg->get_legal_actions(cur, bg->action_buffer, bg->max_actions);
        if (num_actions == 0)
            break;

        void *action;
        if (bg->use_policy)
        {
            /*TODO*/
            /*应当使用快速评估方法, 但是目前没有, 因此使用随机选择, 但保留分支*/
            int idx = rand() % num_actions;
            action = (char *)bg->action_buffer + idx * bg->action_size;
        }
        else
        {
            /* 随机选择 */
            int idx = rand() % num_actions;
            action = (char *)bg->action_buffer + idx * bg->action_size;
        }

        /* 应用动作得到新状态 */
        void *new_state = bg->apply_action(cur, action);
        if (!new_state)
            break; /* 应用失败，停止模拟 */

        /* 释放旧状态（如果不是原始传入状态） */
        if (!is_first)
            pool_return(bg->state_pool, cur);

        cur = new_state;
        is_first = 0;
    }

    float value = bg->evaluate(cur);

    /* 释放最终状态（如果不是原始状态） */
    if (!is_first)
        pool_return(bg->state_pool, cur);

    return value;
}

/**
 * 评估阶段：如果 use_policy 则调用外部 evaluate，否则进行 rollout。
 * @param node 要评估的节点
 * @param bg   MCTS 背景
 * @return 评估价值
 */
float mcts_evaluate(MCTSNode *node, MCTSBackground *bg)
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
 * 扩展阶段：为叶子节点添加一个子节点。
 * @param leaf         叶子节点（num_untried > 0）
 * @param bg           MCTS 背景
 * @return 新子节点指针，失败返回 NULL
 */
MCTSNode *mcts_expand(MCTSNode *leaf, MCTSBackground *bg)
{
    /* 弹出随机未尝试动作 */
    char action_buffer[bg->action_size]; /* 局部缓冲区，避免覆盖bg->action_buffer */
    node_pop_random_untried(leaf, action_buffer, bg);

    /* 应用动作得到新状态 */
    void *new_state = bg->apply_action(leaf->state, action_buffer);
    if (!new_state)
        return NULL;

    /* 获取新状态的合法动作 */
    int num_actions = bg->get_legal_actions(new_state, bg->action_buffer, bg->max_actions);
    if (num_actions < 0)
    {
        pool_return(bg->state_pool, new_state);
        return NULL;
    }

    /* 创建子节点 */
    MCTSNode *child = node_create(bg, new_state, leaf,
                                  action_buffer, bg->action_buffer, num_actions);
    if (!child)
    {
        pool_return(bg->state_pool, new_state);
        return NULL;
    }

    /* 将子节点加入父节点 */
    node_add_child(leaf, child);
    return child;
}

/**
 * 回溯阶段：从叶子节点向上更新所有祖先节点的统计信息。
 * @param leaf  本次模拟的叶子节点
 * @param value 本次模拟获得的原始价值（从叶子节点视角）
 */
void mcts_backup(MCTSNode *leaf, float value)
{
    MCTSNode *node = leaf;
    float v = value;
    while (node)
    {
        node_update(node, v);
        v = -v; /* 翻转价值，用于父节点（零和博弈） */
        node = node->parent;
    }
}

/**
 * 从根节点获取当前最优动作（访问次数最多的子节点对应的动作），拷贝到 action_out。
 * @param root        根节点
 * @param action_out  输出缓冲区，大小至少为 bg->action_size
 * @param bg          MCTS 背景（用于获取动作大小）
 *
 * 前置条件：root 至少有一个子节点。
 */
void mcts_best_action(MCTSNode *root, void *action_out, MCTSBackground *bg)
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

/**
 * 创建根节点
 */
MCTSNode *mcts_create_root(MCTSBackground *bg, void *state)
{
    int num_actions = bg->get_legal_actions(state, bg->action_buffer, bg->max_actions);
    if (num_actions < 0)
        return NULL;
    return node_create(bg, state, NULL, NULL, bg->action_buffer, num_actions);
}

/**
 * 执行多次MCTS迭代
 */
void mcts_search(MCTSNode *root, MCTSBackground *bg, int num_iterations)
{
    for (int i = 0; i < num_iterations; ++i)
    {
        MCTSNode *leaf = mcts_select(root, bg->exploration_constant, bg->is_terminal);
        if (!leaf)
            continue;
        if (!bg->is_terminal(leaf->state) && leaf->num_untried > 0)
        {
            leaf = mcts_expand(leaf, bg);
            if (!leaf)
                continue;
        }
        float value = mcts_evaluate(leaf, bg);
        mcts_backup(leaf, value);
    }
}

/**
 * 推荐动作：创建根节点，执行搜索，输出最佳动作，并清理内部资源
 */
void mcts_recommend(void *state, MCTSBackground *bg, void *action_out, int num_iterations, Arena *node_arena)
{
    MCTSNode *root = mcts_create_root(bg, state);
    mcts_search(root, bg, num_iterations);
    mcts_best_action(root, action_out, bg);
    /*TODO: 清理*/
}