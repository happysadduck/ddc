#ifndef MCTS_H
#define MCTS_H

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

#endif