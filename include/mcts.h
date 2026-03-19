#ifndef MCTS_H
#define MCTS_H

#include "pool.h"

typedef struct MCTSBackground
{
    int action_size;    // action类的大小
    int state_size;     // state类的大小
    int data_size;      // 数据区大小(各种内存池, 临时缓冲区, arena的剩余空间都是节点)
    Pool *state_pool;   // 状态内存池
    Pool *untried_pool; // action类的池
    Arena *total_arena; // 总区域分配器

    // 规则函数和评估函数
    int (*get_legal_actions)(void *state,
                             void *actions_buffer,
                             int buffer_capacity);    // 返回合法动作数量, 错误则返回负数错误码
    void *(*apply_action)(void *state, void *action); // 返回新状态（从 state_pool 分配）
    int (*is_terminal)(void *state);                  // 是否终局
    float (*evaluate)(void *state);                   // 神经网络评估

    // 策略开关与函数
    int use_policy;                                       // 0: 随机 rollout, 1: 使用策略网络
    void (*policy_sample)(void *state, void *action_out); // 策略采样，将动作写入 action_out

    // 临时缓冲区（由外部提供，保证足够大）
    void *action_buffer; // 大小至少为 max_actions * action_size
    int *index_buffer;   // 大小至少为 max_actions
    int max_actions;     // 缓冲区容量

    float exploration_constant; // MCTS 参数: 探索常数
} MCTSBackground;

typedef struct MCTSNode
{
    void *state;               // 当前状态（外部管理）
    struct MCTSNode *parent;   // 父节点（若为根则为NULL）
    int visits;                // 访问次数
    float total_value;         // 累计价值（从当前玩家视角）
    struct MCTSNode *children; // 子节点链表的头
    struct MCTSNode *next;     // 侵入式链表
    void *untried_actions;     // 尚未扩展的合法动作链表头
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