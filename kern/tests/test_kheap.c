#include "test_kheap.h"

#include <inc/memlayout.h>
#include <inc/queue.h>
#include <inc/dynamic_allocator.h>
#include <kern/cpu/sched.h>
#include <kern/disk/pagefile_manager.h>
#include "../mem/kheap.h"
#include "../mem/memory_manager.h"


#define Mega  (1024*1024)
#define kilo (1024)

//2017
#define DYNAMIC_ALLOCATOR_DS 0 //ROUNDUP(NUM_OF_KHEAP_PAGES * sizeof(struct MemBlock), PAGE_SIZE)
#define INITIAL_KHEAP_ALLOCATIONS (DYNAMIC_ALLOCATOR_DS + KERNEL_SHARES_ARR_INIT_SIZE + KERNEL_SEMAPHORES_ARR_INIT_SIZE) // + ROUNDUP(num_of_ready_queues * sizeof(uint8), PAGE_SIZE) + ROUNDUP(num_of_ready_queues * sizeof(struct Env_Queue), PAGE_SIZE))
#define ACTUAL_START ((KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE) + INITIAL_KHEAP_ALLOCATIONS)

extern uint32 sys_calculate_free_frames() ;
extern void sys_bypassPageFault(uint8);
extern uint32 sys_rcr2();
extern int execute_command(char *command_string);

extern char end_of_kernel[];

extern int CB(uint32 *ptr_dir, uint32 va, int bn);

struct MyStruct
{
	char a;
	short b;
	int c;
};

