#ifndef MCTS_HELPER_H
#define MCTS_HELPER_H

#include "mcts.h"

typedef struct UntriedAction
{
    struct UntriedAction *next;
    // 动作数据紧跟在后面，大小为 bg->action_size
} UntriedAction;

MCTSNode *node_create(MCTSBackground *bg,
                      void *state, MCTSNode *parent, void *action,
                      const void *legal_actions, int num_actions);
MCTSNode *mcts_select(MCTSNode *start, float exploration, int (*is_terminal)(void *));
MCTSNode *mcts_expand(MCTSNode *leaf, MCTSBackground *bg);
float mcts_evaluate(MCTSNode *node, MCTSBackground *bg);
void mcts_backup(MCTSNode *leaf, float value);
void mcts_best_action(MCTSNode *root, void *action_out, MCTSBackground *bg);

#endif