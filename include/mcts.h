#ifndef MCTS_H
#define MCTS_H

typedef struct MCTSNode
{
    void *action;
    void *untried_actions;
    void *state;
    struct MCTSNode *children;
    struct MCTSNode *parent;
    struct MCTSNode *next;

    int visit_cnt;
    float total_value;
    float prior_prob;
    int player_id;
} MCTSNode;

#endif