#ifndef POOL_H
#define POOL_H

typedef struct
{
    char *data;
    char *top;
    int max_size;
} Arena;

typedef struct
{
    char *data;
    char **stack;
    char **stack_top;
    int block_size;
} Pool;

Arena *prepare_Arena(int size);
void *arena_alloc(Arena *arena, int size);
void arena_clear(Arena *arena);
void destroy_arena(Arena *arena);
Pool *prepare_pool_from_arena(Arena *mem_src, int size, int cnt);
Pool *prepare_pool(int size, int cnt);
void destroy_pool(Pool *pool);
void *pool_alloc(Pool *pool);
int pool_return(Pool *pool, void *ptr);

#endif
