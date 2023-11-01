/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include "trap.h"
#include "syscall.h"
#include <kern/cons/console.h>
#include <kern/conc/semaphore_manager.h>
#include <kern/cpu/sched.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>
#include <kern/mem/shared_memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/tests/test_working_set.h>

extern uint8 bypassInstrLength ;

/*******************************/
/* STRING I/O SYSTEM CALLS */
/*******************************/

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void sys_cputs(const char *s, uint32 len, uint8 printProgName)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.

	// Print the string supplied by the user.
	if (printProgName)
		cprintf("[%s %d] ",curenv->prog_name, curenv->env_id);
	cprintf("%.*s",len, s);
	//cprintf("%.*s", len, s);
}


// Print a char to the system console.
static void sys_cputc(const char c)
{
	// Print the char supplied by the user.
	cprintf("%c",c);
}


// Read a character from the system console.
// Returns the character.
static int
sys_cgetc(void)
{
	int c;

	// The cons_getc() primitive doesn't wait for a character,
	// but the sys_cgetc() system call does.
	//while ((c = cons_getc()) == 0)
	//2013: instead of busywaiting inside kernel code , we moved the waiting to user code,
	// this allows any other running user programs to run beacause the clock interrupt works in user mode
	// (the kernel system call by default disables the interrupts, so any loops inside the kernel while
	// handling an interrupt will cause the clock interrupt to be disabled)
	c = cons_getc2();
	/* do nothing */;

	return c;
}


/*******************************/
/* MEMORY SYSTEM CALLS */
/*******************************/
// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.
//
// Return 0 on success, < 0 on error.  Errors are:
//	E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	E_INVAL if va >= UTOP, or va is not page-aligned.
//	E_INVAL if perm is inappropriate (see above).
//	E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int __sys_allocate_page(void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	int r;
	struct Env *e = curenv;

	//if ((r = envid2env(envid, &e, 1)) < 0)
	//return r;

	struct FrameInfo *ptr_frame_info ;
	r = allocate_frame(&ptr_frame_info) ;
	if (r == E_NO_MEM)
		return r ;

	//check virtual address to be paged_aligned and < USER_TOP
	if ((uint32)va >= USER_TOP || (uint32)va % PAGE_SIZE != 0)
		return E_INVAL;

	//check permissions to be appropriate
	if ((perm & (~PERM_AVAILABLE & ~PERM_WRITEABLE)) != (PERM_USER))
		return E_INVAL;


	uint32 physical_address = to_physical_address(ptr_frame_info) ;

#if USE_KHEAP
	{
		//FIX: we should implement a better solution for this, but for now
		//		we are using an unsed VA in the invalid area of kernel at 0xef800000 (the current USER_LIMIT)
		//		to do temp initialization of a frame.
		map_frame(e->env_page_directory, ptr_frame_info, USER_LIMIT, PERM_WRITEABLE);
		memset((void*)USER_LIMIT, 0, PAGE_SIZE);

		// Temporarily increase the references to prevent unmap_frame from removing the frame
		// we just got from allocate_frame, we will use it for the new page
		ptr_frame_info->references += 1;
		unmap_frame(e->env_page_directory, USER_LIMIT);

		//return it to the original status
		ptr_frame_info->references -= 1;
	}
#else
	{
		memset(STATIC_KERNEL_VIRTUAL_ADDRESS(physical_address), 0, PAGE_SIZE);
	}
#endif
	r = map_frame(e->env_page_directory, ptr_frame_info, (uint32)va, perm) ;
	if (r == E_NO_MEM)
	{
		decrement_references(ptr_frame_info);
		return r;
	}
	return 0 ;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int __sys_map_frame(int32 srcenvid, void *srcva, int32 dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	panic("sys_map_frame not implemented");
	return 0;

}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int __sys_unmap_frame(int32 envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	panic("sys_page_unmap not implemented");
	return 0;
}

uint32 sys_calculate_required_frames(uint32 start_virtual_address, uint32 size)
{
	return calculate_required_frames(curenv->env_page_directory, start_virtual_address, size);
}

uint32 sys_calculate_free_frames()
{
	struct freeFramesCounters counters = calculate_available_frames();
	//	cprintf("Free Frames = %d : Buffered = %d, Not Buffered = %d\n", counters.freeBuffered + counters.freeNotBuffered, counters.freeBuffered ,counters.freeNotBuffered);
	return counters.freeBuffered + counters.freeNotBuffered;
}
uint32 sys_calculate_modified_frames()
{
	struct freeFramesCounters counters = calculate_available_frames();
	//	cprintf("================ Modified Frames = %d\n", counters.modified) ;
	return counters.modified;
}

