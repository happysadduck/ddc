#ifndef RULES_H
#define RULES_H

void init_state(void *state);
int get_legal_actions(void *state, void *actions_buffer);
void apply_action(void *state, void *action);
int is_terminal(void *state);
float evaluate(void *state);
void policy(void *state, float *action_weights);
void print_board(void *state);

#endif