uint32 da_limit = KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE ;
int test_kmalloc()
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE FOR SOME
	 * IMPLEMENTATIONS TO DYNAMICALLY ALLOCATE SPECIAL DATA
	 * STRUCTURE TO MANAGE THE PAGE ALLOCATOR.
	 *********************************************************/

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2, *byteArr3 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfByte3, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;
	int eval = 0;
	bool correct = 1 ;
	int freeFrames, freeDiskFrames;
	uint32 sizeOfKHeap;
	void* ptr_allocations[20] = {0};
	correct = 1 ;
	{
		//Insufficient space
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		sizeOfKHeap = (KERNEL_HEAP_MAX - ACTUAL_START + 1) ;
		ptr_allocations[0] = kmalloc(sizeOfKHeap);
		if (ptr_allocations[0] != NULL) { correct = 0; cprintf("Allocating insufficient space: should return NULL\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//2 KB - 1 (should be allocated by dynamic allocator not page allocator)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*kilo-1);
		if ((uint32) ptr_allocations[2] < KERNEL_HEAP_START || ptr_allocations[2] >= sbrk(0) || (uint32) ptr_allocations[2] >= da_limit)
			{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//2 KB - 1 (should be allocated by dynamic allocator not page allocator)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo-1);
		if ((uint32) ptr_allocations[3] < KERNEL_HEAP_START || ptr_allocations[3] >= sbrk(0) || (uint32) ptr_allocations[3] >= da_limit)
			{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		//if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega /*+ 8*kilo*/)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 2) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 8*kilo) ) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(6*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 1536) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(14*kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 13*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 4) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
	}
	if (correct)	eval+=40 ;

	correct = 1 ;
	//Checking read/write on the allocated spaces
	{

			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;

			//Write values
			//In 1st 2 MB
			lastIndexOfByte = (2*Mega-kilo)/sizeof(char) - 1;
			byteArr = (char *) ptr_allocations[0];
			byteArr[0] = minByte ;
			byteArr[lastIndexOfByte] = maxByte ;

			//In 2nd 2 MB
			shortArr = (short *) ptr_allocations[1];
			lastIndexOfShort = (2*Mega-kilo)/sizeof(short) - 1;
			shortArr[0] = minShort;
			shortArr[lastIndexOfShort] = maxShort;

			//In Dynamic Allocator Area
			{
				//In 2 KB - 1
				intArr = (int *) ptr_allocations[2];
				lastIndexOfInt = (2*kilo-1)/sizeof(int) - 1;
				intArr[0] = minInt;
				intArr[lastIndexOfInt] = maxInt;

				//In 2 KB - 1
				byteArr2 = (char *) ptr_allocations[3];
				lastIndexOfByte2 = (2*kilo-1)/sizeof(char) - 1;
				byteArr2[0] = minByte;
				byteArr2[lastIndexOfByte2] = maxByte;
			}

			//In 7 KB
			structArr = (struct MyStruct *) ptr_allocations[4];
			lastIndexOfStruct = (7*kilo)/sizeof(struct MyStruct) - 1;
			structArr[0].a = minByte; structArr[0].b = minShort; structArr[0].c = minInt;
			structArr[lastIndexOfStruct].a = maxByte; structArr[lastIndexOfStruct].b = maxShort; structArr[lastIndexOfStruct].c = maxInt;

			//In 6 MB
			lastIndexOfByte3 = (6*Mega-kilo)/sizeof(char) - 1;
			byteArr3 = (char *) ptr_allocations[6];
			byteArr3[0] = minByte ;
			byteArr3[lastIndexOfByte3 / 2] = maxByte / 2;
			byteArr3[lastIndexOfByte3] = maxByte ;

			//In 14 KB
			shortArr2 = (short *) ptr_allocations[7];
			lastIndexOfShort2 = (14*kilo)/sizeof(short) - 1;
			shortArr2[0] = minShort;
			shortArr2[lastIndexOfShort2] = maxShort;

			//Read values: check that the values are successfully written
			if (byteArr[0] 	!= minByte 	|| byteArr[lastIndexOfByte] 	!= maxByte) { correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }
			if (shortArr[0] != minShort || shortArr[lastIndexOfShort] 	!= maxShort) { correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }
			if (intArr[0] 	!= minInt 	|| intArr[lastIndexOfInt] 		!= maxInt) { correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }
			if (byteArr2[0] != minByte || byteArr2[lastIndexOfByte2] != maxByte) { correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }

			if (structArr[0].a != minByte 	|| structArr[lastIndexOfStruct].a != maxByte) 	{ correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }
			if (structArr[0].b != minShort 	|| structArr[lastIndexOfStruct].b != maxShort) 	{ correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }
			if (structArr[0].c != minInt 	|| structArr[lastIndexOfStruct].c != maxInt) 	{ correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }

			if (byteArr3[0] != minByte || byteArr3[lastIndexOfByte3/2] != maxByte/2 || byteArr3[lastIndexOfByte3] != maxByte) { correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }
			if (shortArr2[0] != minShort || shortArr2[lastIndexOfShort2] != maxShort) { correct = 0; cprintf("Wrong allocation: stored values are wrongly changed!\n"); }

			if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }

	}
	if (correct)	eval+=30 ;

	correct = 1 ;
	//Insufficient space again
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		uint32 restOfKHeap = (KERNEL_HEAP_MAX - ACTUAL_START + 2*PAGE_SIZE) - (2*Mega+2*Mega+/*4*kilo+4*kilo+*/8*kilo+3*Mega+6*Mega+16*kilo) ;
		ptr_allocations[8] = kmalloc(restOfKHeap);
		if (ptr_allocations[8] != NULL) { correct = 0; cprintf("Allocating insufficient space: should return NULL\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//permissions
	{
		uint32 lastAllocAddress = (uint32)ptr_allocations[7] + 16*kilo ;
		uint32 va;
		for (va = ACTUAL_START; va < lastAllocAddress; va+=PAGE_SIZE)
		{
			unsigned int * table;
			get_page_table(ptr_page_directory, va, &table);
			uint32 perm = table[PTX(va)] & 0xFFF;
			if ((perm & PERM_USER) == PERM_USER)
			{
				if (correct)
				{
					correct = 0; cprintf("Wrong permissions: pages should be mapped with Supervisor permission only\n");
				}
			}
		}
	}
	if (correct)	eval+=10 ;

	cprintf("\ntest kmalloc completed. Evaluation = %d%\n", eval);

	return 1;

}


int test_kmalloc_firstfit1()
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE FOR SOME
	 * IMPLEMENTATIONS TO DYNAMICALLY ALLOCATE SPECIAL DATA
	 * STRUCTURE TO MANAGE THE PAGE ALLOCATOR.
	 *********************************************************/

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	void* ptr_allocations[20] = {0};
	uint32 freeFrames;
	uint32 freeDiskFrames;
	int eval = 0;
	bool correct = 1 ;

	correct = 1 ;
	//[1] Allocate all
	{
		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[0] != (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 1*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 2*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 3*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 6*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[6] !=  (ACTUAL_START + 8*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 11*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: \n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//[2] Free some to create holes
	{
		//1 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 256) { correct = 0; cprintf("Wrong free: \n"); }

		//2 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[4]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512) { correct = 0; cprintf("Wrong free: \n"); }

		//3 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 768) { correct = 0; cprintf("Wrong free: \n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//[3] Allocate again [test first fit]
	{
		//Allocate 512 KB - should be placed in 1st hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(512*kilo - kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 1*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 128) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 1 MB - should be placed in 2nd hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(1*Mega - kilo);
		if ((uint32) ptr_allocations[9] != (ACTUAL_START + 4*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: \n"); }


		//Allocate 256 KB - should be placed in remaining of 1st hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(256*kilo - kilo);
		if ((uint32) ptr_allocations[10] != (ACTUAL_START + 1*Mega + 512*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 64) { correct = 0; cprintf("Wrong allocation: \n"); }

		//Allocate 2 MB - should be placed in 3rd hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[11] = kmalloc(2*Mega);
		if ((uint32) ptr_allocations[11] != (ACTUAL_START + 8*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }


		//Allocate 4 MB - should be placed in end of all allocations
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[12] = kmalloc(4*Mega - kilo);
		if ((uint32) ptr_allocations[12] != (ACTUAL_START + 14*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 1024) { correct = 0; cprintf("Wrong allocation: \n"); }
	}
	if (correct)	eval+=40 ;

	correct = 1 ;
	//[4] Free contiguous allocations
	{
		//1 MB Hole appended to previous 256 KB hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 256) { correct = 0; cprintf("Wrong free: \n"); }

		//Next 1 MB Hole appended also
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[3]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 256) { correct = 0; cprintf("Wrong free: \n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//[5] Allocate again [test first fit]
	{
		//[FIRST FIT Case]
		//Allocate 1 MB - should be placed in the contiguous hole (256 KB + 2 MB)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[13] = kmalloc(1*Mega);
		if ((uint32) ptr_allocations[13] != (ACTUAL_START + 1*Mega + 768*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: \n"); }
	}
	if (correct)	eval+=30 ;

	cprintf("test FIRST FIT allocation (1) completed. Eval = %d%\n", eval);

	return 1;
}

int test_kmalloc_firstfit2()
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE FOR SOME
	 * IMPLEMENTATIONS TO DYNAMICALLY ALLOCATE SPECIAL DATA
	 * STRUCTURE TO MANAGE THE PAGE ALLOCATOR.
	 *********************************************************/

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	void* ptr_allocations[20] = {0};
	uint32 freeFrames;
	uint32 freeDiskFrames;
	int eval = 0;
	bool correct = 1 ;

	correct = 1 ;
	//[1] Attempt to allocate more than heap size
	{
		ptr_allocations[0] = kmalloc(KERNEL_HEAP_MAX - ACTUAL_START + 1);
		if (ptr_allocations[0] != NULL) { correct = 0; cprintf("kmalloc: Attempt to allocate more than heap size, should return NULL\n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//[2] Attempt to allocate space more than any available fragment
	//	a) Create Fragments
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] != (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: \n"); }

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: \n"); }

		//1 KB (should be allocated by dynamic allocator not page allocator)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(1*kilo);
		if ((uint32) ptr_allocations[2] < KERNEL_HEAP_START || ptr_allocations[2] >= sbrk(0) || (uint32) ptr_allocations[2] >= da_limit)
			{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//2 KB (should be allocated by dynamic allocator not page allocator)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] < KERNEL_HEAP_START || ptr_allocations[3] >= sbrk(0) || (uint32) ptr_allocations[3] >= da_limit)
			{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//1 KB (should be allocated by dynamic allocator not page allocator)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(1*kilo);
		if ((uint32) ptr_allocations[4] < KERNEL_HEAP_START || ptr_allocations[4] >= sbrk(0) || (uint32) ptr_allocations[4] >= da_limit)
			{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//1 KB Hole in Dynamic Allocator Area
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong free: freeing a block from the dynamic allocator should not affect the free frames\n"); }

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega /*+ 8*kilo*/)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 2) { correct = 0; cprintf("Wrong allocation: \n"); }

		//2 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512) { correct = 0; cprintf("Wrong free: \n"); }

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 4*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) <  3*Mega/PAGE_SIZE) { correct = 0; cprintf("Wrong allocation: \n"); }

		//2 MB + 6 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(2*Mega + 6*kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 7*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) <  514) { correct = 0; cprintf("Wrong allocation: \n"); }

		//3 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 768) { correct = 0; cprintf("Wrong free: \n"); }

		//2 KB Hole in Dynamic Allocator Area [Resulting Hole = 1 KB + 2 KB = 3 KB]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[3]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong free: freeing a block from the dynamic allocator should not affect the free frames\n"); }

		//2 MB Hole [Resulting Hole = 2 MB + 2 MB = 4 MB]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((sys_calculate_free_frames() - freeFrames) < 512) { correct = 0; cprintf("Wrong free: \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }

		//5 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(5*Mega-kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 9*Mega + 16*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) <   5*Mega/PAGE_SIZE) { correct = 0; cprintf("Wrong allocation: \n"); }

		//8 KB Hole [Resulting Hole = 2 MB + 2 MB + 8 KB + 3 MB = 7 MB + 8 KB]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[5]);
		if((pf_calculate_free_frames() - freeDiskFrames) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 2) { correct = 0; cprintf("Wrong free: \n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	{
		//[FIRST FIT Case#1] Should be allocated in the resulting hole inside Page Allocator Area
		//7 MB + 1 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(7*Mega+kilo);
		if ((uint32) ptr_allocations[9] != (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((freeDiskFrames - pf_calculate_free_frames()) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) <  (7*Mega+4*kilo)/PAGE_SIZE) { correct = 0; cprintf("Wrong allocation: \n"); }

		//[FIRST FIT Case#2] Should be allocated in the remaining area of resulting hole inside Page Allocator Area
		//3 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(3*kilo);
		if ((uint32)ptr_allocations[10] != (ACTUAL_START + 7*Mega + 4*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((freeDiskFrames - pf_calculate_free_frames()) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 1) { correct = 0; cprintf("Wrong allocation: \n"); }
	}
	if (correct)	eval+=35 ;

	correct = 1 ;
	{
		//[FIRST FIT Case#3] Should be allocated in the resulting hole inside DYNAMIC Allocator Area
		//1 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[11] = kmalloc(1*kilo);
		if ((ptr_allocations[11] < ptr_allocations[2]) || (ptr_allocations[11] > (ptr_allocations[2] + 1*kilo)))
			{ correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((freeDiskFrames - pf_calculate_free_frames()) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: \n"); }

		//[FIRST FIT Case#4] Should be allocated in the remaining of resulting hole inside DYNAMIC Allocator Area
		//1 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[12] = kmalloc(1*kilo);
		if ((ptr_allocations[12] < ptr_allocations[2] + 1*kilo) || (ptr_allocations[12] > (ptr_allocations[2] + 2*kilo)))
			{ correct = 0; cprintf("Wrong start address for the allocated space... \n"); }
		if((freeDiskFrames - pf_calculate_free_frames()) !=  0)  { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: \n"); }

	}
	if (correct)	eval+=35 ;

	correct = 1 ;
	//	b) Attempt to allocate large segment with no suitable fragment to fit on
	{
		//Large Allocation
		ptr_allocations[13] = kmalloc((KERNEL_HEAP_MAX - ACTUAL_START - 14*Mega));
		if (ptr_allocations[13] != NULL) { correct = 0; cprintf("Kmalloc: Attempt to allocate large segment with no suitable fragment to fit on, should return NULL\n"); }

	}
	if (correct)	eval+=10 ;

	cprintf("test FIRST FIT allocation (2) completed. Eval = %d%\n", eval);

	return 1;
}


int test_kfree_bestfirstfit()
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE FOR SOME
	 * IMPLEMENTATIONS TO DYNAMICALLY ALLOCATE SPECIAL DATA
	 * STRUCTURE TO MANAGE THE PAGE ALLOCATOR.
	 *********************************************************/

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;
	//	void* expected;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};
	int sums[20] = {0};

	int eval = 0;
	bool correct = 1;

	correct = 1;
	void* ptr_allocations[20] = {0};
	{
		//[BLOCK ALLOCATOR]
		{
			//2 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[2] = kmalloc(2*kilo);
			if ((uint32) ptr_allocations[2] < KERNEL_HEAP_START || ptr_allocations[2] >= sbrk(0) || (uint32) ptr_allocations[2] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			//		if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[2] = (2*kilo)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[2];
			for (i = 0; i < lastIndices[2]; ++i)
			{
				ptr[i] = 2 ;
			}

			//2 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[3] = kmalloc(2*kilo);
			if ((uint32) ptr_allocations[3] < KERNEL_HEAP_START || ptr_allocations[3] >= sbrk(0) || (uint32) ptr_allocations[3] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			//		if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[3] = (2*kilo)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[3];
			for (i = 0; i < lastIndices[3]; ++i)
			{
				ptr[i] = 3 ;
			}
		}

		//[PAGE ALLOCATOR]
		{
			//2 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[0] = kmalloc(2*Mega-kilo);
			if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[0] = (2*Mega-kilo)/sizeof(char) - 1;

			//2 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[1] = kmalloc(2*Mega-kilo);
			if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[1] = (2*Mega-kilo)/sizeof(char) - 1;


			//7 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[4] = kmalloc(7*kilo);
			if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega /* + 8*kilo*/)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			if ((freeFrames - sys_calculate_free_frames()) < 2) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[4] = (7*kilo)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[4];
			for (i = 0; i < lastIndices[4]; ++i)
			{
				ptr[i] = 4 ;
			}

			//3 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[5] = kmalloc(3*Mega-kilo);
			if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 8*kilo) ) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[5] = (3*Mega-kilo)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[5];
			for (i = 0; i < lastIndices[5]; ++i)
			{
				ptr[i] = 5 ;
			}

			//6 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[6] = kmalloc(6*Mega-kilo);
			if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			if ((freeFrames - sys_calculate_free_frames()) < 1536) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[6] = (6*Mega-kilo)/sizeof(char) - 1;

			//14 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[7] = kmalloc(14*kilo);
			if ((uint32) ptr_allocations[7] != (ACTUAL_START + 13*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			if ((freeFrames - sys_calculate_free_frames()) < 4) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
			lastIndices[7] = (14*kilo)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[7];
			for (i = 0; i < lastIndices[7]; ++i)
			{
				ptr[i] = 7 ;
			}
		}
	}

	//kfree some of the allocated spaces [10%]
	{
		//kfree 1st 2 MB
		int freeFrames = sys_calculate_free_frames() ;
		int freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512 ) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 1st 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) != 0 ) { correct = 0; cprintf("Wrong free: freeing a block from the dynamic allocator should not affect the free frames\n"); }

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 6*Mega/4096) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//Check memory access after kfree [10%]
	{
		//2 KB
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			sums[3] += ptr[i] ;
		}
		if (sums[3] != 3*lastIndices[3])	{ correct = 0; cprintf("kfree: invalid read after freeing some allocations\n"); }

		//7 KB
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			sums[4] += ptr[i] ;
		}
		if (sums[4] != 4*lastIndices[4])	{ correct = 0; cprintf("kfree: invalid read after freeing some allocations\n"); }

		//3 MB
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			sums[5] += ptr[i] ;
		}
		if (sums[5] != 5*lastIndices[5])	{ correct = 0; cprintf("kfree: invalid read after freeing some allocations\n"); }

		//14 KB
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			sums[7] += ptr[i] ;
		}
		if (sums[7] != 7*lastIndices[7])	{ correct = 0; cprintf("kfree: invalid read after freeing some allocations\n"); }
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//Allocate after kfree [15%]
	{
		//Allocate in merged freed space
		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(3*Mega);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
		lastIndices[8] = (3*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[8];
		for (i = 0; i < lastIndices[8]; ++i)
		{
			ptr[i] = 8 ;
		}

		//1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(1*Mega);
		if ((uint32) ptr_allocations[10] != (ACTUAL_START + 3*Mega /*+ 4*kilo*/)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 256) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
		lastIndices[10] = (1*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[10];
		for (i = 0; i < lastIndices[10]; ++i)
		{
			ptr[i] = 10 ;
		}

		//1 KB [Should be allocated in 1st hole in the Dynamic Allocator]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(1*kilo);
		if ((ptr_allocations[9] < ptr_allocations[2]) || (ptr_allocations[9] > (ptr_allocations[2] + 1*kilo)))
			{ correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: it's allocated in a previously allocated block. Should not allocate any pages from physical memory\n"); }
		lastIndices[9] = (1*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[9];
		for (i = 0; i < lastIndices[9]; ++i)
		{
			ptr[i] = 9 ;
		}

	}
	if (correct)	eval+=15 ;

	correct = 1 ;
	//kfree remaining allocated spaces [15%]
	{
		//kfree 3 MB [PAGE ALLOCATOR: Should be Merged with NEXT 6 MB hole - total = 9MB]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[5]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 3*Mega/4096) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 7 KB [PAGE ALLOCATOR: Should be Merged with NEXT 9 MB hole - total = 9MB + 8KB]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[4]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 2) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 1 KB [DYNAMIC ALLOCATOR]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[9]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 2nd 2 KB [DYNAMIC ALLOCATOR: Should be Merged with PREV remaining area of 2KB & NEXT free space]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[3]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong free: freeing a block from the dynamic allocator should not affect the free frames\n"); }

		//kfree 14 KB [PAGE ALLOCATOR: Should be Merged with PREV 9MB + 8KB hole - total = 9MB + 24KB]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[7]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 4) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 1 MB [PAGE ALLOCATOR: Should be Merged with NEXT remaining hole ]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[10]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 1*Mega/4096) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 3 MB [PAGE ALLOCATOR: Should be Merged with PREV 9MB + 24KB hole & NEXT remaining hole - total = ALL PAGE ALLOCATOR Space]
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 3*Mega/4096) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//				if(start_freeFrames != (sys_calculate_free_frames())) {{ correct = 0; cprintf("Wrong kfree: not all pages removed correctly at end\n"); }}
	}
	if (correct)	eval+=15 ;

	correct = 1 ;
	//Check memory access after kfree [15%]
	{
		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		for (i = 0; i <= 10; ++i)
		{
			//SKIP CHECKING THOSE IN DYNAMIC ALLOCATOR AREA
			if (i == 2 || i == 3 || i == 9)
			{
				continue;
			}
			ptr = (char *) ptr_allocations[i];
			ptr[0] = 10;
			//cprintf("\n\ncr2 = %x, faulted addr = %x", sys_rcr2(), (uint32)&(ptr[0]));
			if (sys_rcr2() != (uint32)&(ptr[0]))
				if (correct)
				{ correct = 0; cprintf("kfree: successful access to freed space!! it should not be succeeded\n"); }
			ptr[lastIndices[i]] = 10;
			if (sys_rcr2() != (uint32)&(ptr[lastIndices[i]]))
				if (correct)
				{ correct = 0; cprintf("kfree: successful access to freed space!! it should not be succeeded\n"); }
		}

		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);
	}
	if (correct)	eval+=15 ;

	correct = 1 ;

	//	//kfree non-exist item [10%]
	//	{
	//		//kfree 2 MB
	//		freeFrames = sys_calculate_free_frames() ;
	//		freeDiskFrames = pf_calculate_free_frames() ;
	//		kfree(ptr_allocations[0]);
	//		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
	//		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing\n"); }
	//
	//		//kfree 2 KB
	//		freeFrames = sys_calculate_free_frames() ;
	//		freeDiskFrames = pf_calculate_free_frames() ;
	//		kfree(ptr_allocations[2]);
	//		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
	//		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing\n"); }
	//
	//		//kfree 20 KB
	//		freeFrames = sys_calculate_free_frames() ;
	//		freeDiskFrames = pf_calculate_free_frames() ;
	//		kfree(ptr_allocations[8]);
	//		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
	//		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing\n"); }
	//
	//		//kfree 1 MB
	//		freeFrames = sys_calculate_free_frames() ;
	//		freeDiskFrames = pf_calculate_free_frames() ;
	//		kfree(ptr_allocations[9]);
	//		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
	//		if ((sys_calculate_free_frames() - freeFrames) != 0) { correct = 0; cprintf("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing\n"); }
	//
	//	}
	//	cprintf("\b\b\b75%\n"); }

	//Allocate after kfree ALL [30%]
	{
		//[DYNAMIC ALLOCATOR] Allocate in merged freed space
		//1 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[11] = kmalloc(1*kilo);
		if ((ptr_allocations[11] < ptr_allocations[2]) || (ptr_allocations[11] > (ptr_allocations[2] + 1*kilo)))
			{ correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: it's allocated in a previously allocated block. Should not allocate any pages from physical memory\n"); }
		lastIndices[11] = (1*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[11];
		for (i = 0; i < lastIndices[11]; ++i)
		{
			ptr[i] = 11 ;
		}

		//[DYNAMIC ALLOCATOR] Allocate in merged freed space
		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[12] = kmalloc(2*kilo);
		//expected = ptr_allocations[2] + 1*kilo + sizeOfMetaData();
		//if (ptr_allocations[12] != expected)
		if ((ptr_allocations[12] < ptr_allocations[2] + 1*kilo) || (ptr_allocations[12] > (ptr_allocations[2] + 2*kilo)))
		{
			correct = 0;
			cprintf("Wrong start address for the allocated space... check return address of kmalloc. Expected [%x, %x], Actual %x\n", (ptr_allocations[2] + 1*kilo), (ptr_allocations[2] + 2*kilo), ptr_allocations[12]);
		}
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: it's allocated in a previously allocated block. Should not allocate any pages from physical memory\n"); }
		lastIndices[12] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[12];
		for (i = 0; i < lastIndices[12]; ++i)
		{
			ptr[i] = 12 ;
		}

		//[DYNAMIC ALLOCATOR] Allocate in merged freed space
		//1.5 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[13] = kmalloc(3*kilo/2);
		//if (ptr_allocations[13] != ptr_allocations[12] + 2*kilo + sizeOfMetaData())
		if ((ptr_allocations[13] < ptr_allocations[2] + 3*kilo) || (ptr_allocations[13] > (ptr_allocations[2] + 4*kilo)))
			{ correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) != 0) { correct = 0; cprintf("Wrong allocation: it's allocated in a previously allocated block. Should not allocate any pages from physical memory\n"); }
		lastIndices[13] = (3*kilo/2)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[13];
		for (i = 0; i < lastIndices[13]; ++i)
		{
			ptr[i] = 13 ;
		}

		//[PAGE ALLOCATOR] Allocate in merged freed space
		//30 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(30*Mega);
		if ((uint32) ptr_allocations[10] != (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 30*Mega/PAGE_SIZE) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
		lastIndices[10] = (30*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[10];
		for (i = 0; i < lastIndices[10]; ++i)
		{
			ptr[i] = 10 ;
		}


		//30 MB
		ptr = (char*)ptr_allocations[10];
		for (i = 0; i < lastIndices[10]; ++i)
		{
			sums[10] += ptr[i] ;
		}
		if (sums[10] != 10*lastIndices[10])	{ correct = 0; cprintf("kfree: invalid read - data is corrupted\n"); }

		//1 KB
		ptr = (char*)ptr_allocations[11];
		for (i = 0; i < lastIndices[11]; ++i)
		{
			sums[11] += ptr[i] ;
		}
		if (sums[11] != 11*lastIndices[11])	{ correct = 0; cprintf("kfree: invalid read - data is corrupted\n"); }

		//2 KB
		ptr = (char*)ptr_allocations[12];
		for (i = 0; i < lastIndices[12]; ++i)
		{
			sums[12] += ptr[i] ;
		}
		if (sums[12] != 12*lastIndices[12])	{ correct = 0; cprintf("kfree: invalid read - data is corrupted\n"); }

		//1.5 KB
		ptr = (char*)ptr_allocations[13];
		for (i = 0; i < lastIndices[13]; ++i)
		{
			sums[13] += ptr[i] ;
		}
		if (sums[13] != 13*lastIndices[13])	{ correct = 0; cprintf("kfree: invalid read - data is corrupted\n"); }
	}
	if (correct)	eval+=30 ;

	correct = 1 ;
	//check tables	[5%]
	{
		long long va;
		for (va = KERNEL_HEAP_START; va < (long long)KERNEL_HEAP_MAX; va+=PTSIZE)
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, (uint32)va, &ptr_table);
			if (ptr_table == NULL)
			{
				if (correct)
				{ correct = 0; cprintf("Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree\n"); }
			}
		}
	}
	if (correct)	eval+=5 ;

	cprintf("\ntest kfree completed. Eval = %d%\n", eval);

	return 1;

}

int test_kheap_phys_addr()
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE FOR SOME
	 * IMPLEMENTATIONS TO DYNAMICALLY ALLOCATE SPECIAL DATA
	 * STRUCTURE TO MANAGE THE PAGE ALLOCATOR.
	 *********************************************************/

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};
	int sums[20] = {0};
	int eval = 0;
	bool correct = 1;
	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//[DYNAMIC ALLOCATOR]
		{
			//1 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[2] = kmalloc(1*kilo);
			if ((uint32) ptr_allocations[2] < KERNEL_HEAP_START || ptr_allocations[2] >= sbrk(0) || (uint32) ptr_allocations[2] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

			//2 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[3] = kmalloc(2*kilo);
			if ((uint32) ptr_allocations[3] < KERNEL_HEAP_START || ptr_allocations[3] >= sbrk(0) || (uint32) ptr_allocations[3] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

			//1.5 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[4] = kmalloc(3*kilo/2);
			if ((uint32) ptr_allocations[4] < KERNEL_HEAP_START || ptr_allocations[4] >= sbrk(0) || (uint32) ptr_allocations[4] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega /*+ 8*kilo*/)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 2) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 4*Mega + 8*kilo) ) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(6*Mega-kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 7*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 1536) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(14*kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 13*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 4) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
	}

	//[PAGE ALLOCATOR] test kheap_physical_address after kmalloc only [30%]
	{
		uint32 va;
		uint32 endVA = ACTUAL_START + 13*Mega + 24*kilo;
		uint32 allPAs[(13*Mega + 24*kilo + INITIAL_KHEAP_ALLOCATIONS)/PAGE_SIZE] ;
		i = 0;
		uint32 offset = 1;
		uint32 startVA = da_limit + PAGE_SIZE;
		for (va = startVA; va < endVA; va+=PAGE_SIZE+offset)
		{
			allPAs[i++] = kheap_physical_address(va);
		}
		int ii = i ;
		i = 0;
		int j;
		for (va = startVA; va < endVA; )
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, va, &ptr_table);
			if (ptr_table == NULL)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }

			for (j = PTX(va); i < ii && j < 1024 && va < endVA; ++j, ++i)
			{
				if (((ptr_table[j] & 0xFFFFF000)+(va & 0x00000FFF))!= allPAs[i])
				{
					//cprintf("\nVA = %x, table entry = %x, khep_pa = %x\n",va + j*PAGE_SIZE, (ptr_table[j] & 0xFFFFF000) , allPAs[i]);
					if (correct)
					{ correct = 0; cprintf("Wrong kheap_physical_address\n"); }
				}
				va+=PAGE_SIZE+offset;
			}
		}
	}
	if (correct)	eval+=30 ;

	correct = 1 ;
	//[DYNAMIC ALLOCATOR] test kheap_physical_address after kmalloc only [10%]
	{
		int i;
		uint32 va, pa;
		for (i = 2; i <= 4; i++)
		{
			va = (uint32)ptr_allocations[i];
			pa = kheap_physical_address(va);
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, va, &ptr_table);
			if (ptr_table == NULL)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }

			if (((ptr_table[PTX(va)] & 0xFFFFF000)+(va & 0x00000FFF))!= pa)
			{
				//cprintf("\nVA = %x, table entry = %x, khep_pa = %x\n",va + j*PAGE_SIZE, (ptr_table[j] & 0xFFFFF000) , allPAs[i]);
				if (correct)
				{ correct = 0; cprintf("Wrong kheap_physical_address\n"); }
			}
		}
	}
	if (correct)	eval+=10 ;

	correct = 1 ;
	//kfree some of the allocated spaces
	{
		//kfree 1st 2 MB
		int freeFrames = sys_calculate_free_frames() ;
		int freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512 ) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[7]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 6*Mega/4096) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }
	}

	//[PAGE ALLOCATOR] test kheap_physical_address after kmalloc and kfree [20%]
	{
		uint32 va;
		uint32 endVA = ACTUAL_START + 13*Mega + 24*kilo;
		uint32 allPAs[(13*Mega + 24*kilo + INITIAL_KHEAP_ALLOCATIONS)/PAGE_SIZE] ;
		i = 0;
		uint32 startVA = da_limit + PAGE_SIZE;

		for (va = startVA; va < endVA; va+=PAGE_SIZE)
		{
			allPAs[i++] = kheap_physical_address(va);
		}
		int ii = i ;
		i = 0;
		int j;
		for (va = startVA; va < endVA; )
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, va, &ptr_table);
			if (ptr_table == NULL)
				if (correct)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }

			for (j = PTX(va); i < ii && j < 1024 && va < endVA; ++j, ++i)
			{
				if (((ptr_table[j] & 0xFFFFF000)+((ptr_table[j] & PERM_PRESENT) == 0? 0 : va & 0x00000FFF)) != allPAs[i])
				{
					//cprintf("\nVA = %x, table entry = %x, khep_pa = %x\n",va + j*PAGE_SIZE, (ptr_table[j] & 0xFFFFF000) , allPAs[i]);
					if (correct)
					{ correct = 0; cprintf("Wrong kheap_physical_address\n"); }
				}
				va += PAGE_SIZE;
			}
		}
	}
	if (correct)	eval+=20 ;

	correct = 1 ;
	//[DYNAMIC ALLOCATOR] test kheap_physical_address on the entire allocated area [30%]
	{
		uint32 va, pa;
		for (va = KERNEL_HEAP_START; va < (uint32)sbrk(0); va++)
		{
			pa = kheap_physical_address(va);
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, va, &ptr_table);
			if (ptr_table == NULL)
				if (correct)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }

			if (((ptr_table[PTX(va)] & 0xFFFFF000)+(va & 0x00000FFF))!= pa)
			{
				//cprintf("\nVA = %x, table entry = %x, khep_pa = %x\n",va + j*PAGE_SIZE, (ptr_table[j] & 0xFFFFF000) , allPAs[i]);
				if (correct)
				{ correct = 0; cprintf("Wrong kheap_physical_address\n"); }
			}
		}
	}
	if (correct)	eval+=30 ;

	correct = 1 ;
	//test kheap_physical_address on non-mapped area [10%]
	{
		uint32 va;
		uint32 startVA = ACTUAL_START + 16*Mega;
		i = 0;
		for (va = startVA; va < KERNEL_HEAP_MAX; va+=PAGE_SIZE)
		{
			i++;
		}
		int ii = i ;
		i = 0;
		int j;
		long long va2;
		for (va2 = startVA; va2 < (long long)KERNEL_HEAP_MAX; va2+=PTSIZE)
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, (uint32)va2, &ptr_table);
			if (ptr_table == NULL)
			{
				if (correct)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }
			}
			for (j = 0; i < ii && j < 1024; ++j, ++i)
			{
				//if ((ptr_table[j] & 0xFFFFF000) != allPAs[i])
				unsigned int page_va = startVA+i*PAGE_SIZE;
				unsigned int supposed_kheap_phys_add = kheap_physical_address(page_va);
				if (((ptr_table[j] & 0xFFFFF000)+((ptr_table[j] & PERM_PRESENT) == 0? 0 : page_va & 0x00000FFF)) != supposed_kheap_phys_add)
				{
					//cprintf("\nVA = %x, table entry = %x, khep_pa = %x\n",va2 + j*PAGE_SIZE, (ptr_table[j] & 0xFFFFF000) , allPAs[i]);
					if (correct)
					{ correct = 0; cprintf("Wrong kheap_physical_address\n"); }
				}
			}
		}
	}
	if (correct)	eval+=10 ;

	cprintf("\ntest kheap_physical_address completed. Eval = %d%\n", eval);

	return 1;

}

int test_kheap_virt_addr()
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE FOR SOME
	 * IMPLEMENTATIONS TO DYNAMICALLY ALLOCATE SPECIAL DATA
	 * STRUCTURE TO MANAGE THE PAGE ALLOCATOR.
	 *********************************************************/

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};
	int sums[20] = {0};

	int eval = 0;
	bool correct = 1;

	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 512) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//[DYNAMIC ALLOCATOR]
		{
			//1 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[2] = kmalloc(1*kilo);
			if ((uint32) ptr_allocations[2] < KERNEL_HEAP_START || ptr_allocations[2] >= sbrk(0) || (uint32) ptr_allocations[2] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

			//2 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[3] = kmalloc(2*kilo);
			if ((uint32) ptr_allocations[3] < KERNEL_HEAP_START || ptr_allocations[3] >= sbrk(0) || (uint32) ptr_allocations[3] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
			//if ((freeFrames - sys_calculate_free_frames()) != 1) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

			//1.5 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[4] = kmalloc(3*kilo/2);
			if ((uint32) ptr_allocations[4] < KERNEL_HEAP_START || ptr_allocations[4] >= sbrk(0) || (uint32) ptr_allocations[4] >= da_limit)
				{ correct = 0; cprintf("Wrong start address for the allocated space... should allocated by the dynamic allocator! check return address of kmalloc and/or sbrk\n"); }
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega /*+ 8*kilo*/)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 2) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 4*Mega + 8*kilo) ) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 768) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(6*Mega-kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 7*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 1536) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }

		//14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(14*kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 13*Mega + 8*kilo)) { correct = 0; cprintf("Wrong start address for the allocated space... check return address of kmalloc\n"); }
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((freeFrames - sys_calculate_free_frames()) < 4) { correct = 0; cprintf("Wrong allocation: pages are not loaded successfully into memory\n"); }
	}

	uint32 allocatedSpace = (13*Mega + 24*kilo + (INITIAL_KHEAP_ALLOCATIONS));
	uint32 allPAs[allocatedSpace/PAGE_SIZE] ;
	int numOfFrames = allocatedSpace/PAGE_SIZE ;

	//test kheap_virtual_address after kmalloc only [20%]
	{
		uint32 va;
		uint32 endVA = ACTUAL_START + 13*Mega + 24*kilo;
		uint32 startVA = da_limit + PAGE_SIZE;
		int i = 0;
		int j;
		for (va = startVA; va < endVA; )
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, va, &ptr_table);
			if (ptr_table == NULL)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }

			for (j = PTX(va); i < numOfFrames && j < 1024 && va < endVA; ++j, ++i)
			{
				uint32 offset = j;
				allPAs[i] = (ptr_table[j] & 0xFFFFF000) + offset;
				uint32 retrievedVA = kheap_virtual_address(allPAs[i]);
				//cprintf("va to check = %x\n", va);
				if (retrievedVA != (va+offset))
				{
					if (correct)
					{
						cprintf("\nretrievedVA = %x, Actual VA = %x, table entry = %x, khep_pa = %x\n",retrievedVA, va + offset /*+ j*PAGE_SIZE*/, (ptr_table[j] & 0xFFFFF000) , allPAs[i]);
						correct = 0; cprintf("Wrong kheap_virtual_address\n");
					}
				}
				va+=PAGE_SIZE;
			}
		}
	}
	if (correct)	eval+=20 ;

	correct = 1 ;
	//kfree some of the allocated spaces
	{
		//kfree 1st 2 MB
		int freeFrames = sys_calculate_free_frames() ;
		int freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512 ) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 512) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }

		//kfree 6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[7]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) { correct = 0; cprintf("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n"); }
		if ((sys_calculate_free_frames() - freeFrames) < 6*Mega/4096) { correct = 0; cprintf("Wrong kfree: pages in memory are not freed correctly\n"); }
	}


	//test kheap_virtual_address after kmalloc and kfree [20%]
	{
		uint32 va;
		uint32 endVA = ACTUAL_START + 13*Mega + 24*kilo;
		uint32 startVA = da_limit + PAGE_SIZE;
		int i = 0;
		int j;
		//frames of first 4 MB
		uint32 startIndex = (INITIAL_KHEAP_ALLOCATIONS) / PAGE_SIZE;
		for (i = startIndex ; i < startIndex + 4*Mega/PAGE_SIZE; ++i)
		{
			uint32 retrievedVA = kheap_virtual_address(allPAs[i]);
			if (retrievedVA != 0)
			{
				if (correct)
				{ correct = 0; cprintf("Wrong kheap_virtual_address\n"); }
			}

		}
		//next frames until 6 MB
		for (i = startIndex + 4*Mega/PAGE_SIZE; i < startIndex + (7*Mega + 8*kilo)/PAGE_SIZE; ++i)
		{
			uint32 retrievedVA = kheap_virtual_address(allPAs[i]);
			if (retrievedVA != ((startVA + i*PAGE_SIZE) + (allPAs[i] & 0xFFF)))
			{
				if (correct)
				{ correct = 0; cprintf("Wrong kheap_virtual_address\n"); }
			}
		}
		//frames of 6 MB
		for (i = startIndex + (7*Mega + 8*kilo)/PAGE_SIZE; i < startIndex + (13*Mega + 8*kilo)/PAGE_SIZE; ++i)
		{
			uint32 retrievedVA = kheap_virtual_address(allPAs[i]);
			if (retrievedVA != 0)
			{
				if (correct)
				{ correct = 0; cprintf("Wrong kheap_virtual_address\n"); }
			}
		}
		//frames of last allocation (14 KB)
		for (i = startIndex + (13*Mega + 8*kilo)/PAGE_SIZE; i < startIndex + (13*Mega + 24*kilo)/PAGE_SIZE; ++i)
		{
			uint32 retrievedVA = kheap_virtual_address(allPAs[i]);
			if (retrievedVA != ((startVA + i*PAGE_SIZE) + (allPAs[i] & 0xFFF)))
			{
				if (correct)
				{ correct = 0; cprintf("Wrong kheap_virtual_address\n"); }
			}
		}
	}
	if (correct)	eval+=20 ;

	correct = 1 ;
	//[DYNAMIC ALLOCATOR] test kheap_virtual_address each address [40%]
	{
		uint32 va, pa;
		for (va = KERNEL_HEAP_START; va < (uint32)sbrk(0); va++)
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, va, &ptr_table);
			if (ptr_table == NULL)
				{ correct = 0; panic("one of the kernel tables is wrongly removed! Tables of Kernel Heap should not be removed\n"); }
			pa = (ptr_table[PTX(va)] & 0xFFFFF000) + (va & 0xFFF);
			uint32 retrievedVA = kheap_virtual_address(pa);
			if (retrievedVA != va)
			{
				if (correct)
				{
					cprintf("\nPA = %x, retrievedVA = %x expectedVA = %x\n", pa, retrievedVA, va);
					correct = 0; cprintf("Wrong kheap_virtual_address\n");
				}
			}
		}
	}
	if (correct)	eval+=40 ;

	correct = 1 ;
	//test kheap_virtual_address on frames of KERNEL CODE [20%]
	{
		uint32 i;
		for (i = 1*Mega; i < (uint32)(end_of_kernel - KERNEL_BASE); i+=PAGE_SIZE)
		{
			uint32 retrievedVA = kheap_virtual_address(i);
			if (retrievedVA != 0)
			{
				if (correct)
				{
					cprintf("\nPA = %x, retrievedVA = %x\n", i, retrievedVA);
					correct = 0; cprintf("Wrong kheap_virtual_address\n");
				}
			}
		}
	}
	if (correct)	eval+=20 ;

	cprintf("\ntest kheap_virtual_address completed. Eval = %d%\n", eval);

	return 1;

}



