uint32 sys_calculate_notmod_frames()
{
	struct freeFramesCounters counters = calculate_available_frames();
	//	cprintf("================ Not Modified Frames = %d\n", counters.freeBuffered) ;
	return counters.freeBuffered;
}

int sys_calculate_pages_tobe_removed_ready_exit(uint32 WS_or_MEMORY_flag)
{
	return calc_no_pages_tobe_removed_from_ready_exit_queues(WS_or_MEMORY_flag);
}

void sys_scarce_memory(void)
{
	scarce_memory();
}

void sys_clearFFL()
{
	int size = LIST_SIZE(&free_frame_list) ;
	int i = 0 ;
	struct FrameInfo* ptr_tmp_FI ;
	for (; i < size ; i++)
	{
		allocate_frame(&ptr_tmp_FI) ;
	}
}

/*******************************/
/* PAGE FILE SYSTEM CALLS */
/*******************************/
int sys_pf_calculate_allocated_pages(void)
{
	return pf_calculate_allocated_pages(curenv);
}

/*******************************/
/* USER HEAP SYSTEM CALLS */
/*******************************/
void sys_free_user_mem(uint32 virtual_address, uint32 size)
{
	if(isBufferingEnabled())
	{
		__free_user_mem_with_buffering(curenv, virtual_address, size);
	}
	else
	{
		free_user_mem(curenv, virtual_address, size);
	}
	return;
}

void sys_allocate_user_mem(uint32 virtual_address, uint32 size)
{
	allocate_user_mem(curenv, virtual_address, size);
	return;
}

void sys_allocate_chunk(uint32 virtual_address, uint32 size, uint32 perms)
{
	allocate_chunk(curenv->env_page_directory, virtual_address, size, perms);
	return;
}

//2014
void sys_move_user_mem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	move_user_mem(curenv, src_virtual_address, dst_virtual_address, size);
	return;
}

//2015
uint32 sys_get_heap_strategy()
{
	return _UHeapPlacementStrategy ;
}
void sys_set_uheap_strategy(uint32 heapStrategy)
{
	_UHeapPlacementStrategy = heapStrategy;
}
/*******************************/
/* INTERRUPTS SYSTEM CALLS */
/*******************************/
//NEW!! 2012...
void sys_disable_interrupt()
{
	curenv->env_tf.tf_eflags &= ~FL_IF ;
}
void sys_enable_interrupt()
{
	curenv->env_tf.tf_eflags |= FL_IF ;
}

/*******************************/
/* SEMAPHORES SYSTEM CALLS */
/*******************************/
int sys_createSemaphore(char* semaphoreName, uint32 initialValue)
{
	return createSemaphore(curenv->env_id, semaphoreName, initialValue);
}

void sys_waitSemaphore(int32 ownerEnvID, char* semaphoreName)
{
	waitSemaphore(ownerEnvID, semaphoreName);
}

void sys_signalSemaphore(int32 ownerEnvID, char* semaphoreName)
{
	signalSemaphore(ownerEnvID, semaphoreName);
}

int sys_getSemaphoreValue(int32 ownerEnvID, char* semaphoreName)
{
	int semID = get_semaphore_object_ID(ownerEnvID, semaphoreName);
	assert(semID >= 0 && semID < MAX_SEMAPHORES) ;

	return semaphores[semID].value ;
}

/*******************************/
/* SHARED MEMORY SYSTEM CALLS */
/*******************************/
int sys_createSharedObject(char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	return createSharedObject(curenv->env_id, shareName, size, isWritable, virtual_address);
}

int sys_getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	return getSizeOfSharedObject(ownerID, shareName);
}

int sys_getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	return getSharedObject(ownerID, shareName, virtual_address);
}

int sys_freeSharedObject(int32 sharedObjectID, void *startVA)
{
	return freeSharedObject(sharedObjectID, startVA);
}

uint32 sys_getMaxShares()
{
	return MAX_SHARES;
}

/*********************************/
/* USER ENVIRONMENT SYSTEM CALLS */
/*********************************/
// Returns the current environment's envid.
//2017
static int32 sys_getenvid(void)
{
	return curenv->env_id;
}

