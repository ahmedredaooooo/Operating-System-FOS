/*
 * user_programs.h
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#ifndef KERN_USER_PROGRAMS_H_
#define KERN_USER_PROGRAMS_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

extern int NUM_USER_PROGS;

#define DECLARE_START_OF(binary_name)  \
	extern uint8 _binary_obj_user_##binary_name##_start[];

#define PTR_START_OF(binary_name) ( \
	(uint8*) _binary_obj_user_##binary_name##_start \
	)

//=========================================================
struct UserProgramInfo {
	const char *name;
	const char *desc;
	uint8* ptr_start;
};

struct UserProgramInfo*  get_user_program_info(char* user_program_name);
struct UserProgramInfo* get_user_program_info_by_env(struct Env* e);

///===================================================================================
/// To add FOS support for new user program, just add the appropriate lines like below

//The input for any DECLARE_START_OF macro must be the ".c" filename of the user program
DECLARE_START_OF(fos_helloWorld)
DECLARE_START_OF(fos_add)
DECLARE_START_OF(fos_alloc)
DECLARE_START_OF(fos_input)
DECLARE_START_OF(game)
DECLARE_START_OF(fos_static_data_section)
DECLARE_START_OF(fos_data_on_stack)
DECLARE_START_OF(test_ptf_stack)
DECLARE_START_OF(quicksort_heap)

//2012
DECLARE_START_OF(fos_fibonacci)
DECLARE_START_OF(fos_factorial)

DECLARE_START_OF(tst_placement)
DECLARE_START_OF(tst_invalid_access)
/*2023*/
DECLARE_START_OF(tst_invalid_access_slave1)
DECLARE_START_OF(tst_invalid_access_slave2)
DECLARE_START_OF(tst_invalid_access_slave3)
DECLARE_START_OF(tst_invalid_access_slave4)
/********************************************/
DECLARE_START_OF(tst_free_1);
/*2023*/DECLARE_START_OF(tst_free_1_slave1);
/*2023*/DECLARE_START_OF(tst_free_1_slave2);
DECLARE_START_OF(tst_free_2);
DECLARE_START_OF(tst_malloc_1);
DECLARE_START_OF(tst_malloc_2);
DECLARE_START_OF(tst_first_fit_1);
DECLARE_START_OF(tst_first_fit_2);
//2023
DECLARE_START_OF(tst_syscalls_1);
DECLARE_START_OF(tst_syscalls_2);
DECLARE_START_OF(tst_syscalls_2_slave1);
DECLARE_START_OF(tst_syscalls_2_slave2);
DECLARE_START_OF(tst_syscalls_2_slave3);

//MS3
DECLARE_START_OF(tst_page_replacement_alloc)
DECLARE_START_OF(tst_page_replacement_stack)
DECLARE_START_OF(tst_page_replacement_FIFO_1);
DECLARE_START_OF(tst_page_replacement_FIFO_2);
DECLARE_START_OF(tst_placement_1);
DECLARE_START_OF(tst_placement_2);
DECLARE_START_OF(tst_placement_3);
DECLARE_START_OF(tst_page_replacement_LRU_Lists_1);
DECLARE_START_OF(tst_page_replacement_LRU_Lists_2);
DECLARE_START_OF(tst_page_replacement_stack_LRU_Lists);
DECLARE_START_OF(mergesort_leakage);
DECLARE_START_OF(mergesort_noleakage);
DECLARE_START_OF(quicksort_noleakage);


#endif /* KERN_USER_PROGRAMS_H_ */
