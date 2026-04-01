#ifndef MCTS_HELPER_H
#define MCTS_HELPER_H

#include "mcts.h"

MCTSNode *node_create(const MCTSBackground *bg,
                      void *state, MCTSNode *parent, void *action,
                      void *legal_actions, int num_actions);
MCTSNode *mcts_select(MCTSNode *start, float exploration, int (*is_terminal)(void *));
MCTSNode *mcts_expand(MCTSNode *leaf, const MCTSBackground *bg);
void mcts_backup(MCTSNode *leaf, float value);
void *mcts_best_action(MCTSNode *root);

#endif