/*
 * test_working_set.h
 *
 *  Created on: Oct 13, 2023
 *      Author: HP
 */
#include <inc/types.h>
#include <inc/mmu.h>
#include <inc/assert.h>

#ifndef KERN_TESTS_TEST_WORKING_SET_H_
#define KERN_TESTS_TEST_WORKING_SET_H_

int 	sys_check_LRU_lists(uint32* active_list_content, uint32* second_list_content, int actual_active_list_size, int actual_second_list_size);
int 	sys_check_LRU_lists_free(uint32* list_content, int list_size);
int 	sys_check_WS_list(uint32* WS_list_content, int actual_WS_list_size, uint32 last_WS_element_content, bool chk_in_order);

#endif /* KERN_TESTS_TEST_WORKING_SET_H_ */
