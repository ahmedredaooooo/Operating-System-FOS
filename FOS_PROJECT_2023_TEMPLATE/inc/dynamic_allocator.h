#ifndef FOS_INC_DYNBLK_MANAGE_H
#define FOS_INC_DYNBLK_MANAGE_H
#include <inc/queue.h>
#include <inc/types.h>
#include <inc/environment_definitions.h>

/*Data*/
/*Max Size for the Dynamic Allocator*/
#define DYN_ALLOC_MAX_SIZE (32<<20) //32MB
#define DYN_ALLOC_MAX_BLOCK_SIZE (1<<11) //2KB

/*Allocation Type*/
enum
{
	DA_FF = 1,
	DA_NF,
	DA_BF,
	DA_WF
};

LIST_HEAD(MemBlock_LIST, BlockMetaData);

struct BlockMetaData
{
	uint32 size;		//block size (including size of its meta data)
	uint8 is_free;		//is_free block?
	LIST_ENTRY(BlockMetaData) prev_next_info;	/* linked list links */
};

#define sizeOfMetaData() (sizeof(struct BlockMetaData))


/*Functions*/

/*2024*/
//should be implemented inside kern/mem/kheap.c
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit);
//should be implemented inside kern/proc/user_environment.c
void initialize_uheap_dynamic_allocator(struct Env* env, uint32 daStart, uint32 daLimit);

//TODO: [PROJECT'23.MS1 - #0 GIVENS] DYNAMIC ALLOCATOR helper functions
uint32 get_block_size(void* va);
int8 is_free_block(void* va);
void print_blocks_list(struct MemBlock_LIST list);
//===================================================================

//Required Functions
//In KernelHeap: should be implemented inside kern/mem/kheap.c
//In UserHeap: should be implemented inside lib/uheap.c
void* sbrk(int increment);

void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace);
void *alloc_block(uint32 size, int ALLOC_STRATEGY);
void *alloc_block_FF(uint32 size);
void *alloc_block_BF(uint32 size);
void *alloc_block_WF(uint32 size);
void *alloc_block_NF(uint32 size);
void free_block(void* va);
void *realloc_block_FF(void* va, uint32 new_size);

#endif
