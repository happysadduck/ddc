#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mcts.h"
#include "mcts_helper.h"
#include "pool.h"

/*TODO: 给mcts_recommend添加调试信息打印*/
/*TODO: 类似horse_chess和sp井字棋游戏, 最大步数和通常情况下的最大步数严重不一致, 导致空间浪费*/

static MCTSNode *mcts_create_root(MCTSBackground *bg, void *state)
{
    void *new_actions = arena_alloc(bg->total_arena, bg->action_size * bg->max_actions);
    int num_actions = bg->get_legal_actions(state, new_actions);
    float *action_weights = arena_alloc(bg->total_arena, sizeof(float) * bg->max_actions);
    bg->policy(state, action_weights);
    if (num_actions < 0)
        return NULL;
    return node_create(bg, state, NULL, NULL, new_actions, action_weights, num_actions);
}

static void mcts_search(MCTSNode *root, MCTSBackground *bg, int num_iterations)
{
    for (int i = 0; i < num_iterations; ++i)
    {
        MCTSNode *leaf = mcts_select(root, bg->exploration_constant, bg->is_terminal);
        if (!bg->is_terminal(leaf->state))
        {
            leaf = mcts_expand(leaf, bg);
            float value = bg->evaluate(leaf->state);
            mcts_backup(leaf, value);
        }
        else
            mcts_backup(leaf, 1.0f);
    }
}

static void *mcts_best_action(MCTSNode *root)
{
    MCTSNode *best_child = NULL;
    int max_visits = -1;

    for (MCTSNode *child = root->children; child; child = child->next)
    {
        printf("%d ", child->visits);
        if (child->visits > max_visits)
        {
            max_visits = child->visits;
            best_child = child;
        }
    }
    printf("\nbest: %d\n", best_child->visits);
    printf("total: %d\n\n", root->visits);
    if (best_child)
        return best_child->action;
    return NULL;
}

void mcts_recommend(void *state, MCTSBackground *bg, int num_iterations, void *out)
{
    void *action_out;
    MCTSNode *root = mcts_create_root(bg, state);
    mcts_search(root, bg, num_iterations);
    action_out = mcts_best_action(root);
    memcpy(out, action_out, bg->action_size);
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
    int actions_mem = max_nodes * max_actions * (action_size + sizeof(float)); // 假设每个节点都全都扩展, 非常保守
    total += actions_mem;
    // 由于所有节点自身的action实际上都来自于父节点的untried_action数组, 因此无需分配内存.
    return total;
}

void make_mcts_bg(
    int (*get_legal_actions)(void *state, void *actions_buffer),
    void (*apply_action)(void *state, void *action),
    int (*is_terminal)(void *state),
    int action_size,
    int state_size,
    int max_actions,
    void (*policy)(void *state, float *weights),
    float (*evaluate)(void *state),
    float exploration_constant,
    void *buf, int buf_size, MCTSBackground *out)
{
    Arena *arena = prepare_arena(buf_size - sizeof(Arena), buf);
    out->get_legal_actions = get_legal_actions;
    out->apply_action = apply_action;
    out->is_terminal = is_terminal;
    out->action_size = action_size;
    out->state_size = state_size;
    out->policy = policy;
    out->evaluate = evaluate;
    out->exploration_constant = exploration_constant;
    out->max_actions = max_actions;

    out->total_arena = arena;
}