int test_kmalloc_nextfit()
{
	panic("not handled yet after applying dynamic allocator with page allocator");

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	void* ptr_allocations[160] = {0};
	cprintf("This test has THREE cases. A pass message will be displayed after each one.\n");

	// allocate pages
	int freeFrames = sys_calculate_free_frames() ;
	int freeDiskFrames = pf_calculate_free_frames() ;

	int i;
	//ptr_allocations[0] = kmalloc(2*Mega - KERNEL_SHARES_ARR_INIT_SIZE - KERNEL_SEMAPHORES_ARR_INIT_SIZE);
	for(i = 0; i< 79 ;i++)
	{
		ptr_allocations[i] = kmalloc(2*Mega);
	}
	ptr_allocations[79] = kmalloc(2*Mega - PAGE_SIZE - INITIAL_KHEAP_ALLOCATIONS);


	// randomly check the addresses of the allocation
	if( 	(uint32)ptr_allocations[0] != ACTUAL_START ||
			(uint32)ptr_allocations[2] != (ACTUAL_START + 4*Mega) ||
			(uint32)ptr_allocations[8] != (ACTUAL_START + 16*Mega) ||
			(uint32)ptr_allocations[10] != (ACTUAL_START + 20*Mega) ||
			(uint32)ptr_allocations[15] != (ACTUAL_START + 30*Mega) ||
			(uint32)ptr_allocations[20] != (ACTUAL_START + 40*Mega) ||
			(uint32)ptr_allocations[25] != (ACTUAL_START + 50*Mega) ||
			(uint32)ptr_allocations[79] != (ACTUAL_START + 158*Mega ))
		panic("Wrong allocation, Check next fitting strategy is working correctly");

	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (160*Mega - PAGE_SIZE - INITIAL_KHEAP_ALLOCATIONS)/(PAGE_SIZE) ) panic("Wrong allocation");

	// Make memory holes.
	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;

	kfree(ptr_allocations[0]);		// Hole 1 = 2 M
	kfree(ptr_allocations[2]);		// Hole 2 = 4 M
	kfree(ptr_allocations[3]);
	kfree(ptr_allocations[5]);		// Hole 3 = 2 M
	kfree(ptr_allocations[10]);		// Hole 4 = 6 M
	kfree(ptr_allocations[12]);
	kfree(ptr_allocations[11]);
	kfree(ptr_allocations[20]);		// Hole 5 = 2 M
	kfree(ptr_allocations[25]);		// Hole 6 = 2 M
	kfree(ptr_allocations[79]);		// Hole 7 = 2 M - 4 KB

	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((sys_calculate_free_frames() - freeFrames) != ((10*2*Mega) - PAGE_SIZE - INITIAL_KHEAP_ALLOCATIONS)/PAGE_SIZE) panic("Wrong free: Extra or less pages are removed from main memory");

	// Test next fit
	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	void* tempAddress = kmalloc(Mega-kilo);		// Use Hole 1 -> Hole 1 = 1 M
	if((uint32)tempAddress != ACTUAL_START)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (1*Mega)/PAGE_SIZE) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(kilo);					// Use Hole 1 -> Hole 1 = 1 M - Kilo -> requires one page only
	if((uint32)tempAddress != ACTUAL_START + 0x00100000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(5*Mega); 			   // Use Hole 4 -> Hole 4 = 1 M
	if((uint32)tempAddress != ACTUAL_START + 0x01400000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (5*Mega)/PAGE_SIZE) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(1*Mega); 			   // Use Hole 4 -> Hole 4 = 0 M
	if((uint32)tempAddress != ACTUAL_START + 0x01900000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (1*Mega)/PAGE_SIZE) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	kfree(ptr_allocations[15]);					// Make a new hole => 2 M
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((sys_calculate_free_frames() - freeFrames) !=  (2*Mega)/PAGE_SIZE) panic("Wrong free: Extra or less pages are removed from main memory");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(kilo); 			   // Use new Hole = 2 M - 4 kilo
	if((uint32)tempAddress != ACTUAL_START + 0x01E00000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(Mega + 1016*kilo); 	// Use new Hole = 4 kilo
	if((uint32)tempAddress != ACTUAL_START + 0x01E01000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");

	if ((freeFrames - sys_calculate_free_frames()) != (1*Mega+1016*kilo)/PAGE_SIZE) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(512*kilo); 			   // Use Hole 5 -> Hole 5 = 1.5 M
	if((uint32)tempAddress != ACTUAL_START + 0x02800000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (512*kilo)/PAGE_SIZE) panic("Wrong allocation");

	cprintf("\nCASE1: (next fit without looping back) is succeeded...\n") ;
	/******************************/

	// Check that next fit is looping back to check for free space
	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(3*Mega + 512*kilo); 			   // Use Hole 2 -> Hole 2 = 0.5 M
	if((uint32)tempAddress != ACTUAL_START + 0x00400000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (3*Mega+512*kilo)/PAGE_SIZE) panic("Wrong allocation");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	kfree(ptr_allocations[24]);		// Increase size of Hole 6 to 4 M
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((sys_calculate_free_frames() - freeFrames) != (2*Mega)/PAGE_SIZE) panic("Wrong free: Extra or less pages are removed from main memory");

	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(4*Mega-kilo);		// Use Hole 6 -> Hole 6 = 0 M
	if((uint32)tempAddress != ACTUAL_START + 0x03000000)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (4*Mega)/PAGE_SIZE) panic("Wrong allocation");

	cprintf("\nCASE2: (next fit WITH looping back) is succeeded...\n") ;
	/******************************/

	// Check that next fit returns null in case all holes are not free
	freeDiskFrames = pf_calculate_free_frames() ;
	freeFrames = sys_calculate_free_frames() ;
	tempAddress = kmalloc(6*Mega); 			   // No Suitable Hole is available
	if((uint32)tempAddress != 0x0)
		panic("Next Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != 0) panic("Wrong allocation");

	cprintf("\nCASE3: (next fit with insufficient space) is succeeded...\n") ;
	/******************************/

	cprintf("Congratulations!! test Next Fit completed successfully.\n");
	return 1;

}

int test_kmalloc_bestfit1()
{
	panic("not handled yet after applying dynamic allocator with page allocator");

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	void* ptr_allocations[20] = {0};
	uint32 freeFrames;
	uint32 freeDiskFrames;

	//[1] Allocate all
	{
		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[0] != (ACTUAL_START)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != ((3*Mega)/PAGE_SIZE)) panic("Wrong allocation: ");

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[1] !=  (ACTUAL_START + 3*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != ((3*Mega)/PAGE_SIZE)) panic("Wrong allocation: ");

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[2] !=  (ACTUAL_START + 6*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != ((2*Mega)/PAGE_SIZE)) panic("Wrong allocation: ");

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 8*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  ((2*Mega)/PAGE_SIZE)) panic("Wrong allocation: ");

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[4] !=  (ACTUAL_START + 10*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: ");

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 11*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: ");

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 12*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: ");

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 13*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: ");
	}

	//[2] Free some to create holes
	{
		//3 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != ((3*Mega)/PAGE_SIZE)) panic("Wrong free: ");

		//2 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[3]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != ((2*Mega)/PAGE_SIZE)) panic("Wrong free: ");

		//1 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[5]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256) panic("Wrong free: ");
	}

	//[3] Allocate again [test best fit]
	{
		//Allocate 512 KB - should be placed in 3rd hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(512*kilo);
		if ((uint32) ptr_allocations[8] !=  (ACTUAL_START + 11*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 128) panic("Wrong allocation: ");

		//Allocate 1 MB - should be placed in 2nd hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(1*Mega - kilo);
		if ((uint32) ptr_allocations[9] !=  (ACTUAL_START + 8*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: ");

		//Allocate 256 KB - should be placed in remaining of 3rd hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(256*kilo - kilo);
		if ((uint32) ptr_allocations[10] !=  (ACTUAL_START + 11*Mega + 512*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 64) panic("Wrong allocation: ");

		//Allocate 4 MB - should be placed in end of all allocations
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[11] = kmalloc(4*Mega - kilo);
		if ((uint32) ptr_allocations[11] != (ACTUAL_START + 14*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1024) panic("Wrong allocation: ");
	}

	//[4] Free contiguous allocations
	{
		//1M Hole appended to already existing 1M hole in the middle
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[4]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256) panic("Wrong free: ");

		//another 512 KB Hole appended to the hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 128) panic("Wrong free: ");
	}

	//[5] Allocate again [test best fit]
	{
		//Allocate 2 MB - should be placed in the contiguous hole (2 MB + 512 KB)
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[12] = kmalloc(2*Mega - kilo);
		if ((uint32) ptr_allocations[12] != (ACTUAL_START + 9*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: ");
	}

	cprintf("Congratulations!! test BEST FIT allocation (1) completed successfully.\n");

	return 1;

}

int test_kmalloc_bestfit2()
{
	panic("not handled yet after applying dynamic allocator with page allocator");

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	void* ptr_allocations[20] = {0};
	uint32 freeFrames;
	uint32 freeDiskFrames;

	//[1] Attempt to allocate more than heap size
	{
		ptr_allocations[0] = kmalloc(KERNEL_HEAP_MAX - ACTUAL_START + 1);
		if (ptr_allocations[0] != NULL) panic("Kmalloc: Attempt to allocate more than heap size, should return NULL");
	}

	//[2] Attempt to allocate space more than any available fragment
	//	a) Create Fragments
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] != (ACTUAL_START)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  512) panic("Wrong allocation: ");

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  512) panic("Wrong allocation: ");

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[2] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 4*Mega)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  1) panic("Wrong allocation: ");

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  1) panic("Wrong allocation: ");

		//4 KB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[2]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 1) panic("Wrong allocation: ");

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  2) panic("Wrong allocation: ");

		//2 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[0]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512) panic("Wrong free: Extra or less pages are removed from main memory");

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  768) panic("Wrong allocation: ");

		//2 MB + 6 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[6] = kmalloc(2*Mega + 6*kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  514) panic("Wrong allocation: ");

		//5 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[7] = kmalloc(5*Mega-kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 9*Mega + 24*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  ((5*Mega)/PAGE_SIZE)) panic("Wrong allocation: ");

		//2 MB + 8 KB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[6]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) !=  514) panic("Wrong free: Extra or less pages are removed from main memory");

		//2 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[1]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) !=  512) panic("Wrong free: Extra or less pages are removed from main memory.");

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[8] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  512) panic("Wrong allocation:");

		//6 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[9] = kmalloc(6*kilo);
		if ((uint32) ptr_allocations[9] != (ACTUAL_START + 9*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  2) panic("Wrong allocation:");

		//3 MB Hole
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[5]);
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) !=  768) panic("Wrong free: Extra or less pages are removed from main memory.");

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[10] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[10] != (ACTUAL_START + 4*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) !=  ((3*Mega)/4096)) panic("Wrong free: Extra or less pages are removed from main memory.");

		//4 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames();
		ptr_allocations[11] = kmalloc(4*Mega-kilo);
		if ((uint32) ptr_allocations[11] != (ACTUAL_START)) panic("Wrong start address for the allocated space... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != ((4*Mega)/4096)) panic("Wrong free: Extra or less pages are removed from main memory.");

	}

	//	b) Attempt to allocate large segment with no suitable fragment to fit on
	{
		//Large Allocation
		ptr_allocations[12] = kmalloc((KERNEL_HEAP_MAX - ACTUAL_START - 14*Mega));
		if (ptr_allocations[12] != NULL) panic("Kmalloc: Attempt to allocate large segment with no suitable fragment to fit on, should return NULL");

		cprintf("Congratulations!! test BEST FIT allocation (2) completed successfully.\n");
	}
	return 1;

}