//2017
static int32 sys_getenvindex(void)
{
	//return curenv->env_id;
	return (curenv - envs) ;
}

//2017
static int32 sys_getparentenvid(void)
{
	return curenv->env_parent_id;
}

// Destroy a given environment whatever its state & DON'T place it in EXIT
// if envid=0, destroy the currently running environment --> schedule the next (if any)
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_destroy_env(int32 envid)
{
	int r;
	struct Env *e;
	if (envid == 0)
	{
		e = curenv ;
	}
	else if ((r = envid2env(envid, &e, 0)) < 0)
	{
		return r;
	}

	if (e == curenv)
	{
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	}
	else
	{
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	}
	//2015
	sched_kill_env(e->env_id);

	return 0;
}

//Just place the current env into the EXIT queue & schedule the next one
static void sys_exit_env()
{
	//2015
	env_exit();
	//env_run_cmd_prmpt();
}

//New update in 2020
//Create a new env & add it to the NEW queue
int sys_create_env(char* programName, unsigned int page_WS_size,unsigned int LRU_second_list_size, unsigned int percent_WS_pages_to_remove)
{
	struct Env* env =  env_create(programName, page_WS_size, LRU_second_list_size, percent_WS_pages_to_remove);
	if(env == NULL)
	{
		return E_ENV_CREATION_ERROR;
	}

	//2015
	sched_new_env(env);

	return env->env_id;
}

//Place a new env into the READY queue
void sys_run_env(int32 envId)
{
	sched_run_env(envId);
}


//====================================
/*******************************/
/* ETC... SYSTEM CALLS */
/*******************************/

struct uint64 sys_get_virtual_time()
{
	struct uint64 t = get_virtual_time();
	return t;
}

uint32 sys_rcr2()
{
	return rcr2();
}
void sys_bypassPageFault(uint8 instrLength)
{
	bypassInstrLength = instrLength;
}





/**********************************/
/* DYNAMIC ALLOCATOR SYSTEM CALLS */
/**********************************/
/*2024*/
void* sys_sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #08] [2] USER HEAP - Block Allocator - sys_sbrk() [Kernel Side]
	//MS2: COMMENT THIS LINE BEFORE START CODING====
	return (void*)-1 ;
	//====================================================

	/*2023*/
	/* increment > 0: move the segment break of the current user program to increase the size of its heap,
	 * 				you should allocate NOTHING,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the current user program to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) As in real OS, allocate pages lazily. While sbrk moves the segment break, pages are not allocated
	 * 		until the user program actually tries to access data in its heap (i.e. will be allocated via the fault handler).
	 * 	4) Allocating additional pages for a process’ heap will fail if, for example, the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sys_sbrk fails, the net effect should
	 * 		be that sys_sbrk returns (void*) -1 and that the segment break and the process heap are unaffected.
	 * 		You might have to undo any operations you have done so far in this case.
	 */
	struct Env* env = curenv; //the current running Environment to adjust its break limit


}

