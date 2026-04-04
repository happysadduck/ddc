#include <stdio.h>
#include <stdlib.h>
#include "rules.h"
#include "mcts.h"

int main()
{
    const int max_iterations = 2000;
    const int max_actions = 64;
    const int action_size = 4;
    const int state_size = 16;
    char state[16];
    init_state(state);
    int mcts_bg_size = mcts_bg_mem_estimate(max_iterations, max_actions, action_size, state_size);
    char *buf = malloc(mcts_bg_size);
    MCTSBackground bg;
    make_mcts_bg(
        get_legal_actions,
        apply_action, is_terminal,
        action_size, state_size, max_actions,
        policy,
        evaluate,
        0.707,
        buf,
        mcts_bg_size,
        &bg);
    print_board(state);
    while (!is_terminal(state))
    {
        char action[4];
        mcts_recommend(state, &bg, max_iterations, action);
        apply_action(state, action);
        print_board(state);
    }
    return 0;
}