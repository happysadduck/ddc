# include"pool.h"

struct Pool{
	char*data;
	char**stack;
	char**stack_top;
	int block_size;
};

Pool*prepare_pool(
				  int size,
				  int cnt){
	int actual_size;
	int loop_cnt;
	int data_zone_size;
	int total_size;
	char*total_pool=NULL;
	Pool*pool=NULL;
	actual_size=1;
	loop_cnt=0;
	while(actual_size<size &&
		  loop_cnt<10000){
		loop_cnt++;
		actual_size<<=1;
	}size=actual_size;
	data_zone_size=size*cnt;
	total_size=data_zone_size+
	sizeof(Pool)+cnt*sizeof(char*);
	total_pool=(char*)malloc(total_size);
	if(!total_pool)
		return NULL;
	memset(total_pool, 0, total_size);
	pool=(Pool*)
	(total_pool+data_zone_size);
	pool->block_size=size;
	pool->data=total_pool;
	pool->stack=(char**)
	((char*)pool+sizeof(Pool));
	pool->stack_top=pool->stack;
	for(loop_cnt=0; 
		loop_cnt<cnt; 
		loop_cnt++){
		*(pool->stack_top)++
		=pool->data+
		(loop_cnt*pool->block_size);
	}pool->stack_top--;
	total_pool=NULL;
	return pool;
}

void destroy_pool(Pool*pool){
	if(!pool)
		return;
	if(!pool->data)
		return;
	free(pool->data);
}

void*pool_alloc(Pool*pool){
	if(pool->stack_top<pool->stack)
		return NULL;
	return *(pool->stack_top)--;
}

int pool_return(Pool*pool,
				 void*ptr){
	if((char*)ptr<pool->data || 
	   (char*)ptr>=(char*)pool)
		return 0;
	if(((char*)ptr-pool->data)&
		(pool->block_size-1))
		return 0;
	pool->stack_top++;
	*pool->stack_top=ptr;
	return 1;
}
