/*
 * working_set_manager.h
 *
 *  Created on: Oct 11, 2022
 *      Author: HP
 */

#ifndef KERN_MEM_WORKING_SET_MANAGER_H_
#define KERN_MEM_WORKING_SET_MANAGER_H_

#include <inc/environment_definitions.h>

// Page WS helper functions ===================================================
void env_page_ws_print(struct Env *curenv);
inline void env_page_ws_invalidate(struct Env* e, uint32 virtual_address);

#if USE_KHEAP
/*2024*/
inline struct WorkingSetElement* env_page_ws_list_create_element(struct Env* e, uint32 virtual_address);
#else
inline uint32 env_page_ws_get_size(struct Env *e);
inline void env_page_ws_set_entry(struct Env* e, uint32 entry_index, uint32 virtual_address);
inline void env_page_ws_clear_entry(struct Env* e, uint32 entry_index);
inline uint32 env_page_ws_get_virtual_address(struct Env* e, uint32 entry_index);
inline uint32 env_page_ws_get_time_stamp(struct Env* e, uint32 entry_index);
inline uint32 env_page_ws_is_entry_empty(struct Env* e, uint32 entry_index);
#endif



// Table WS helper functions ===================================================
inline uint32 env_table_ws_get_size(struct Env *e);
inline void env_table_ws_invalidate(struct Env* e, uint32 virtual_address);
inline void env_table_ws_set_entry(struct Env* e, uint32 entry_index, uint32 virtual_address);
inline void env_table_ws_clear_entry(struct Env* e, uint32 entry_index);
inline uint32 env_table_ws_get_virtual_address(struct Env* e, uint32 entry_index);
inline uint32 env_table_ws_get_time_stamp(struct Env* e, uint32 entry_index);
inline uint32 env_table_ws_is_entry_empty(struct Env* e, uint32 entry_index);
void env_table_ws_print(struct Env *curenv);


#endif /* KERN_MEM_WORKING_SET_MANAGER_H_ */
