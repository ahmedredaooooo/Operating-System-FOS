/*
 * user_programs.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */
#include <kern/proc/user_environment.h>
#include <inc/string.h>
#include <inc/assert.h>



//User Programs Table
//The input for any PTR_START_OF macro must be the ".c" filename of the user program
struct UserProgramInfo userPrograms[] = {
		{ "fos_helloWorld", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_helloWorld)},
		{ "fos_add", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_add)},
		{ "fos_alloc", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_alloc)},
		{ "fos_input", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_input)},
		{ "fos_game", "Created by FOS team, fos@nowhere.com", PTR_START_OF(game)},
		{ "fos_static_data_section", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_static_data_section)},
		{ "fos_data_on_stack", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_data_on_stack)},

		/*TESTING 2023*/
		//[1] READY MADE TESTS
		{ "tst_syscalls_1", "Tests correct handling of 3 system calls", PTR_START_OF(tst_syscalls_1)},
		{ "tst_syscalls_2", "Tests correct validation of syscalls params", PTR_START_OF(tst_syscalls_2)},
		{ "tsc2_slave1", "Slave program for tst_syscalls_2", PTR_START_OF(tst_syscalls_2_slave1)},
		{ "tsc2_slave2", "Slave program for tst_syscalls_2", PTR_START_OF(tst_syscalls_2_slave2)},
		{ "tsc2_slave3", "Slave program for tst_syscalls_2", PTR_START_OF(tst_syscalls_2_slave3)},

		{ "tm1", "tests malloc (1): PAGE ALLOCATOR", PTR_START_OF(tst_malloc_1)},
		{ "tm2", "tests malloc (2): BLOCK ALLOCATOR", PTR_START_OF(tst_malloc_2)},
		{ "tf1", "tests free (1): PAGE ALLOCATOR", PTR_START_OF(tst_free_1)},
		{ "tf1_slave1", "tests free (1) slave1: try accessing values in freed spaces", PTR_START_OF(tst_free_1_slave1)},
		{ "tf1_slave2", "tests free (1) slave2: try accessing values in freed spaces that is not accessed before", PTR_START_OF(tst_free_1_slave2)},
		{ "tf2", "tests free (2): BLOCK ALLOCATOR", PTR_START_OF(tst_free_2)},
		
		{ "tff1", "tests first fit (1): PAGE ALLOCATOR", PTR_START_OF(tst_first_fit_1)},
		{ "tff2", "tests first fit (2): BLOCK ALLOCATOR", PTR_START_OF(tst_first_fit_2)},
		{ "tpp", "Tests the Page placement", PTR_START_OF(tst_placement)},
		{ "tia", "tests handling of invalid memory access", PTR_START_OF(tst_invalid_access)},
		{ "tia_slave1", "tia: access kernel", PTR_START_OF(tst_invalid_access_slave1)},
		{ "tia_slave2", "tia: write on read only user page", PTR_START_OF(tst_invalid_access_slave2)},
		{ "tia_slave3", "tia: access an unmarked (non-reserved) user heap page", PTR_START_OF(tst_invalid_access_slave3)},
		{ "tia_slave4", "tia: access a non-exist page in page file, stack and heap", PTR_START_OF(tst_invalid_access_slave4)},
		//[2] PROGRAMS
		{ "fact", "Factorial Recursive", PTR_START_OF(fos_factorial)},
		{ "fib", "Fibonacci Recursive", PTR_START_OF(fos_fibonacci)},

		//[3] BONUSES

};

///=========================================================

// To be used as extern in other files
struct UserProgramInfo* ptr_UserPrograms = &userPrograms[0];

// Number of user programs in the program table
int NUM_USER_PROGS = (sizeof(userPrograms)/sizeof(userPrograms[0]));

struct UserProgramInfo* get_user_program_info(char* user_program_name)
{
	int i;
	for (i = 0; i < NUM_USER_PROGS; i++) {
		if (strcmp(user_program_name, userPrograms[i].name) == 0)
			break;
	}
	if(i==NUM_USER_PROGS)
	{
		cprintf("Unknown user program '%s'\n", user_program_name);
		return 0;
	}

	return &userPrograms[i];
}

struct UserProgramInfo* get_user_program_info_by_env(struct Env* e)
{
	int i;
	for (i = 0; i < NUM_USER_PROGS; i++) {
		if ( strcmp( e->prog_name , userPrograms[i].name) ==0)
			break;
	}
	if(i==NUM_USER_PROGS)
	{
		cprintf("Unknown user program \n");
		return 0;
	}

	return &userPrograms[i];
}
