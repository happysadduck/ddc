#ifndef MCTS_H
#define MCTS_H

#include "pool.h"

typedef struct MCTSBackground
{
    int action_size; // action类的大小
    int state_size;  // state类的大小
    int max_actions; // 一个局面最多可能的动作数量

    int data_size;      // 数据区大小(各种内存池, 临时缓冲区, arena的剩余空间都是节点)
    Arena *total_arena; // 总区域分配器

    // 规则函数和评估函数
    int (*get_legal_actions)(void *state,
                             void *actions_buffer,
                             int buffer_capacity);   // 返回合法动作数量, 错误则返回负数错误码, 合法动作直接写进actions_buffer里
    void (*apply_action)(void *state, void *action); // 根据动作更新棋盘
    int (*is_terminal)(void *state);                 // 是否终局
    float (*evaluate)(void *state);                  // 神经网络评估
    float (*rollout)(void *state);                   // 没有神经网络? 那就随机模拟

    // 策略开关与函数
    int use_policy;                                       // 0: 随机 rollout, 1: 使用策略网络
    void (*policy_sample)(void *state, void *action_out); // 策略采样，将动作写入 action_out

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
    void *action;              // 父节点到此节点所进行的动作
    int num_untried;           // 剩余未尝试动作数
} MCTSNode;

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
    void *buf, int buf_size, MCTSBackground *out);

#endif