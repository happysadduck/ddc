#include <stddef.h>
#include "pool.h"

int sizeof_arena(int size)
{
	return sizeof(Arena) + size;
}

Arena *prepare_arena(int size, void *data)
{
	Arena *out = (Arena *)data;
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

void arena_cut(Arena *arena, int size)
{
	arena->top -= size;
}

void *start_of_arena(Arena *arena)
{
	return (void *)arena;
}

int sizeof_pool(int size, int cnt)
{
	int actual_size = 1;
	while (actual_size < size)
		actual_size <<= 1;
	size = actual_size;
	return size * cnt + sizeof(Pool) + cnt * sizeof(char *);
}

Pool *prepare_pool(int size, int cnt, void *data)
{
	int actual_size;
	int loop_cnt;
	int data_zone_size;
	char *total_pool = NULL;
	Pool *pool = NULL;
	actual_size = 1;
	while (actual_size < size)
		actual_size <<= 1;
	size = actual_size;
	data_zone_size = size * cnt;
	total_pool = (char *)data;
	pool = (Pool *)(total_pool + data_zone_size);
	pool->block_size = size;
	pool->data = total_pool;
	pool->stack = (char **)((char *)pool + sizeof(Pool));
	pool->stack_top = pool->stack;
	for (loop_cnt = 0; loop_cnt < cnt; loop_cnt++)
		*(pool->stack_top)++ = pool->data + (loop_cnt * pool->block_size);
	pool->stack_top--;
	return pool;
}

void *start_of_pool(Pool *pool)
{
	return (void *)(pool->data);
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

void pool_clear(Pool *pool)
{
	int block_count = ((char *)pool - pool->data) / pool->block_size;
	char **stack_ptr = pool->stack;

	for (int i = 0; i < block_count; i++)
	{
		*stack_ptr++ = pool->data + i * pool->block_size;
	}
	pool->stack_top = stack_ptr - 1;
}