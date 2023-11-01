/*
 * paging_helpers.c
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */
#include "memory_manager.h"

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear)
{
	//[1] Get the table
	uint32* ptr_page_table ;
	int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);

	//[2] If exists, update permissions
	if (ptr_page_table != NULL)
	{
		ptr_page_table[PTX(virtual_address)] |= (permissions_to_set);
		ptr_page_table[PTX(virtual_address)] &= (~permissions_to_clear);

	}
	//[3] Else, should "panic" since the table should be exist
	else
	{
		cprintf("va=%x not exist and has no page table\n", virtual_address);
		//cprintf("[%s] va = %x\n", ptr_env->prog_name, virtual_address) ;
		panic("function pt_set_page_permissions() called with invalid virtual address. The corresponding page table doesn't exist\n") ;
	}

	//[4] Invalidate the cache memory (TLB) [call tlb_invalidate(..)]
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address )
{
	//[1] Get the table
	uint32* ptr_page_table ;
	int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);

	//[2] If exists, return the permissions
	if (ptr_page_table != NULL)
	{
		//cprintf("va=%x perm = %x\n", virtual_address, ptr_page_table[PTX(virtual_address)] & 0x00000FFF);
		return (ptr_page_table[PTX(virtual_address)] & 0x00000FFF);
	}
	//[3] Else, return -1
	else
	{
		//cprintf("va=%x not exist and has no page table\n", virtual_address);
		return -1;
	}
}

inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address)
{
	//[1] Get the table
	uint32* ptr_page_table ;
	int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);

	//[2] If exists, update permissions
	if (ptr_page_table != NULL)
	{
		cprintf("va=%x before clearing has perm = %x\n", virtual_address, ptr_page_table[PTX(virtual_address)]);
		ptr_page_table[PTX(virtual_address)] = 0;
	}
	//[3] Else, should "panic" since the table should be exist
	else
	{
		//cprintf("[%s] va = %x\n", ptr_env->prog_name, virtual_address) ;
		panic("function pt_clear_page_table_entry() called with invalid virtual address. The corresponding page table doesn't exist\n") ;
	}

	//[4] Invalidate the cache memory (TLB) [call tlb_invalidate(..)]
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

/***********************************************************************************************/

/***********************************************************************************************/

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

///============================================================================================
/// Dealing with page directory entry flags

inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address)
{
	return ( (page_directory[PDX(virtual_address)] & PERM_USED) == PERM_USED ? 1 : 0);
}

inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] &= (~PERM_USED);
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] = 0 ;
	tlbflush();
}
