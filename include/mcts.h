#ifndef MCTS_H
#define MCTS_H

#include "pool.h"

typedef struct MCTSBackground
{
    int action_size; // action类的大小
    int state_size;  // state类的大小
    int max_actions; // 一个局面最多可能的动作数量

    Arena *total_arena; // 总区域分配器

    // 规则函数和评估函数
    int (*get_legal_actions)(void *state, void *actions_buffer); // 返回合法动作数量
    void (*apply_action)(void *state, void *action);             // 根据动作更新棋盘
    int (*is_terminal)(void *state);                             // 是否终局, 要求同局面下没有合法动作必须要终止
    float (*evaluate)(void *state);                              // 神经网络评估
    void (*policy)(void *state, float *action_weights);          // 策略采样, 将概率写进action_weights
    // 要求同state下, get_legal_actions得到的动作和policy得到的概率必须一一对应

    float exploration_constant; // MCTS 参数: 探索常数
} MCTSBackground;

typedef struct MCTSNode
{
    void *state;               // 当前状态(外部管理)
    struct MCTSNode *parent;   // 父节点(若为根则为NULL)
    int visits;                // 访问次数
    float total_value;         // 累计价值(从当前玩家视角)
    struct MCTSNode *children; // 子节点链表的头
    struct MCTSNode *next;     // 侵入式链表, 指向下一个同辈child
    void *untried_actions;     // 尚未扩展的合法动作列表
    float *action_weights;     // 评估得到的每个动作的权重, 越大的权重选中概率越大
    void *action;              // 父节点到此节点所进行的动作
    int num_untried;           // 剩余未尝试动作数
} MCTSNode;

int mcts_bg_mem_estimate(
    int max_iterations, int max_actions, int action_size, int state_size);
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
    void *buf, int buf_size, MCTSBackground *out);
void mcts_recommend(void *state, MCTSBackground *bg, int num_iterations, void *out);

#endif
