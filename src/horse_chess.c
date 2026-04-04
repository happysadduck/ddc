#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rules.h"

// 棋盘大小
#define BOARD_SIZE 64

// 骑士移动偏移量（8个方向）
static const int knight_moves[8][2] = {
    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};

// 游戏状态结构体
typedef struct
{
    uint64_t board;     // 位图，第 i 位为 1 表示格子 i 已放置马
    int last_pos;       // 上一匹马的位置，-1 表示第一步
    int current_player; // 0 或 1，表示当前轮到谁走
} GameState;

// 全局常量（供外部使用）
const int action_size = sizeof(int); // 动作用一个 int 表示格子索引
const int state_size = sizeof(GameState);
const int max_actions = 64; // 第一步最多 64 个合法动作

// 初始化游戏状态
void init_state(void *state)
{
    GameState *s = (GameState *)state;
    s->board = 0ULL;       // 所有格子空
    s->last_pos = -1;      // 没有上一步
    s->current_player = 0; // 0 表示先手玩家
}

// 辅助函数：检查坐标是否在棋盘内
static inline int is_in_board(int x, int y)
{
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

// 辅助函数：获取从给定格子出发的所有合法骑士跳目标（不检查占用）
static int get_knight_targets(int pos, int targets[8])
{
    int x = pos % 8;
    int y = pos / 8;
    int count = 0;
    for (int i = 0; i < 8; ++i)
    {
        int nx = x + knight_moves[i][0];
        int ny = y + knight_moves[i][1];
        if (is_in_board(nx, ny))
        {
            targets[count++] = ny * 8 + nx;
        }
    }
    return count;
}

// 获取合法动作，存入 actions_buffer，返回合法动作数量
int get_legal_actions(void *state, void *actions_buffer)
{
    GameState *s = (GameState *)state;
    int *actions = (int *)actions_buffer;
    int count = 0;

    if (s->last_pos == -1)
    { // 第一步：所有空位均合法
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (!(s->board & (1ULL << i)))
            {
                actions[count++] = i;
            }
        }
    }
    else
    { // 后续步：只能从上一步的马跳到空位
        int targets[8];
        int num_targets = get_knight_targets(s->last_pos, targets);
        for (int i = 0; i < num_targets; ++i)
        {
            int t = targets[i];
            if (!(s->board & (1ULL << t)))
            {
                actions[count++] = t;
            }
        }
    }
    return count;
}

// 应用动作，更新状态
void apply_action(void *state, void *action)
{
    GameState *s = (GameState *)state;
    int pos = *(int *)action;

    // 放置马
    s->board |= (1ULL << pos);
    s->last_pos = pos;
    // 切换玩家
    s->current_player ^= 1;
}

// 判断是否终局（当前玩家无合法动作）
int is_terminal(void *state)
{
    int dummy[64];
    return get_legal_actions(state, dummy) == 0;
}

// 随机模拟一次，返回当前玩家是否获胜
static int simulate_one(GameState *state)
{
    GameState sim = *state; // 复制状态
    while (!is_terminal(&sim))
    {
        int actions[64];
        int n = get_legal_actions(&sim, actions);
        if (n == 0)
            break;
        // 随机选择一个合法动作
        int idx = rand() % n;
        apply_action(&sim, &actions[idx]);
    }
    // 游戏结束时的状态：当前玩家无步可走，所以当前玩家输，对手赢
    // 因此如果原始玩家的对手是当前玩家，则原始玩家赢
    // 原始玩家 = state->current_player
    // 当前玩家 = sim.current_player
    // 如果 sim.current_player != state->current_player，说明最后一步是原始玩家走的，对手无步可走，原始玩家赢
    return (sim.current_player != state->current_player) ? 1 : 0;
}

// 评估函数：随机模拟 100 次，返回当前玩家获胜的评分（-1.0 ~ 1.0）, 越大越赢
float evaluate(void *state)
{
    if (is_terminal(state))
        return -1.0f;
    const int SIMULATIONS = 100;
    int wins = 0;
    for (int i = 0; i < SIMULATIONS; ++i)
    {
        if (simulate_one((GameState *)state))
            wins++;
    }
    return ((float)wins / (float)SIMULATIONS - 0.5) * 2.0;
}

// 策略函数：将动作权重清零（占位实现）
void policy(void *state, float *action_weights)
{
    (void)state; // 避免未使用参数警告
    memset(action_weights, 0, max_actions * sizeof(float));
}

// 打印棋盘（带坐标轴）
void print_board(void *state)
{
    GameState *s = (GameState *)state;
    printf("turn: %d\n", s->current_player);
    // 打印列标 (a-h)
    printf("  ");
    for (char col = 'a'; col <= 'h'; ++col)
    {
        printf("%c ", col);
    }
    printf("\n");

    for (int y = 7; y >= 0; --y)
    {                         // 行从上到下打印，y=7为第1行（顶部）
        printf("%d ", y + 1); // 行号 1-8
        for (int x = 0; x < 8; ++x)
        {
            int pos = y * 8 + x;
            if (s->board & (1ULL << pos))
            {
                if (pos == s->last_pos)
                {
                    printf("C "); // 当前马
                }
                else
                {
                    printf("X "); // 历史马
                }
            }
            else
            {
                printf(". ");
            }
        }
        printf("\n");
    }
    printf("\n");
}