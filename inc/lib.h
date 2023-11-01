// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef FOS_INC_LIB_H
#define FOS_INC_LIB_H 1

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <inc/environment_definitions.h>
#include <inc/memlayout.h>
#include <inc/syscall.h>
#include <inc/uheap.h>
#include <inc/ubuddy.h>
#include "dynamic_allocator.h"

#define USED(x)		(void)(x)
#define RAND(s,e)	((sys_get_virtual_time().low % (e-s) + s))

// libos.c or entry.S
extern volatile char *binaryname;
extern volatile struct Env *myEnv;
extern volatile struct Env envs[NENV];
extern volatile struct FrameInfo frames_info[];
void	__destroy(void);
void	exit(void);

/* readline.c */
void readline(const char *buf, char*);
void atomic_readline(const char *buf, char*);


/*syscall.c*/

//String
void	sys_cputs(const char *string, uint32 len, uint8 printProgName);
int	sys_cgetc(void);
void 	sys_cputc(const char c);

//User Env
int32	sys_getenvindex(void);
int32	sys_getenvid(void);
int32	sys_getparentenvid(void);
void 	sys_exit_env();
//2016. Edited @ 2018 @2020 to add secondlist size for LRU list approximation
int 	sys_create_env(char* programName, unsigned int page_WS_size,unsigned int LRU_second_list_size,unsigned int percent_WS_pages_to_remove);
int		sys_destroy_env(int32 envId);
void	sys_run_env(int32 envId);

//Memory
int 	__sys_allocate_page(void *va, int perm);
int 	__sys_map_frame(int32 srcenv, void *srcva, int32 dstenv, void *dstva, int perm);
int 	__sys_unmap_frame(int32 envid, void *va);
uint32 	sys_calculate_required_frames(uint32 start_virtual_address, uint32 size);
uint32 	sys_calculate_free_frames();
uint32 	sys_calculate_modified_frames();
uint32 	sys_calculate_notmod_frames();
int		sys_calculate_pages_tobe_removed_ready_exit(uint32 WS_or_MEMORY_flag);
void 	sys_scarce_memory();

/*2024*/ //Dynamic Allocator
void* 	sys_sbrk(int increment);
//******************

//User Heap
void 	sys_free_user_mem(uint32 virtual_address, uint32 size);
void	sys_allocate_user_mem(uint32 virtual_address, uint32 size);
void	sys_allocate_chunk(uint32 virtual_address, uint32 size, uint32 perms);
void 	sys_move_user_mem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size);
uint32 	sys_isUHeapPlacementStrategyFIRSTFIT();
uint32 	sys_isUHeapPlacementStrategyBESTFIT();
uint32 	sys_isUHeapPlacementStrategyNEXTFIT();
uint32 	sys_isUHeapPlacementStrategyWORSTFIT();
void 	sys_set_uheap_strategy(uint32 heapStrategy);

//Page File
int 	sys_pf_calculate_allocated_pages(void);

//Interrupt
void 	sys_disable_interrupt();
void 	sys_enable_interrupt();

//Semaphores
int 	sys_createSemaphore(char* semaphoreName, uint32 initialValue);
void 	sys_waitSemaphore(int32 ownerEnvID, char* semaphoreName);
void	sys_signalSemaphore(int32 ownerEnvID, char* semaphoreName);
int		sys_getSemaphoreValue(int32 ownerEnvID, char* semaphoreName);

//Sharing
//2017
int 	sys_createSharedObject(char* shareName, uint32 size, uint8 isWritable, void* virtual_address);
int 	sys_getSizeOfSharedObject(int32 ownerID, char* shareName);
int 	sys_getSharedObject(int32 ownerID, char* shareName, void* virtual_address );
int 	sys_freeSharedObject(int32 sharedObjectID, void *startVA);
uint32 	sys_getMaxShares();

//etc...
uint32	sys_rcr2();
void 	sys_bypassPageFault(uint8 instrLength);
void 	rsttst();
void 	inctst();
uint32 	gettst();
void 	chktst(uint32 n);
void 	tst(uint32 n, uint32 v1, uint32 v2, char c, int inv);
struct uint64 sys_get_virtual_time();
//2020
int 	sys_check_LRU_lists(uint32* active_list_content, uint32* second_list_content, int actual_active_list_size, int actual_second_list_size);
int 	sys_check_LRU_lists_free(uint32* list_content, int list_size);
//2023
int 	sys_check_WS_list(uint32* WS_list_content, int actual_WS_list_size, uint32 last_WS_element_content, bool chk_in_order);

/* console.c */
void	atomic_cputchar(int c);
void	atomic_cputchar(int c);
int	atomic_getchar(void);
int	atomic_getchar(void);

/* concurrency.c */
void env_sleep(uint32 apprxMilliSeconds);
uint32 busy_wait(uint32 loopMax);
#define CYCLES_PER_MILLISEC 10000

int	iscons(int fd);
int	opencons(void);


/* panic.c */
void CheckWSArrayWithoutLastIndex(uint32 *expectedPages, int arraySize);

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

#endif	// !FOS_INC_LIB_H
