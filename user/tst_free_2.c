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
	sys_set_uheap_strategy(UHP_PLACE_FIRSTFIT);

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
	uint32 actualSize, block_size, expectedSize, blockIndex;
	int8 block_status;
	//====================================================================//
	/*INITIAL ALLOC Scenario 1: Try to allocate set of blocks with different sizes*/
	cprintf("PREREQUISITE#1: Try to allocate set of blocks with different sizes [all should fit]\n\n") ;
	{
		is_correct = 1;
		void* curVA = (void*) USER_HEAP_START ;
		uint32 actualSize;
		for (int i = 0; i < numOfAllocs; ++i)
		{
			for (int j = 0; j < allocCntPerSize; ++j)
			{
				actualSize = allocSizes[i] - sizeOfMetaData();
				va = startVAs[idx++] = malloc(actualSize);
				//				if (j == 0)
				//					cprintf("first va of size %x = %x\n",allocSizes[i], va);

				//Check returned va
				if(va == NULL || (va < curVA))
				{
					if (is_correct)
					{
						is_correct = 0;
						panic("malloc() #0.%d: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", idx, curVA + sizeOfMetaData() ,va);
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
			}
			//if (is_correct == 0)
			//break;
		}
	}

	freeFrames = sys_calculate_free_frames() ;

	//====================================================================//
	/*Free set of blocks with different sizes (first block of each size)*/
	cprintf("1: Free set of blocks with different sizes (first block of each size)\n\n") ;
	{
		is_correct = 1;
		for (int i = 0; i < numOfAllocs; ++i)
		{
			free(startVAs[i*allocCntPerSize]);
		}

		//Free block before last
		free(startVAs[numOfAllocs*allocCntPerSize - 2]);
		block_size = get_block_size(startVAs[numOfAllocs*allocCntPerSize - 2]) ;
		if (block_size != allocSizes[numOfAllocs-1])
		{
			is_correct = 0;
			cprintf("test_free_2 #1.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[numOfAllocs-1],block_size);
		}
		block_status = is_free_block(startVAs[numOfAllocs*allocCntPerSize-2]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #1.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}

		//Reallocate first block
		actualSize = allocSizes[0] - sizeOfMetaData();
		va = malloc(actualSize);
		//Check returned va
		if(va == NULL || (va != (void*)(USER_HEAP_START + sizeOfMetaData())))
		{
			is_correct = 0;
			cprintf("test_free_2 #1.3: WRONG ALLOC - alloc_block_FF return wrong address.\n");
		}

		//Free 2nd block
		free(startVAs[1]);
		block_size = get_block_size(startVAs[1]) ;
		if (block_size != allocSizes[0])
		{
			is_correct = 0;
			cprintf("test_free_2 #1.4: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[0],block_size);
		}
		block_status = is_free_block(startVAs[1]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #1.5: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}

		if (is_correct)
		{
			eval += 10;
		}
	}

	//====================================================================//
	/*free Scenario 2: Merge with previous ONLY (AT the tail)*/
	cprintf("2: Free some allocated blocks [Merge with previous ONLY]\n\n") ;
	{
		cprintf("	2.1: at the tail\n\n") ;
		is_correct = 1;
		//Free last block (coalesce with previous)
		blockIndex = numOfAllocs*allocCntPerSize-1;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex-1]) ;
		expectedSize = 2*allocSizes[numOfAllocs-1];
		if (block_size != expectedSize)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.1.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n", expectedSize,block_size);
		}
		block_status = is_free_block(startVAs[blockIndex-1]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.1.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}

		if (get_block_size(startVAs[blockIndex]) != 0 || is_free_block(startVAs[blockIndex]) != 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.1.3: WRONG FREE! make sure to ZEROing the size & is_free values of the vanishing block.\n");
		}

		//====================================================================//
		/*free Scenario 3: Merge with previous ONLY (between 2 blocks)*/
		cprintf("	2.2: between 2 blocks\n\n") ;
		blockIndex = 2*allocCntPerSize+1 ;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex-1]) ;
		expectedSize = allocSizes[2]+allocSizes[2];
		if (block_size != expectedSize)
		{
			is_correct = 0;
			cprintf	("test_free_2 #2.2.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",expectedSize,block_size);
		}
		block_status = is_free_block(startVAs[blockIndex-1]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.2.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}

		if (get_block_size(startVAs[blockIndex]) != 0 || is_free_block(startVAs[blockIndex]) != 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.2.3: WRONG FREE! make sure to ZEROing the size & is_free values of the vanishing block.\n");
		}

		if (is_correct)
		{
			eval += 10;
		}
	}


	//====================================================================//
	/*free Scenario 4: Merge with next ONLY (AT the head)*/
	cprintf("3: Free some allocated blocks [Merge with next ONLY]\n\n") ;
	{
		cprintf("	3.1: at the head\n\n") ;
		is_correct = 1;
		blockIndex = 0 ;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex]) ;
		expectedSize = allocSizes[0]+allocSizes[0];
		if (block_size != expectedSize)
		{
			is_correct = 0;
			cprintf	("test_free_2 #3.1.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",expectedSize,block_size);
		}
		block_status = is_free_block(startVAs[blockIndex]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #3.1.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}
		if (get_block_size(startVAs[blockIndex+1]) != 0 || is_free_block(startVAs[blockIndex+1]) != 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #3.1.3: WRONG FREE! make sure to ZEROing the size & is_free values of the vanishing block.\n");
		}

		//====================================================================//
		/*free Scenario 5: Merge with next ONLY (between 2 blocks)*/
		cprintf("	3.2: between 2 blocks\n\n") ;
		blockIndex = 1*allocCntPerSize - 1 ;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex]) ;
		expectedSize = allocSizes[0]+allocSizes[1];
		if (block_size != expectedSize)
		{
			is_correct = 0;
			cprintf	("test_free_2 #3.2.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",expectedSize,block_size);
		}
		block_status = is_free_block(startVAs[blockIndex]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #3.2.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}
		if (get_block_size(startVAs[blockIndex+1]) != 0 || is_free_block(startVAs[blockIndex+1]) != 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #3.2.3: WRONG FREE! make sure to ZEROing the size & is_free values of the vanishing block.\n");
		}
		if (is_correct)
		{
			eval += 10;
		}
	}
	//====================================================================//
	/*free Scenario 6: Merge with prev & next */
	cprintf("4: Free some allocated blocks [Merge with previous & next]\n\n") ;
	{
		is_correct = 1;
		blockIndex = 4*allocCntPerSize - 2 ;
		free(startVAs[blockIndex]);

		blockIndex = 4*allocCntPerSize - 1 ;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex-1]) ;
		expectedSize = allocSizes[3]+allocSizes[3]+allocSizes[4];
		if (block_size != expectedSize)
		{
			is_correct = 0;
			cprintf	("test_free_2 #4.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",expectedSize,block_size);
		}
		block_status = is_free_block(startVAs[blockIndex-1]) ;
		if (block_status != 1)
		{
			is_correct = 0;
			cprintf("test_free_2 #4.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
		}
		if (get_block_size(startVAs[blockIndex]) != 0 || is_free_block(startVAs[blockIndex]) != 0 ||
				get_block_size(startVAs[blockIndex+1]) != 0 || is_free_block(startVAs[blockIndex+1]) != 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #4.3: WRONG FREE! make sure to ZEROing the size & is_free values of the vanishing block.\n");
		}
		if (is_correct)
		{
			eval += 20;
		}
	}


	//====================================================================//
	/*Allocate After Free Scenarios */
	cprintf("5: Allocate After Free [should be placed in coalesced blocks]\n\n") ;
	{
		cprintf("	5.1: in block coalesces with NEXT\n\n") ;
		is_correct = 1;
		actualSize = 4*sizeof(int);
		va = malloc(actualSize);
		//Check returned va
		void* expected = (void*)(USER_HEAP_START + sizeOfMetaData());
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.1.1: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}
		actualSize = 2*sizeof(int) ;
		va = malloc(actualSize);
		//Check returned va
		expected = (void*)(USER_HEAP_START + sizeOfMetaData() + 4*sizeof(int) + sizeOfMetaData());
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.1.2: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}
		actualSize = 5*sizeof(int);
		va = malloc(actualSize);
		//Check returned va
		expected = startVAs[1*allocCntPerSize - 1];
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.1.3: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}
		if (is_correct)
		{
			eval += 10;
		}

		cprintf("	5.2: in block coalesces with PREV & NEXT\n\n") ;
		is_correct = 1;
		actualSize = 3*kilo/2;
		va = malloc(actualSize);
		//Check returned va
		expected = startVAs[4*allocCntPerSize - 2];
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.2: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}
		if (is_correct)
		{
			eval += 10;
		}

		cprintf("	5.3: in block coalesces with PREV\n\n") ;
		is_correct = 1;
		actualSize = 30*sizeof(char) ;
		va = malloc(actualSize);
		//Check returned va
		expected = startVAs[2*allocCntPerSize];
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.3.1: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}
		actualSize = 3*kilo/2 ;

		//dummy allocation to consume the 1st 1.5 KB free block
		{
			va = malloc(actualSize);
		}
		//dummy allocation to consume the 1st 2 KB free block
		{
			va = malloc(actualSize);
		}
		va = malloc(actualSize);
		//Check returned va
		expected = startVAs[numOfAllocs*allocCntPerSize-2];
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.3.2: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}

		actualSize = 3*kilo/2 ;
		va = malloc(actualSize);

		//Check returned va
		expected = (void*)startVAs[numOfAllocs*allocCntPerSize-2] + 3*kilo/2 + sizeOfMetaData();
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("test_free_2 #5.3.3: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
		}
		if (is_correct)
		{
			eval += 10;
		}
	}


	//====================================================================//
	/*Check memory allocation*/
	cprintf("6: Check memory allocation [should not be changed due to free]\n\n") ;
	{
		is_correct = 1;
		if ((freeFrames - sys_calculate_free_frames()) != 0)
		{
			cprintf("test_free_2 #6: number of allocated pages in MEMORY is changed due to free() while it's not supposed to!\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
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

	//====================================================================//
	/*Check WS elements*/
	cprintf("7: Check WS Elements [should not be changed due to free]\n\n") ;
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
			cprintf("test_free_2 #7: page is either not added to WS or removed from it\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	cprintf("test free() with FIRST FIT completed [DYNAMIC ALLOCATOR]. Evaluation = %d%\n", eval);

	return;
}