/**************************************************************************/
/************************* SYSTEM CALLS HANDLER ***************************/
/**************************************************************************/
// Dispatches to the correct kernel function, passing the arguments.
uint32 syscall(uint32 syscallno, uint32 a1, uint32 a2, uint32 a3, uint32 a4, uint32 a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	switch(syscallno)
	{
	/*2023*/
	//TODO: [PROJECT'23.MS1 - #4] [2] SYSTEM CALLS - Add suitable code here

	//=====================================================================
	case SYS_cputs:
		sys_cputs((const char*)a1,a2,(uint8)a3);
		return 0;
		break;
	case SYS_cgetc:
		return sys_cgetc();
		break;
	case SYS_calc_req_frames:
		return sys_calculate_required_frames(a1, a2);
		break;
	case SYS_calc_free_frames:
		return sys_calculate_free_frames();
		break;
	case SYS_calc_modified_frames:
		return sys_calculate_modified_frames();
		break;
	case SYS_calc_notmod_frames:
		return sys_calculate_notmod_frames();
		break;

	case SYS_pf_calc_allocated_pages:
		return sys_pf_calculate_allocated_pages();
		break;
	case SYS_calculate_pages_tobe_removed_ready_exit:
		return sys_calculate_pages_tobe_removed_ready_exit(a1);
		break;
	case SYS_scarce_memory:
		sys_scarce_memory();
		return 0;
		break;
	case SYS_allocate_chunk_in_mem:
		sys_allocate_chunk(a1, (uint32)a2, a3);
		return 0;
		break;

		//======================
	case SYS_allocate_page:
		__sys_allocate_page((void*)a1, a2);
		return 0;
		break;
	case SYS_map_frame:
		__sys_map_frame(a1, (void*)a2, a3, (void*)a4, a5);
		return 0;
		break;
	case SYS_unmap_frame:
		__sys_unmap_frame(a1, (void*)a2);
		return 0;
		break;
		//	case SYS_allocateChunkInPageFile:
		//		//LOG_STATMENT(cprintf("KERNEL syscall: a2 %x\n", a2));
		//		sys_allocate_user_mem(a1, (uint32)a2);
		//		return 0;
		//		break;
		//2022

	case SYS_disableINTR:
		sys_disable_interrupt();
		return 0;
		break;

	case SYS_enableINTR:
		sys_enable_interrupt();
		return 0;
		break;

	case SYS_cputc:
		sys_cputc((const char)a1);
		return 0;
		break;

	case SYS_clearFFL:
		sys_clearFFL((const char)a1);
		return 0;
		break;

	case SYS_create_semaphore:
		return sys_createSemaphore((char*)a1, a2);
		break;

	case SYS_wait_semaphore:
		sys_waitSemaphore((int32)a1, (char*)a2);
		return 0;
		break;

	case SYS_signal_semaphore:
		sys_signalSemaphore((int32)a1, (char*)a2);
		return 0;
		break;

	case SYS_get_semaphore_value:
		return sys_getSemaphoreValue((int32)a1, (char*)a2);
		break;

	case SYS_create_shared_object:
		return sys_createSharedObject((char*)a1, a2, a3, (void*)a4);
		break;

	case SYS_get_shared_object:
		return sys_getSharedObject((int32)a1, (char*)a2, (void*)a3);
		break;

	case SYS_free_shared_object:
		return sys_freeSharedObject((int32)a1, (void *)a2);
		break;

	case SYS_get_size_of_shared_object:
		return sys_getSizeOfSharedObject((int32)a1, (char*)a2);
		break;

	case SYS_get_max_shares:
		return sys_getMaxShares();
		break;

	case SYS_create_env:
		return sys_create_env((char*)a1, (uint32)a2, (uint32)a3, (uint32)a4);
		break;

	case SYS_run_env:
		sys_run_env((int32)a1);
		return 0;
		break;
	case SYS_getenvindex:
		return sys_getenvindex();
		break;
	case SYS_getenvid:
		return sys_getenvid();
		break;
	case SYS_getparentenvid:
		return sys_getparentenvid();
		break;
	case SYS_destroy_env:
		return sys_destroy_env(a1);
		break;
	case SYS_exit_env:
		sys_exit_env();
		return 0;
		break;
	case SYS_get_virtual_time:
	{
		struct uint64 res = sys_get_virtual_time();
		uint32* ptrlow = ((uint32*)a1);
		uint32* ptrhi = ((uint32*)a2);
		*ptrlow = res.low;
		*ptrhi = res.hi;
		return 0;
		break;
	}
	case SYS_move_user_mem:
		sys_move_user_mem(a1, a2, a3);
		return 0;
		break;
	case SYS_rcr2:
		return sys_rcr2();
		break;
	case SYS_bypassPageFault:
		sys_bypassPageFault(a1);
		return 0;

	case SYS_rsttst:
		rsttst();
		return 0;
	case SYS_inctst:
		inctst();
		return 0;
	case SYS_chktst:
		chktst(a1);
		return 0;
	case SYS_gettst:
		return gettst();
	case SYS_testNum:
		tst(a1, a2, a3, (char)a4, a5);
		return 0;

	case SYS_get_heap_strategy:
		return sys_get_heap_strategy();

	case SYS_set_heap_strategy:
		sys_set_uheap_strategy(a1);
		return 0;

	case SYS_check_LRU_lists:
		return sys_check_LRU_lists((uint32*)a1, (uint32*)a2, (int)a3, (int)a4);

	case SYS_check_LRU_lists_free:
		return sys_check_LRU_lists_free((uint32*)a1, (int)a2);

	case SYS_check_WS_list:
		return sys_check_WS_list((uint32*)a1, (int)a2, (uint32)a3, (bool)a4);

	case NSYSCALLS:
		return 	-E_INVAL;
		break;
	}
	//panic("syscall not implemented");
	return -E_INVAL;
}
