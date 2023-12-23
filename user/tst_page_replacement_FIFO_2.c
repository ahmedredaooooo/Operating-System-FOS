/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 11 */
/* *********************************************************** */

#include <inc/lib.h>

char arr[PAGE_SIZE*12];
char* ptr = (char* )0x0801000 ;
char* ptr2 = (char* )0x0804000 ;
uint32 expectedInitialVAs[11] = {
		0x200000, 0x201000, 0x202000, 0x203000, 0x204000, 0x205000, 	//Unused
		0x800000, 0x801000, 0x802000, 0x803000,							//Code & Data
		0xeebfd000,  													//Stack
} ;


#define kilo 1024
void _main(void)
{
	uint32 expectedMidVAs[11] ;
	//	= {
	//			0xeebfd000, 																					//Stack
	//			0x80a000, 0x80b000, 0x804000, 0x80c000,0x807000,0x808000,0x800000,0x801000,0x809000,0x803000,	//Code & Data
	//	} ;

	{
		expectedMidVAs[0] = 0xeebfd000;
		expectedMidVAs[1] = 0x80a000;
		expectedMidVAs[2] = 0x80b000;
		expectedMidVAs[3] = 0x804000;
		expectedMidVAs[4] = 0x80c000;
		expectedMidVAs[5] = 0x807000;
		expectedMidVAs[6] = 0x808000;
		expectedMidVAs[7] = 0x800000;
		expectedMidVAs[8] = 0x801000;
		expectedMidVAs[9] = 0x809000;
		expectedMidVAs[10] = 0x803000;
	}
//	uint32 expectedFinalVAs[11] = {
//			0x80b000,0x804000,0x80c000,0x800000,0x801000, //Code & Data
//			0xeebfd000, 					 //Stack
//			0x803000,0x805000,0x806000,0x807000,0x808000, //Data
//	} ;

	uint32 expectedFinalVAs[11] ;
	{
		expectedFinalVAs[0] =  0x80b000;
		expectedFinalVAs[1] =  0x804000;
		expectedFinalVAs[2] =  0x80c000;
		expectedFinalVAs[3] =  0x800000;
		expectedFinalVAs[4] =  0x801000;
		expectedFinalVAs[5] =  0xeebfd000;
		expectedFinalVAs[6] =  0x803000;
		expectedFinalVAs[7] =  0x805000;
		expectedFinalVAs[8] =  0x806000;
		expectedFinalVAs[9] =  0x807000;
		expectedFinalVAs[10] =  0x808000;
	}

	char* tempArr = (char*)0x90000000;
	uint32 tempArrSize = 5*PAGE_SIZE;
	//("STEP 0: checking Initial WS entries ...\n");
	bool found ;

#if USE_KHEAP
	{
		found = sys_check_WS_list(expectedInitialVAs, 11, 0x200000, 1);
		if (found != 1) panic("INITIAL PAGE WS entry checking failed! Review size of the WS!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif

	int freePages = sys_calculate_free_frames();
	int usedDiskPages = sys_pf_calculate_allocated_pages();

	//Reading (Not Modified)
	char garbage1 = arr[PAGE_SIZE*11-1];
	char garbage2 = arr[PAGE_SIZE*12-1];
	char garbage4, garbage5;

	//Writing (Modified)
	int i;
	for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
	{
		arr[i] = 'A' ;
		/*2016: this BUGGY line is REMOVED el7! it overwrites the KERNEL CODE :( !!!*/
		//*ptr = *ptr2 ;
		//ptr++ ; ptr2++ ;
		/*==========================================================================*/
		//always use pages at 0x801000 and 0x804000
		garbage4 = *ptr ;
		garbage5 = *ptr2 ;
	}

	//Check FIFO 1
	{
		found = sys_check_WS_list(expectedMidVAs, 11, 0x807000, 1);
		if (found != 1) panic("Page FIFO algo failed.. trace it by printing WS before and after page fault");
	}

	//char* tempArr = malloc(4*PAGE_SIZE);
	sys_allocate_user_mem((uint32)tempArr, tempArrSize);
	//cprintf("1\n");

	int c;
	for(c = 0;c < tempArrSize - 1;c++)
	{
		tempArr[c] = 'a';
	}

	//cprintf("2\n");

	sys_free_user_mem((uint32)tempArr, tempArrSize);

	//cprintf("3\n");

	//Check after free either push records up or leave them empty
	for (i = PAGE_SIZE*0 ; i < PAGE_SIZE*6 ; i+=PAGE_SIZE/2)
	{
		arr[i] = 'A' ;
		//always use pages at 0x801000 and 0x804000
		garbage4 = *ptr ;
		garbage5 = *ptr2 ;
	}
	//cprintf("4\n");

	//===================

	//cprintf("Checking PAGE FIFO algorithm after Free and replacement... \n");
	{
		found = sys_check_WS_list(expectedFinalVAs, 11, 0x80b000, 1);
		if (found != 1) panic("Page FIFO algo failed [AFTER Freeing an Allocated Space].. MAKE SURE to update the last_WS_element & the correct FIFO order after freeing space");
	}

	{
		if (garbage4 != *ptr) panic("test failed!");
		if (garbage5 != *ptr2) panic("test failed!");
	}

	cprintf("Congratulations!! test PAGE replacement [FIFO 2] is completed successfully.\n");
	return;
}
