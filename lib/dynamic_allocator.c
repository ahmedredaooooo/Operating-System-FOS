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
		next->is_free = 1;
		next->size = cur->size - (size + sizeOfMetaData());
		LIST_INSERT_AFTER(&mem_block_list, cur, next);
		cur->size = size + sizeOfMetaData(); // split
	}

	cur->is_free = 0;
	return va;
}

void *expand_mem(uint32 requiredFitSize)
{
	struct BlockMetaData* tail = LIST_LAST(&mem_block_list);

	if(!~(int)(sbrk(requiredFitSize + sizeOfMetaData() - (uint32)(tail->is_free) * tail->size)))
		return NULL;

	if(tail->is_free)
		tail->is_free = 0,
		tail->size = requiredFitSize + sizeOfMetaData();
	else
	{
		struct BlockMetaData *next = (struct BlockMetaData *)((uint32)LIST_LAST(&mem_block_list)+LIST_LAST(&mem_block_list)->size);
		next->size = requiredFitSize + sizeOfMetaData();
		LIST_INSERT_TAIL(&mem_block_list, next);
	}
	return (void *)(tail + 1);
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

	LIST_INIT(&mem_block_list);
	LIST_INSERT_HEAD(&mem_block_list, firstBlock);

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

    LIST_FOREACH(ptr,&mem_block_list)
    {
        uint32 x=(size+sizeOfMetaData());
        if(ptr->is_free==1 && (uint32)x<=(uint32)ptr->size)
            return alloc_block_at(ptr + 1, size);
    }

    uint32 ret;
    if(LIST_LAST(&mem_block_list)->is_free==1) // list in empty NULL access
    {
        ret=(uint32)sbrk(size+sizeOfMetaData()-LIST_LAST(&mem_block_list)->size);
    }
    else
    {
        ret=(uint32)sbrk(size+sizeOfMetaData());
    }

	if (!~ret)
		return NULL;

    if(LIST_LAST(&mem_block_list)->is_free==1)
    {
        LIST_LAST(&mem_block_list)->size += (uint32)sbrk(0) - ret;
    }
    else
    {
        next=(struct BlockMetaData *)ret;
        next->is_free = 1;
        next->size = (uint32)sbrk(0) - ret;
        LIST_INSERT_TAIL(&mem_block_list, next);
    }

    return alloc_block_FF(size);
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

	LIST_FOREACH(it, &mem_block_list)            // find the smallest Empty-Block >= the program Program-Size     Note::(if there is multiple blocks with the same Size we take the first on)
		if (it->is_free && size <= it->size - sizeOfMetaData() && bestFitSize > it->size - sizeOfMetaData())
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

	struct BlockMetaData *ptr,*removedBlock;
	uint32 address=(uint32)va-sizeOfMetaData();
	removedBlock=(struct BlockMetaData *)address;
	removedBlock->is_free=1;

	if(LIST_NEXT(removedBlock)!=NULL && LIST_NEXT(removedBlock)->is_free==1)
	{
		removedBlock->size+=LIST_NEXT(removedBlock)->size;
		LIST_NEXT(removedBlock)->size=0;
		LIST_NEXT(removedBlock)->is_free=0;
		struct BlockMetaData *x=LIST_NEXT(removedBlock);
		LIST_REMOVE(&mem_block_list,x);
	}
	if(LIST_PREV(removedBlock)!=NULL && LIST_PREV(removedBlock)->is_free==1 )
	{
		LIST_PREV(removedBlock)->is_free=1;
		LIST_PREV(removedBlock)->size+=removedBlock->size;
		removedBlock->is_free=0;
		removedBlock->size=0;
		LIST_REMOVE(&mem_block_list,removedBlock);
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

	struct BlockMetaData* cur = (struct BlockMetaData*) va - 1;
	cur->is_free = 1;

	// IF THE FOLLOWING META-BLOCK EMPTY
	// THEN free_block(NEXT(CURRENT))
	struct BlockMetaData* next = LIST_NEXT(cur);
	if (next && next->is_free)
		free_block(next + 1);

	// CHECK SIZE OF META-BLOCK @ (va) IF ENOUGH
	// THEN alloc_block_at(va, new_size)
	// ELSE THEN alloc_block_FF(new_size)
	void* ret = alloc_block_at(va, new_size);
	if (!ret)
		return free_block(cur + 1), alloc_block_FF(new_size);
	return ret;
}
