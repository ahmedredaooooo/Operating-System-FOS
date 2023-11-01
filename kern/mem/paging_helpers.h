/*
 * paging_helpers.h
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */

#ifndef KERN_MEM_PAGING_HELPERS_H_
#define KERN_MEM_PAGING_HELPERS_H_

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address);
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear);
inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address );

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/* PAGE DIR ENTRIES MANIPULATION */
inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address);
inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address);
inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address);

#endif /* KERN_MEM_PAGING_HELPERS_H_ */
