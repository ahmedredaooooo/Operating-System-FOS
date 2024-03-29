/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================


/*
void print_blocks_list(struct MemBlock_LIST list)
{
        cprintf("=========================================\n");
        struct BlockMetaData* blk ;
        cprintf("\nDynAlloc Blocks List:\n");
        LIST_FOREACH(blk, &list)
        {
                cprintf("(va: %x, size: %d, isFree: %d)\n", blk, blk->size, blk->is_free) ;
        }
        cprintf("sbrk -> %x\n=========================================\n", sbrk(0));

}

 */

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}

//
////********************************************************************************//
////********************************************************************************//

// OUR HELPER FUNCTIONS
void *alloc_block_at(void* va, uint32 size)
{
	struct BlockMetaData* cur = (struct BlockMetaData*)va - 1;

	// check if the capacity is enough
	if (cur->size - sizeOfMetaData() < size)
		return NULL;

	// add Meta-data after block if there's enough capacity
	// else take the whole capacity for the program
	if (cur->size - size >= 2 * sizeOfMetaData()) // split
	{
		struct BlockMetaData* next = (struct BlockMetaData *)(size + sizeOfMetaData() + (uint32)cur);
/*		void* roundedNext = ROUNDUP((void*)next, PAGE_SIZE);
		uint32 diff = (roundedNext - (void*)next), extra = 0;
		if (diff > 0 && diff < sizeOfMetaData())
			extra = diff, next = roundedNext;*/
		next->is_free = 1;
		next->size = cur->size - (size + sizeOfMetaData());
		LIST_INSERT_AFTER(&free_mem_block_list, cur, next);
		cur->size = size + sizeOfMetaData(); // split
	}
	LIST_REMOVE(&free_mem_block_list, cur);
	cur->is_free = 0;
	return va;
}

