/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 11 */
/* MAKE SURE Second_List_Size = 5 */
/* TEST THE LRU ALGO ON THE 2ND CHANCE LIST*/
/* *********************************************************** */

#include <inc/lib.h>

char arr[PAGE_SIZE*13];
char* ptr = (char* )0x0801000 ;
char* ptr2 = (char* )0x0804000 ;
uint32 actual_active_list_init[6] = {0x803000, 0x801000, 0x800000, 0xeebfd000, 0x204000, 0x203000};
uint32 actual_second_list_init[5] = {0x202000, 0x201000, 0x200000, 0x802000, 0x205000};

void _main(void)
{
	//	cprintf("envID = %d\n",envID);
	int x = 0;

	//("STEP 0: checking Initial WS entries ...\n");
	{
		int check = sys_check_LRU_lists(actual_active_list_init, actual_second_list_init, 6, 5);
		if(check == 0)
			panic("INITIAL PAGE LRU LISTs entry checking failed! Review size of the LRU lists!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}

	//Reading (Not Modified)
	char garbage1 = arr[PAGE_SIZE*11-1] ;
	char garbage2 = arr[PAGE_SIZE*12-1] ;
	char garbage4,garbage5;

	//Writing (Modified)
	int i ;
	for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
	{
		arr[i] = -1 ;
		/*2016: this BUGGY line is REMOVED el7! it overwrites the KERNEL CODE :( !!!*/
		//*ptr = *ptr2 ;
		/*==========================================================================*/
		//always use pages at 0x801000 and 0x804000
		garbage4 = *ptr + garbage5;
		garbage5 = *ptr2 + garbage4;
		ptr++ ; ptr2++ ;
	}

	//===================

	//("STEP 1: checking LRU LISTS after new page FAULTS...\n");
	//uint32 actual_active_list[6] = {0x803000, 0x801000, 0x800000, 0xeebfd000, 0x804000, 0x80c000};
	uint32 actual_active_list[6] ;
	{
		actual_active_list[0] = 0x803000;
		actual_active_list[1] = 0x801000;
		actual_active_list[2] = 0x800000;
		actual_active_list[3] = 0xeebfd000;
		actual_active_list[4] = 0x804000;
		actual_active_list[5] = 0x80c000;
	}
	//uint32 actual_second_list[5] = {0x80b000, 0x80a000, 0x809000, 0x808000, 0x807000};
	uint32 actual_second_list[5] ;
	{
		actual_second_list[0] = 0x80b000 ;
		actual_second_list[1] = 0x80a000 ;
		actual_second_list[2] = 0x809000 ;
		actual_second_list[3] = 0x808000 ;
		actual_second_list[4] = 0x807000 ;
	}
	int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 6, 5);
	if(check == 0)
		panic("PAGE LRU Lists entry checking failed when new PAGE FAULTs occurred..!!");


	//("STEP 2: Checking PAGE LRU LIST algorithm after faults due to ACCESS in the second chance list... \n");
	{
		uint32* secondlistVA = (uint32*)0x809000;
		x = x + *secondlistVA;
		secondlistVA = (uint32*)0x807000;
		x = x + *secondlistVA;
		secondlistVA = (uint32*)0x804000;
		x = x + *secondlistVA;

		actual_active_list[0] = 0x801000;
		actual_active_list[1] = 0x800000;
		actual_active_list[2] = 0xeebfd000;
		actual_active_list[3] = 0x804000;
		actual_active_list[4] = 0x807000;
		actual_active_list[5] = 0x809000;

		actual_second_list[0] = 0x803000;
		actual_second_list[1] = 0x80c000;
		actual_second_list[2] = 0x80b000;
		actual_second_list[3] = 0x80a000;
		actual_second_list[4] = 0x808000;
		int check = sys_check_LRU_lists(actual_active_list, actual_second_list, 6, 5);
		if(check == 0)
			panic("PAGE LRU Lists entry checking failed when a new PAGE ACCESS from the SECOND LIST is occurred..!!");
	}

	//("STEP 3: NEW FAULTS to test applying LRU algorithm on the second list by removing the LRU page... \n");
	{
		//Reading (Not Modified)
		char garbage3 = arr[PAGE_SIZE*13-1] ;
		actual_active_list[0] = 0x810000;
		actual_active_list[1] = 0x801000;
		actual_active_list[2] = 0x800000;
		actual_active_list[3] = 0xeebfd000;
		actual_active_list[4] = 0x804000;
		actual_active_list[5] = 0x807000;

		actual_second_list[0] = 0x809000;
		actual_second_list[1] = 0x803000;
		actual_second_list[2] = 0x80c000;
		actual_second_list[3] = 0x80b000;
		actual_second_list[4] = 0x80a000;
		check = sys_check_LRU_lists(actual_active_list, actual_second_list, 6, 5);
		if(check == 0)
			panic("PAGE LRU Lists entry checking failed when a new PAGE FAULT occurred..!!");
	}
	cprintf("Congratulations!! test PAGE replacement [LRU Alg. on the 2nd chance list] is completed successfully.\n");
	return;
}
