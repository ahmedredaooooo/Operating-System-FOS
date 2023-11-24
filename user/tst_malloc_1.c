/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 2000 */
/* *********************************************************** */

#include <inc/lib.h>

struct MyStruct
{
	char a;
	short b;
	int c;
};

void _main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN DYNAMIC ALLOCATOR DUE TO sbrk()
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH).
	 *********************************************************/

	//cprintf("1\n");
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
#if USE_KHEAP
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif

	//	/*Dummy malloc to enforce the UHEAP initializations*/
	//	malloc(0);
	/*=================================================*/

	//cprintf("2\n");

	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB

	int Mega = 1024*1024;
	int kilo = 1024;
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
	int freeFrames, usedDiskPages, found;
	int expectedNumOfFrames, actualNumOfFrames;
	void* ptr_allocations[20] = {0};
	{
		//cprintf("3\n");

		//2 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[0] = malloc(2*Mega-kilo);
			if ((uint32) ptr_allocations[0] != (pagealloc_start)) panic("Wrong start address for the allocated space... ");
			if ((freeFrames - sys_calculate_free_frames()) >= 512) panic("Wrong allocation: pages are allocated in memory while it's not supposed to!");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");


			freeFrames = sys_calculate_free_frames() ;
			lastIndexOfByte = (2*Mega-kilo)/sizeof(char) - 1;
			byteArr = (char *) ptr_allocations[0];
			byteArr[0] = minByte ;
			byteArr[lastIndexOfByte] = maxByte ;
			expectedNumOfFrames = 2 /*+1 table already created in malloc due to marking the allocated pages*/ ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (actualNumOfFrames < expectedNumOfFrames)
				panic("Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);

			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(byteArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr[lastIndexOfByte])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) panic("malloc: page is not added to WS");
		}
		//cprintf("4\n");

		//2 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[1] = malloc(2*Mega-kilo);
			if ((uint32) ptr_allocations[1] != (pagealloc_start + 2*Mega)) panic("Wrong start address for the allocated space... ");
			if ((freeFrames - sys_calculate_free_frames()) >= 512) panic("Wrong allocation: pages are allocated in memory while it's not supposed to!");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");

			freeFrames = sys_calculate_free_frames() ;
			shortArr = (short *) ptr_allocations[1];
			lastIndexOfShort = (2*Mega-kilo)/sizeof(short) - 1;
			shortArr[0] = minShort;
			shortArr[lastIndexOfShort] = maxShort;
			expectedNumOfFrames = 2 /*+1 table already created in malloc due to marking the allocated pages*/;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (actualNumOfFrames < expectedNumOfFrames)
				panic("Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);
			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(shortArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr[lastIndexOfShort])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) panic("malloc: page is not added to WS");
		}
		//cprintf("5\n");

		//3 KB
		{
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[2] = malloc(3*kilo);
			if ((uint32) ptr_allocations[2] != (pagealloc_start + 4*Mega)) panic("Wrong start address for the allocated space... ");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");

			freeFrames = sys_calculate_free_frames() ;
			intArr = (int *) ptr_allocations[2];
			lastIndexOfInt = (2*kilo)/sizeof(int) - 1;
			intArr[0] = minInt;
			intArr[lastIndexOfInt] = maxInt;
			expectedNumOfFrames = 1 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (actualNumOfFrames < expectedNumOfFrames)
				panic("Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);
			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(intArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(intArr[lastIndexOfInt])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) panic("malloc: page is not added to WS");
		}

		//3 KB
		{
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[3] = malloc(3*kilo);
			if ((uint32) ptr_allocations[3] != (pagealloc_start + 4*Mega + 4*kilo)) panic("Wrong start address for the allocated space... ");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");
		}

		//7 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[4] = malloc(7*kilo);
			if ((uint32) ptr_allocations[4] != (pagealloc_start + 4*Mega + 8*kilo)) panic("Wrong start address for the allocated space... ");
			if ((freeFrames - sys_calculate_free_frames()) >= 2) panic("Wrong allocation: pages are allocated in memory while it's not supposed to!");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");

			freeFrames = sys_calculate_free_frames() ;
			structArr = (struct MyStruct *) ptr_allocations[4];
			lastIndexOfStruct = (7*kilo)/sizeof(struct MyStruct) - 1;
			structArr[0].a = minByte; structArr[0].b = minShort; structArr[0].c = minInt;
			structArr[lastIndexOfStruct].a = maxByte; structArr[lastIndexOfStruct].b = maxShort; structArr[lastIndexOfStruct].c = maxInt;
			expectedNumOfFrames = 2 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (actualNumOfFrames < expectedNumOfFrames)
				panic("Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);
			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(structArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(structArr[lastIndexOfStruct])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) panic("malloc: page is not added to WS");
		}

		//3 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[5] = malloc(3*Mega-kilo);
			if ((uint32) ptr_allocations[5] != (pagealloc_start + 4*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
			if ((freeFrames - sys_calculate_free_frames()) >= 3*Mega/PAGE_SIZE) panic("Wrong allocation: pages are allocated in memory while it's not supposed to!");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");
		}

		//6 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[6] = malloc(6*Mega-kilo);
			if ((uint32) ptr_allocations[6] != (pagealloc_start + 7*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
			if ((freeFrames - sys_calculate_free_frames()) >= 6*Mega/PAGE_SIZE) panic("Wrong allocation: pages are allocated in memory while it's not supposed to!");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");

			freeFrames = sys_calculate_free_frames() ;
			lastIndexOfByte2 = (6*Mega-kilo)/sizeof(char) - 1;
			byteArr2 = (char *) ptr_allocations[6];
			byteArr2[0] = minByte ;
			byteArr2[lastIndexOfByte2 / 2] = maxByte / 2;
			byteArr2[lastIndexOfByte2] = maxByte ;
			expectedNumOfFrames = 3 /*+2 tables already created in malloc due to marking the allocated pages*/ ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (actualNumOfFrames < expectedNumOfFrames)
				panic("Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);
			uint32 expectedVAs[3] = { ROUNDDOWN((uint32)(&(byteArr2[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr2[lastIndexOfByte2/2])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr2[lastIndexOfByte2])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 3, 0, 2);
			if (found != 1) panic("malloc: page is not added to WS");
		}

		//14 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[7] = malloc(14*kilo);
			if ((uint32) ptr_allocations[7] != (pagealloc_start + 13*Mega + 16*kilo)) panic("Wrong start address for the allocated space... ");
			if ((freeFrames - sys_calculate_free_frames()) >= 16*kilo/PAGE_SIZE) panic("Wrong allocation: pages are allocated in memory while it's not supposed to!");
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) panic("Extra or less pages are allocated in PageFile");

			freeFrames = sys_calculate_free_frames() ;
			shortArr2 = (short *) ptr_allocations[7];
			lastIndexOfShort2 = (14*kilo)/sizeof(short) - 1;
			shortArr2[0] = minShort;
			shortArr2[lastIndexOfShort2 / 2] = maxShort / 2;
			shortArr2[lastIndexOfShort2] = maxShort;
			expectedNumOfFrames = 3 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (actualNumOfFrames < expectedNumOfFrames)
				panic("Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);
			uint32 expectedVAs[3] = { ROUNDDOWN((uint32)(&(shortArr2[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr2[lastIndexOfShort2/2])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr2[lastIndexOfShort2])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 3, 0, 2);
			if (found != 1) panic("malloc: page is not added to WS");
		}
	}

	//Check that the values are successfully stored
	{
		if (byteArr[0] 	!= minByte 	|| byteArr[lastIndexOfByte] 	!= maxByte) panic("Wrong allocation: stored values are wrongly changed!");
		if (shortArr[0] != minShort || shortArr[lastIndexOfShort] 	!= maxShort) panic("Wrong allocation: stored values are wrongly changed!");
		if (intArr[0] 	!= minInt 	|| intArr[lastIndexOfInt] 		!= maxInt) panic("Wrong allocation: stored values are wrongly changed!");

		if (structArr[0].a != minByte 	|| structArr[lastIndexOfStruct].a != maxByte) 	panic("Wrong allocation: stored values are wrongly changed!");
		if (structArr[0].b != minShort 	|| structArr[lastIndexOfStruct].b != maxShort) 	panic("Wrong allocation: stored values are wrongly changed!");
		if (structArr[0].c != minInt 	|| structArr[lastIndexOfStruct].c != maxInt) 	panic("Wrong allocation: stored values are wrongly changed!");

		if (byteArr2[0]  != minByte  || byteArr2[lastIndexOfByte2/2]   != maxByte/2 	|| byteArr2[lastIndexOfByte2] 	!= maxByte) panic("Wrong allocation: stored values are wrongly changed!");
		if (shortArr2[0] != minShort || shortArr2[lastIndexOfShort2/2] != maxShort/2 || shortArr2[lastIndexOfShort2] 	!= maxShort) panic("Wrong allocation: stored values are wrongly changed!");

	}
	cprintf("Congratulations!! test malloc (1) completed successfully.\n");

	return;
}
