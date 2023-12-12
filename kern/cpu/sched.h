/* See COPYRIGHT for copyright information. */

#ifndef FOS_KERN_SCHED_H
#define FOS_KERN_SCHED_H
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/environment_definitions.h>
#include <inc/fixed_point.h>
#include <kern/cpu/sched_helpers.h>

//2018
#define SCH_RR 0
#define SCH_MLFQ 1
#define SCH_BSD 2

unsigned scheduler_method ;


///Scheduler Queues
//=================
struct Env_Queue env_new_queue;	// queue of all new envs
//2015:
struct Env_Queue env_exit_queue;	// queue of all exited envs
//2018:
//2020:
#if USE_KHEAP
	struct Env_Queue *env_ready_queues;	// Ready queue(s) for the MLFQ or RR
	uint8 *quantums ;					// Quantum(s) in ms for each level of the ready queue(s)
#else
	//RR ONLY
	struct Env_Queue env_ready_queues[1];	// Ready queue(s) for the RR
	uint8 quantums[1] ;					// Quantum in ms for RR
#endif
uint8 num_of_ready_queues ;			// Number of ready queue(s)
//===============

//2015
#define SCH_UNINITIALIZED -1
#define SCH_STOPPED 0
#define SCH_STARTED 1
int scheduler_status ;

#define INIT_QUANTUM_IN_MS 10 //milliseconds

//2017
//#define CLOCK_INTERVAL_IN_CNTS TIMER_DIV((1000/CLOCK_INTERVAL_IN_MS))

/*2023*/
/********* for BSD Priority Scheduler *************/
#define PRI_MIN 0
#define PRI_MAX 63
int64 ticks;
int64 timer_ticks() ;
struct Env* fos_scheduler_BSD();
void sched_init_BSD(uint8 numOfLevels, uint8 quantum);
uint32 isSchedMethodBSD();
/********* for BSD Priority Scheduler *************/

void sched_init_RR(uint8 quantum);
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel);
uint32 isSchedMethodMLFQ();
uint32 isSchedMethodRR();

//2012
// This function does not return.
void fos_scheduler(void) __attribute__((noreturn));
struct Env* fos_scheduler_MLFQ();

void sched_init();
void clock_interrupt_handler();
void update_WS_time_stamps();

#endif	// !FOS_KERN_SCHED_H
