#include <inc/lib.h>

struct BuddyNode FreeNodes[BUDDY_NUM_FREE_NODES];

/* This file is dedicated for implementing the BUDDY system
 * inside the user heap
 */

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// [1] CLEAR NODE DATA:
//===========================
void ClearNodeData(struct BuddyNode* node)
{
	node->level = 0;
	node->status = FREE;
	node->va = 0;
	node->parent = NULL;
	node->myBuddy = NULL;
}

//===========================
// [2] INITIALIZE FREE LIST:
//===========================
void initialize_buddy()
{
	for (int i = 0; i < BUDDY_NUM_FREE_NODES; ++i)
	{
		LIST_INSERT_HEAD(&BuddyFreeNodesList, &(FreeNodes[i]));
		ClearNodeData(&(FreeNodes[i]));
	}
}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=========================
// [1] FIND ALLOCATION:
//=========================
void* FindAllocationUsingBuddy(int size)
{
	panic("not implemented");
	return NULL;
}

//============================
// [2] CREATE NEW BUDDY SPACE:
//============================
void CreateNewBuddySpace()
{
	panic("not implemented");
}


//=========================
// [3] FREE ALLOCATION:
//=========================
void FreeAllocationUsingBuddy(uint32 va)
{
	panic("not implemented");
}