void *expand_mem(uint32 requiredFitSize)
{
	struct BlockMetaData *next;
    uint32 ret;
    {
        ret=(uint32)sbrk(requiredFitSize+sizeOfMetaData());
    }

	if (!~ret)
		return NULL;
    {
        next = (struct BlockMetaData *)ret;
        next->is_free = 1;
        next->size = (uint32)sbrk(0) - ret;
        LIST_INSERT_TAIL(&free_mem_block_list, next);
        //free_block(next + 1); //after commenting if-else
    }

    return alloc_block_at(next + 1, requiredFitSize);
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized = 1;
	//=========================================
	//=========================================
	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//panic("initialize_dynamic_allocator is not implemented yet");

	struct BlockMetaData *firstBlock = (struct BlockMetaData *)daStart;

	firstBlock->is_free=1;
	firstBlock->size=initSizeOfAllocatedSpace;

	LIST_INIT(&free_mem_block_list);
	LIST_INSERT_HEAD(&free_mem_block_list, firstBlock);
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void* alloc_block_FF(uint32 size)
{
    //TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
    //panic("alloc_block_FF is not implemented yet");
    if(size==0)
        return NULL;

    if (!is_initialized)
    {
        uint32 required_size = size + sizeOfMetaData();
        uint32 da_start = (uint32)sbrk(required_size);
        //get new break since it's page aligned! thus, the size can be more than the required one
        uint32 da_break = (uint32)sbrk(0);
        initialize_dynamic_allocator(da_start, da_break - da_start);
    }

    struct BlockMetaData *next;
    int last_block_size;
    struct BlockMetaData *ptr;

    LIST_FOREACH(ptr, &free_mem_block_list)
    	if (size <= ptr->size - sizeOfMetaData())
    		return alloc_block_at(ptr + 1, size);

    uint32 ret;
/*    if(LIST_LAST(&mem_block_list)->is_free==1) // list in empty NULL access
    {
        ret=(uint32)sbrk(size+sizeOfMetaData()-LIST_LAST(&mem_block_list)->size);
    }
    else*/
    {
        ret=(uint32)sbrk(size+sizeOfMetaData());
    }

	if (!~ret)
		return NULL;

/*    if(LIST_LAST(&mem_block_list)->is_free==1)
    {
        LIST_LAST(&mem_block_list)->size += (uint32)sbrk(0) - ret;
    }
    else*/
    {
        next = (struct BlockMetaData *)ret;
        next->is_free = 1;
        next->size = (uint32)sbrk(0) - ret;
        LIST_INSERT_TAIL(&free_mem_block_list, next);
        //free_block(next + 1); //after commenting if-else
    }

    return alloc_block_at(next + 1,size);
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	//panic("alloc_block_BF is not implemented yet");

	if (!size)
		return NULL;
	// Init the BestFitSize with Large Value to minimize later
	uint32 bestFitSize = -1ul;
	struct BlockMetaData* bestFitaddress = NULL, *it;

	LIST_FOREACH(it, &free_mem_block_list)            // find the smallest Empty-Block >= the program Program-Size     Note::(if there is multiple blocks with the same Size we take the first on)
		if (size <= it->size - sizeOfMetaData() && bestFitSize > it->size - sizeOfMetaData())
			bestFitSize = it->size - sizeOfMetaData(), bestFitaddress = it + 1;

	// if a Block is found we alloc_block_at();
	if (bestFitaddress)
		return alloc_block_at(bestFitaddress, size);
	return expand_mem(size); // if cannot expand it returns NULL
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");
	if(va==NULL)
		return;



	struct BlockMetaData *ptr = NULL,*removedBlock;
	uint32 address=(uint32)va-sizeOfMetaData();
	removedBlock=(struct BlockMetaData *)address;
	if (removedBlock->is_free)
		return;
	removedBlock->is_free=1;

	LIST_FOREACH(ptr, &free_mem_block_list)
		if (ptr > removedBlock)
			break;
	if (!ptr)
		LIST_INSERT_TAIL(&free_mem_block_list, removedBlock);
	else
		LIST_INSERT_BEFORE(&free_mem_block_list, ptr, removedBlock);


	struct BlockMetaData* next_free = LIST_NEXT(removedBlock), *prev_free = LIST_PREV(removedBlock);
	if(next_free && (uint32)removedBlock + removedBlock->size == (uint32)next_free)
	{
		removedBlock->size+=LIST_NEXT(removedBlock)->size;
		LIST_NEXT(removedBlock)->size=0;
		LIST_NEXT(removedBlock)->is_free=0;
		struct BlockMetaData *x=LIST_NEXT(removedBlock);
		LIST_REMOVE(&free_mem_block_list,x);
	}
	if(prev_free && (uint32)prev_free + prev_free->size == (uint32)removedBlock)
	{
		LIST_PREV(removedBlock)->is_free=1;
		LIST_PREV(removedBlock)->size+=removedBlock->size;
		removedBlock->is_free=0;
		removedBlock->size=0;
		LIST_REMOVE(&free_mem_block_list,removedBlock);
	}

}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");

	// SPECIAL CASES
	if (!va)
		return alloc_block_FF(new_size);
	if (!new_size)
		return free_block(va), NULL;

	struct BlockMetaData* cur = (struct BlockMetaData*) va - 1,* ptr = NULL;
	uint32 old_size = cur->size - sizeOfMetaData();
	cur->is_free = 1;

	LIST_FOREACH(ptr, &free_mem_block_list)
		if (ptr > cur)
			break;
	if (!ptr)
		LIST_INSERT_TAIL(&free_mem_block_list, cur);
	else
		LIST_INSERT_BEFORE(&free_mem_block_list, ptr, cur);

	// IF THE FOLLOWING META-BLOCK EMPTY
	// THEN free_block(NEXT(CURRENT))
	struct BlockMetaData* next = LIST_NEXT(cur);
	if (next && (uint32)cur + cur->size == (uint32)next)
	{
		next->is_free = 0;
		LIST_REMOVE(&free_mem_block_list, next);
		free_block(next + 1);
	}

	// CHECK SIZE OF META-BLOCK @ (va) IF ENOUGH
	// THEN alloc_block_at(va, new_size)
	// ELSE THEN alloc_block_FF(new_size)
	void* ret = alloc_block_at(va, new_size);
	if (ret)
		return ret;

	// may be prev is free so try to merge
	ptr = LIST_PREV(cur);
	cur->is_free = 0;
	LIST_REMOVE(&free_mem_block_list, cur);
	free_block(cur + 1);

	ret = alloc_block_FF(new_size);
	// if allocff can't alloc this size may be can't move sbrk or we reached hard_limit
	// then act as if it's not been called
	if (!ret)
	{
		// if the prev block was free then it's merged,
		// we have to undo this merge again by editting the size
		struct BlockMetaData* last_free = ptr;
		if ((uint32)last_free + last_free->size > (uint32)cur)
			last_free->size = cur - last_free;

		return alloc_block_at(va, old_size);
	}
	// copy prog to new address
	memcpy(ret, va, old_size);
	return ret;
}
