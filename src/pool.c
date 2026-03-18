#include <stdlib.h>
#include <string.h>
#include "pool.h"

Arena *prepare_Arena(int size)
{
	Arena *out = (Arena *)malloc(sizeof(Arena) + size);
	out->data = (char *)(out + 1);
	out->max_size = size;
	out->top = out->data;
	return out;
}

void *arena_alloc(Arena *arena, int size)
{
	char *out = arena->top;
	if (arena->top - arena->data + size > arena->max_size)
		return NULL;
	arena->top += size;
	return (void *)out;
}

void arena_clear(Arena *arena)
{
	arena->top = arena->data;
}

void arena_clear_to(Arena *arena, int size)
{
	arena->top = arena->data + size;
}

void destroy_arena(Arena *arena)
{
	free(arena);
}

static prepare_pool_general(int size, int cnt, Arena *arena, int is_from_arena)
{
	int actual_size;
	int loop_cnt;
	int data_zone_size;
	int total_size;
	char *total_pool = NULL;
	Pool *pool = NULL;
	actual_size = 1;
	loop_cnt = 0;
	while (actual_size < size &&
		   loop_cnt < 10000)
	{
		loop_cnt++;
		actual_size <<= 1;
	}
	size = actual_size;
	data_zone_size = size * cnt;
	total_size = data_zone_size + sizeof(Pool) + cnt * sizeof(char *);
	if (is_from_arena)
		total_pool = (char *)arena_alloc(arena, total_size);
	else
		total_pool = (char *)malloc(total_size);
	if (!total_pool)
		return NULL;
	memset(total_pool, 0, total_size);
	pool = (Pool *)(total_pool + data_zone_size);
	pool->block_size = size;
	pool->data = total_pool;
	pool->stack = (char **)((char *)pool + sizeof(Pool));
	pool->stack_top = pool->stack;
	for (loop_cnt = 0; loop_cnt < cnt; loop_cnt++)
		*(pool->stack_top)++ = pool->data + (loop_cnt * pool->block_size);
	pool->stack_top--;
	total_pool = NULL;
	return pool;
}

Pool *prepare_pool_from_arena(Arena *arena, int size, int cnt)
{
	return prepare_pool_general(size, cnt, arena, 1);
}

Pool *
prepare_pool(int size, int cnt)
{
	return prepare_pool_general(size, cnt, NULL, 0);
}

void destroy_pool(Pool *pool)
{
	if (!pool)
		return;
	if (!pool->data)
		return;
	free(pool->data); /*TODO!*/
}

void *pool_alloc(Pool *pool)
{
	if (pool->stack_top < pool->stack)
		return NULL;
	return *(pool->stack_top)--;
}

int pool_return(Pool *pool,
				void *ptr)
{
	if ((char *)ptr < pool->data ||
		(char *)ptr >= (char *)pool)
		return 0;
	if (((char *)ptr - pool->data) &
		(pool->block_size - 1))
		return 0;
	pool->stack_top++;
	*pool->stack_top = ptr;
	return 1;
}
