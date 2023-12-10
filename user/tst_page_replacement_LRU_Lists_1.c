/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 10 */
/* MAKE SURE Second_List_Size = 5 */
/* TEST THE PAGE REPLACEMENT*/
/* *********************************************************** */

#include <inc/lib.h>

char arr[PAGE_SIZE*12];
char* ptr = (char* )0x0801000 ;
char* ptr2 = (char* )0x0804000 ;

void _main(void)
{

	//	cprintf("envID = %d\n",envID);

	//("STEP 0: checking Initial WS entries ...\n");
	{
		//uint32 actual_active_list[5] = {0x803000, 0x801000, 0x800000, 0xeebfd000, 0x203000};
		uint32 actual_active_list[5] ;
		{
			actual_active_list[0] = 0x803000;
			actual_active_list[1] = 0x801000;
			actual_active_list[2] = 0x800000;
			actual_active_list[3] = 0xeebfd000;
			actual_active_list[4] = 0x203000;
		}
		//uint32 actual_second_list[5] = {0x202000, 0x201000, 0x200000, 0x802000, 0x205000};
		uint32 actual_second_list[5] ;
		{
			actual_second_list[0] = 0x202000 ;
			actual_second_list[1] = 0x201000 ;
			actual_second_list[2] = 0x200000 ;
			actual_second_list[3] = 0x802000 ;
			actual_second_list[4] = 0x205000 ;
		}
		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 5, 5);
		if(check == 0)
			panic("INITIAL PAGE LRU LISTs entry checking failed! Review size of the LRU lists!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}

	int freePages = sys_calculate_free_frames();
	int usedDiskPages = sys_pf_calculate_allocated_pages();

	//Reading (Not Modified)
	char garbage1 = arr[PAGE_SIZE*11-1] ;
	char garbage2 = arr[PAGE_SIZE*12-1] ;
	char garbage4,garbage5;

	//Writing (Modified)
	int i ;
	for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
	{
		arr[i] = 'A' ;
		/*2016: this BUGGY line is REMOVED el7! it overwrites the KERNEL CODE :( !!!*/
		//*ptr = *ptr2 ;
		/*==========================================================================*/
		//always use pages at 0x801000 and 0x804000
		garbage4 = *ptr + garbage5;
		garbage5 = *ptr2 + garbage4;
		ptr++ ; ptr2++ ;
	}

	//===================

	cprintf("Checking Allocation in Mem & Page File... \n");
	{
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("Unexpected extra/less pages have been added to page file.. NOT Expected to add new pages to the page file");

		uint32 freePagesAfter = (sys_calculate_free_frames() + sys_calculate_modified_frames());
		if( (freePages - freePagesAfter) != 0 )
			panic("Extra memory are wrongly allocated... It's REplacement: expected that no extra frames are allocated");
	}

	cprintf("\nChecking CONTENT in Mem ... \n");
	{
		for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
			if( arr[i] != 'A') panic("Modified page(s) not restored correctly");
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("Unexpected extra/less pages have been added to page file.. NOT Expected to add new pages to the page file");

		uint32 freePagesAfter = (sys_calculate_free_frames() + sys_calculate_modified_frames());
		if( (freePages - freePagesAfter) != 0 )
			panic("Extra memory are wrongly allocated... It's REplacement: expected that no extra frames are allocated");

	}

	cprintf("Congratulations!! test PAGE replacement [ALLOCATION] using APRROXIMATED LRU is completed successfully.\n");
	return;
}
