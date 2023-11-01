/*
 * test_commands.h
 *
 *  Created on: Oct 16, 2022
 *      Author: ghada
 */

#ifndef KERN_TESTS_TEST_COMMANDS_H_
#define KERN_TESTS_TEST_COMMANDS_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <kern/cmd/command_prompt.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

int TestAutoCompleteCommand();
int test_cut_paste_pages();
int test_copy_paste_chunk();
int test_share_chunk();
int test_allocate_chunk();
int test_calculate_required_frames();
int test_calculate_allocated_space();

//Testing Paging Helpers functions
int test_pt_set_page_permissions();
int test_pt_set_page_permissions_invalid_va();
int test_pt_get_page_permissions();
int test_pt_clear_page_table_entry();
int test_pt_clear_page_table_entry_invalid_va();

#endif /* KERN_TESTS_TEST_COMMANDS_H_ */
