/*
 * commands.h
 *
 *  Created on: Oct 14, 2022
 *      Author: HP
 */

#ifndef KERN_CMD_COMMANDS_H_
#define KERN_CMD_COMMANDS_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif
#include <inc/types.h>
#include <kern/cmd/command_prompt.h>

//Structure for each command
struct Command
{
	char *name;
	char *description;
	// return -1 to force command prompt to exit
	int (*function_to_execute)(int number_of_arguments, char** arguments);
	int num_of_args ;
	Command_LIST_entry_t prev_next_info;	/* linked list links */
};

//Array of commands
extern struct Command commands[] ;
extern uint32 NUM_OF_COMMANDS ;

//=================================================================//
// Declaration of functions that implement command prompt commands.//
//=================================================================//
int command_help(int , char **);
int command_kernel_info(int , char **);
int command_calc_space(int number_of_arguments, char **arguments);
int command_run_program(int argc, char **argv);
int command_readmem_k(int number_of_arguments, char **arguments);
int command_writemem_k(int number_of_arguments, char **arguments);
int command_kill_program(int number_of_arguments, char **arguments);

int commnad_load_env(int number_of_arguments, char **arguments);
int command_run_all(int number_of_arguments, char **arguments);
int command_print_all(int number_of_arguments, char **arguments);
int command_kill_all(int number_of_arguments, char **arguments);

int command_writeusermem(int number_of_arguments, char **arguments);
int command_readusermem(int number_of_arguments, char **arguments);
int command_readuserblock(int number_of_arguments, char **arguments);
int command_remove_table(int number_of_arguments, char **arguments);
int command_allocuserpage(int number_of_arguments, char **arguments);
int command_meminfo(int number_of_arguments, char **arguments);

int command_set_page_rep_FIFO(int number_of_arguments, char **arguments);
int command_set_page_rep_CLOCK(int number_of_arguments, char **arguments);
int command_set_page_rep_LRU(int number_of_arguments, char **arguments);
int command_set_page_rep_ModifiedCLOCK(int number_of_arguments, char **arguments);
int command_set_page_rep_nthCLOCK(int number_of_arguments, char **arguments);
int command_print_page_rep(int number_of_arguments, char **arguments);

int command_set_uheap_plac_FIRSTFIT(int number_of_arguments, char **arguments);
int command_set_uheap_plac_BESTFIT(int number_of_arguments, char **arguments);
int command_set_uheap_plac_NEXTFIT(int number_of_arguments, char **arguments);
int command_set_uheap_plac_WORSTFIT(int number_of_arguments, char **arguments);
int command_print_uheap_plac(int number_of_arguments, char **arguments);

int command_set_kheap_plac_CONTALLOC(int number_of_arguments, char **arguments);
int command_set_kheap_plac_FIRSTFIT(int number_of_arguments, char **arguments);
int command_set_kheap_plac_BESTFIT(int number_of_arguments, char **arguments);
int command_set_kheap_plac_NEXTFIT(int number_of_arguments, char **arguments);
int command_set_kheap_plac_WORSTFIT(int number_of_arguments, char **arguments);
int command_print_kheap_plac(int number_of_arguments, char **arguments);

int command_disable_modified_buffer(int number_of_arguments, char **arguments);
int command_enable_modified_buffer(int number_of_arguments, char **arguments);

//2016
int command_disable_buffering(int number_of_arguments, char **arguments);
int command_enable_buffering(int number_of_arguments, char **arguments);

int command_set_modified_buffer_length(int number_of_arguments, char **arguments);
int command_get_modified_buffer_length(int number_of_arguments, char **arguments);

//2018
int command_sch_RR(int number_of_arguments, char **arguments);
int command_sch_MLFQ(int number_of_arguments, char **arguments);
int command_sch_BSD(int number_of_arguments, char **arguments);
int command_print_sch_method(int number_of_arguments, char **arguments);
int command_sch_test(int number_of_arguments, char **arguments);

//2023
int command_tst(int number_of_arguments, char **arguments);

#endif /* KERN_CMD_COMMANDS_H_ */
