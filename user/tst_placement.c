/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 20 */
/* *********************************************************** */

#include <inc/lib.h>
uint32 expectedInitialVAs[14] = {
		0x200000, 0x201000, 0x202000, 0x203000, 0x204000, 0x205000, 0x206000,0x207000,	//Data
		0x800000, 0x801000, 0x802000, 0x803000,		//Code
		0xeebfd000, 0xedbfd000 /*will be created during the call of sys_check_WS_list*/} ;//Stack

void _main(void)
{

	//	cprintf("envID = %d\n",envID);

	char arr[PAGE_SIZE*1024*4];
	bool found ;
	//("STEP 0: checking Initial WS entries ...\n");
	{
		found = sys_check_WS_list(expectedInitialVAs, 14, 0, 1);
		if (found != 1) panic("INITIAL PAGE WS entry checking failed! Review size of the WS..!!");

		/*NO NEED FOR THIS IF REPL IS "LRU"*/
		if( myEnv->page_last_WS_element !=  NULL)
			panic("INITIAL PAGE last WS checking failed! Review size of the WS..!!");
		/*====================================*/
	}
	int eval = 0;
	bool is_correct = 1;
	int usedDiskPages = sys_pf_calculate_allocated_pages() ;
	int freePages = sys_calculate_free_frames();
	int i=0;
	for(;i<=PAGE_SIZE;i++)
	{
		arr[i] = 1;
	}

	i=PAGE_SIZE*1024;
	for(;i<=(PAGE_SIZE*1024 + PAGE_SIZE);i++)
	{
		arr[i] = 2;
	}

	i=PAGE_SIZE*1024*2;
	for(;i<=(PAGE_SIZE*1024*2 + PAGE_SIZE);i++)
	{
		arr[i] = 3;
	}

	is_correct = 1;
	cprintf("STEP A: checking PLACEMENT fault handling ... \n");
	{
		if( arr[0] !=  1)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}
		if( arr[PAGE_SIZE] !=  1)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}

		if( arr[PAGE_SIZE*1024] !=  2)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}
		if( arr[PAGE_SIZE*1025] !=  2)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}

		if( arr[PAGE_SIZE*1024*2] !=  3)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}
		if( arr[PAGE_SIZE*1024*2 + PAGE_SIZE] !=  3)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}


		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) { is_correct = 0; cprintf("new stack pages should NOT be written to Page File until evicted as victim\n");}

		int expected = 5 /*pages*/ + 2 /*tables*/;
		if( (freePages - sys_calculate_free_frames() ) != expected )
		{ is_correct = 0; cprintf("allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", expected, (freePages - sys_calculate_free_frames() ));}
	}
	cprintf("STEP A finished: PLACEMENT fault handling !\n\n\n");

	if (is_correct)
	{
		eval += 40;
	}
	is_correct = 1;

	cprintf("STEP B: checking WS entries ...\n");
	{
		//		uint32 expectedPages[19] = {
		//				0x200000,0x201000,0x202000,0x203000,0x204000,0x205000,0x206000,0x207000,
		//				0x800000,0x801000,0x802000,0x803000,
		//				0xeebfd000,0xedbfd000,0xedbfe000,0xedffd000,0xedffe000,0xee3fd000,0xee3fe000};
		uint32 expectedPages[19] ;
		{
			expectedPages[0] = 0x200000 ;
			expectedPages[1] = 0x201000 ;
			expectedPages[2] = 0x202000 ;
			expectedPages[3] = 0x203000 ;
			expectedPages[4] = 0x204000 ;
			expectedPages[5] = 0x205000 ;
			expectedPages[6] = 0x206000 ;
			expectedPages[7] = 0x207000 ;
			expectedPages[8] = 0x800000 ;
			expectedPages[9] = 0x801000 ;
			expectedPages[10] = 0x802000 ;
			expectedPages[11] = 0x803000 ;
			expectedPages[12] = 0xeebfd000 ;
			expectedPages[13] = 0xedbfd000 ;
			expectedPages[14] = 0xedbfe000 ;
			expectedPages[15] = 0xedffd000 ;
			expectedPages[16] = 0xedffe000 ;
			expectedPages[17] = 0xee3fd000 ;
			expectedPages[18] = 0xee3fe000 ;
		}
		found = sys_check_WS_list(expectedPages, 19, 0, 1);
		if (found != 1)
		{ is_correct = 0; cprintf("PAGE WS entry checking failed... trace it by printing page WS before & after fault\n");}
	}
	cprintf("STEP B finished: WS entries test \n\n\n");
	if (is_correct)
	{
		eval += 30;
	}
	is_correct = 1;
	cprintf("STEP C: checking working sets WHEN BECOMES FULL...\n");
	{
		/*NO NEED FOR THIS IF REPL IS "LRU"*/
		if(myEnv->page_last_WS_element != NULL)
		{ is_correct = 0; cprintf("wrong PAGE WS pointer location... trace it by printing page WS before & after fault\n");}

		i=PAGE_SIZE*1024*3;
		for(;i<=(PAGE_SIZE*1024*3);i++)
		{
			arr[i] = 4;
		}

		if( arr[PAGE_SIZE*1024*3] != 4)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}
		//		if( arr[PAGE_SIZE*1024*3 + PAGE_SIZE] !=  -1)  { is_correct = 0; cprintf("PLACEMENT of stack page failed\n");}

//		uint32 expectedPages[20] = {
//				0x200000,0x201000,0x202000,0x203000,0x204000,0x205000,0x206000,0x207000,
//				0x800000,0x801000,0x802000,0x803000,
//				0xeebfd000,0xedbfd000,0xedbfe000,0xedffd000,0xedffe000,0xee3fd000,0xee3fe000,0xee7fd000};
		uint32 expectedPages[19] ;
		{
			expectedPages[0] = 0x200000 ;
			expectedPages[1] = 0x201000 ;
			expectedPages[2] = 0x202000 ;
			expectedPages[3] = 0x203000 ;
			expectedPages[4] = 0x204000 ;
			expectedPages[5] = 0x205000 ;
			expectedPages[6] = 0x206000 ;
			expectedPages[7] = 0x207000 ;
			expectedPages[8] = 0x800000 ;
			expectedPages[9] = 0x801000 ;
			expectedPages[10] = 0x802000 ;
			expectedPages[11] = 0x803000 ;
			expectedPages[12] = 0xeebfd000 ;
			expectedPages[13] = 0xedbfd000 ;
			expectedPages[14] = 0xedbfe000 ;
			expectedPages[15] = 0xedffd000 ;
			expectedPages[16] = 0xedffe000 ;
			expectedPages[17] = 0xee3fd000 ;
			expectedPages[18] = 0xee3fe000 ;
			expectedPages[19] = 0xee7fd000 ;
		}
		found = sys_check_WS_list(expectedPages, 20, 0x200000, 1);
		if (found != 1)
		{ is_correct = 0; cprintf("PAGE WS entry checking failed... trace it by printing page WS before & after fault\n");}
		/*NO NEED FOR THIS IF REPL IS "LRU"*/
		//if(myEnv->page_last_WS_index != 0) { is_correct = 0; cprintf("wrong PAGE WS pointer location... trace it by printing page WS before & after fault\n");}

	}
	cprintf("STEP C finished: WS is FULL now\n\n\n");
	if (is_correct)
	{
		eval += 30;
	}
	is_correct = 1;
	//	/cprintf("Congratulations!! Test of PAGE PLACEMENT completed successfully!!\n\n\n");
	cprintf("[AUTO_GR@DING_PARTIAL]%d\n", eval);

	return;
}

