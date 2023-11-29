/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 20 */
/* MAKE SURE Second_List_Size = 7*/
/*THIRD SCENARIO ACTIVE LIST FULL, SECOND LIST NOT FULL AND FAULT VA IN SECOND LIST*/
/* *********************************************************** */

#include <inc/lib.h>
extern uint32 initFreeFrames;

void _main(void)
{

	int8 arr[PAGE_SIZE*1024*4];
	int x = 0;
	uint32 actual_active_list[13] = {0xedbfd000,0xeebfd000,0x803000,0x802000,0x801000,0x800000,0x205000,0x204000,0x203000,0x202000,0x201000,0x200000};
	uint32 actual_second_list[7] = {};
	("STEP 0: checking Initial LRU lists entries ...\n");
	{
		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 12, 0);
		if(check == 0)
			panic("INITIAL PAGE LRU LISTs entry checking failed! Review size of the LRU lists!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}

	int usedDiskPages = sys_pf_calculate_allocated_pages() ;
	int freePages = sys_calculate_free_frames();

	int i=0;
	for(;i<=PAGE_SIZE;i++)
	{
		arr[i] = -1;
	}

	i=PAGE_SIZE*1024;
	for(;i<=(PAGE_SIZE*1024 + PAGE_SIZE);i++)
	{
		arr[i] = -1;
	}

	i=PAGE_SIZE*1024*2;
	for(;i<=(PAGE_SIZE*1024*2 + PAGE_SIZE);i++)
	{
		arr[i] = -1;
	}

	uint32* secondlistVA= (uint32*)0x200000;
	x = x + *secondlistVA;
	secondlistVA = (uint32*) 0x202000;
	x = x + *secondlistVA;

	actual_second_list[0]=0X205000;
	actual_second_list[1]=0X204000;
	actual_second_list[2]=0x203000;
	actual_second_list[3]=0x201000;
	for (int i=12;i>6;i--)
		actual_active_list[i]=actual_active_list[i-7];

	actual_active_list[0]=0x202000;
	actual_active_list[1]=0x200000;
	actual_active_list[2]=0xee3fe000;
	actual_active_list[3]=0xee3fd000;
	actual_active_list[4]=0xedffe000;
	actual_active_list[5]=0xedffd000;
	actual_active_list[6]=0xedbfe000;

	uint32 expected, actual ;
	cprintf("STEP A: checking PLACEMENT fault handling ... \n");
	{
		if( arr[0] !=  -1)  panic("PLACEMENT of stack page failed");
		if( arr[PAGE_SIZE] !=  -1)  panic("PLACEMENT of stack page failed");

		if( arr[PAGE_SIZE*1024] !=  -1)  panic("PLACEMENT of stack page failed");
		if( arr[PAGE_SIZE*1025] !=  -1)  panic("PLACEMENT of stack page failed");

		if( arr[PAGE_SIZE*1024*2] !=  -1)  panic("PLACEMENT of stack page failed");
		if( arr[PAGE_SIZE*1024*2 + PAGE_SIZE] !=  -1)  panic("PLACEMENT of stack page failed");

		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("new stack pages should NOT written to Page File until it's replaced");

		expected = 6 /*pages*/ + 3 /*tables*/ - 2 /*table + page due to a fault in the 1st call of sys_calculate_free_frames*/;
		actual = (freePages - sys_calculate_free_frames()) ;
		//actual = (initFreeFrames - sys_calculate_free_frames()) ;
		if(actual != expected) panic("allocated memory size incorrect. Expected = %d, Actual = %d", expected, actual);
	}
	cprintf("STEP A passed: PLACEMENT fault handling works!\n\n\n");

	cprintf("STEP B: checking LRU lists entries After Required PAGES IN SECOND LIST...\n");
	{
		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 13, 4);
			if(check == 0)
				panic("LRU lists entries are not correct, check your logic again!!");
	}
	cprintf("STEP B passed: checking LRU lists entries After Required PAGES IN SECOND LIST test are correct\n\n\n");
	cprintf("Congratulations!! Test of PAGE PLACEMENT THIRD SCENARIO completed successfully!!\n\n\n");
	return;
}

