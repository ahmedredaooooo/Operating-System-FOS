/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 2000 */
/* *********************************************************** */

#include <inc/lib.h>

#define Mega  (1024*1024)
#define kilo (1024)
#define numOfAllocs 7
#define allocCntPerSize 200
/*NOTE: these sizes include the size of MetaData within it &
 * ALL sizes are in increasing order to ensure that they allocated contiguously
 * & no later block can be allocated in a previous unused free block at the end of page
 */
uint32 allocSizes[numOfAllocs] = {3*sizeof(int) + sizeOfMetaData(), 2*sizeOfMetaData(), 20*sizeof(char) + sizeOfMetaData(), kilo/2, 1*kilo, 3*kilo/2, 2*kilo} ;
short* startVAs[numOfAllocs*allocCntPerSize+1] ;
short* midVAs[numOfAllocs*allocCntPerSize+1] ;
short* endVAs[numOfAllocs*allocCntPerSize+1] ;

void _main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN KERNEL DYNAMIC ALLOCATOR sbrk()
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH).
	 *********************************************************/

	//cprintf("1\n");
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
	/*=================================================*/


	int eval = 0;
	bool is_correct = 1;
	int targetAllocatedSpace = 3*Mega;

	void * va ;
	int idx = 0;
	bool chk;
	int usedDiskPages = sys_pf_calculate_allocated_pages() ;
	int freeFrames = sys_calculate_free_frames() ;

	//====================================================================//
	/*INITIAL ALLOC Scenario 1: Try to allocate set of blocks with different sizes*/
	cprintf("	1: Try to allocate set of blocks with different sizes [all should fit]\n\n") ;
	{
		is_correct = 1;
		void* curVA = (void*) USER_HEAP_START ;
		uint32 actualSize;
		for (int i = 0; i < numOfAllocs; ++i)
		{
			for (int j = 0; j < allocCntPerSize; ++j)
			{
				actualSize = allocSizes[i] - sizeOfMetaData();
				va = startVAs[idx] = malloc(actualSize);
				midVAs[idx] = va + actualSize/2 ;
				endVAs[idx] = va + actualSize - sizeof(short);
				//Check returned va
				if(va == NULL || (va < curVA))
				{
					if (is_correct)
					{
						is_correct = 0;
						cprintf("alloc_block_xx #1.%d: WRONG ALLOC - alloc_block_xx return wrong address. Expected %x, Actual %x\n", idx, curVA + sizeOfMetaData() ,va);
					}
				}
				curVA += allocSizes[i] ;

				//============================================================
				//Check if the remaining area doesn't fit the DynAllocBlock,
				//so update the curVA to skip this area
				void* rounded_curVA = ROUNDUP(curVA, PAGE_SIZE);
				int diff = (rounded_curVA - curVA) ;
				if (diff > 0 && diff < sizeOfMetaData())
				{
					curVA = rounded_curVA;
				}
				//============================================================
				*(startVAs[idx]) = idx ;
				*(midVAs[idx]) = idx ;
				*(endVAs[idx]) = idx ;
				idx++;
			}
			//if (is_correct == 0)
			//break;
		}
		if (is_correct)
		{
			eval += 30;
		}
	}

	//====================================================================//
	/*INITIAL ALLOC Scenario 2: Check stored data inside each allocated block*/
	cprintf("	2: Check stored data inside each allocated block\n\n") ;
	{
		is_correct = 1;

		for (int i = 0; i < idx; ++i)
		{
			if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
			{
				is_correct = 0;
				cprintf("alloc_block_xx #2.%d: WRONG! content of the block is not correct. Expected %d\n",i, i);
				break;
			}
		}
		if (is_correct)
		{
			eval += 40;
		}
	}

	/*Check page file*/
	{
		is_correct = 1;
		if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0)
		{
			cprintf("page(s) are allocated in PageFile while not expected to\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 5;
		}
	}

	uint32 expectedAllocatedSize = 0;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		expectedAllocatedSize += allocCntPerSize * allocSizes[i] ;
	}
	expectedAllocatedSize = ROUNDUP(expectedAllocatedSize, PAGE_SIZE);
	uint32 expectedAllocNumOfPages = expectedAllocatedSize / PAGE_SIZE; 				/*# pages*/
	uint32 expectedAllocNumOfTables = ROUNDUP(expectedAllocatedSize, PTSIZE) / PTSIZE; 	/*# tables*/

	/*Check memory allocation*/
	{
		is_correct = 1;
		if ((freeFrames - sys_calculate_free_frames()) < (expectedAllocNumOfPages + expectedAllocNumOfTables))
		{
			cprintf("number of allocated pages in MEMORY are less than the its expected lower bound\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	/*Check WS elements*/
	{
		is_correct = 1;
		uint32* expectedVAs = malloc(expectedAllocNumOfPages*sizeof(int));
		int i = 0;
		for (uint32 va = USER_HEAP_START; va < USER_HEAP_START + expectedAllocatedSize; va+=PAGE_SIZE)
		{
			expectedVAs[i++] = va;
		}
		chk = sys_check_WS_list(expectedVAs, expectedAllocNumOfPages, 0, 2);
		if (chk != 1)
		{
			cprintf("malloc: page is not added to WS\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 15;
		}
	}

	cprintf("test malloc (2) [DYNAMIC ALLOCATOR] is finished. Evaluation = %d%\n", eval);

	return;
}
