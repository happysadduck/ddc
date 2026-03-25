#include <stddef.h>
#include <math.h>
#include "mcts.h"
#include "mcts_helper.h"
#include "pool.h"

/**
 * 创建根节点
 */
MCTSNode *mcts_create_root(MCTSBackground *bg, void *state)
{
    void *new_actions = arena_alloc(bg->total_arena, bg->action_size * bg->max_actions);
    int num_actions = bg->get_legal_actions(state, new_actions, bg->max_actions);
    if (num_actions < 0)
        return NULL;
    return node_create(bg, state, NULL, new_actions, num_actions);
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
 * 推荐动作：创建根节点，执行搜索，输出最佳动作并清理
 */
void mcts_recommend(void *state, MCTSBackground *bg, void *action_out, int num_iterations)
{
    MCTSNode *root = mcts_create_root(bg, state);
    mcts_search(root, bg, num_iterations);
    mcts_best_action(root, action_out, bg);
    arena_clear(bg->total_arena);
}

int mcts_bg_mem_estimate(
    int max_iterations, int max_actions, int action_size, int state_size)
{
    int total = 0;
    // Arena 控制块本身
    total += sizeof(Arena);
    // 节点内存(包括每个节点挂载的一个state)
    int max_nodes = max_iterations + 1;
    int node_mem = max_nodes * (sizeof(MCTSNode) + state_size);
    total += node_mem;
    // 所有action的内存
    int actions_mem = max_nodes * max_actions; // 假设每个节点都全都扩展, 非常保守
    total += actions_mem;
    return total;
}

void make_mcts_bg(
    int (*get_legal_actions)(void *state,
                             void *actions_buffer,
                             int buffer_capacity),
    void *(*apply_action)(void *state, void *action),
    int (*is_terminal)(void *state),
    int action_size,
    int state_size,
    int max_actions,
    int use_policy,
    void (*policy_sample)(void *state, void *action_out),
    float (*evaluate)(void *state),
    float (*rollout)(void *state),
    float exploration_constant,
    int max_iterations,
    void *buf, int buf_size, MCTSBackground *out)
{
    Arena *arena = prepare_arena(buf_size, buf);
    int data_lenth = 0;
    out->get_legal_actions = get_legal_actions;
    out->apply_action = apply_action;
    out->is_terminal = is_terminal;
    out->action_size = action_size;
    out->use_policy = use_policy;
    out->policy_sample = policy_sample;
    out->evaluate = evaluate;
    out->rollout = rollout;
    out->exploration_constant = exploration_constant;
    out->max_actions = max_actions;

    out->total_arena = arena;
}