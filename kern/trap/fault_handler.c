/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

/// OUR HELPER FUNCTION
// this function is called when (AL is FULL and SL is NOT FULL)
void handle_overflow_from_AL_to_SL_and_insert_new_WSE_in_AL(struct Env *e, struct WorkingSetElement *WSE)
{
	struct WorkingSetElement* removedFromAL = LIST_LAST(&(curenv->ActiveList));
	LIST_REMOVE(&(curenv->ActiveList), removedFromAL);
	LIST_INSERT_HEAD(&(curenv->SecondList), removedFromAL);
	pt_set_page_permissions(curenv->env_page_directory, removedFromAL->virtual_address, PERM_SECOND_LIST, PERM_PRESENT);

	LIST_INSERT_HEAD(&(curenv->ActiveList), WSE);
	pt_set_page_permissions(curenv->env_page_directory, WSE->virtual_address, PERM_PRESENT, PERM_SECOND_LIST);
}
//=========================================

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
		fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif




	//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
	//refer to the project presentation and documentation for details
	if(isPageReplacmentAlgorithmFIFO())
	{
		//
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");

		if(wsSize < (curenv->page_WS_max_size))
		{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
			//refer to the project presentation and documentation for details
	//		fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
			struct FrameInfo *ptr_frame_info = NULL;
			bool place_in_mem = 0;

			allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_MARKED);
			if (pf_read_env_page(curenv, (void*)fault_va) == E_PAGE_NOT_EXIST_IN_PF)
			{
				if (((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
					place_in_mem = 1;
			}
			else
				place_in_mem = 1;

			if(place_in_mem)
			{
				struct WorkingSetElement* WSE = env_page_ws_list_create_element(curenv, fault_va);
				//ptr_frame_info->element = WSE;

				//cprintf("\n\n fault_va = %x, wse = %x \n\n", fault_va, WSE);

				LIST_INSERT_TAIL(&(curenv->page_WS_list), WSE);

				uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
				if (wsSize == curenv->page_WS_max_size)
					curenv->page_last_WS_element = (struct WorkingSetElement*) LIST_FIRST(&(curenv->page_WS_list));
				else
					curenv->page_last_WS_element = NULL;
	//			env_page_ws_print(curenv);/////////
			}
			else
				unmap_frame(curenv->env_page_directory, fault_va), sched_kill_env(curenv->env_id);
		}
		else
		{
			uint32 removed_va = curenv->page_last_WS_element->virtual_address;
			struct WorkingSetElement *next = curenv->page_last_WS_element;
			bool is_last = 0;
			if(curenv->page_last_WS_element == LIST_LAST(&(curenv->page_WS_list)))
				is_last = 1, curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			else
				curenv->page_last_WS_element = LIST_NEXT(next);

			uint32 *ptr_page_table;
			int perms = pt_get_page_permissions(curenv->env_page_directory, removed_va);
			struct FrameInfo* modified_page_frame_info = get_frame_info(curenv->env_page_directory, removed_va, &ptr_page_table);

			if(perms & PERM_MODIFIED)
				pf_update_env_page(curenv, removed_va, modified_page_frame_info);

			//modified_page_frame_info->element = next; // O(1) purpose
			fast_env_page_ws_invalidate(curenv, removed_va); // O(1) purpose
			//env_page_ws_invalidate(curenv, removed_va);
			//unmap_frame(curenv->env_page_directory, removed_va);

			//placement code
			fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
			struct FrameInfo *ptr_frame_info = NULL;
			bool place_in_mem = 0;
			allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory,ptr_frame_info,fault_va,PERM_USER|PERM_WRITEABLE|PERM_MARKED);

			if (pf_read_env_page(curenv, (void*)fault_va) == E_PAGE_NOT_EXIST_IN_PF)
			{
				if (((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
					place_in_mem = 1;
			}
			else
				place_in_mem = 1;

			if(place_in_mem)
			{
				struct WorkingSetElement* WSE = env_page_ws_list_create_element(curenv, fault_va);
				//ptr_frame_info->element = WSE;
				if(is_last)
					LIST_INSERT_TAIL(&(curenv->page_WS_list), WSE);
				else
					LIST_INSERT_BEFORE(&(curenv->page_WS_list), curenv->page_last_WS_element,WSE);
			}
			else
				unmap_frame(curenv->env_page_directory, fault_va), sched_kill_env(curenv->env_id);
			//env_page_ws_print(curenv);//sys_check_WS_list
		}

	}

	if (isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
		//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement

		struct WorkingSetElement *WSE, *tmp;
		uint32* ptr_page_table = NULL;
		if (pt_get_page_permissions(curenv->env_page_directory, fault_va) & PERM_SECOND_LIST)
		{
			//env_page_ws_print(curenv);
			tmp = get_frame_info(curenv->env_page_directory, fault_va, &ptr_page_table)->element;
			LIST_REMOVE(&(curenv->SecondList), tmp);
			return handle_overflow_from_AL_to_SL_and_insert_new_WSE_in_AL(curenv, tmp);
		}
//		LIST_FOREACH(tmp, &(curenv->SecondList))
//			if (tmp->virtual_address == fault_va)
//			{
//
//			}

		struct FrameInfo *ptr_frame_info;
		allocate_frame(&ptr_frame_info);
		map_frame(curenv->env_page_directory, ptr_frame_info, fault_va, PERM_USER | PERM_WRITEABLE | PERM_MARKED);

		uint8 page_not_exist_in_pf = pf_read_env_page(curenv, (void *)fault_va) == E_PAGE_NOT_EXIST_IN_PF,
				allowed_VA = ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)),
				place_in_mem = !page_not_exist_in_pf || allowed_VA;

		if (!place_in_mem)
			return unmap_frame(curenv->env_page_directory, fault_va), sched_kill_env(curenv->env_id);

		uint8 ActiveListIsFull = LIST_SIZE(&(curenv->ActiveList)) == curenv->ActiveListSize,
				SecondListIsFull = LIST_SIZE(&(curenv->SecondList)) == curenv->SecondListSize;
		WSE = env_page_ws_list_create_element(curenv, fault_va);
		//ptr_frame_info->element = WSE;
		if (!ActiveListIsFull)
		{
			LIST_INSERT_HEAD(&(curenv->ActiveList), WSE);
			return;
		}
		if (!SecondListIsFull)
			return handle_overflow_from_AL_to_SL_and_insert_new_WSE_in_AL(curenv, WSE);

		ptr_page_table = NULL;
		victimWSElement = LIST_LAST(&(curenv->SecondList));
		struct FrameInfo* modified_page_frame_info = get_frame_info(curenv->env_page_directory, victimWSElement->virtual_address, &ptr_page_table);
		if (pt_get_page_permissions(curenv->env_page_directory, victimWSElement->virtual_address) & PERM_MODIFIED)
			pf_update_env_page(curenv, victimWSElement->virtual_address, modified_page_frame_info);

		//modified_page_frame_info->element = victimWSElement; // O(1) purpose
		fast_env_page_ws_invalidate(curenv, victimWSElement->virtual_address); // O(1) purpose
		//pt_set_page_permissions(curenv->env_page_directory, victimWSElement->virtual_address, PERM_PRESENT, PERM_SECOND_LIST);
		//env_page_ws_invalidate(curenv, victimWSElement->virtual_address);
		handle_overflow_from_AL_to_SL_and_insert_new_WSE_in_AL(curenv, WSE);
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



