#include <stdio.h>
#include <stdlib.h>
#include "rules.h"
#include "mcts.h"

int main()
{
    char state[16];
    init_state(state);
    int mcts_bg_size = mcts_bg_mem_estimate(1000, 8, 4, 16);
    char *buf = malloc(mcts_bg_size);
    MCTSBackground bg;
    make_mcts_bg(
        get_legal_actions,
        apply_action, is_terminal,
        4, 16, 8,
        policy,
        evaluate,
        0.414,
        buf,
        mcts_bg_size,
        &bg);
    while (!is_terminal(state))
    {
        print_board(state);
        char action[4];
        mcts_recommend(state, &bg, 1000, action);
        apply_action(state, action);
    }
    return 0;
}