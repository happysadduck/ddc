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

int sizeof_arena(int size);
Arena *prepare_arena(int size, void *data);
void *arena_alloc(Arena *arena, int size);
void arena_clear(Arena *arena);
void arena_clear_to(Arena *arena, int size);
void *start_of_arena(Arena *arena);
int sizeof_pool(int size, int cnt);
Pool *prepare_pool(int size, int cnt, void *data);
void *start_of_pool(Pool *pool);
void *pool_alloc(Pool *pool);
int pool_return(Pool *pool, void *ptr);

#endif
