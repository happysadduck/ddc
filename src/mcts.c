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
 * 推荐动作：创建根节点，执行搜索，输出最佳动作
 */
void mcts_recommend(void *state, MCTSBackground *bg, void *action_out, int num_iterations, Arena *total_arena)
{
    MCTSNode *root = mcts_create_root(bg, state);
    mcts_search(root, bg, num_iterations);
    mcts_best_action(root, action_out, bg);
}

int mcts_bg_mem_estimate(
    int max_iterations, int max_actions, int action_size, int state_size)
{
    int total = 0;
    // Arena 控制块本身
    total += sizeof(Arena);
    // 节点内存
    int max_nodes = max_iterations + 1;
    int node_mem = max_nodes * (sizeof(MCTSNode) + action_size);
    total += node_mem;
    // 临时缓冲区
    int temp_mem = max_actions * (action_size + sizeof(int));
    total += temp_mem;
    // 节点内存池
    total += sizeof_pool(state_size, max_nodes);
    int untried_block_size = sizeof(void *) + action_size;
    int max_untried_nodes = max_iterations * max_actions;
    total += sizeof_pool(untried_block_size, max_untried_nodes);
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
    float exploration_constant,
    int max_iterations,
    void *buf, int buf_size, MCTSBackground *out)
{
    Arena *arena = prepare_arena(buf_size, buf);
    out->action_buffer = arena_alloc(arena, max_actions * action_size);
    out->index_buffer = (int *)arena_alloc(arena, max_actions * sizeof(int));
    out->state_pool = prepare_pool(
        state_size, max_iterations + 1,
        arena_alloc(arena, sizeof_pool(state_size, max_iterations + 1)));
    out->untried_pool = prepare_pool(
        sizeof(void *) + action_size, max_iterations * max_actions,
        arena_alloc(
            arena, sizeof_pool((sizeof(void *) + action_size),
                               max_iterations * max_actions)));
    out->get_legal_actions = get_legal_actions;
    out->apply_action = apply_action;
    out->is_terminal = is_terminal;
    out->action_size = action_size;
    out->use_policy = use_policy;
    out->policy_sample = policy_sample;
    out->evaluate = evaluate;
    out->exploration_constant = exploration_constant;
    out->max_actions = max_actions;

    out->total_arena = arena;
}