/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 10000 */
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
	uint32 actualSize;

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
				//Check returned va
				if(va == NULL || (va < curVA))
				{
					if (is_correct)
					{
						is_correct = 0;
						panic("malloc() #1.%d: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", idx, curVA + sizeOfMetaData() ,va);
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

	//====================================================================//
	/*Free set of blocks with different sizes (first block of each size)*/
	cprintf("1: Free set of blocks with different sizes (first block of each size)\n\n") ;
	{
		for (int i = 0; i < numOfAllocs; ++i)
		{
			free(startVAs[i*allocCntPerSize]);
		}
	}

#define numOfFFTests 3
	short* tstStartVAs[numOfFFTests+1] ;
	short* tstMidVAs[numOfFFTests+1] ;
	short* tstEndVAs[numOfFFTests+1] ;

	//====================================================================//
	/*FF ALLOC Scenario 2: Try to allocate blocks with sizes smaller than existing free blocks*/
	cprintf("2: Try to allocate set of blocks with different sizes smaller than existing free blocks\n\n") ;

	uint32 testSizes[numOfFFTests] =
	{
			kilo/4,								//expected to be allocated in 4th free block
			8*sizeof(char) + sizeOfMetaData(), 	//expected to be allocated in 1st free block
			kilo/8,								//expected to be allocated in remaining of 4th free block
	} ;

	uint32 startOf1stFreeBlock = (uint32)startVAs[0*allocCntPerSize];
	uint32 startOf4thFreeBlock = (uint32)startVAs[3*allocCntPerSize];

	{
		is_correct = 1;

		uint32 expectedVAs[numOfFFTests] =
		{
				startOf4thFreeBlock,
				startOf1stFreeBlock,
				startOf4thFreeBlock + testSizes[0]
		};
		for (int i = 0; i < numOfFFTests; ++i)
		{
			actualSize = testSizes[i] - sizeOfMetaData();
			va = tstStartVAs[i] = malloc(actualSize);
			tstMidVAs[i] = va + actualSize/2 ;
			tstEndVAs[i] = va + actualSize - sizeof(short);
			//Check returned va
			if(tstStartVAs[i] == NULL || (tstStartVAs[i] != (short*)expectedVAs[i]))
			{
				is_correct = 0;
				cprintf("malloc() #2.%d: WRONG FF ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", i, expectedVAs[i] ,tstStartVAs[i]);
				//break;
			}
			*(tstStartVAs[i]) = 353 + i;
			*(tstMidVAs[i]) = 353 + i;
			*(tstEndVAs[i]) = 353 + i;
		}
		//Check stored sizes
		if(get_block_size(tstStartVAs[1]) != allocSizes[0])
		{
			is_correct = 0;
			cprintf("malloc() #3: WRONG FF ALLOC - make sure if the remaining free space doesn’t fit a dynamic allocator block, then this area should be added to the allocated area and counted as internal fragmentation\n");
			//break;
		}
		if (is_correct)
		{
			eval += 30;
		}
	}

	//====================================================================//
	/*FF ALLOC Scenario 3: Try to allocate a block with a size equal to the size of the first existing free block*/
	cprintf("3: Try to allocate a block with equal to the first existing free block\n\n") ;
	{
		is_correct = 1;

		actualSize = kilo/8 - sizeOfMetaData(); 	//expected to be allocated in remaining of 4th free block
		va = tstStartVAs[numOfFFTests] = malloc(actualSize);
		tstMidVAs[numOfFFTests] = va + actualSize/2 ;
		tstEndVAs[numOfFFTests] = va + actualSize - sizeof(short);
		//Check returned va
		void* expected = (void*)(startOf4thFreeBlock + testSizes[0] + testSizes[2]) ;
		if(va == NULL || (va != expected))
		{
			is_correct = 0;
			cprintf("malloc() #4: WRONG FF ALLOC - alloc_block_FF return wrong address.expected %x, actual %x\n", expected, va);
		}
		*(tstStartVAs[numOfFFTests]) = 353 + numOfFFTests;
		*(tstMidVAs[numOfFFTests]) = 353 + numOfFFTests;
		*(tstEndVAs[numOfFFTests]) = 353 + numOfFFTests;

		if (is_correct)
		{
			eval += 30;
		}
	}
	//====================================================================//
	/*FF ALLOC Scenario 4: Check stored data inside each allocated block*/
	cprintf("4: Check stored data inside each allocated block\n\n") ;
	{
		is_correct = 1;

		for (int i = 0; i <= numOfFFTests; ++i)
		{
			//cprintf("startVA = %x, mid = %x, last = %x\n", tstStartVAs[i], tstMidVAs[i], tstEndVAs[i]);
			if (*(tstStartVAs[i]) != (353+i) || *(tstMidVAs[i]) != (353+i) || *(tstEndVAs[i]) != (353+i) )
			{
				is_correct = 0;
				cprintf("malloc #5.%d: WRONG! content of the block is not correct. Expected=%d, val1=%d, val2=%d, val3=%d\n",i, (353+i), *(tstStartVAs[i]), *(tstMidVAs[i]), *(tstEndVAs[i]));
				break;
			}
		}

		if (is_correct)
		{
			eval += 20;
		}
	}

	//====================================================================//
	/*FF ALLOC Scenario 5: Test a Non-Granted Request */
	cprintf("5: Test a Non-Granted Request\n\n") ;
	{
		is_correct = 1;
		actualSize = 2*kilo - sizeOfMetaData();

		//Fill the 7th free block
		va = malloc(actualSize);

		//Fill the remaining area
		uint32 numOfRem2KBAllocs = ((USER_HEAP_START + DYN_ALLOC_MAX_SIZE - (uint32)sbrk(0)) / PAGE_SIZE) * 2;
		for (int i = 0; i < numOfRem2KBAllocs; ++i)
		{
			va = malloc(actualSize);
			if(va == NULL)
			{
				is_correct = 0;
				cprintf("malloc() #6.%d: WRONG FF ALLOC - alloc_block_FF return NULL address while it's expected to return correct one.\n");
				break;
			}
		}

		//Test two more allocs
		va = malloc(actualSize);
		va = malloc(actualSize);

		if(va != NULL)
		{
			is_correct = 0;
			cprintf("malloc() #7: WRONG FF ALLOC - alloc_block_FF return an address while it's expected to return NULL since it reaches the hard limit.\n");
		}
		if (is_correct)
		{
			eval += 20;
		}
	}
	cprintf("test FIRST FIT (2) [DYNAMIC ALLOCATOR] is finished. Evaluation = %d%\n", eval);

	return;
}