int test_kmalloc_worstfit()
{
	panic("not handled yet after applying dynamic allocator with page allocator");

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	void* ptr_allocations[160] = {0};

	// allocate pages
	int freeFrames = sys_calculate_free_frames() ;
	int freeDiskFrames = pf_calculate_free_frames() ;

	int count = 0;
	int i;
	for(i = 0; i< 79 ;i++)
	{
		ptr_allocations[i] = kmalloc(2*Mega);
	}
	ptr_allocations[79] = kmalloc(2*Mega - PAGE_SIZE - KERNEL_SHARES_ARR_INIT_SIZE - KERNEL_SEMAPHORES_ARR_INIT_SIZE);

	// randomly check the addresses of the allocation
	if( 	(uint32)ptr_allocations[0] != ACTUAL_START ||
			(uint32)ptr_allocations[2] != (ACTUAL_START + 4*Mega) ||
			(uint32)ptr_allocations[8] != (ACTUAL_START + 16*Mega) ||
			(uint32)ptr_allocations[10] != (ACTUAL_START + 20*Mega) ||
			(uint32)ptr_allocations[15] != (ACTUAL_START + 30*Mega) ||
			(uint32)ptr_allocations[20] != (ACTUAL_START + 40*Mega) ||
			(uint32)ptr_allocations[50] != (ACTUAL_START + 100*Mega) ||
			(uint32)ptr_allocations[79] != (ACTUAL_START + 158*Mega))
		panic("Wrong allocation, Check worst fitting strategy is working correctly");

	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) != (160*Mega - PAGE_SIZE - KERNEL_SHARES_ARR_INIT_SIZE - KERNEL_SEMAPHORES_ARR_INIT_SIZE)/(PAGE_SIZE) ) panic("Wrong allocation");

	//make memory holes
	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames() ;

	kfree(ptr_allocations[0]);		//Hole 1 = 2 M
	kfree(ptr_allocations[2]);		//Hole 2 = 4 M
	kfree(ptr_allocations[3]);
	kfree(ptr_allocations[10]);		//Hole 3 = 6 M
	kfree(ptr_allocations[12]);
	kfree(ptr_allocations[11]);
	kfree(ptr_allocations[30]);		//Hole 4 = 10 M
	kfree(ptr_allocations[31]);
	kfree(ptr_allocations[32]);
	kfree(ptr_allocations[33]);
	kfree(ptr_allocations[34]);
	kfree(ptr_allocations[70]); 	//Hole 5 = 8 M
	kfree(ptr_allocations[71]);
	kfree(ptr_allocations[72]);
	kfree(ptr_allocations[73]);

	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((sys_calculate_free_frames() - freeFrames) != ((15*2*Mega))/PAGE_SIZE) panic("Wrong free: Extra or less pages are removed from main memory");

	// Test worst fit
	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	void* tempAddress = kmalloc(Mega);		//Use Hole 4 -> Hole 4 = 9 M
	if((uint32)tempAddress != ACTUAL_START + 0x03C00000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  1*Mega/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(4 * Mega);			//Use Hole 4 -> Hole 4 = 5 M
	if((uint32)tempAddress != ACTUAL_START + 0x03D00000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  4*Mega/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(6*Mega); 			   //Use Hole 5 -> Hole 5 = 2 M
	if((uint32)tempAddress != ACTUAL_START + 0x08C00000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  6*Mega/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(5*Mega); 			   //Use Hole 3 -> Hole 3 = 1 M
	if((uint32)tempAddress != ACTUAL_START + 0x01400000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  5*Mega/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(4*Mega); 			   // Use Hole 4 -> Hole 4 = 1 M
	if((uint32)tempAddress != ACTUAL_START + 0x04100000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  4*Mega/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(2 * Mega); 			// Use Hole 2 -> Hole 2 = 2 M
	if((uint32)tempAddress != ACTUAL_START + 0x00400000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  2*Mega/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(1*Mega + 512*kilo);    // Use Hole 1 -> Hole 1 = 0.5 M
	if((uint32)tempAddress != ACTUAL_START)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  (1*Mega + 512*kilo)/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(512*kilo); 			   // Use Hole 2 -> Hole 2 = 1.5 M
	if((uint32)tempAddress != ACTUAL_START + 0x00600000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  (512*kilo)/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(kilo); 			   // Use Hole 5 -> Hole 5 = 2 M - K
	if((uint32)tempAddress != ACTUAL_START + 0x09200000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  (4*kilo)/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(2*Mega - 4*kilo); 		// Use Hole 5 -> Hole 5 = 0
	if((uint32)tempAddress != ACTUAL_START + 0x09201000)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  (2*Mega - 4*kilo)/PAGE_SIZE) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	// Check that worst fit returns null in case all holes are not free
	freeFrames = sys_calculate_free_frames() ;
	freeDiskFrames = pf_calculate_free_frames();
	tempAddress = kmalloc(4*Mega); 		//No Suitable hole
	if((uint32)tempAddress != 0x0)
		panic("Worst Fit not working correctly");
	if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames - sys_calculate_free_frames()) !=  0) panic("Wrong allocation:");
	cprintf("Test %d Passed \n", ++count);

	cprintf("Congratulations!! test Worst Fit completed successfully.\n");


	return 1;
}

int test_kfree()
{
	panic("not handled yet after applying dynamic allocator with page allocator");

	cprintf("==============================================\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");

	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};
	int sums[20] = {0};
	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[0] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[1] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 4*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[2] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			ptr[i] = 2 ;
		}

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[3] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			ptr[i] = 3 ;
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 2) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[4] = (7*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			ptr[i] = 4 ;
		}

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 16*kilo) ) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 768) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[5] = (3*Mega-kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			ptr[i] = 5 ;
		}

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(6*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1536) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[6] = (6*Mega-kilo)/sizeof(char) - 1;

		//14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(14*kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 13*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 4) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[7] = (14*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			ptr[i] = 7 ;
		}
	}

	//kfree some of the allocated spaces [15%]
	{
		//kfree 1st 2 MB
		int freeFrames = sys_calculate_free_frames() ;
		int freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512 ) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 1st 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 1 ) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 6*Mega/4096) panic("Wrong kfree: pages in memory are not freed correctly");
	}

	cprintf("\nkfree: current evaluation = 15%");

	//Check memory access after kfree [15%]
	{
		//2 KB
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			sums[3] += ptr[i] ;
		}
		if (sums[3] != 3*lastIndices[3])	panic("kfree: invalid read after freeing some allocations");

		//7 KB
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			sums[4] += ptr[i] ;
		}
		if (sums[4] != 4*lastIndices[4])	panic("kfree: invalid read after freeing some allocations");

		//3 MB
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			sums[5] += ptr[i] ;
		}
		if (sums[5] != 5*lastIndices[5])	panic("kfree: invalid read after freeing some allocations");

		//14 KB
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			sums[7] += ptr[i] ;
		}
		if (sums[7] != 7*lastIndices[7])	panic("kfree: invalid read after freeing some allocations");
	}
	cprintf("\b\b\b30%");

	//Allocate after kfree [15%]
	{
		//20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(20*kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 13*Mega + 32*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 5) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[8] = (20*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[8];
		for (i = 0; i < lastIndices[8]; ++i)
		{
			ptr[i] = 8 ;
		}

		//1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(1*Mega);
		if ((uint32) ptr_allocations[9] != (ACTUAL_START + 13*Mega + 52*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[9] = (1*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[9];
		for (i = 0; i < lastIndices[9]; ++i)
		{
			ptr[i] = 9 ;
		}

		if (isKHeapPlacementStrategyNEXTFIT())
		{
			//Allocate Remaining MBs
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			uint32 reqAllocatedSpace = KERNEL_HEAP_MAX - (ACTUAL_START + 13*Mega + 52*kilo + 1*Mega);
			ptr_allocations[10] = kmalloc(reqAllocatedSpace);
			if ((uint32) ptr_allocations[10] != (ACTUAL_START + 13*Mega + 52*kilo + 1*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((freeFrames - sys_calculate_free_frames()) != reqAllocatedSpace/PAGE_SIZE) panic("Wrong allocation: pages are not loaded successfully into memory");
			lastIndices[10] = (reqAllocatedSpace)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[10];
			//			for (i = 0; i < lastIndices[10]; ++i)
			//			{
			//				ptr[i] = 10;
			//			}

			//Allocate in merged freed space FROM the beginning
			//3 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[11] = kmalloc(3*Mega);
			if ((uint32) ptr_allocations[11] != (ACTUAL_START)) panic("Wrong start address for the allocated space... check return address of kmalloc");
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((freeFrames - sys_calculate_free_frames()) != 768) panic("Wrong allocation: pages are not loaded successfully into memory");
			lastIndices[11] = (3*Mega)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[11];
			for (i = 0; i < lastIndices[11]; ++i)
			{
				ptr[i] = 8 ;
			}

			//2 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[12] = kmalloc(2*kilo);
			if ((uint32) ptr_allocations[12] != (ACTUAL_START + 3*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
			lastIndices[12] = (2*kilo)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[12];
			for (i = 0; i < lastIndices[12]; ++i)
			{
				ptr[i] = 9 ;
			}

			//1 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			ptr_allocations[13] = kmalloc(1*Mega);
			if ((uint32) ptr_allocations[13] != (ACTUAL_START + 3*Mega + 4*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
			if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: pages are not loaded successfully into memory");
			lastIndices[13] = (1*Mega)/sizeof(char) - 1;
			ptr = (char*)ptr_allocations[13];
			for (i = 0; i < lastIndices[13]; ++i)
			{
				ptr[i] = 10 ;
			}
		}
	}
	cprintf("\b\b\b45%");

	//kfree remaining allocated spaces [15%]
	{
		//kfree 7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[4]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 2) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[5]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 3*Mega/4096) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 2nd 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[3]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 1) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[7]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 4) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 5) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[9]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256) panic("Wrong kfree: pages in memory are not freed correctly");

		if (isKHeapPlacementStrategyNEXTFIT())
		{
			//cprintf("FREE in NEXT FIT\n");
			//kfree Remaining MBs
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			kfree(ptr_allocations[10]);
			uint32 reqAllocatedSpace = KERNEL_HEAP_MAX - (ACTUAL_START + 13*Mega + 52*kilo + 1*Mega);
			if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((sys_calculate_free_frames() - freeFrames) != reqAllocatedSpace/PAGE_SIZE) panic("Wrong kfree: pages in memory are not freed correctly");

			//kfree 3 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			kfree(ptr_allocations[11]);
			if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((sys_calculate_free_frames() - freeFrames) != 3*Mega/4096) panic("Wrong kfree: pages in memory are not freed correctly");

			//kfree 2 KB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			kfree(ptr_allocations[12]);
			if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((sys_calculate_free_frames() - freeFrames) != 1) panic("Wrong kfree: pages in memory are not freed correctly");

			//kfree 1 MB
			freeFrames = sys_calculate_free_frames() ;
			freeDiskFrames = pf_calculate_free_frames() ;
			kfree(ptr_allocations[13]);
			if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
			if ((sys_calculate_free_frames() - freeFrames) != 1*Mega/4096) panic("Wrong kfree: pages in memory are not freed correctly");

		}
		if(start_freeFrames != (sys_calculate_free_frames())) {panic("Wrong kfree: not all pages removed correctly at end");}
	}
	cprintf("\b\b\b60%");

	//Check memory access after kfree [15%]
	{
		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		for (i = 0; i < 10; ++i)
		{
			ptr = (char *) ptr_allocations[i];
			ptr[0] = 10;
			//cprintf("\n\ncr2 = %x, faulted addr = %x", sys_rcr2(), (uint32)&(ptr[0]));
			if (sys_rcr2() != (uint32)&(ptr[0])) panic("kfree: successful access to freed space!! it should not be succeeded");
			ptr[lastIndices[i]] = 10;
			if (sys_rcr2() != (uint32)&(ptr[lastIndices[i]])) panic("kfree: successful access to freed space!! it should not be succeeded");
		}

		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);
	}
	cprintf("\b\b\b75%");

	//kfree non-exist item [10%]
	{
		//kfree 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

		//kfree 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

		//kfree 20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

		//kfree 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[9]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

	}
	cprintf("\b\b\b85%");

	//check tables	[15%]
	{
		long long va;
		for (va = KERNEL_HEAP_START; va < (long long)KERNEL_HEAP_MAX; va+=PTSIZE)
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, (uint32)va, &ptr_table);
			if (ptr_table == NULL)
			{
				panic("Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree");
			}
		}
	}

	cprintf("\b\b\b100%\n");



	cprintf("\nCongratulations!! test kfree completed successfully.\n");

	return 1;

}


int initFreeFrames;
int initFreeDiskFrames ;
uint8 firstCall = 1 ;
int test_three_creation_functions()
{
	if (firstCall)
	{
		firstCall = 0;
		initFreeFrames = sys_calculate_free_frames() ;
		initFreeDiskFrames = pf_calculate_free_frames() ;
		//Run simple user program
		{
			char command[100] = "run fos_add 4096";
			execute_command(command) ;
		}
	}
	//Ensure that the user directory, page WS and page tables are allocated in KERNEL HEAP
	{
		struct Env * e = NULL;
		struct Env * ptr_env = NULL;
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			if (strcmp(ptr_env->prog_name, "fos_add") == 0)
			{
				e = ptr_env ;
				break;
			}
		}
		if (e->pageFaultsCounter != 0)
			panic("Page fault is occur while not expected to. Review the three creation functions");

#if USE_KHEAP
		int pagesInWS = LIST_SIZE(&(e->page_WS_list));
#else
		int pagesInWS = env_page_ws_get_size(e);
#endif
		int curFreeFrames = sys_calculate_free_frames() ;
		int curFreeDiskFrames = pf_calculate_free_frames() ;
		//cprintf("\ndiff in page file = %d, pages in WS = %d\n", initFreeDiskFrames - curFreeDiskFrames, pagesInWS);
		if ((initFreeDiskFrames - curFreeDiskFrames) != pagesInWS) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		//cprintf("\ndiff in mem frames = %d, pages in WS = %d\n", initFreeFrames - curFreeFrames, pagesInWS);
		if ((initFreeFrames - curFreeFrames) != 12/*WS*/ + 2*1/*DIR*/ + 2*3/*Tables*/ + 1 /*user WS table*/ + pagesInWS) panic("Wrong allocation: pages are not loaded successfully into memory");

		//allocate 4 KB
		char *ptr = kmalloc(4*kilo);
		if ((uint32) ptr !=  (ACTUAL_START + (12+2*1+2*3+1)*PAGE_SIZE)) panic("Wrong start address for the allocated space... make sure you create the dir, table and page WS in KERNEL HEAP");
	}

	cprintf("\nCongratulations!! test the 3 creation functions is completed successfully.\n");

	return 1;
}



extern void kfreeall() ;

int test_kfreeall()
{
	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};
	int sums[20] = {0};
	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[0] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[1] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 4*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[2] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			ptr[i] = 2 ;
		}

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[3] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			ptr[i] = 3 ;
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 2) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[4] = (7*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			ptr[i] = 4 ;
		}

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 16*kilo) ) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 768) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[5] = (3*Mega-kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			ptr[i] = 5 ;
		}

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(6*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1536) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[6] = (6*Mega-kilo)/sizeof(char) - 1;

		//14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(14*kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 13*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 4) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[7] = (14*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			ptr[i] = 7 ;
		}
	}

	//kfree some of the allocated spaces
	{
		//kfree 1st 2 MB
		int freeFrames = sys_calculate_free_frames() ;
		int freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512 ) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 6*Mega/4096) panic("Wrong kfree: pages in memory are not freed correctly");
	}


	//Check memory access after kfree
	{
		//2 KB
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			sums[2] += ptr[i] ;
		}
		if (sums[2] != 2*lastIndices[2])	panic("kfree: invalid read after freeing some allocations");

		//2 KB
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			sums[3] += ptr[i] ;
		}
		if (sums[3] != 3*lastIndices[3])	panic("kfree: invalid read after freeing some allocations");

		//7 KB
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			sums[4] += ptr[i] ;
		}
		if (sums[4] != 4*lastIndices[4])	panic("kfree: invalid read after freeing some allocations");

		//3 MB
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			sums[5] += ptr[i] ;
		}
		if (sums[5] != 5*lastIndices[5])	panic("kfree: invalid read after freeing some allocations");

		//14 KB
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			sums[7] += ptr[i] ;
		}
		if (sums[7] != 7*lastIndices[7])	panic("kfree: invalid read after freeing some allocations");
	}

	//Allocate after kfree
	{
		//20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(20*kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 13*Mega + 32*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 5) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[8] = (20*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[8];
		for (i = 0; i < lastIndices[8]; ++i)
		{
			ptr[i] = 8 ;
		}

		//1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(1*Mega);
		if ((uint32) ptr_allocations[9] != (ACTUAL_START + 13*Mega + 52*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[9] = (1*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[9];
		for (i = 0; i < lastIndices[9]; ++i)
		{
			ptr[i] = 9 ;
		}
	}

	//kfree entire kernel heap
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kfreeall();

		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != (INITIAL_KHEAP_ALLOCATIONS)/PAGE_SIZE+ 2 + 3*Mega/4096 + 1 + 1 + 4 + 5 + 256) panic("Wrong kfree: pages in memory are not freed correctly");
	}

	//Check memory access after kfreeall
	{
		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		for (i = 0; i < 10; ++i)
		{
			ptr = (char *) ptr_allocations[i];
			ptr[0] = 10;
			//cprintf("\n\ncr2 = %x, faulted addr = %x", sys_rcr2(), (uint32)&(ptr[0]));
			if (sys_rcr2() != (uint32)&(ptr[0])) panic("kfree: successful access to freed space!! it should not be succeeded");
			ptr[lastIndices[i]] = 10;
			if (sys_rcr2() != (uint32)&(ptr[lastIndices[i]])) panic("kfree: successful access to freed space!! it should not be succeeded");
		}
		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);
	}

	//Allocate after kfreeall
	{
		//4 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(4*Mega);
		if ((uint32) ptr_allocations[10] != (KERNEL_HEAP_START)) panic("Wrong start address after kfreeall()... check return address updating of heap ptr");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 4*Mega/PAGE_SIZE) panic("Wrong allocation: pages are not loaded successfully into memory");

		//12 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[11] = kmalloc(12*kilo);
		if ((uint32) ptr_allocations[11] != (KERNEL_HEAP_START + 4*Mega)) panic("Wrong start address after kfreeall()... check return address updating of heap ptr");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 3) panic("Wrong allocation: pages are not loaded successfully into memory");

	}

	//kfree one of the newly allocated space
	{
		//kfree 12 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[11]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 3) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");
	}

	//kfree non-exist item
	{
		//kfree 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

		//kfree 20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

		//kfree 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[9]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 0) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");

	}

	//check tables
	{
		long long va;
		for (va = KERNEL_HEAP_START; va < (long long)KERNEL_HEAP_MAX; va+=PTSIZE)
		{
			uint32 *ptr_table ;
			get_page_table(ptr_page_directory, (uint32)va, &ptr_table);
			if (ptr_table == NULL)
			{
				panic("Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree");
			}
		}
	}


	cprintf("\nCongratulations!! your modification is run successfully.\n");

	return 1;

}


