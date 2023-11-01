/*
 * tst_handler.h
 *
 *  Created on: Oct 9, 2023
 *      Author: HP
 */

#ifndef KERN_TESTS_TST_HANDLER_H_
#define KERN_TESTS_TST_HANDLER_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/disk.h>
#include <inc/error.h>

//Structure for each test
struct Test
{
	char *name;
	char *description;
	int (*function_to_execute)(int number_of_arguments, char** arguments);
};

//Array of tests
extern struct Test tests[] ;
extern uint32 NUM_OF_TESTS;

//Functions declaration
int tst_handler(int number_of_arguments, char **arguments);

int tst_three_creation_functions(int number_of_arguments, char **arguments);
int tst_kfreeall(int number_of_arguments, char **arguments);
int tst_kexpand(int number_of_arguments, char **arguments);
int tst_kshrink(int number_of_arguments, char **arguments);
int tst_kfreelast(int number_of_arguments, char **arguments);
int tst_sc4_MLFQ(int number_of_arguments, char **arguments);
int tst_priority1(int number_of_arguments, char **arguments);
int tst_priority2(int number_of_arguments, char **arguments);
int tst_sc_MLFQ(int number_of_arguments, char **arguments);
/*2022*/
int tst_autocomplete(int number_of_arguments, char **arguments);
int tst_dyn_alloc(int number_of_arguments, char **arguments);
int tst_paging_manipulation(int number_of_arguments, char **arguments);
int tst_chunks(int number_of_arguments, char **arguments);
int tst_kheap(int number_of_arguments, char **arguments);



#endif /* KERN_TESTS_TST_HANDLER_H_ */
