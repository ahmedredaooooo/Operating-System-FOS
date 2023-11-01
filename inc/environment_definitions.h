/* See COPYRIGHT for copyright information. */

#ifndef FOS_INC_ENV_H
#define FOS_INC_ENV_H

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

// An environment ID 'envid_t' has three parts:
//
// +1+---------------21-----------------+--------10--------+
// |0|          Uniqueifier             |   Environment    |
// | |                                  |      Index       |
// +------------------------------------+------------------+
//                                       \--- ENVX(eid) --/
//
// The environment index ENVX(eid) equals the environment's offset in the
// 'envs[]' array.  The uniqueifier distinguishes environments that were
// created at different times, but share the same environment index.
//
// All real environments are greater than 0 (so the sign bit is zero).
// envid_ts less than 0 signify errors.


//Sizes of working sets & LRU 2nd list [if NO KERNEL HEAP]
#define __TWS_MAX_SIZE 	50
#define __PWS_MAX_SIZE 	5000
//2020
#define __LRU_SNDLST_SIZE 500

//2017: moved to shared_memory_manager
//#define MAX_SHARES 100
//====================================
unsigned int _ModifiedBufferLength;

//2017: Max length of program name
#define PROGNAMELEN 64

//2018 Percentage of the pages to be removed from the WS [either for scarce RAM or Full WS]
#define DEFAULT_PERCENT_OF_PAGE_WS_TO_REMOVE	10	// 10% of the loaded pages is required to be removed

// Values of env_status in struct Env
#define ENV_FREE		0
#define ENV_READY		1
#define ENV_RUNNABLE	2
#define ENV_BLOCKED		3
#define ENV_NEW			4
#define ENV_EXIT		5
#define ENV_UNKNOWN		6


uint32 old_pf_counter;
//uint32 mydblchk;
struct WorkingSetElement {
	unsigned int virtual_address;
	uint8 empty;
	//2012
	unsigned int time_stamp ;

	//2021
	unsigned int sweeps_counter;
	//2020
	LIST_ENTRY(WorkingSetElement) prev_next_info;	// list link pointers
};

struct SharingVarInfo
{
	uint32 start_va;
	uint32 size;
	int owner_flag;
	int id_in_shares_array;
};

//2020
LIST_HEAD(WS_List, WorkingSetElement);		// Declares 'struct WS_list'
//======================================================================

struct Env {
	//================
	/*MAIN INFO...*/
	//================
	struct Trapframe env_tf;		// Saved registers
	LIST_ENTRY(Env) prev_next_info;	// Free list link pointers
	int32 env_id;					// Unique environment identifier
	int32 env_parent_id;			// env_id of this env's parent
	unsigned env_status;			// Status of the environment
	int priority;					// Current priority
	char prog_name[PROGNAMELEN];	// Program name (to print it via USER.cprintf in multitasking)

	//================
	/*ADDRESS SPACE*/
	//================
	uint32 *env_page_directory;		// Kernel virtual address of page dir
	uint32 env_cr3;					// Physical address of page dir
	uint32 initNumStackPages ;		// Initial number of allocated stack pages

	//for page file management
	uint32* disk_env_pgdir;
	//2016
	unsigned int disk_env_pgdir_PA;

	//for table file management
	uint32* disk_env_tabledir;
	//2016
	unsigned int disk_env_tabledir_PA;

	/*2023*/
	//TODO: [PROJECT'23.MS2 - #07] [2] USER HEAP - initialize of Env: add suitable code here

	//==================================================================================

	//================
	/*WORKING SET*/
	//================
	//page working set management

#if USE_KHEAP
	//struct WorkingSetElement* ptr_pageWorkingSet;
	struct WS_List page_WS_list ;					//List of WS elements
	struct WorkingSetElement* page_last_WS_element;	//ptr to last inserted WS element
#else
	struct WorkingSetElement ptr_pageWorkingSet[__PWS_MAX_SIZE];
	//uint32 page_last_WS_index;
#endif
	uint32 page_last_WS_index;
	unsigned int page_WS_max_size;

	//table working set management
	struct WorkingSetElement __ptr_tws[__TWS_MAX_SIZE];
	uint32 table_last_WS_index;

	//TODO: [PROJECT'23.MS3 - #0 GIVENS] [1] FAULT HANDLER REPLACEMENT - Data structures of LRU Approx replacement policy
	struct WS_List PageWorkingSetList ;	//LRU Approx: List of available WS elements
	struct WS_List ActiveList ;		//LRU Approx: ActiveList that should work as FCFS
	struct WS_List SecondList ;		//LRU Approx: SecondList that should work as LRU
	int ActiveListSize ;			//LRU Approx: Max allowed size of ActiveList
	int SecondListSize ;			//LRU Approx: Max allowed size of SecondList
	//================================================================================
	//2016
	struct WorkingSetElement* __uptr_pws;

	//Percentage of WS pages to be removed [either for scarce RAM or Full WS]
		unsigned int percentage_of_WS_pages_to_be_removed;

	//================
	/*STATISTICS...*/
	//================
	uint32 pageFaultsCounter;
	uint32 tableFaultsCounter;
	uint32 freeingFullWSCounter;
	uint32 freeingScarceMemCounter;
	uint32 nModifiedPages;
	uint32 nNotModifiedPages;
	uint32 env_runs;			// Number of times environment has run
	//2020
	uint32 nPageIn, nPageOut, nNewPageAdded;
	uint32 nClocks ;

};

#define PRIORITY_LOW    		1
#define PRIORITY_BELOWNORMAL    2
#define PRIORITY_NORMAL		    3
#define PRIORITY_ABOVENORMAL    4
#define PRIORITY_HIGH		    5

//#define NENV			(1 << LOG2NENV)
#define NENV			( (PTSIZE/4) / sizeof(struct Env) )
/*2022: UPDATED*/
#define NEARPOW2NENV	(nearest_pow2_ceil(NENV))
#define LOG2NENV		(log2_ceil(NENV))
#define ENVGENSHIFT		LOG2NENV	// >= LOGNENV
#define ENVX(envid)		((envid) & (NEARPOW2NENV - 1))

#endif // !FOS_INC_ENV_H
