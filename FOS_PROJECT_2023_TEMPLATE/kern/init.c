/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/memlayout.h>
#include <inc/timerreg.h>
#include <kern/cons/console.h>
#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/trap/fault_handler.h>
#include <kern/cpu/picirq.h>
#include <kern/conc/semaphore_manager.h>
#include <inc/dynamic_allocator.h>
#include "kern/cmd/command_prompt.h"
#include <kern/cpu/kclock.h>
#include <kern/cpu/sched.h>
#include <kern/mem/boot_memory_manager.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/mem/shared_memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/tests/test_kheap.h>
#include <kern/disk/pagefile_manager.h>

extern int sys_calculate_free_frames();

//Functions Declaration
//======================
void print_welcome_message();
//=======================================

//First ever function called in FOS kernel
void FOS_initialize()
{
	//get actual addresses after code linking
	extern char start_of_uninitialized_data_section[], end_of_kernel[];

	// Before doing anything else,
	// clear the uninitialized global data (BSS) section of our program, from start_of_uninitialized_data_section to end_of_kernel
	// This ensures that all static/global variables start with zero value.
	memset(start_of_uninitialized_data_section, 0, end_of_kernel - start_of_uninitialized_data_section);

	/*2022*/
	scheduler_status = SCH_UNINITIALIZED;

	// Initialize the console.
	// Can't call cprintf until after we do this!
	console_initialize();

	//print welcome message
	print_welcome_message();

	// Lab 2 memory management initialization functions
	detect_memory();
	initialize_kernel_VM();
	initialize_paging();
	//	page_check();

	// Lab 3 user environment initialization functions
	env_init();
	idt_init();

	//Project initializations

	//2021
	//setPageReplacmentAlgorithmNchanceCLOCK();
	//setPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX);
	//setPageReplacmentAlgorithmFIFO();
	//setPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX);
	setPageReplacmentAlgorithmFIFO();
	setUHeapPlacementStrategyFIRSTFIT();
	setKHeapPlacementStrategyFIRSTFIT();

#if USE_KHEAP
	//2022:
	{
		int freeFrames_before = sys_calculate_free_frames() ;
		int freeDiskFrames_before = pf_calculate_free_frames() ;
		initialize_kheap_dynamic_allocator(KERNEL_HEAP_START, PAGE_SIZE, KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE);
		int freeFrames_after = sys_calculate_free_frames() ;
		int freeDiskFrames_after = pf_calculate_free_frames() ;
		test_initialize_dyn_block_system(freeFrames_before, freeDiskFrames_before, freeFrames_after, freeDiskFrames_after);
	}
	MAX_SHARES = (KERNEL_SHARES_ARR_INIT_SIZE) / sizeof(struct Share);
	MAX_SEMAPHORES = (KERNEL_SEMAPHORES_ARR_INIT_SIZE) / sizeof(struct Semaphore);
#endif
	create_shares_array(MAX_SHARES);
	create_semaphores_array(MAX_SEMAPHORES);


	enableBuffering(0);
	//enableModifiedBuffer(1) ;
	enableModifiedBuffer(0) ;
	setModifiedBufferLength(1000);

	// Lab 4 multitasking initialization functions
	pic_init();
	sched_init() ;



	//	kclock_start(CLOCK_INTERVAL_IN_MS);

	//	int h, c = 0 ;
	//		for (h = 0 ; h < 300000 ; h++)
	//		{
	//			c++ ;
	//		}
	//		cprintf("START LAG = %d\n", c) ;

	//FOR TESTING
	//	kclock_stop();
	//	kclock_resume();
	//	kclock_set_quantum(30);
	//================

	//		int h, c = 0 ;
	//		for (h = 0 ; h < 10000 ; h++)
	//		{
	//			c++;
	//		}
	//		kclock_stop();
	//
	//		uint16 cnt0 = kclock_read_cnt0() ;
	//		cprintf("Timer After Loop: Counter0 Value = %d\n", cnt0 );

	//cprintf("Int Flag before = %d\n", read_eflags() & FL_IF) ;

	//Project initializations

	// start the kernel command prompt.
	while (1==1)
	{
		cprintf("\nWelcome to the FOS kernel command prompt!\n");
		cprintf("Type 'help' for a list of commands.\n");
		run_command_prompt();
	}
}


void print_welcome_message()
{
	cprintf("\n\n\n");
	cprintf("\t\t!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	cprintf("\t\t!!                                                             !!\n");
	cprintf("\t\t!!                   !! FCIS says HELLO !!                     !!\n");
	cprintf("\t\t!!                                                             !!\n");
	cprintf("\t\t!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	cprintf("\n\n\n\n");
}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
static const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", exit the curenv and schedule the next environment.
 */
void _panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	//if (panicstr)
	//	goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	cprintf("\nkernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

	dead:
	/* break into the fos scheduler */
	//2013: Check if the panic occur when running an environment
	if (curenv != NULL && curenv->env_status == ENV_RUNNABLE)
	{
		//2015
		env_exit();
		//env_run_cmd_prmpt() ;
	}

	/*2022*///Check if the scheduler is successfully initialized or not
	if (scheduler_status != SCH_UNINITIALIZED)
	{
		cprintf("scheduler_status=%d\n", scheduler_status);
		//2015
		fos_scheduler();
	}
	else
	{
		curenv = NULL;
		lcr3(phys_page_directory);
		while (1)
			run_command_prompt(NULL);
	}

}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", exit all env's and then enters the kernel command prompt.
 */
void _panic_all(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	//if (panicstr)
	//	goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	cprintf("\nkernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

	dead:
	/* break into the fos scheduler */
	/*2022*///Check if the scheduler is successfully initialized or not
	if (scheduler_status != SCH_UNINITIALIZED)
	{
		//exit all ready env's
		sched_exit_all_ready_envs();
		if (curenv != NULL)
		{
			//cprintf("exit curenv...........\n");
			sched_exit_env(curenv->env_id);
			//env_run_cmd_prmpt() ;
		}

		fos_scheduler();
	}
	else
	{
		curenv = NULL;
		lcr3(phys_page_directory);
		while (1)
			run_command_prompt(NULL);
	}
}


/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", exit the curenv (if any) and break into the command prompt.
 */
void _panic_into_prompt(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	//if (panicstr)
	//	goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	cprintf("\nkernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

	dead:
	/* break into the fos scheduler */
	//2013: Check if the panic occur when running an environment
	if (curenv != NULL && curenv->env_status == ENV_RUNNABLE)
	{
		sched_insert_exit(curenv);
		curenv = NULL;
	}

	lcr3(phys_page_directory);

	/*2022*///Check if the scheduler is successfully initialized or not
	if (scheduler_status != SCH_UNINITIALIZED)
	{
		scheduler_status = SCH_STOPPED;
	}
	while (1)
		run_command_prompt(NULL);
}

/* like panic, but don't enters the kernel command prompt*/
void _warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("\nkernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}

