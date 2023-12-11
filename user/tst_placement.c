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

	cprintf("STEP A: checking PLACEMENT fault handling ... \n");
	{
		if( arr[0] !=  1)  panic("PLACEMENT of stack page failed");
		if( arr[PAGE_SIZE] !=  1)  panic("PLACEMENT of stack page failed");

		if( arr[PAGE_SIZE*1024] !=  2)  panic("PLACEMENT of stack page failed");
		if( arr[PAGE_SIZE*1025] !=  2)  panic("PLACEMENT of stack page failed");

		if( arr[PAGE_SIZE*1024*2] !=  3)  panic("PLACEMENT of stack page failed");
		if( arr[PAGE_SIZE*1024*2 + PAGE_SIZE] !=  3)  panic("PLACEMENT of stack page failed");


		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("new stack pages should NOT be written to Page File until evicted as victim");

		int expected = 5 /*pages*/ + 2 /*tables*/;
		if( (freePages - sys_calculate_free_frames() ) != expected )
			panic("allocated memory size incorrect. Expected Difference = %d, Actual = %d\n", expected, (freePages - sys_calculate_free_frames() ));
	}
	cprintf("STEP A passed: PLACEMENT fault handling works!\n\n\n");



	cprintf("STEP B: checking WS entries ...\n");
	{
		uint32 expectedPages[19] = {
				0x200000,0x201000,0x202000,0x203000,0x204000,0x205000,0x206000,0x207000,
				0x800000,0x801000,0x802000,0x803000,
				0xeebfd000,0xedbfd000,0xedbfe000,0xedffd000,0xedffe000,0xee3fd000,0xee3fe000};
		found = sys_check_WS_list(expectedPages, 19, 0, 1);
		if (found != 1)
			panic("PAGE WS entry checking failed... trace it by printing page WS before & after fault");
	}
	cprintf("STEP B passed: WS entries test are correct\n\n\n");

	cprintf("STEP C: checking working sets WHEN BECOMES FULL...\n");
	{
		/*NO NEED FOR THIS IF REPL IS "LRU"*/
		if(myEnv->page_last_WS_element != NULL)
			panic("wrong PAGE WS pointer location... trace it by printing page WS before & after fault");

		i=PAGE_SIZE*1024*3;
		for(;i<=(PAGE_SIZE*1024*3);i++)
		{
			arr[i] = 4;
		}

		if( arr[PAGE_SIZE*1024*3] != 4)  panic("PLACEMENT of stack page failed");
		//		if( arr[PAGE_SIZE*1024*3 + PAGE_SIZE] !=  -1)  panic("PLACEMENT of stack page failed");

		uint32 expectedPages[20] = {
				0x200000,0x201000,0x202000,0x203000,0x204000,0x205000,0x206000,0x207000,
				0x800000,0x801000,0x802000,0x803000,
				0xeebfd000,0xedbfd000,0xedbfe000,0xedffd000,0xedffe000,0xee3fd000,0xee3fe000,0xee7fd000};

		found = sys_check_WS_list(expectedPages, 20, 0x200000, 1);
		if (found != 1)
			panic("PAGE WS entry checking failed... trace it by printing page WS before & after fault");
		/*NO NEED FOR THIS IF REPL IS "LRU"*/
		//if(myEnv->page_last_WS_index != 0) panic("wrong PAGE WS pointer location... trace it by printing page WS before & after fault");

	}
	cprintf("STEP C passed: WS is FULL now\n\n\n");

	cprintf("Congratulations!! Test of PAGE PLACEMENT completed successfully!!\n\n\n");
	return;
}