extern void kexpand(uint32 newSize) ;

int test_kexpand()
{
	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};

	uint32 *arr;
	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[0] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[1] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 4*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[2] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			ptr[i] = 2 ;
		}

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[3] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			ptr[i] = 3 ;
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 2) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[4] = (7*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			ptr[i] = 4 ;
		}

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 16*kilo) ) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 768) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[5] = (3*Mega-kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			ptr[i] = 5 ;
		}

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(6*Mega);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1536) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[6] = (6*Mega)/sizeof(uint32) - 1;
		arr = (uint32*)ptr_allocations[6];
		for (i = 0; i <= lastIndices[6]; ++i)
		{
			arr[i] = i ;
		}
	}

	//Expand last allocated variable to 7 MB instead of 6 MB
	int newLastIndex = (7*Mega)/sizeof(uint32) - 1;
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kexpand(7*Mega) ;

		assert(pf_calculate_free_frames() - freeDiskFrames == 0) ;
		assert(freeFrames - sys_calculate_free_frames() == 256) ;

		for (i = lastIndices[6]; i < newLastIndex ; ++i)
		{
			arr[i] = i ;
		}
	}

	//Access elements after expansion
	{
		for (i = 0; i < newLastIndex ; ++i)
		{
			assert(arr[i] ==i);
		}
	}

	//Expand it again to 10 MB instead of 7 MB
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kexpand(10*Mega) ;

		assert(pf_calculate_free_frames() - freeDiskFrames == 0) ;
		assert(freeFrames - sys_calculate_free_frames() == 768) ;
	}


	//Allocate after expanding last var
	{
		//4 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(4*Mega);

		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 7*Mega + 16*kilo + 10*Mega)) panic("Wrong start address after kexpand()... ");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 4*Mega/PAGE_SIZE) panic("Wrong allocation: pages are not loaded successfully into memory");
	}

	//kfree the expanded variable
	{
		//kfree 10 MB (expanded)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 10*Mega/PAGE_SIZE) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");
	}

	//Expand last allocated variable to 4 MB + 20 kilo instead of 4 MB
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kexpand(4*Mega + 20*kilo) ;

		assert(pf_calculate_free_frames() - freeDiskFrames == 0) ;
		assert(freeFrames - sys_calculate_free_frames()  == 5) ;
	}

	cprintf("\nCongratulations!! your modification is run successfully.\n");

	return 1;
}

extern void kshrink(uint32 newSize) ;

int test_kshrink()
{
	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};

	uint32 *arr;
	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[0] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[1] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 4*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[2] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			ptr[i] = 2 ;
		}

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[3] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			ptr[i] = 3 ;
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 2) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[4] = (7*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			ptr[i] = 4 ;
		}

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 16*kilo) ) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 768) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[5] = (3*Mega-kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			ptr[i] = 5 ;
		}

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(6*Mega);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1536) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[6] = (6*Mega)/sizeof(uint32) - 1;
		arr = (uint32*)ptr_allocations[6];
		for (i = 0; i <= lastIndices[6]; ++i)
		{
			arr[i] = i ;
		}
	}

	//Shrink last allocated variable to 5 MB instead of 6 MB
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kshrink(5*Mega) ;

		assert(pf_calculate_free_frames() - freeDiskFrames == 0) ;
		assert(sys_calculate_free_frames() - freeFrames == 256) ;
	}

	//Access elements after shrink
	int newLastIndex = (5*Mega)/sizeof(uint32) - 1;
	{
		for (i = 0; i <= newLastIndex ; ++i)
		{
			assert(arr[i] == i);
		}

		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		ptr = (char *) ptr_allocations[6];
		ptr[5*Mega] = 10;
		assert(sys_rcr2() == (uint32)&(ptr[5*Mega])) ;

		ptr[5*Mega+4*kilo] = 10;
		assert(sys_rcr2() == (uint32)&(ptr[5*Mega+4*kilo])) ;

		ptr[6*Mega - kilo] = 10;
		assert(sys_rcr2() == (uint32)&(ptr[6*Mega - kilo])) ;

		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);
	}

	//Shrink it again to 2 MB instead of 5 MB
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kshrink(2*Mega) ;

		assert(pf_calculate_free_frames() - freeDiskFrames == 0) ;
		assert(sys_calculate_free_frames() - freeFrames == 768) ;
	}


	//Allocate after shrinking last var
	{
		//4 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(4*Mega);
		cprintf("ACTUAL = %x, DESIRED = %x\n", (uint32) ptr_allocations[7] ,(ACTUAL_START + 7*Mega + 16*kilo + 2*Mega));
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 7*Mega + 16*kilo + 2*Mega)) panic("Wrong start address after kshrink()... check the updating of your data structures");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 4*Mega/PAGE_SIZE) panic("Wrong allocation: pages are not loaded successfully into memory");
	}


	//kfree the shrunk variable
	{
		//kfree 2 MB (shrunk)
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512) panic("Wrong kfree: attempt to kfree a non-existing ptr. It should do nothing");
	}

	//Shrink last allocated variable to 4 MB - 20 kilo instead of 4 MB
	{
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;

		kshrink(4*Mega - 20*kilo) ;

		assert(pf_calculate_free_frames() - freeDiskFrames == 0) ;
		assert(sys_calculate_free_frames() - freeFrames == 5) ;
	}

	cprintf("\nCongratulations!! your modification is run successfully.\n");

	return 1;
}


