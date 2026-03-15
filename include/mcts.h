#ifndef MCTS_H
#define MCTS_H

typedef struct MCTSBackground
{
    size_t action_size;

    // 状态内存池
    Pool *state_pool;

    // 核心函数
    int (*get_legal_actions)(void *state, void *actions_buffer, int buffer_capacity);
    void *(*apply_action)(void *state, void *action); // 返回新状态（从 state_pool 分配）
    int (*is_terminal)(void *state);
    float (*evaluate)(void *state); // 神经网络评估

    // 策略开关与函数
    int use_policy;                                       // 0: 随机 rollout, 1: 使用策略网络
    void (*policy_sample)(void *state, void *action_out); // 策略采样，将动作写入 action_out

    // 临时缓冲区（由外部提供，保证足够大）
    void *action_buffer; // 大小至少为 max_actions * action_size
    int *index_buffer;   // 大小至少为 max_actions
    int max_actions;     // 缓冲区容量

    // MCTS 参数
    float exploration_constant;
} MCTSBackground;

typedef struct MCTSNode
{
    void *state;                // 当前状态（外部管理）
    struct MCTSNode *parent;    // 父节点（若为根则为NULL）
    int visits;                 // 访问次数
    float total_value;          // 累计价值（从当前玩家视角）
    int num_children;           // 已有子节点数量
    int capacity_children;      // 子节点数组容量
    struct MCTSNode **children; // 子节点指针数组
    void *untried_actions;      // 尚未扩展的合法动作列表
    int num_untried;            // 剩余未尝试动作数
} MCTSNode;

typedef struct UntriedAction
{
    struct UntriedAction *next;
    // 动作数据紧跟在后面，大小为 bg->action_size
} UntriedAction;

#endif