int test_kfreelast()
{
	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	int start_freeFrames = sys_calculate_free_frames() ;

	//malloc some spaces
	int i, ce, freeFrames, freeDiskFrames ;
	char* ptr;
	int lastIndices[20] = {0};
	int sums[20] = {0};
	void* ptr_allocations[20] = {0};
	{
		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[0] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[0] !=  (ACTUAL_START)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[0] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[1] = kmalloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (ACTUAL_START + 2*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 512) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[1] = (2*Mega-kilo)/sizeof(char) - 1;

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[2] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[2] != (ACTUAL_START + 4*Mega)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[2] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			ptr[i] = 2 ;
		}

		//2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[3] = kmalloc(2*kilo);
		if ((uint32) ptr_allocations[3] != (ACTUAL_START + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[3] = (2*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			ptr[i] = 3 ;
		}

		//7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[4] = kmalloc(7*kilo);
		if ((uint32) ptr_allocations[4] != (ACTUAL_START + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 2) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[4] = (7*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			ptr[i] = 4 ;
		}

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[5] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[5] != (ACTUAL_START + 4*Mega + 16*kilo) ) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 768) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[5] = (3*Mega-kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			ptr[i] = 5 ;
		}

		//6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[6] = kmalloc(6*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (ACTUAL_START + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 1536) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[6] = (6*Mega-kilo)/sizeof(char) - 1;

		//14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[7] = kmalloc(14*kilo);
		if ((uint32) ptr_allocations[7] != (ACTUAL_START + 13*Mega + 16*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 4) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[7] = (14*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			ptr[i] = 7 ;
		}
	}

	//kfree some of the allocated spaces
	{
		//kfree 1st 2 MB
		int freeFrames = sys_calculate_free_frames() ;
		int freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512 ) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[1]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 512) panic("Wrong kfree: pages in memory are not freed correctly");

		//kfree 6 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[6]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 6*Mega/4096) panic("Wrong kfree: pages in memory are not freed correctly");
	}

	//Check memory access after kfree
	{
		//2 KB
		ptr = (char*)ptr_allocations[2];
		for (i = 0; i < lastIndices[2]; ++i)
		{
			sums[2] += ptr[i] ;
		}
		if (sums[2] != 2*lastIndices[2])	panic("kfree: invalid read after freeing some allocations");

		//2 KB
		ptr = (char*)ptr_allocations[3];
		for (i = 0; i < lastIndices[3]; ++i)
		{
			sums[3] += ptr[i] ;
		}
		if (sums[3] != 3*lastIndices[3])	panic("kfree: invalid read after freeing some allocations");

		//7 KB
		ptr = (char*)ptr_allocations[4];
		for (i = 0; i < lastIndices[4]; ++i)
		{
			sums[4] += ptr[i] ;
		}
		if (sums[4] != 4*lastIndices[4])	panic("kfree: invalid read after freeing some allocations");

		//3 MB
		ptr = (char*)ptr_allocations[5];
		for (i = 0; i < lastIndices[5]; ++i)
		{
			sums[5] += ptr[i] ;
		}
		if (sums[5] != 5*lastIndices[5])	panic("kfree: invalid read after freeing some allocations");

		//14 KB
		ptr = (char*)ptr_allocations[7];
		for (i = 0; i < lastIndices[7]; ++i)
		{
			sums[7] += ptr[i] ;
		}
		if (sums[7] != 7*lastIndices[7])	panic("kfree: invalid read after freeing some allocations");
	}

	//Allocate after kfree
	{
		//20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[8] = kmalloc(20*kilo);
		if ((uint32) ptr_allocations[8] != (ACTUAL_START + 13*Mega + 32*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 5) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[8] = (20*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[8];
		for (i = 0; i < lastIndices[8]; ++i)
		{
			ptr[i] = 8 ;
		}

		//1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[9] = kmalloc(1*Mega);
		if ((uint32) ptr_allocations[9] != (ACTUAL_START + 13*Mega + 52*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[9] = (1*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[9];
		for (i = 0; i < lastIndices[9]; ++i)
		{
			ptr[i] = 9 ;
		}
	}

	ce = 0;
	//kfree last allocated space
	{
		//kfree 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[9]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256) panic("Wrong kfree: pages in memory are not freed correctly");
	}

	//Allocate after kfree last [25%]
	{
		//30 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[10] = kmalloc(30*kilo);
		if ((uint32) ptr_allocations[10] != (ACTUAL_START + 13*Mega + 52*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 8) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[10] = (30*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[10];
		for (i = 0; i < lastIndices[10]; ++i)
		{
			ptr[i] = 10 ;
		}

		//1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[11] = kmalloc(1*Mega);
		if ((uint32) ptr_allocations[11] != (ACTUAL_START + 13*Mega + 84*kilo)) panic("Wrong start address for the allocated space... check return address of kmalloc");
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((freeFrames - sys_calculate_free_frames()) != 256) panic("Wrong allocation: pages are not loaded successfully into memory");
		lastIndices[11] = (1*Mega)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[11];
		for (i = 0; i < lastIndices[11]; ++i)
		{
			ptr[i] = 11 ;
		}
	}
	ce += 25;
	cprintf("\nkfreelast: current evaluation = %d%\n", ce);

	int f = 0;
	//kfree last allocated two spaces
	{
		//kfree 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[11]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 256) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 30 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[10]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 8) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}
	}

	//Allocate after kfree last allocated two spaces (in order) [10%]
	{
		//10 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[12] = kmalloc(10*kilo);
		if ((uint32) ptr_allocations[12] != (ACTUAL_START + 13*Mega + 52*kilo)) if (!f) {f=1; cprintf("\nWrong start address for the allocated space... check return address of kmalloc");}
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((freeFrames - sys_calculate_free_frames()) != 3) if (!f) {f=1; cprintf("\nWrong allocation: pages are not loaded successfully into memory");}
		lastIndices[12] = (10*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[12];
		for (i = 0; i < lastIndices[12]; ++i)
		{
			ptr[i] = 12 ;
		}
	}

	if (!f) {ce += 10;cprintf("\nkfreelast: current evaluation = %d%\n", ce);} f=0;

	//Check memory access after kfree last and kalloc [15%]
	{
		//10 KB
		ptr = (char*)ptr_allocations[12];
		for (i = 0; i < lastIndices[12]; ++i)
		{
			sums[12] += ptr[i] ;
		}
		if (sums[12] != 12*lastIndices[12])	if (!f) {f=1; cprintf("\nkfree: invalid read after freeing some allocations");}


		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		ptr = (char *) ptr_allocations[12] + 12*kilo;

		for (i = 0; i < 30*kilo; ++i)
		{
			ptr[i] = 10;
			//cprintf("\n\ncr2 = %x, faulted addr = %x", sys_rcr2(), (uint32)&(ptr[0]));
			if (sys_rcr2() != (uint32)&(ptr[i])) if (!f) {f=1; cprintf("\nkfree: successful access to freed space!! it should not be succeeded");}
		}

		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);

	}

	if (!f) {ce += 15;cprintf("\nkfreelast: current evaluation = %d%\n", ce);} f=0;

	//kfree last allocated three spaces [but with different order]
	{
		//kfree 10 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[12]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 3) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 14 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[7]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 4) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 5) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}
	}


	//Allocate after kfree last allocated 3 spaces with different order [25%]
	{
		//50 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[13] = kmalloc(50*kilo);
		if ((uint32) ptr_allocations[13] != (ACTUAL_START + 7*Mega + 16*kilo)) if (!f) {f=1; cprintf("\nWrong start address for the allocated space... check return address of kmalloc");}
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((freeFrames - sys_calculate_free_frames()) != 13) if (!f) {f=1; cprintf("\nWrong allocation: pages are not loaded successfully into memory");}
		lastIndices[13] = (50*kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[13];
		for (i = 0; i < lastIndices[13]; ++i)
		{
			ptr[i] = 13 ;
		}

		//3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		ptr_allocations[14] = kmalloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[14] != (ACTUAL_START + 7*Mega + 68*kilo) ) if (!f) {f=1; cprintf("\nWrong start address for the allocated space... check return address of kmalloc");}
		if ((pf_calculate_free_frames() - freeDiskFrames) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((freeFrames - sys_calculate_free_frames()) != 768) if (!f) {f=1; cprintf("\nWrong allocation: pages are not loaded successfully into memory");}
		lastIndices[14] = (3*Mega-kilo)/sizeof(char) - 1;
		ptr = (char*)ptr_allocations[14];
		for (i = 0; i < lastIndices[14]; ++i)
		{
			ptr[i] = 14 ;
		}
	}

	if (!f) {ce += 25;cprintf("\nkfreelast: current evaluation = %d%\n", ce);} f=0;

	//kfree one of the newly allocated space that override a previously allocated one
	{
		//kfree 50 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[13]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 13) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}
	}

	//Check memory access after kfree last and kalloc [15%]
	{
		//50 KB

		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		ptr = (char *) ptr_allocations[13];

		for (i = 0; i < 50*kilo; ++i)
		{
			ptr[i] = 10;
			//cprintf("\n\ncr2 = %x, faulted addr = %x", sys_rcr2(), (uint32)&(ptr[0]));
			if (sys_rcr2() != (uint32)&(ptr[i])) if (!f) {f=1; cprintf("\nkfree: successful access to freed space!! it should not be succeeded");}
		}

		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);


		//3 MB
		ptr = (char*)ptr_allocations[14];
		for (i = 0; i < lastIndices[14]; ++i)
		{
			sums[14] += ptr[i] ;
		}
		if (sums[14] != 14*lastIndices[14])	if (!f) {f=1; cprintf("\nkfree: invalid read after freeing some allocations");}
	}

	if (!f) {ce += 15;cprintf("\nkfreelast: current evaluation = %d%\n", ce);} f=0;

	//kfree all remaining allocations
	{
		//kfree 7 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[4]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 2) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 2nd 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[14]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 3*Mega/4096) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 1st 3 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[5]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 3*Mega/4096) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 1st 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 1) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		//kfree 2nd 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[3]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 1) if (!f) {f=1; cprintf("\nWrong kfree: pages in memory are not freed correctly");}

		if(start_freeFrames != (sys_calculate_free_frames())) if (!f) {f=1; cprintf("\nWrong kfree: not all pages removed correctly at end");}
	}

	//Check memory access after kfree
	{
		//Bypass the PAGE FAULT on <MOVB immediate, reg> instruction by setting its length
		//and continue executing the remaining code
		sys_bypassPageFault(3);

		for (i = 0; i < 15; ++i)
		{
			ptr = (char *) ptr_allocations[i];
			ptr[0] = 10;
			//cprintf("\n\ncr2 = %x, faulted addr = %x", sys_rcr2(), (uint32)&(ptr[0]));
			if (sys_rcr2() != (uint32)&(ptr[0])) if (!f) {f=1; cprintf("\nkfree: successful access to freed space!! it should not be succeeded");}
			ptr[lastIndices[i]] = 10;
			if (sys_rcr2() != (uint32)&(ptr[lastIndices[i]])) if (!f) {f=1; cprintf("\nkfree: successful access to freed space!! it should not be succeeded");}
		}

		//set it to 0 again to cancel the bypassing option
		sys_bypassPageFault(0);
	}

	//kfree non-exist item
	{
		//kfree 2 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[0]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 0) if (!f) {f=1; cprintf("\nWrong kfree: attempt to kfree a non-existing ptr. It should do nothing");}

		//kfree 2 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[2]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 0) if (!f) {f=1; cprintf("\nWrong kfree: attempt to kfree a non-existing ptr. It should do nothing");}

		//kfree 20 KB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[8]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 0) if (!f) {f=1; cprintf("\nWrong kfree: attempt to kfree a non-existing ptr. It should do nothing");}

		//kfree 1 MB
		freeFrames = sys_calculate_free_frames() ;
		freeDiskFrames = pf_calculate_free_frames() ;
		kfree(ptr_allocations[9]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0) if (!f) {f=1; cprintf("\nPage file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");}
		if ((sys_calculate_free_frames() - freeFrames) != 0) if (!f) {f=1; cprintf("\nWrong kfree: attempt to kfree a non-existing ptr. It should do nothing");}

	}

	if (!f) {ce += 10;cprintf("\nkfreelast: current evaluation = %d%\n", ce);} f=0;

	//cprintf("\nCongratulations!! your modification is run successfully.\n");

	return 1;
}

int test_krealloc() {
	cprintf("==============================================\n");
	cprintf(
			"MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");
	char minByte = 1 << 7;
	char maxByte = 0x7F;
	short minShort = 1 << 15;
	short maxShort = 0x7FFF;
	int minInt = 1 << 31;
	int maxInt = 0x7FFFFFFF;
	char *byteArr, *byteArr2;
	short *shortArr, *shortArr2;
	int *intArr;
	struct MyStruct *structArr;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2,
	lastIndexOfInt, lastIndexOfStruct;
	//[1] Test calling krealloc with VA = NULL. It should call malloc
	void* ptr_allocations[20] = { 0 };
	char* ptr;
	void* newAddress = NULL;
	int freeDiskFrames;


	int lastIndices[20] = { 0 };
	int sums[20] = { 0 };
	int freeFrames;
	//[1] Allocate all
	{
		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[0] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[0] < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[0] != ACTUAL_START)
			panic("krealloc: Wrong start address for allocated space");

		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");

		lastIndices[0] = (1 * Mega - kilo) / sizeof(char) - 1;

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[1] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[1] < (KERNEL_HEAP_START + 1 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[1] != ACTUAL_START + (1 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");

		lastIndices[1] = (1 * Mega - kilo) / sizeof(char) - 1;

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[2] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[2] < (KERNEL_HEAP_START + 2 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[2] != ACTUAL_START + (2 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");
		lastIndices[2] = (1 * Mega - kilo) / sizeof(int) - 1;

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[3] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[3] < (KERNEL_HEAP_START + 3 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[3] != ACTUAL_START + (3 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");
		lastIndices[3] = (1 * Mega - kilo) / sizeof(int) - 1;

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[4] = krealloc(NULL, 2 * Mega - kilo);
		if ((uint32) ptr_allocations[4] < (KERNEL_HEAP_START + 4 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[4] != ACTUAL_START + (4 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 512)
			panic("krealloc: Wrong allocation: ");
		lastIndices[4] = (2 * Mega - kilo) / sizeof(short) - 1;

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[5] = krealloc(NULL, 2 * Mega - kilo);
		if ((uint32) ptr_allocations[5] < (KERNEL_HEAP_START + 6 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[5] != ACTUAL_START + (6 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 512)
			panic("krealloc: Wrong allocation: ");
		lastIndices[5] = (2 * Mega - kilo) / sizeof(short) - 1;

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[6] = krealloc(NULL, 3 * Mega - kilo);
		if ((uint32) ptr_allocations[6] < (KERNEL_HEAP_START + 8 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[6] != ACTUAL_START + (8 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 768)
			panic("Wrong allocation: ");
		lastIndices[6] = (3 * Mega - kilo) / sizeof(struct MyStruct) - 1;

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[7] = krealloc(NULL, 3 * Mega - kilo);
		if ((uint32) ptr_allocations[7] < (KERNEL_HEAP_START + 11 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[7] != ACTUAL_START + (11 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 768)
			panic("krealloc: Wrong allocation: ");
		lastIndices[7] = (3 * Mega - kilo) / sizeof(struct MyStruct) - 1;
	}


	char *byteArr1;
	//[3] Test read write access
	{
		//cprintf("\nTest read write access");
		//Test access for the first 1 MB
		freeFrames = sys_calculate_free_frames();

		//Write values
		//In 1st 1 MB
		lastIndexOfByte = (1 * Mega - kilo) / sizeof(char) - 1;
		byteArr = (char *) ptr_allocations[0];
		byteArr[0] = minByte;
		byteArr[lastIndexOfByte] = maxByte;

		//In 2nd 1 MB
		ptr = (char*) ptr_allocations[1];
		for (int i = 0; i <= lastIndices[1]; ++i) {
			ptr[i] = 2;
		}

		//In 3rd 1 MB
		intArr = (int*) ptr_allocations[2];
		intArr[0] = 3;
		intArr[lastIndices[2]] = 3;

		//In 4th 1 MB
		intArr = (int*) ptr_allocations[3];
		for (int i = 0; i <= lastIndices[3]; ++i) {
			intArr[i] = 4;
		}

		//In 1st 2 MB
		shortArr = (short*) ptr_allocations[4];
		for (int i = 0; i <= lastIndices[4]; ++i) {
			shortArr[i] = 5;
		}

		//In the 2nd 2 MB
		shortArr = (short*) ptr_allocations[5];
		shortArr[0] = 6;
		shortArr[lastIndices[5]] = 6;

		//In the 1st 3 MB
		structArr = (struct MyStruct *) ptr_allocations[6];
		for (int i = 0; i <= lastIndices[6]; i++) {
			structArr[i].a = 7;
			structArr[i].b = 7;
			structArr[i].c = 7;
		}

		//In the last 3 MB
		structArr = (struct MyStruct*) ptr_allocations[7];
		structArr[0].a = 8;
		structArr[0].b = 8;
		structArr[0].c = 8;
		structArr[lastIndices[7]].a = 8;
		structArr[lastIndices[7]].b = 8;
		structArr[lastIndices[7]].c = 8;

		//Read values: check that the values are successfully written
		if (byteArr[0] != minByte || byteArr[lastIndices[0]] != maxByte)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		ptr = (char*) ptr_allocations[1];
		if (ptr[0] != 2 || ptr[lastIndices[1]] != 2)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!, char = %c",
					ptr[0]);

		intArr = (int*) ptr_allocations[2];
		if (intArr[0] != 3 || intArr[lastIndices[2]] != 3)
			panic("Wrong allocation stored values are wrongly changed!");

		intArr = (int*) ptr_allocations[3];
		if (intArr[0] != 4 || intArr[lastIndices[3]] != 4)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		shortArr = (short*) ptr_allocations[4];
		if (shortArr[0] != 5 || shortArr[lastIndices[4]] != 5)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		shortArr = (short*) ptr_allocations[5];
		if (shortArr[0] != 6 || shortArr[lastIndices[5]] != 6)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		structArr = (struct MyStruct*) ptr_allocations[6];
		if (structArr[0].a != 7 || structArr[lastIndices[6]].a != 7)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].b != 7 || structArr[lastIndices[6]].b != 7)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].c != 7 || structArr[lastIndices[6]].c != 7)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		structArr = (struct MyStruct*) ptr_allocations[7];
		if (structArr[0].a != 8 || structArr[lastIndices[7]].a != 8)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].b != 8 || structArr[lastIndices[7]].b != 8)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].c != 8 || structArr[lastIndices[7]].c != 8)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		if ((freeFrames - sys_calculate_free_frames()) != 0)
			panic(
					"krealloc: Wrong allocation pages are not loaded successfully into memory");

	}
	cprintf("\nkrealloc: current evaluation = 10%");

	//[3] Test krealloc by passing size = 0
	{
		//kfree 1st 1 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[0], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"krealloc: Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256)
			panic("krealloc: pages in memory are not freed correctly");

		//kfree 3rd 1 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();

		krealloc(ptr_allocations[2], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"krealloc: Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256)
			panic("krealloc: pages in memory are not freed correctly");

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[5], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 2 * Mega / PAGE_SIZE)
			panic("krealloc: pages in memory are not freed correctly");

		//kfree last 3 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[7], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"krealloc: Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 3 * Mega / PAGE_SIZE)
			panic(
					"krealloc: Wrong kfree: pages in memory are not freed correctly");
		//check tables	[15%]
		{
			long long va;
			for (va = KERNEL_HEAP_START; va < (long long) KERNEL_HEAP_MAX; va +=
					PTSIZE)
			{
				uint32 *ptr_table;
				get_page_table(ptr_page_directory,  (uint32) va,
						&ptr_table);
				if (ptr_table == NULL) {
					panic(
							"Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree");
				}
			}
		}
	}
	cprintf("\b\b\b20%");
	//Check memory access after kfree by checking sum
	{
		//2nd 1 MB
		//cprintf("2nd 1 MB\n");
		ptr = (char*) ptr_allocations[1];
		int i;
		for (i = 0; i <= lastIndices[1]; ++i) {
			sums[0] += ptr[i];

		}
		//cprintf("sum for 2nd 1 MB = %d LIM1 = %d\n", sums[0], (lastIndices[1] - 1));
		if (sums[0] != (lastIndices[1] + 1) * 2)
			panic("krealloc: invalid read after freeing some allocations");

		//4th 1 MB
		//cprintf("4th 1 MB\n");
		intArr = (int*) ptr_allocations[3];

		for (i = 0; i <= lastIndices[3]; ++i) {
			sums[1] += intArr[i];
		}
		if (sums[1] != (lastIndices[3] + 1) * 4)
			panic("krealloc: invalid read after freeing some allocations");

		//1st 2 MB
		//cprintf("1st 2 MB\n");
		shortArr = (short*) ptr_allocations[4];

		for (i = 0; i <= lastIndices[4]; ++i) {
			sums[2] += shortArr[i];
		}
		if (sums[2] != (lastIndices[4] + 1) * 5)
			panic("krealloc: invalid read after freeing some allocations");

		//1st 3 MB
		//cprintf("1st 3 MB\n");
		structArr = (struct MyStruct*) ptr_allocations[6];

		for (i = 0; i <= lastIndices[6]; ++i) {
			sums[3] += structArr[i].a;
			sums[4] += structArr[i].b;
			sums[5] += structArr[i].c;
		}
		if (sums[3] != (lastIndices[6] + 1) * 7
				|| sums[4] != (lastIndices[6] + 1) * 7
				|| sums[5] != (lastIndices[6] + 1) * 7)
			panic("krealloc: invalid read after freeing some allocations");

	}

	//[4] Test krealloc reallocation with valid and invalid sizes
	{
		int freeDiskFrames;
		void* newAddress = NULL;
		//Try to reallocate 2nd 1 MB with a size smaller than its current size (it should return the same VA and do nothing)
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[1], 15 * kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[1])
			panic(
					"krealloc: Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");

		//Try to reallocate 1st 2 MB with a size smaller than its current size (it should return the same VA and do nothing)
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[4], 1 * Mega - kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[4])
			panic(
					"krealloc: Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");

		//Try to reallocate 2nd 1 MB with the same size it should return the same VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[1], 1 * Mega - kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[1])
			panic(
					"krealloc: Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");



		//Try to reallocate 4th 1 MB with the same size it should return the same VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[3], 1 * Mega - kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[3])
			panic(
					"Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");
	}
	cprintf("\b\b\b30%");
	{
		//Reallocate 2nd 1 MB to 1 MB + 7 KB
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[1],
				(1 * Mega - kilo) + (7 * kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[1])
			panic(
					"Wrong allocation: krealloc reallocated a new address while there is a sufficient space after it (it should return same VA)");
		if (freeFrames - sys_calculate_free_frames() != 2)
			panic("krealloc: pages in memory are not loaded correctly");

		//Reallocate 1st 2 MB to 2 MB + 2 MB
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[4], (4 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[4])
			panic(
					"Wrong allocation: krealloc reallocated a new address while there is a sufficient space after it (it should return same VA)");
		//2 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 512)
			panic("krealloc: pages in memory are not loaded correctly");


	}
	cprintf("\b\b\b60%");
	//Test krealloc: Cut & paste
	{
		//Reallocate 1st 2 MB (already reallocated to 4 MB) to 10 MB. It should return new VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[4], (10 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) newAddress != ACTUAL_START + (14 * Mega))
			panic(
					"krealloc: Wrong start address for reallocated space, NSA = %x\nbbb",
					(uint32 )newAddress);
		if (newAddress == ptr_allocations[4])
			panic(
					"Wrong allocation: krealloc reallocated at the same address while there is NO sufficient space after it (it should return new VA)");
		//6 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 1536)
			panic("krealloc: pages in memory are not loaded correctly");

		ptr_allocations[4] = newAddress;
		//lastIndices[4] = (10 * Mega - kilo) / sizeof(short) - 1;

		//Reallocate 1st 3 MB to 6 MB
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[6], (6 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[6])
			panic(
					"Wrong allocation: krealloc reallocated a new address while there is a sufficient space after it (it should return same VA)");
		//3 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 768)
			panic("krealloc: pages in memory are not loaded correctly");

		//Reallocate 1st 3 MB (already reallocated to 6 MB) to 20 MB. It should return new VA

		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[6], (20 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress == ptr_allocations[6])
			panic(
					"Wrong allocation: krealloc reallocated at the same address while there is NO sufficient space after it (it should return new VA)");
		if ((uint32) newAddress != ACTUAL_START + (24 * Mega))
			panic("krealloc: Wrong start address for reallocated space");
		//3 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 3584)
			panic("krealloc: pages in memory are not loaded correctly");

		ptr_allocations[6] = newAddress;

		//Test read write access for the new allocated size of 2nd 1 MB
		ptr = (char*) ptr_allocations[1];
		int i;
		sums[0] = 0;
		for (i = 0; i <= lastIndices[1]; ++i) {
			sums[0] += ptr[i];

		}
		//cprintf("sum for 2nd 1 MB = %d LIM1 = %d\n", sums[0], (lastIndices[1] - 1));
		if (sums[0] != (lastIndices[1] + 1) * 2)
			panic("krealloc: invalid read after re-allocations");

		//Test read write access for the new allocated size of 1st 3 MB
		structArr = (struct MyStruct*) ptr_allocations[6];

		sums[0] = 0;
		sums[1] = 0;
		sums[2] = 0;
		for (i = 0; i <= lastIndices[6]; ++i) {
			sums[0] += structArr[i].a;
			sums[1] += structArr[i].b;
			sums[2] += structArr[i].c;

		}
		//cprintf("sum for 2nd 1 MB = %d LIM1 = %d\n", sums[0], (lastIndices[1] - 1));
		if (sums[0] != (lastIndices[6] + 1) * 7
				|| sums[1] != (lastIndices[6] + 1) * 7
				|| sums[2] != (lastIndices[6] + 1) * 7)
			panic("krealloc: invalid read after re-allocations");

		//Test read write access for the new allocated size of 1st 2 MB
		shortArr = (short*) ptr_allocations[4];

		sums[0] = 0;
		for (i = 0; i <= lastIndices[4]; ++i) {
			sums[0] += shortArr[i];
		}
		if (sums[0] != (lastIndices[4] + 1) * 5)
			panic("krealloc: invalid read after re-allocations");

		//Test krealloc with size = 0 after krealloc 1st 3 MB to 20 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[6], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 20 * Mega / PAGE_SIZE)
			panic("krealloc: pages in memory are not freed correctly");

		//Test kfree after krealloc 1st 2 MB to 10 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[4]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 10 * Mega / PAGE_SIZE)
			panic("krealloc: pages in memory are not freed correctly");

		//check tables	[15%]
		{
			long long va;
			for (va = KERNEL_HEAP_START; va < (long long) KERNEL_HEAP_MAX; va +=
					PTSIZE)
			{
				uint32 *ptr_table;
				get_page_table(ptr_page_directory,  (uint32) va,
						&ptr_table);
				if (ptr_table == NULL) {
					panic(
							"Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree");
				}
			}
		}

	}

	cprintf("\b\b\b100%\n");

	cprintf("\nCongratulations!! test krealloc completed successfully.\n");
	return 0;
}


int test_krealloc_BF() {
	cprintf("==============================================\n");
	cprintf(
			"MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("==============================================\n");
	char minByte = 1 << 7;
	char maxByte = 0x7F;
	short minShort = 1 << 15;
	short maxShort = 0x7FFF;
	int minInt = 1 << 31;
	int maxInt = 0x7FFFFFFF;
	char *byteArr, *byteArr2;
	short *shortArr, *shortArr2;
	int *intArr;
	struct MyStruct *structArr;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2,
	lastIndexOfInt, lastIndexOfStruct;
	//[1] Test calling krealloc with VA = NULL. It should call malloc
	void* ptr_allocations[20] = { 0 };
	char* ptr;
	void* newAddress = NULL;
	int freeDiskFrames;


	int lastIndices[20] = { 0 };
	int sums[20] = { 0 };
	int freeFrames;
	//[1] Allocate all
	{
		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[0] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[0] < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[0] != ACTUAL_START)
			panic("krealloc: Wrong start address for allocated space");

		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");

		lastIndices[0] = (1 * Mega - kilo) / sizeof(char) - 1;

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[1] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[1] < (KERNEL_HEAP_START + 1 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[1] != ACTUAL_START + (1 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");

		lastIndices[1] = (1 * Mega - kilo) / sizeof(char) - 1;

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[2] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[2] < (KERNEL_HEAP_START + 2 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[2] != ACTUAL_START + (2 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");
		lastIndices[2] = (1 * Mega - kilo) / sizeof(int) - 1;

		//Allocate 1 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[3] = krealloc(NULL, 1 * Mega - kilo);
		if ((uint32) ptr_allocations[3] < (KERNEL_HEAP_START + 3 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[3] != ACTUAL_START + (3 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 256)
			panic("krealloc: Wrong allocation: ");
		lastIndices[3] = (1 * Mega - kilo) / sizeof(int) - 1;

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[4] = krealloc(NULL, 2 * Mega - kilo);
		if ((uint32) ptr_allocations[4] < (KERNEL_HEAP_START + 4 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[4] != ACTUAL_START + (4 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 512)
			panic("krealloc: Wrong allocation: ");
		lastIndices[4] = (2 * Mega - kilo) / sizeof(short) - 1;

		//Allocate 2 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[5] = krealloc(NULL, 2 * Mega - kilo);
		if ((uint32) ptr_allocations[5] < (KERNEL_HEAP_START + 6 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[5] != ACTUAL_START + (6 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 512)
			panic("krealloc: Wrong allocation: ");
		lastIndices[5] = (2 * Mega - kilo) / sizeof(short) - 1;

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[6] = krealloc(NULL, 3 * Mega - kilo);
		if ((uint32) ptr_allocations[6] < (KERNEL_HEAP_START + 8 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[6] != ACTUAL_START + (8 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 768)
			panic("Wrong allocation: ");
		lastIndices[6] = (3 * Mega - kilo) / sizeof(struct MyStruct) - 1;

		//Allocate 3 MB
		freeFrames = sys_calculate_free_frames();
		ptr_allocations[7] = krealloc(NULL, 3 * Mega - kilo);
		if ((uint32) ptr_allocations[7] < (KERNEL_HEAP_START + 11 * Mega))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) ptr_allocations[7] != ACTUAL_START + (11 * Mega))
			panic("krealloc: Wrong start address for allocated space");
		if ((freeFrames - sys_calculate_free_frames()) != 768)
			panic("krealloc: Wrong allocation: ");
		lastIndices[7] = (3 * Mega - kilo) / sizeof(struct MyStruct) - 1;
	}


	char *byteArr1;
	//[3] Test read write access
	{
		//cprintf("\nTest read write access");
		//Test access for the first 1 MB
		freeFrames = sys_calculate_free_frames();

		//Write values
		//In 1st 1 MB
		lastIndexOfByte = (1 * Mega - kilo) / sizeof(char) - 1;
		byteArr = (char *) ptr_allocations[0];
		byteArr[0] = minByte;
		byteArr[lastIndexOfByte] = maxByte;

		//In 2nd 1 MB
		ptr = (char*) ptr_allocations[1];
		for (int i = 0; i <= lastIndices[1]; ++i) {
			ptr[i] = 2;
		}

		//In 3rd 1 MB
		intArr = (int*) ptr_allocations[2];
		intArr[0] = 3;
		intArr[lastIndices[2]] = 3;

		//In 4th 1 MB
		intArr = (int*) ptr_allocations[3];
		for (int i = 0; i <= lastIndices[3]; ++i) {
			intArr[i] = 4;
		}

		//In 1st 2 MB
		shortArr = (short*) ptr_allocations[4];
		for (int i = 0; i <= lastIndices[4]; ++i) {
			shortArr[i] = 5;
		}

		//In the 2nd 2 MB
		shortArr = (short*) ptr_allocations[5];
		shortArr[0] = 6;
		shortArr[lastIndices[5]] = 6;

		//In the 1st 3 MB
		structArr = (struct MyStruct *) ptr_allocations[6];
		for (int i = 0; i <= lastIndices[6]; i++) {
			structArr[i].a = 7;
			structArr[i].b = 7;
			structArr[i].c = 7;
		}

		//In the last 3 MB
		structArr = (struct MyStruct*) ptr_allocations[7];
		structArr[0].a = 8;
		structArr[0].b = 8;
		structArr[0].c = 8;
		structArr[lastIndices[7]].a = 8;
		structArr[lastIndices[7]].b = 8;
		structArr[lastIndices[7]].c = 8;

		//Read values: check that the values are successfully written
		if (byteArr[0] != minByte || byteArr[lastIndices[0]] != maxByte)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		ptr = (char*) ptr_allocations[1];
		if (ptr[0] != 2 || ptr[lastIndices[1]] != 2)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!, char = %c",
					ptr[0]);

		intArr = (int*) ptr_allocations[2];
		if (intArr[0] != 3 || intArr[lastIndices[2]] != 3)
			panic("Wrong allocation stored values are wrongly changed!");

		intArr = (int*) ptr_allocations[3];
		if (intArr[0] != 4 || intArr[lastIndices[3]] != 4)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		shortArr = (short*) ptr_allocations[4];
		if (shortArr[0] != 5 || shortArr[lastIndices[4]] != 5)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		shortArr = (short*) ptr_allocations[5];
		if (shortArr[0] != 6 || shortArr[lastIndices[5]] != 6)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		structArr = (struct MyStruct*) ptr_allocations[6];
		if (structArr[0].a != 7 || structArr[lastIndices[6]].a != 7)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].b != 7 || structArr[lastIndices[6]].b != 7)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].c != 7 || structArr[lastIndices[6]].c != 7)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		structArr = (struct MyStruct*) ptr_allocations[7];
		if (structArr[0].a != 8 || structArr[lastIndices[7]].a != 8)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].b != 8 || structArr[lastIndices[7]].b != 8)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");
		if (structArr[0].c != 8 || structArr[lastIndices[7]].c != 8)
			panic(
					"krealloc: Wrong allocation stored values are wrongly changed!");

		if ((freeFrames - sys_calculate_free_frames()) != 0)
			panic(
					"krealloc: Wrong allocation pages are not loaded successfully into memory");

	}
	cprintf("\nkrealloc: current evaluation = 10%");

	//[3] Test krealloc by passing size = 0
	{
		//kfree 1st 1 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[0], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"krealloc: Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256)
			panic("krealloc: pages in memory are not freed correctly");

		//kfree 3rd 1 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();

		krealloc(ptr_allocations[2], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"krealloc: Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 256)
			panic("krealloc: pages in memory are not freed correctly");

		//kfree 2nd 2 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[5], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 2 * Mega / PAGE_SIZE)
			panic("krealloc: pages in memory are not freed correctly");

		//kfree last 3 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[7], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"krealloc: Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 3 * Mega / PAGE_SIZE)
			panic(
					"krealloc: Wrong kfree: pages in memory are not freed correctly");
		//check tables	[15%]
		{
			long long va;
			for (va = KERNEL_HEAP_START; va < (long long) KERNEL_HEAP_MAX; va +=
					PTSIZE)
			{
				uint32 *ptr_table;
				get_page_table(ptr_page_directory,  (uint32) va,
						&ptr_table);
				if (ptr_table == NULL) {
					panic(
							"Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree");
				}
			}
		}
	}
	cprintf("\b\b\b20%");
	//Check memory access after kfree by checking sum
	{
		//2nd 1 MB
		//cprintf("2nd 1 MB\n");
		ptr = (char*) ptr_allocations[1];
		int i;
		for (i = 0; i <= lastIndices[1]; ++i) {
			sums[0] += ptr[i];

		}
		//cprintf("sum for 2nd 1 MB = %d LIM1 = %d\n", sums[0], (lastIndices[1] - 1));
		if (sums[0] != (lastIndices[1] + 1) * 2)
			panic("krealloc: invalid read after freeing some allocations");

		//4th 1 MB
		//cprintf("4th 1 MB\n");
		intArr = (int*) ptr_allocations[3];

		for (i = 0; i <= lastIndices[3]; ++i) {
			sums[1] += intArr[i];
		}
		if (sums[1] != (lastIndices[3] + 1) * 4)
			panic("krealloc: invalid read after freeing some allocations");

		//1st 2 MB
		//cprintf("1st 2 MB\n");
		shortArr = (short*) ptr_allocations[4];

		for (i = 0; i <= lastIndices[4]; ++i) {
			sums[2] += shortArr[i];
		}
		if (sums[2] != (lastIndices[4] + 1) * 5)
			panic("krealloc: invalid read after freeing some allocations");

		//1st 3 MB
		//cprintf("1st 3 MB\n");
		structArr = (struct MyStruct*) ptr_allocations[6];

		for (i = 0; i <= lastIndices[6]; ++i) {
			sums[3] += structArr[i].a;
			sums[4] += structArr[i].b;
			sums[5] += structArr[i].c;
		}
		if (sums[3] != (lastIndices[6] + 1) * 7
				|| sums[4] != (lastIndices[6] + 1) * 7
				|| sums[5] != (lastIndices[6] + 1) * 7)
			panic("krealloc: invalid read after freeing some allocations");

	}

	//[4] Test krealloc reallocation with valid and invalid sizes
	{
		int freeDiskFrames;
		void* newAddress = NULL;
		//Try to reallocate 2nd 1 MB with a size smaller than its current size (it should return the same VA and do nothing)
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[1], 15 * kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[1])
			panic(
					"krealloc: Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");

		//Try to reallocate 1st 2 MB with a size smaller than its current size (it should return the same VA and do nothing)
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[4], 1 * Mega - kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[4])
			panic(
					"krealloc: Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");

		//Try to reallocate 2nd 1 MB with the same size it should return the same VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[1], 1 * Mega - kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[1])
			panic(
					"krealloc: Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");



		//Try to reallocate 4th 1 MB with the same size it should return the same VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[3], 1 * Mega - kilo);
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[3])
			panic(
					"Wrong allocation: krealloc reallocated an address with the same size (it should return same VA)");
		if (freeFrames != sys_calculate_free_frames())
			panic(
					"krealloc: Wrong number of frames after krealloc with the same size");
	}
	cprintf("\b\b\b30%");
	{
		//Reallocate 2nd 1 MB to 1 MB + 7 KB
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[1], (1 * Mega - kilo) + (7 * kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[1])
			panic(
					"Wrong allocation: krealloc reallocated a new address while there is a sufficient space after it (it should return same VA)");
		if (freeFrames - sys_calculate_free_frames() != 2)
			panic("krealloc: pages in memory are not loaded correctly");

		//Reallocate 1st 2 MB to 2 MB + 2 MB
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[4], (4 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress != ptr_allocations[4])
			panic(
					"Wrong allocation: krealloc reallocated a new address while there is a sufficient space after it (it should return same VA)");
		//2 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 512)
			panic("krealloc: pages in memory are not loaded correctly");


	}
	cprintf("\b\b\b60%");
	//Test krealloc: Cut & paste
	{
		//Reallocate 1st 2 MB (already reallocated to 4 MB) to 10 MB. It should return new VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[4], (10 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if ((uint32) newAddress != ACTUAL_START + (11 * Mega))
			panic(
					"krealloc: Wrong start address for reallocated space, NSA = %x\nbbb",
					(uint32 )newAddress);
		if (newAddress == ptr_allocations[4])
			panic(
					"Wrong allocation: krealloc reallocated at the same address while there is NO sufficient space after it (it should return new VA)");
		//6 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 1536)
			panic("krealloc: pages in memory are not loaded correctly");

		ptr_allocations[4] = newAddress;
		//lastIndices[4] = (10 * Mega - kilo) / sizeof(short) - 1;

		//Reallocate 1st 3 MB to 4 MB
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[6], (4 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress == ptr_allocations[6])
			panic("Wrong allocation: krealloc reallocated at the same address while there is NO sufficient space after it (it should return new VA)");
		if ((uint32)newAddress != ACTUAL_START + 4 * Mega) panic("krealloc: Wrong start address for allocated space");

		//1 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != 256)
			panic("krealloc: pages in memory are not loaded correctly");
		ptr_allocations[6] = newAddress;
		//Reallocate 1st 3 MB (already reallocated to 4 MB) to 20 MB. It should return new VA
		freeFrames = sys_calculate_free_frames();
		newAddress = krealloc(ptr_allocations[6], (20 * Mega - kilo));
		if ((uint32) newAddress < (KERNEL_HEAP_START))
			panic("krealloc: Wrong start address for the allocated space... ");
		if (newAddress == ptr_allocations[6])
			panic("Wrong allocation: krealloc reallocated at the same address while there is NO sufficient space after it (it should return new VA)");
		if ((uint32) newAddress != ACTUAL_START + (21 * Mega))
			panic("krealloc: Wrong start address for reallocated space\n");
		//3 MB only for the new size
		if (freeFrames - sys_calculate_free_frames() != (16 * Mega) / PAGE_SIZE)
			panic("krealloc: pages in memory are not loaded correctly");

		ptr_allocations[6] = newAddress;

		//Test read write access for the new allocated size of 2nd 1 MB
		ptr = (char*) ptr_allocations[1];
		int i;
		sums[0] = 0;
		for (i = 0; i <= lastIndices[1]; ++i) {
			sums[0] += ptr[i];

		}
		//cprintf("sum for 2nd 1 MB = %d LIM1 = %d\n", sums[0], (lastIndices[1] - 1));
		if (sums[0] != (lastIndices[1] + 1) * 2)
			panic("krealloc: invalid read after re-allocations");

		//Test read write access for the new allocated size of 1st 3 MB
		structArr = (struct MyStruct*) ptr_allocations[6];

		sums[0] = 0;
		sums[1] = 0;
		sums[2] = 0;
		for (i = 0; i <= lastIndices[6]; ++i) {
			sums[0] += structArr[i].a;
			sums[1] += structArr[i].b;
			sums[2] += structArr[i].c;

		}
		//cprintf("sum for 2nd 1 MB = %d LIM1 = %d\n", sums[0], (lastIndices[1] - 1));
		if (sums[0] != (lastIndices[6] + 1) * 7
				|| sums[1] != (lastIndices[6] + 1) * 7
				|| sums[2] != (lastIndices[6] + 1) * 7)
			panic("krealloc: invalid read after re-allocations");

		//Test read write access for the new allocated size of 1st 2 MB
		shortArr = (short*) ptr_allocations[4];

		sums[0] = 0;
		for (i = 0; i <= lastIndices[4]; ++i) {
			sums[0] += shortArr[i];
		}
		if (sums[0] != (lastIndices[4] + 1) * 5)
			panic("krealloc: invalid read after re-allocations");

		//Test krealloc with size = 0 after krealloc 1st 3 MB to 20 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		krealloc(ptr_allocations[6], 0);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 20 * Mega / PAGE_SIZE)
			panic("krealloc: pages in memory are not freed correctly");

		//Test kfree after krealloc 1st 2 MB to 10 MB
		freeFrames = sys_calculate_free_frames();
		freeDiskFrames = pf_calculate_free_frames();
		kfree(ptr_allocations[4]);
		if ((freeDiskFrames - pf_calculate_free_frames()) != 0)
			panic(
					"Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
		if ((sys_calculate_free_frames() - freeFrames) != 10 * Mega / PAGE_SIZE)
			panic("krealloc: pages in memory are not freed correctly");

		//check tables	[15%]
		{
			long long va;
			for (va = KERNEL_HEAP_START; va < (long long) KERNEL_HEAP_MAX; va +=
					PTSIZE)
			{
				uint32 *ptr_table;
				get_page_table(ptr_page_directory,  (uint32) va,
						&ptr_table);
				if (ptr_table == NULL) {
					panic(
							"Wrong kfree: one of the kernel tables is wrongly removed! Tables should not be removed here in kfree");
				}
			}
		}

	}

	cprintf("\b\b\b100%\n");

	cprintf("\nCongratulations!! test krealloc BF completed successfully.\n");
	return 0;
}

//2022
int test_initialize_dyn_block_system(int freeFrames_before, int freeDiskFrames_before, int freeFrames_after, int freeDiskFrames_after)
{
	/*	if(USE_KHEAP != 1)
		panic("USE_KHEAP = 0 & it shall be 1. Go to 'inc/memlayout.h' and set USE_KHEAP by 1. Then, repeat the test again.");
	if(STATIC_MEMBLOCK_ALLOC != 0)
		panic("STATIC_MEMBLOCK_ALLOC = 1 & it shall be 0. Go to 'inc/dynamic_allocator.h' and set STATIC_MEMBLOCK_ALLOC by 0. Then, repeat the test again.");

	//Check MAX_MEM_BLOCK_CNT
	if(MAX_MEM_BLOCK_CNT != ((0xFFFFF000-0xF6000000)/4096))
	{
		panic("Wrong initialize: MAX_MEM_BLOCK_CNT is not set with the correct size of the array");
	}

	//Check number of nodes in AvailableMemBlocksList
	if (LIST_SIZE(&(AvailableMemBlocksList)) != MAX_MEM_BLOCK_CNT-1)
	{
		panic("Wrong initialize: Wrong size for the AvailableMemBlocksList");
	}

	//Check number of nodes in AllocMemBlocksList
	if (LIST_SIZE(&(AllocMemBlocksList)) != 0)
	{
		panic("Wrong initialize: Wrong size for the AllocMemBlocksList");
	}

	//Check number of nodes in FreeMemBlocksList
	if (LIST_SIZE(&(FreeMemBlocksList)) != 1)
	{
		panic("Wrong initialize: Wrong size for the FreeMemBlocksList");
	}

	//Check content of FreeMemBlocksList
	struct MemBlock* block = LIST_FIRST(&FreeMemBlocksList);
	if(block == NULL || block->size != (KERNEL_HEAP_MAX-0xF6000000-DYNAMIC_ALLOCATOR_DS) || block->sva != 0xF6000000+DYNAMIC_ALLOCATOR_DS)
	{
		panic("Wrong initialize: Wrong content for the FreeMemBlocksList.");
	}

	//Check number of disk and memory frames
	if ((freeDiskFrames_after - freeDiskFrames_before) != 0) panic("Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)");
	if ((freeFrames_before - freeFrames_after) != 160) panic("Wrong allocation: pages are not loaded successfully into memory %d", (freeFrames_before - freeFrames_after));

	//Checking permissions on the allocated spaces
	{
		uint32 a = 0xF6000000;
		while(1)
		{
			if (CB(ptr_page_directory, a, 0)!=1 || CB(ptr_page_directory, a, 1)!=1 || CB(ptr_page_directory, a, 2)!=0)
				panic("Wrong initialize: Wrong permissions - pages mapped with wrong permissions. Check them again.");

			a+=4096;
			if(a == (0xF6000000+DYNAMIC_ALLOCATOR_DS))
				break;
		}
	}

	//===============================================//
	cprintf("\nCongratulations!! test initialize_dyn_block_system of KHEAP completed successfully.\n");
	 */	return 0;
}
