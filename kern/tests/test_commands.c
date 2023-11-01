/*
 * test_commands.c
 *
 *  Created on: Oct 16, 2022
 *      Author: ghada
 */

#include <kern/tests/test_commands.h>
#include <kern/cmd/command_prompt.h>
#include <kern/mem/memory_manager.h>
extern uint32 sys_calculate_free_frames();
extern struct Env* env_create(char* user_program_name, unsigned int page_WS_size, unsigned int LRU_second_list_size, unsigned int percent_WS_pages_to_remove);

//define the white-space symbols
#define WHITESPACE "\t\r\n "
// Values of type of checking
#define CHK_CUT_PASTE		0
#define CHK_COPY_PASTE		1
#define CHK_SHARE 			2
#define CHK_ALLOC			3

void ClearUserSpace(uint32 *ptr_dir);
int CE(uint32 *ptr_dir, uint32 va);
uint32 GP(uint32 *ptr_dir, uint32 va);
int CB(uint32 *ptr_dir, uint32 va, int bn);
int SB(uint32 *ptr_dir, uint32 va, int bn , int v);
int CP(uint32* pd, uint32 va, uint32 ps, uint32 pc);
int CA(uint32 *ptr_dir, uint32 va);
int CPs(uint32 *ptr_dir, uint32 va, uint32 perms, uint32 which);
int CCP(uint32 *ptr_dir, uint32 ptr1, uint32 ptr2, uint32 size, int ref, uint32 dst_perms, uint32 dst_to_chk, uint32 src_perms, uint32 src_to_chk, uint8 chk_type);
int TestAutoCompleteCommand()
{
	cprintf("Automatic Testing of Autocomplete:\n");
	cprintf("\n========================\n");
	//	cprintf("========================\n");
	//	cprintf("Q2 Test: manually try the test cases in the doc. \n..."
	//			"OR, you can do it automatically by un-commenting the code in this function, it should output the same results in the Examples exist in the MS1 ppt\n");
	//
	//	int retValue = 0;
	int i = 0;

	//CASE1:
	//should execute the kernel_info command
	cprintf("==>Testing now AUTOCOMPLETE for: kernel_info\n");
	char cr0[100] = "kernel_info";
	execute_command(cr0) ;
	cprintf("=================\n\n");

	//CASE2: should print the commands that start with he ---> Shall print (help)
	cprintf("==>Testing now AUTOCOMPLETE for: he\n");
	char cr2[100] = "he";
	execute_command(cr2) ;
	cprintf("=================\n\n");


	//CASE3: should print the commands that start with ru ---> Shall print (rum, rub, rut, run, runall) .. Each in a separate line
	cprintf("==>Testing now AUTOCOMPLETE for: ru\n");
	char cr3[100] = "ru";
	execute_command(cr3) ;
	cprintf("=================\n\n");

	//CASE4: should print unknown command
	cprintf("==>Testing now AUTOCOMPLETE for: smm\n");
	char cr4[100] = "smm";
	execute_command(cr4) ;
	cprintf("=================\n\n");
	return 0;
}

//===============================================================================================
//===============================================================================================

/*******************************/
/*TESTs OF PAGING HELPERS*/
/*******************************/

//=====================================
// 1) TEST SET/CLEAR PAGE PERMISSIONS:
//=====================================
int test_pt_set_page_permissions()
{
	//Case 1: Check setting a permission
	uint32 va = 0xEF800000;
	uint32 permissions_to_set = PERM_PRESENT;
	uint32 permissions_to_clear = 0;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	int ret = CP(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	if (ret != 1)
	{
		panic("[EVAL] #1 Set Permission Failed.\n");
	}

	//Case 2: Check setting MORE THAN ONE permission
	va = 0xEF801000;
	permissions_to_set = PERM_MODIFIED|PERM_USER;
	permissions_to_clear = 0;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);

	ret = CP(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	if (ret != 1)
	{
		panic("[EVAL] #2 Set Permission Failed.\n");
	}

	va = 0xEF800000;
	permissions_to_set = PERM_MODIFIED|PERM_USER|PERM_USED|PERM_PRESENT;
	permissions_to_clear = 0;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);

	ret = CP(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	if (ret != 1)
	{
		panic("[EVAL] #3 Set Permission Failed.\n");
	}

	//Case 3: Check clearing a permission
	va = 0xF0000000;
	permissions_to_set = 0;
	permissions_to_clear = PERM_PRESENT;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);

	ret = CP(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	if (ret != 1)
	{
		panic("[EVAL] #4 Clear Permission Failed.\n");
	}

	//Case 4: Check clearing MORE THAN ONE permission
	va = 0xEF800000;
	permissions_to_set = 0;
	permissions_to_clear = PERM_MODIFIED|PERM_USER;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);

	ret = CP(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	if (ret != 1)
	{
		panic("[EVAL] #5 Clear Permission Failed.\n");
	}

	//Case 5: Check settiing & clearing MORE THAN ONE permission together
	va = 0xF0001000;
	permissions_to_set = PERM_USER|PERM_BUFFERED;
	permissions_to_clear = PERM_WRITEABLE|PERM_USED|PERM_MODIFIED;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);

	ret = CP(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	if (ret != 1)
	{
		panic("[EVAL] #6 Set & Clear Permission Failed.\n");
	}

	cprintf("Congratulations!! test pt_set&clear_page_permissions completed successfully.\n");
	return 0;
}

int test_pt_set_page_permissions_invalid_va()
{
	uint32 va = 0x0;
	uint32 permissions_to_set = PERM_PRESENT;
	uint32 permissions_to_clear = 0;
	pt_set_page_permissions(ptr_page_directory, va, permissions_to_set, permissions_to_clear);
	panic("WRONG PANIC - This test shall panic with your error message. Check handling setting permissions of an invalid virtual address with non existing page table.");
	return 0;
}

//=====================================
// 2) TEST GET PAGE PERMISSIONS:
//=====================================
int test_pt_get_page_permissions()
{
	//Case 1: Check getting a permission of a non existing VA with NO table
	uint32 va = 0xeebfe000;
	int ret = pt_get_page_permissions(ptr_page_directory, va);
	if (ret != -1)
	{
		panic("[EVAL] #1 Get Permission Failed.\n");
	}

	//Case 2: Check getting a permission of a non existing VA with an existing table
	va = 0xEF800000;
	ret = pt_get_page_permissions(ptr_page_directory, va);
	if (ret != 0)
	{
		panic("[EVAL] #2 Get Permission Failed.\n");
	}

	//Case 3: Check getting a permission of an existing VA with an existing table
	va = 0xf0000000;
	ret = pt_get_page_permissions(ptr_page_directory, va);
	if (ret != 3)
	{
		panic("[EVAL] #3 Get Permission Failed.\n");
	}

	va = 0xF1000000;
	ret = pt_get_page_permissions(ptr_page_directory, va);
	if (ret != 3)
	{
		panic("[EVAL] #4 Get Permission Failed.\n");
	}

	va = 0xF0001000;
	ret = pt_get_page_permissions(ptr_page_directory, va);
	if (ret != 99)
	{
		panic("[EVAL] #5 Get Permission Failed.\n");
	}
	cprintf("Congratulations!! test pt_get_page_permissions completed successfully.\n");
	return 0;
}

//=====================================
// 3) TEST CLEAR PAGE TABLE ENTRY:
//=====================================
int test_pt_clear_page_table_entry()
{
	uint32 va = 0xF1000000;
	pt_clear_page_table_entry(ptr_page_directory, va);
	int ret = CE(ptr_page_directory, va);
	if (ret != 1)
	{
		panic("[EVAL] #1 Clear Page Table Entry Failed.\n");
	}

	va = 0xF0001000;
	pt_clear_page_table_entry(ptr_page_directory, va);
	ret = CE(ptr_page_directory, va);
	if (ret != 1)
	{
		panic("[EVAL] #2 Clear Page Table Entry Failed.\n");
	}

	va = 0xEF800000;
	pt_clear_page_table_entry(ptr_page_directory, va);
	ret = CE(ptr_page_directory, va);
	if (ret != 1)
	{
		panic("[EVAL] #3 Clear Page Table Entry Failed.\n");
	}

	va = 0xF0000000;
	pt_clear_page_table_entry(ptr_page_directory, va);
	ret = CE(ptr_page_directory, va);
	if (ret != 1)
	{
		panic("[EVAL] #4 Clear Page Table Entry Failed.\n");
	}

	cprintf("Congratulations!! test pt_clear_page_table_entry completed successfully.\n");
	return 0;
}

int test_pt_clear_page_table_entry_invalid_va()
{
	uint32 va = 0x1000;
	pt_clear_page_table_entry(ptr_page_directory, va);
	panic("WRONG PANIC - This test shall panic with your error message. Check handling clearing the entry of an invalid virtual address non existing page table.");
	return 0;
}

/*******************************/
/*TESTs OF CHUNKS MANIPULATION */
/*******************************/

//===============================
// 1) TEST CUT-PASTE PAGES:
//===============================
int test_cut_paste_pages()
{
	//Create a Temp. User Process
	char prog_name[50] = "fos_helloWorld";
	struct Env* env = env_create(prog_name, 20, 10, 0);
	uint32 *proc_directory = env->env_page_directory ;
	lcr3(env->env_cr3) ;
	char aup[20] = "aup " ;
	char env_id[20] ; ltostr(env->env_id, env_id) ;
	char aup_cmd[50];
	strcconcat(aup, env_id, aup_cmd);
	//===================================================
	int numOfArgs = 0;
	char *args[MAX_ARGUMENTS] ;

	char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6;
	int kilo = 1024 ;
	int mega = 1024*1024 ;

	ClearUserSpace(proc_directory);

	char ap1[100] ;strcconcat(aup_cmd, " 0x2800000", ap1); execute_command(ap1);
	char ap2[100] ;strcconcat(aup_cmd, " 0x2801000", ap2); execute_command(ap2);
	char ap3[100] ;strcconcat(aup_cmd, " 0x2802000", ap3); execute_command(ap3);

	ptr1 = (char*)0x2800000; *ptr1 = 'a';
	ptr1 = (char*)0x28017FF; *ptr1 = 'b';
	ptr1 = (char*)0x2802FFF; *ptr1 = 'c';

	uint32 perms = GP(proc_directory, (uint32)ptr1);

	int eval = 0;
	int correct = 1;
	int ff1 = sys_calculate_free_frames();

	/*=============================================*/
	/*PART I: Destination Pages Does NOT Exist 60% */
	/*=============================================*/
	cprintf("CASE I: Destination Pages Does NOT Exist [60%]\n") ;
	int ret = cut_paste_pages(proc_directory, 0x2800000, 0x2900000, 3) ;

	int ff2 = sys_calculate_free_frames();

	correct = 1 ;
	if (ret != 0 || ff1 != ff2)
	{
		warn("[EVAL] cut_paste_pages: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, ff1 - ff2);
		correct = 0;
	}
	if (correct) eval += 5 ;
	correct = 1 ;

	if (CCP(proc_directory, 0x2800000, 0x2900000, 3*PAGE_SIZE, 1, perms, 0xFFF, 0, 0x001, CHK_CUT_PASTE) != 1)
	{
		warn("[EVAL] cut_paste_pages: Failed (problem in permissions and/or references\n");
		correct = 0;
	}
	if (correct) eval += 15 ;
	correct = 1 ;

	if (CB(proc_directory, 0x2900000, 0) && CB(proc_directory, 0x2901000, 0) && CB(proc_directory, 0x2902000, 0))
	{
		ptr1 = (char*)0x2900000;
		ptr2 = (char*)0x29017FF;
		ptr3 = (char*)0x2902FFF;
		if ((*ptr1) != 'a' || (*ptr2) != 'b' || (*ptr3) != 'c')
		{
			warn("[EVAL] cut_paste_pages: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CB(proc_directory, 0x2901000, 1))
		{
			*ptr2 = 'y';
			if ((*ptr2) != 'y')
			{
				warn("[EVAL] cut_paste_pages: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;
		}
	}


	ff1 = ff2 ;

	ret = cut_paste_pages(proc_directory, 0x2901000, 0x2BFF000, 2) ;

	ff2 = sys_calculate_free_frames();

	if (ret != 0 || ff1 - ff2 != 1)
	{
		warn("[EVAL] cut_paste_pages: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, ff1 - ff2);
		correct = 0;
	}
	if (correct) eval += 10 ;
	correct = 1 ;

	if (CCP(proc_directory, 0x2901000, 0x2BFF000, 2*PAGE_SIZE, 1, perms , 0xFFF, 0, 0x001, CHK_CUT_PASTE) != 1)
	{
		warn("[EVAL] cut_paste_pages: Failed (problem in permissions and/or references\n");
		correct = 0;
	}
	if (correct) eval += 10 ;
	correct = 1 ;

	if (CB(proc_directory, 0x2BFF7FF, 0) && CB(proc_directory, 0x2C00FFF, 0))
	{
		ptr1 = (char*)0x2BFF7FF;
		ptr2 = (char*)0x2C00FFF;
		if ((*ptr1) != 'y' || (*ptr2) != 'c')
		{
			warn("[EVAL] cut_paste_pages: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("CASE I: END\n") ;

	/*========================================*/
	/*PART II: Destination Pages Exist 40%	  */
	/*========================================*/
	cprintf("\nCASE II: Destination Pages Exist [40%]\n") ;

	char ap4[100] ;strcconcat(aup_cmd, " 0x1400000", ap4); execute_command(ap4);
	char ap5[100] ;strcconcat(aup_cmd, " 0x1401000", ap5); execute_command(ap5);
	char ap6[100] ;strcconcat(aup_cmd, " 0x1402000", ap6); execute_command(ap6);
	char ap7[100] ;strcconcat(aup_cmd, " 0x1C00000", ap7); execute_command(ap7);

	ptr1 = (char*)0x1400000; *ptr1 = 'a';
	ptr1 = (char*)0x14007FF; *ptr1 = 'b';
	ptr1 = (char*)0x1400FFF; *ptr1 = 'c';
	ptr1 = (char*)0x1C00000; *ptr1 = 'x';
	ptr1 = (char*)0x1C007FF; *ptr1 = 'y';
	ptr1 = (char*)0x1C00FFF; *ptr1 = 'z';
	uint32 srcp = GP(proc_directory, 0x1C00000) ;
	uint32 dstp = GP(proc_directory, 0x1400000) ;

	ff1 = sys_calculate_free_frames();

	ret = cut_paste_pages(proc_directory, 0x1C00000, 0x1400000, 1) ;

	ff2 = sys_calculate_free_frames();

	if (ret != -1 || ff1 - ff2 != 0)
	{
		warn("[EVAL] cut_paste_pages: Failed (dest is exist... operation should be denied) ret=%d diff=%d\n", ret, ff1 - ff2);
		correct = 0;
	}
	if (correct) eval += 10 ;
	correct = 1 ;

	int chk_cntnt = 1 ;
	if (CCP(proc_directory, 0x1C00000, 0x1400000, 1*PAGE_SIZE, 1, dstp , 0xFFF, srcp, 0xFFF, CHK_CUT_PASTE) != 1)
	{
		warn("[EVAL] cut_paste_pages: Failed (problem in permissions and/or references\n");
		correct = 0;
		chk_cntnt = 0;
	}
	if (correct) eval += 5 ;
	correct = 1 ;

	if (chk_cntnt)
	{
		ptr1 = (char*)0x1400000;
		ptr2 = (char*)0x1C00000;
		ptr3 = (char*)0x14007FF;
		ptr4 = (char*)0x1C007FF;
		ptr5 = (char*)0x1400FFF;
		ptr6 = (char*)0x1C00FFF;
		if ((*ptr1) != 'a' || (*ptr2) != 'x' || (*ptr3) != 'b' ||
				(*ptr4) != 'y'|| (*ptr5) != 'c'|| (*ptr6) != 'z')
		{
			warn("[EVAL] cut_paste_pages: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}

	ff1 = sys_calculate_free_frames();

	ret = cut_paste_pages(proc_directory, 0x1400000, 0x1BFF000, 3) ;

	ff2 = sys_calculate_free_frames();

	if (ret != -1 || ff1 - ff2 != 0)
	{
		warn("[EVAL] cut_paste_pages: Failed (dest is exist... operation should be denied) ret=%d diff=%d\n", ret, ff1 - ff2);
		correct = 0;
	}
	if (correct) eval += 10 ;
	correct = 1 ;
	chk_cntnt = 1;
	if (CB(proc_directory, 0x1400000, 0) != 1 || CB(proc_directory, 0x1401000, 0) != 1 || CB(proc_directory, 0x1402000, 0) != 1 ||
			CB(proc_directory, 0x1BFF000, 0) != 0 || CB(proc_directory, 0x1C00000, 0) != 1 || CB(proc_directory, 0x1C01000, 0) != 0)
	{
		warn("[EVAL] cut_paste_pages: Failed (problem in permissions)\n");
		correct = 0;
		chk_cntnt = 0;
	}
	if (correct) eval += 5 ;
	correct = 1 ;

	if (chk_cntnt)
	{
		ptr1 = (char*)0x1400000;
		ptr2 = (char*)0x1C00000;
		ptr3 = (char*)0x14007FF;
		ptr4 = (char*)0x1C007FF;
		ptr5 = (char*)0x1400FFF;
		ptr6 = (char*)0x1C00FFF;
		if ((*ptr1) != 'a' || (*ptr2) != 'x' || (*ptr3) != 'b' ||
				(*ptr4) != 'y'|| (*ptr5) != 'c'|| (*ptr6) != 'z')
		{
			correct = 0;
			chk_cntnt = 0;
			warn("[EVAL] cut_paste_pages: Failed (content is not correct)\n");
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}

	cprintf("CASE II: END\n") ;

	cprintf("[EVAL] cut_paste_pages: FINISHED. Evaluation = %d\n", eval);
	if(eval == 100)
		cprintf("Congratulations!! test cut_paste_pages completed successfully.\n");

	//return back to the kernel directory
	lcr3(phys_page_directory) ;

	return 0;
}

//===============================
// 2) TEST COPY-PASTE CHUNK:
//===============================
int test_copy_paste_chunk()
{
	//Create a Temp. User Process
	char prog_name[50] = "fos_helloWorld";
	struct Env* env = env_create(prog_name, 20, 10, 0);
	uint32 *proc_directory = env->env_page_directory ;
	lcr3(env->env_cr3) ;
	char aup[20] = "aup " ;
	char env_id[20] ; ltostr(env->env_id, env_id) ;
	char aup_cmd[50];
	strcconcat(aup, env_id, aup_cmd);
	//===================================================

	ClearUserSpace(proc_directory);
	int numOfArgs = 0;
	char *args[MAX_ARGUMENTS] ;
	uint32 res =0;
	uint32 eval = 0; int correct = 1 ;
	uint32 numOfFreeFramesBefore, numOfFreeFramesAfter ;
	char *ch1, *ch2, *ch3, *ch4, *ch5, *ch6, *ch7,*ch8, *ch9, *ch10, *ch11, *ch12 ;
	char tch[13];
	int kilo = 1024 ;
	int mega = 1024*1024 ;
	/*==================================================*/
	/*PART I: Destination page(s) exist & read only 20% */
	/*==================================================*/
	cprintf("\nCASE I: Destination page(s) exist & read only [20%]\n") ;
	{
		/*allocate page*/char c1[100] ;strcconcat(aup_cmd, " 0x0", c1); execute_command(c1);
		/*allocate another page ====*/ strcconcat(aup_cmd, " 0x1000", c1); execute_command(c1);
		/*write on 1st page*/
		char c2[100] = "wum 0x000000 a";execute_command(c2);
		char c3[100] = "wum 0x0007FF b";execute_command(c3);
		char c4[100] = "wum 0x000FFF c";execute_command(c4);
		/*write on 2nd page*/
		char c22[100] = "wum 0x001000 d";execute_command(c22);
		char c23[100] = "wum 0x0017FF e";execute_command(c23);
		char c24[100] = "wum 0x001FFF f";execute_command(c24);
		/*allocate page*/char c5[100] ;strcconcat(aup_cmd, " 0x100000", c5); execute_command(c5);
		/*allocate another page ====*/ strcconcat(aup_cmd, " 0x101000 r", c5); execute_command(c5);
		char c6[100] = "wum 0x100000 x";execute_command(c6);
		char c7[100] = "wum 0x1007FF y";execute_command(c7);
		char c8[100] = "wum 0x100FFF z";execute_command(c8);

		ch1 = (char*)0x000000; ch2 = (char*)0x100000;
		ch3 = (char*)0x0007FF; ch4 = (char*)0x1007FF;
		ch5 = (char*)0x000FFF; ch6 = (char*)0x100FFF;
		ch7 = (char*)0x001000; ch8 = (char*)0x101000;
		ch9 = (char*)0x0017FF; ch10= (char*)0x1017FF;
		ch11= (char*)0x001FFF; ch12= (char*)0x101FFF;

		tch[8] = *ch8 ;tch[10] = *ch10 ;tch[12] = *ch12 ;

		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = copy_paste_chunk(proc_directory, 0x0, 0x100000, 6*kilo);

		numOfFreeFramesAfter = sys_calculate_free_frames();

		correct = 1 ;
		if (ret != -1 || numOfFreeFramesBefore != numOfFreeFramesAfter)
		{
			warn("[EVAL] copy_paste_chunk: Failed (dest is read-only... operation should be denied) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		if (*ch1 != 'a' || *ch2 != 'x' || *ch3 != 'b' || *ch4 != 'y' || *ch5 != 'c' || *ch6 != 'z'
				||  *ch7 != 'd' || *ch8 != tch[8] || *ch9 != 'e' || *ch10 != tch[10] || *ch11!= 'f' || *ch12 != tch[12])
		{
			warn("[EVAL] copy_paste_chunk: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

	}
	cprintf("\nCASE I: END \n") ;

	/*==================================================*/
	/*PART II: Destination page(s) exist & writable 40% */
	/*==================================================*/
	cprintf("\nCASE II: Destination page(s) exist & writable [40%]\n") ;
	{
		/*allocate page*/char c1[100] ;strcconcat(aup_cmd, " 0x200000", c1); execute_command(c1);
		/*allocate another page ====*/ strcconcat(aup_cmd, " 0x201000", c1); execute_command(c1);
		/*write on 1st page*/
		char c2[100] = "wum 0x200000 a";execute_command(c2);
		char c3[100] = "wum 0x2007FF b";execute_command(c3);
		char c4[100] = "wum 0x200FFF c";execute_command(c4);
		/*write on 2nd page*/
		char c22[100] = "wum 0x201000 d";execute_command(c22);
		char c23[100] = "wum 0x2017FF e";execute_command(c23);
		char c24[100] = "wum 0x201FFF f";execute_command(c24);
		/*allocate page*/char c5[100] ;strcconcat(aup_cmd, " 0x400000", c5); execute_command(c5);
		/*allocate another page ====*/ strcconcat(aup_cmd, " 0x401000", c5); execute_command(c5);
		char c6[100] = "wum 0x400000 x";execute_command(c6);
		char c7[100] = "wum 0x4007FF y";execute_command(c7);
		char c8[100] = "wum 0x400FFF z";execute_command(c8);

		//Test1
		ch1 = (char*)0x200000; ch2 = (char*)0x400000;
		ch3 = (char*)0x2007FF; ch4 = (char*)0x4007FF;
		ch5 = (char*)0x200FFF; ch6 = (char*)0x400FFF;
		ch7 = (char*)0x201000; ch8 = (char*)0x401000;
		ch9 = (char*)0x2017FF; ch10= (char*)0x4017FF;
		ch11= (char*)0x201FFF; ch12= (char*)0x401FFF;

		tch[12] = *ch12 ;
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = copy_paste_chunk(proc_directory, 0x200000, 0x400000, 6*kilo);

		numOfFreeFramesAfter = sys_calculate_free_frames();

		correct = 1 ;
		if (ret != 0 || numOfFreeFramesBefore != numOfFreeFramesAfter)
		{
			warn("[EVAL] copy_paste_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chkcnt = 1;
		if (CCP(proc_directory, 0x200000, 0x400000, 2*PAGE_SIZE, 1, 0x007, 0x007, 0x007, 0x007, CHK_COPY_PASTE) != 1)
		{
			warn("[EVAL] copy_paste_chunk: Failed (problem in permissions and/or references)\n");
			correct = 0;
			chkcnt = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		*ch3 = 'y' ;	// wum 0x2007FF y
		*ch6 = 'z' ;	// wum 0x400FFF z
		*ch7 = 'w' ;	// wum 0x201000 w

		if (*ch1 != 'a' || *ch2 != 'a' || *ch3 != 'y' || *ch4 != 'b' || *ch5 != 'c' || *ch6 != 'z'
				||  *ch7 != 'w' || *ch8 != 'd' || *ch9 != 'e' || *ch10!= 'e' || *ch11!= 'f' || *ch12 != tch[12])
		{
			warn("[EVAL] copy_paste_chunk: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		//Test2
		*ch10 = 'x';	// wum 0x4017FF y
		numOfFreeFramesBefore = sys_calculate_free_frames();

		ret = copy_paste_chunk(proc_directory, 0x400800, 0x200800, 3*kilo);

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || numOfFreeFramesBefore != numOfFreeFramesAfter)
		{
			warn("[EVAL] copy_paste_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CCP(proc_directory, 0x400000, 0x200000, 2*PAGE_SIZE, 1, 0x007, 0x007, 0x007, 0x007, CHK_COPY_PASTE) != 1)
		{
			warn("[EVAL] copy_paste_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (*ch1 != 'a' || *ch2 != 'a' || *ch3 != 'y' || *ch4 != 'b' || *ch5 != 'z' || *ch6 != 'z' ||
				*ch7 != 'd' || *ch8 != 'd' || *ch9 != 'e' || *ch10!= 'x' || *ch11!= 'f'  || *ch12 != tch[12])
		{
			warn("[EVAL] copy_paste_chunk: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

	}
	cprintf("\nCASE II: END\n") ;

	/*================================================*/
	/*PART III: Destination page(s) doesn't exist 40% */
	/*================================================*/
	cprintf("\nCASE III: Destination page(s) doesn't exist [40%]\n") ;
	{
		/*allocate page*/char c1[100] ;strcconcat(aup_cmd, " 0x800000", c1); execute_command(c1);
		/*allocate another page ====*/ strcconcat(aup_cmd, " 0x801000", c1); execute_command(c1);
		/*allocate another page ====*/ strcconcat(aup_cmd, " 0x802000", c1); execute_command(c1);
		char c14[100] = "wum 0x800000 a"; execute_command(c14);
		char c15[100] = "wum 0x8017FF b"; execute_command(c15);
		char c16[100] = "wum 0x802FFF c"; execute_command(c16);

		//Test3
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = copy_paste_chunk(proc_directory, 0x800000, 0x900000, 12*kilo);

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 3)
		{
			warn("[EVAL] copy_paste_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chkcnt = 1 ;
		if (CCP(proc_directory, 0x800000, 0x900000, 3*PAGE_SIZE, 1, 0x007, 0x007, 0x007, 0x007, CHK_COPY_PASTE) != 1)
		{
			warn("[EVAL] copy_paste_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chkcnt = 0 ;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (chkcnt)
		{
			ch1 = (char*)0x800000; ch2 = (char*)0x900000;
			ch3 = (char*)0x8017FF; ch4 = (char*)0x9017FF;
			ch5 = (char*)0x802FFF; ch6 = (char*)0x902FFF;

			*ch3 = 'y';	//wum 0x8017FF y
			*ch6 = 'z';	//wum 0x902FFF z

			if (*ch1 != 'a' || *ch2 != 'a' || *ch3 != 'y' || *ch4 != 'b' || *ch5 != 'c' || *ch6 != 'z')
			{
				warn("[EVAL] copy_paste_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 10 ;
			correct = 1 ;
		}
		//Test4
		numOfFreeFramesBefore = sys_calculate_free_frames();
		SB(proc_directory, 0x901000, 2 , 0) ;
		SB(proc_directory, 0x902000, 2 , 0) ;

		ret = copy_paste_chunk(proc_directory, 0x901000, 0xBFF000, 8*kilo);

		numOfFreeFramesAfter = sys_calculate_free_frames();
		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 3)
		{
			warn("[EVAL] copy_paste_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		chkcnt = 1 ;
		if (CCP(proc_directory, 0x901000, 0xBFF000, 2*PAGE_SIZE, 1, 0x003, 0x007, 0x003, 0x007, CHK_COPY_PASTE) != 1)
		{
			warn("[EVAL] copy_paste_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chkcnt = 0 ;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		if (chkcnt)
		{
			ch1 = (char*)0x9017FF; ch2 = (char*)0xBFF7FF; ch3 = (char*)0x902FFF;ch4 = (char*)0xC00FFF;
			if (*ch1 != 'b' || *ch2 != 'b' || *ch3 != 'z' || *ch4 != 'z')
			{
				warn("[EVAL] copy_paste_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;
		}
	}
	cprintf("\nCASE III: END\n") ;

	cprintf("[EVAL] copy_paste_chunk: FINISHED. Evaluation = %d\n", eval);
	if(eval == 100)
		cprintf("Congratulations!! test copy_paste_chunk completed successfully.\n");

	//return back to the kernel directory
	lcr3(phys_page_directory) ;

	return 0;
}

//===============================
// 3) TEST SHARE CHUNK:
//===============================
int test_share_chunk()
{
	//Create a Temp. User Process
	char prog_name[50] = "fos_helloWorld";
	struct Env* env = env_create(prog_name, 20, 10, 0);
	uint32 *proc_directory = env->env_page_directory ;
	lcr3(env->env_cr3) ;
	//===================================================

	ClearUserSpace(proc_directory);

	char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;
	char tptr[10] ;
	int kilo = 1024 ;
	int mega = 1024*1024 ;
	uint32 eval = 0;
	uint8 correct =1 ;
	uint32 numOfFreeFramesBefore, numOfFreeFramesAfter;
	extern char end_of_kernel[];

	/*======================================*/
	/*PART I: Destination page(s) exist 20% */
	/*======================================*/
	cprintf("\nCASE I: Destination page(s) exist [20%]\n") ;
	{
		ptr1 = (char*)0xF0100000;
		ptr2 = (char*)0xF0104000;
		tptr[1] = *ptr1 ;
		tptr[2] = *ptr2 ;

		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = share_chunk(proc_directory, 0xF0100000,0xF0104000, 6*kilo, PERM_WRITEABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != -1 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 0)
		{
			warn("[EVAL] share_chunk: Failed (dest is exist... operation should be denied) ret=%d diff=%d\n", ret, (numOfFreeFramesBefore - numOfFreeFramesAfter));
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CCP(proc_directory, 0xF0100000, 0xF0104000, 8*kilo, 1, 0x003, 0x007, 0x003, 0x007, ~CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;

		*ptr1 = 'A' ;
		*ptr2 = 'B' ;

		if ((*ptr1) != 'A' || (*ptr2) != 'B')
		{
			warn("[EVAL] share_chunk: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		*ptr1 = tptr[1] ;
		*ptr2 = tptr[2] ;
	}
	cprintf("\nCASE I: END\n") ;

	/*========================================================*/
	/*PART II: Destination page(s) not exist [Supervisor] 25% */
	/*========================================================*/
	cprintf("\nCASE II: Destination page(s) not exist [Supervisor] [25%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = share_chunk(proc_directory, 0xF0000000,0x40000000, 32*mega, PERM_WRITEABLE | PERM_AVAILABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != (32*mega) / (4*mega))
		{
			warn("[EVAL] share_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0xF0000000, 0x40000000, 32*mega, -1, 0xE03, 0xE07, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}

		if (CCP(proc_directory, 0xF0000000, 0x40000000, 12*kilo, 2, 0xE03, 0xE07, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		extern char end_of_kernel[];
		uint32 endRange = ((uint32)end_of_kernel - KERNEL_BASE);
		if (CCP(proc_directory, 0xF0000000+PHYS_IO_MEM, 0x40000000+PHYS_IO_MEM, endRange - PHYS_IO_MEM, 2, 0xE03, 0xE07, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (chk_cnt)
		{
			ptr1 = (char*)0xF00007FF; *ptr1 = 'A' ;
			ptr2 = (char*)0x400007FF;

			if ((*ptr1) != 'A' || (*ptr2) != 'A')
			{
				warn("[EVAL] share_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;

			ptr1 = (char*)0x41000FFF; *ptr1 = 'C' ;
			ptr2 = (char*)0xF1000FFF;

			if ((*ptr1) != 'C' || (*ptr2) != 'C')
			{
				warn("[EVAL] share_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;
		}
	}
	cprintf("\nCASE II: END\n") ;

	/*========================================================*/
	/*PART III: Destination page(s) not exist [User r/w] 25%  */
	/*========================================================*/
	cprintf("\nCASE III: Destination page(s) not exist [User r/w] [25%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = share_chunk(proc_directory, 0x40000000,0x0, 648*kilo, PERM_WRITEABLE|PERM_USER) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 1)
		{
			warn("[EVAL] share_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0x40000000, 0x0, PHYS_IO_MEM + 4*kilo, -1, 0x007, 0x007, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}

		if (CCP(proc_directory, 0x40000000, 0x0, 12*kilo, 3, 0x007, 0x007, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CCP(proc_directory, 0x40003000, 0x3000, PHYS_IO_MEM - 12*kilo, 2, 0x007, 0x007, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CCP(proc_directory, 0x40000000+PHYS_IO_MEM, PHYS_IO_MEM, 4*kilo, 3, 0x007, 0x007, 0x003, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (chk_cnt)
		{
			ptr1 = (char*)0x7FF;
			ptr2 = (char*)0xF00007FF;
			ptr3 = (char*)0x400007FF;
			ptr4 = (char*)0x9FFFF; *ptr4 = 'D';
			ptr5 = (char*)0xF009FFFF;
			ptr6 = (char*)0x4009FFFF;

			if ((*ptr1) != 'A' || (*ptr2) != 'A' || (*ptr3) != 'A' ||
					(*ptr4) != 'D' || (*ptr5) != 'D'|| (*ptr6) != 'D')
			{
				warn("[EVAL] share_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;
		}
	}
	cprintf("\nCASE III: END\n") ;

	/*========================================================*/
	/*PART IV: Destination page(s) not exist [User r] 30%     */
	/*========================================================*/
	cprintf("\nCASE IV: Destination page(s) not exist [User r] [30%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = share_chunk(proc_directory, 0x9FC00,0x3FFC00, 7*kilo, PERM_USER) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 1)
		{
			warn("[EVAL] share_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0x9F000, 0x3FF000, 12*kilo, -1, 0x005, 0x007, 0x007, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CCP(proc_directory, 0x9F000, 0x3FF000, 4*kilo, 3, 0x005, 0x007, 0x007, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (CCP(proc_directory, 0xA0000, 0x400000, 8*kilo, 4, 0x005, 0x007, 0x007, 0x007, CHK_SHARE) == 0)
		{
			warn("[EVAL] share_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if (chk_cnt)
		{
			ptr1 = (char*)0x0009FFFF;
			ptr2 = (char*)0x003FFFFF;
			ptr3 = (char*)0x4009FFFF;
			ptr4 = (char*)0xF009FFFF;

			ptr5 = (char*)0x000A1001;
			ptr6 = (char*)0x00401001;
			ptr7 = (char*)0x400A1001;
			ptr8 = (char*)0xF00A1001;

			if ((*ptr1) != 'D' || (*ptr2) != 'D' || (*ptr3) != 'D' || (*ptr4) != 'D' ||
					(*ptr5) != (*ptr6) || (*ptr5) != (*ptr7) ||(*ptr5) != (*ptr8))
			{
				warn("[EVAL] share_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;
		}
	}
	cprintf("\nCASE IV: END\n") ;

	cprintf("[EVAL] share_chunk: FINISHED. Evaluation = %d\n", eval);
	if(eval == 100)
		cprintf("Congratulations!! test share_chunk completed successfully.\n");

	//return back to the kernel directory
	lcr3(phys_page_directory) ;

	return 0;
}

//===============================
// 4) TEST ALLOCATE CHUNK:
//===============================
int test_allocate_chunk()
{
	//Create a Temp. User Process
	char prog_name[50] = "fos_helloWorld";
	struct Env* env = env_create(prog_name, 20, 10, 0);
	uint32 *proc_directory = env->env_page_directory ;
	lcr3(env->env_cr3) ;
	//===================================================

	ClearUserSpace(proc_directory);

	char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;
	char tptr[10] ;
	int kilo = 1024 ;
	int mega = 1024*1024 ;
	uint32 eval = 0;
	uint8 correct =1 ;
	uint32 numOfFreeFramesBefore, numOfFreeFramesAfter;
	extern char end_of_kernel[];

	/*======================================*/
	/*PART I: Destination page(s) exist 30% */
	/*======================================*/
	cprintf("\nCASE I: Destination page(s) exist [30%]\n") ;
	{
		ptr1 = (char*)KERNEL_STACK_TOP - 1;
		ptr2 = (char*)KERNEL_STACK_TOP - 2;
		while ((ptr1 > (char*)(KERNEL_STACK_TOP - PAGE_SIZE)) && *ptr1 == 0)	ptr1-- ;
		if (ptr1 == (char*)(KERNEL_STACK_TOP - PAGE_SIZE))	*ptr1 = 'A' ;
		tptr[1] = *ptr1 ;
		tptr[2] = *ptr2 ;
		cprintf("*ptr1 = %c\n", *ptr1) ;
		cprintf("*ptr2 = %c\n", *ptr2) ;
		uint32 old_perms = GP(proc_directory, KERNEL_STACK_TOP - 1*PAGE_SIZE) ;
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = allocate_chunk(proc_directory, KERNEL_STACK_TOP - 1*PAGE_SIZE, 4*kilo, PERM_WRITEABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != -1 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 0)
		{
			warn("[EVAL] allocate_chunk: Failed (dest is exist... operation should be denied) ret=%d diff=%d expected=%d\n", ret, (numOfFreeFramesBefore - numOfFreeFramesAfter), 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		numOfFreeFramesBefore = sys_calculate_free_frames();

		ret = allocate_chunk(proc_directory, KERNEL_STACK_TOP - 5*kilo, 2*kilo, PERM_WRITEABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != -1 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != 0)
		{
			warn("[EVAL] allocate_chunk: Failed (dest is exist... operation should be denied) ret=%d diff=%d expected=%d\n", ret, (numOfFreeFramesBefore - numOfFreeFramesAfter), 0);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		if (CCP(proc_directory, 0, KERNEL_STACK_TOP-1*PAGE_SIZE, 4*kilo, 1, old_perms, 0xFFF, 0, 0, CHK_ALLOC) == 0)
		{
			warn("[EVAL] allocate_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		if ((*ptr1) != tptr[1] || (*ptr2) != tptr[2])
		{
			warn("[EVAL] allocate_chunk: Failed (content is not correct)\n");
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("\nCASE I: END\n") ;

	/*============================================================*/
	/*PART II: Destination page(s) not exist [Supervisor r/w] 20% */
	/*============================================================*/
	cprintf("\nCASE II: Destination page(s) not exist [Supervisor r/w] [20%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = allocate_chunk(proc_directory, 0x0, 32*mega, PERM_WRITEABLE | PERM_AVAILABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != ((32*mega)/(4*mega) + (32*mega)/(4*kilo)))
		{
			warn("[EVAL] allocate_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d expected=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter, ((32*mega)/(4*mega) + (32*mega)/(4*kilo)));
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0, 0x0, 32*mega, 1, 0xE03, 0xE07, 0, 0, CHK_ALLOC) == 0)
		{
			warn("[EVAL] allocate_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		if (chk_cnt)
		{
			ptr1 = (char*)(0x0+2*kilo); *ptr1 = 'K' ;
			ptr2 = (char*)(0x0+2*mega); *ptr2 = 'M' ;

			if ((*ptr1) != 'K' || (*ptr2) != 'M')
			{
				warn("[EVAL] allocate_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;
		}
	}
	cprintf("\nCASE II: END\n") ;

	/*============================================================*/
	/*PART III: Destination page(s) not exist [Supervisor r] 15%  */
	/*============================================================*/
	cprintf("\nCASE III: Destination page(s) not exist [Supervisor r] [15%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = allocate_chunk(proc_directory, 0x0+32*mega, 64*mega, 0) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != ((64*mega)/(4*mega) + (64*mega)/(4*kilo)))
		{
			warn("[EVAL] allocate_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d expected=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter, ((64*mega)/(4*mega) + (64*mega)/(4*kilo)));
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0, 0x0+32*mega, 64*mega, 1, 0x001, 0xE07, 0, 0, CHK_ALLOC) == 0)
		{
			warn("[EVAL] allocate_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("\nCASE III: END\n") ;

	/*========================================================*/
	/*PART IV: Destination page(s) not exist [User r/w] 20%  */
	/*========================================================*/
	cprintf("\nCASE IV: Destination page(s) not exist [User r/w] [20%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = allocate_chunk(proc_directory, USER_HEAP_START, 64*mega, PERM_WRITEABLE|PERM_USER|PERM_AVAILABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != ((64*mega)/(4*kilo)+(64*mega)/(4*mega)))
		{
			warn("[EVAL] allocate_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d expected=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter, ((64*mega)/(4*kilo)+(64*mega)/(4*mega)));
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0, USER_HEAP_START, 64*mega, 1, 0xE07, 0xE07, 0, 0, CHK_ALLOC) == 0)
		{
			warn("[EVAL] allocate_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		if (chk_cnt)
		{
			ptr1 = (char*)(USER_HEAP_START+2*kilo); *ptr1 = 'K' ;
			ptr2 = (char*)(USER_HEAP_START+22*mega);*ptr2 = 'M' ;

			if ((*ptr1) != 'K' || (*ptr2) != 'M')
			{
				warn("[EVAL] allocate_chunk: Failed (content is not correct)\n");
				correct = 0;
			}
			if (correct) eval += 5 ;
			correct = 1 ;

		}
	}
	cprintf("\nCASE IV: END\n") ;

	/*========================================================*/
	/*PART V: Destination page(s) not exist [User r] 15%     */
	/*========================================================*/
	cprintf("\nCASE V: Destination page(s) not exist [User r] [15%]\n") ;
	{
		numOfFreeFramesBefore = sys_calculate_free_frames();

		int ret = allocate_chunk(proc_directory,0x403FFC00, 7*kilo, PERM_USER|PERM_AVAILABLE) ;

		numOfFreeFramesAfter = sys_calculate_free_frames();

		if (ret != 0 || (numOfFreeFramesBefore - numOfFreeFramesAfter) != (3+2))
		{
			warn("[EVAL] allocate_chunk: Failed (# allocated frames is not correct) ret=%d diff=%d expected=%d\n", ret, numOfFreeFramesBefore - numOfFreeFramesAfter, (3+2));
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		int chk_cnt = 1 ;
		if (CCP(proc_directory, 0, 0x403FF000, 12*kilo, 1, 0xE05, 0xE07, 0, 0, CHK_ALLOC) == 0)
		{
			warn("[EVAL] allocate_chunk: Failed (problem in permissions and/or references\n");
			correct = 0;
			chk_cnt = 0 ;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

	}
	cprintf("\nCASE V: END\n") ;

	cprintf("[EVAL] allocate_chunk: FINISHED. Evaluation = %d\n", eval);
	if(eval == 100)
		cprintf("Congratulations!! test allocate_chunk completed successfully.\n");

	//return back to the kernel directory
	lcr3(phys_page_directory) ;

	return 0;
}

//======================================
// 5) [+]TEST CALCULATE REQUIRED FRAMES:
//======================================
int test_calculate_required_frames()
{
	//Create a Temp. User Process
	char prog_name[50] = "fos_helloWorld";
	struct Env* env = env_create(prog_name, 20, 10, 0);
	uint32 *proc_directory = env->env_page_directory ;
	lcr3(env->env_cr3) ;
	char aup[20] = "aup " ;
	char env_id[20] ; ltostr(env->env_id, env_id) ;
	char aup_cmd[50];
	strcconcat(aup, env_id, aup_cmd);
	//===================================================

	char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;
	char tptr[10] ;
	int kilo = 1024 ;
	int mega = 1024*1024 ;
	uint32 eval = 0;
	uint32 res =0;
	uint8 correct = 1;
	uint32 expected;
	uint32 numOfFreeFramesBefore, numOfFreeFramesAfter;

	ClearUserSpace(proc_directory);

	/*================================================*/
	/*PART I: ALL pages and tables are not exist 50%  */
	/*================================================*/
	cprintf("\nCASE I: ALL pages and tables are not exist [50%]\n") ;
	{
		//Test1
		res = calculate_required_frames(proc_directory, 0x0, 8*kilo);
		if (res != 3)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 3);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test2
		res = calculate_required_frames(proc_directory, 0x0, 4*mega);
		if (res != 1025)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 1025);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test3
		res = calculate_required_frames(proc_directory, 0x0, 1024*mega);
		if (res != ((1024*mega)/(4*mega) + (1024*mega)/(4*kilo)))
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, ((1024*mega)/(4*mega) + (1024*mega)/(4*kilo)));
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}

	{
		//Test4
		res = calculate_required_frames(proc_directory, 0x1000, 6*kilo);
		if (res != 3)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 3);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test5
		res = calculate_required_frames(proc_directory, 0x1800, 3*kilo);
		if (res != 3)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 3);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test6
		res = calculate_required_frames(proc_directory, 0x400000, 10*mega);
		if (res != 2563)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 2563);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}

	{
		//Test7
		res = calculate_required_frames(proc_directory, 0x700000, 2*mega);
		if (res != 514)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 514);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		//Test8
		res = calculate_required_frames(proc_directory, 0x3FFFFF, 1*kilo);
		if (res != 4)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 4);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("\nCASE I: END\n") ;

	/*==================================================*/
	/*PART II: SOME pages and/or tables are exist [50%] */
	/*==================================================*/
	cprintf("\nCASE II: SOME pages and/or tables are exist [50%]\n") ;
	{
		//Test1
		/*allocate page*/char c1[100] ;strcconcat(aup_cmd, " 0x0", c1); execute_command(c1);

		res = calculate_required_frames(proc_directory, 0x0, 8*kilo);
		expected = 1 ;
		if (res != expected)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, expected);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		/*allocate page*/char c2[100] ;strcconcat(aup_cmd, " 0x100000", c2); execute_command(c2);
		/*allocate page*/char c3[100] ;strcconcat(aup_cmd, " 0x10000000", c3); execute_command(c3);

		//Test2
		res = calculate_required_frames(proc_directory, 0x0, 8*mega);
		expected = 2047;
		if (res != expected)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, expected);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test3
		res = calculate_required_frames(proc_directory, 0x0, 1024*mega);
		expected = ((1024*mega)/(4*mega) + (1024*mega)/(4*kilo)) - 2 - 1 - 2;
		if (res != expected)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, expected);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}

	{
		/*allocate page*/char c3[100] ;strcconcat(aup_cmd, " 0x2000", c3); execute_command(c3);

		//Test4
		res = calculate_required_frames(proc_directory, 0x1800, 3*kilo);
		expected = 1 ;
		if (res != expected)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, expected);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		/*allocate page*/char c4[100] ;strcconcat(aup_cmd, " 0x800000", c4); execute_command(c4);

		//Test5
		res = calculate_required_frames(proc_directory, 0x400000, 10*mega);
		if (res != 2561)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, 2563);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}

	{
		/*allocate page*/char c3[100] ;strcconcat(aup_cmd, " 0x801000", c3); execute_command(c3);
		/*allocate page*/char c4[100] ;strcconcat(aup_cmd, " 0x810000", c4); execute_command(c4);

		//Test7
		res = calculate_required_frames(proc_directory, 0x700000, 2*mega);
		expected = 510 ;
		if (res != expected)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, expected);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		//Test8
		res = calculate_required_frames(proc_directory, 0x3FFFFF, 1*kilo);
		expected = 3 ;
		if (res != expected)
		{
			warn("[EVAL] calculate_required_frames: Failed (count is not correct). res=%d, expected=%d\n", res, expected);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("\nCASE II: END\n") ;

	cprintf("[EVAL] calculate_required_frames: FINISHED. Evaluation = %d\n", eval);
	if(eval == 100)
		cprintf("Congratulations!! test calculate_required_frames completed successfully.\n");

	//return back to the kernel directory
	lcr3(phys_page_directory) ;

	return 0;
}

int test_calculate_allocated_space()
{
	//Create a Temp. User Process
	char prog_name[50] = "fos_helloWorld";
	struct Env* env = env_create(prog_name, 20, 10, 0);
	uint32 *proc_directory = env->env_page_directory ;
	lcr3(env->env_cr3) ;
	char aup[20] = "aup " ;
	char env_id[20] ; ltostr(env->env_id, env_id) ;
	char aup_cmd[50];
	strcconcat(aup, env_id, aup_cmd);
	//===================================================

	char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;
	char tptr[10] ;
	int kilo = 1024 ;
	int mega = 1024*1024 ;
	uint32 eval = 0;
	uint8 correct = 1;
	uint32 expected_num_pages;
	uint32 expected_num_tables;
	uint32 numOfFreeFramesBefore, numOfFreeFramesAfter;
	uint32 num_pages = 0;
	uint32 num_tables = 0;
	ClearUserSpace(proc_directory);

	/*================================================*/
	/*PART I: ALL pages and tables are not exist 50%  */
	/*================================================*/
	cprintf("\nCASE I: ALL pages and tables are not exist [50%]\n") ;
	{
		//Test1
		calculate_allocated_space(proc_directory, 0x0, 0x0+8*kilo, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test2
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x0, 0x0+4*mega, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test3
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x0, 0x0+1024*mega, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}
	{
		//Test4
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x1000, 0x1000+6*kilo, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test5
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x1800, 0x1800+3*kilo, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test6
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x400000, 0x400000+10*mega, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}
	{
		//Test7
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x700000, 0x700000+2*mega, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		//Test8
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x3FFFFF, 0x3FFFFF+1*kilo, &num_tables, &num_pages);
		if (num_tables != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, 0);
			correct = 0;
		}
		if (num_pages != 0)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, 0);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("\nCASE I: END\n") ;

	/*==================================================*/
	/*PART II: SOME pages and/or tables are exist [50%] */
	/*==================================================*/
	cprintf("\nCASE II: SOME pages and/or tables are exist [50%]\n") ;
	{
		//Test1
		/*allocate page*/char c1[100] ;strcconcat(aup_cmd, " 0x0", c1); execute_command(c1);

		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x0, 0x0+8*kilo, &num_tables, &num_pages);
		expected_num_tables = 1 ;
		expected_num_pages = 1 ;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		/*allocate page*/char c2[100] ;strcconcat(aup_cmd, " 0x100000", c2); execute_command(c2);
		/*allocate page*/char c3[100] ;strcconcat(aup_cmd, " 0x10000000", c3); execute_command(c3);

		//Test2
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x0, 0x0+8*mega, &num_tables, &num_pages);
		expected_num_tables = 1 ;
		expected_num_pages = 2 ;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		//Test3
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x0, 0x0+1024*mega, &num_tables, &num_pages);
		expected_num_tables = 1 + 1;
		expected_num_pages = 1 + 1 + 1;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	{
		/*allocate page*/char c3[100] ;strcconcat(aup_cmd, " 0x2000", c3); execute_command(c3);

		//Test4
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x1800, 0x1800+3*kilo, &num_tables, &num_pages);
		expected_num_tables = 1;
		expected_num_pages = 1;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;

		/*allocate page*/char c4[100] ;strcconcat(aup_cmd, " 0x800000", c4); execute_command(c4);

		//Test5
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x400000, 0x400000+10*mega, &num_tables, &num_pages);
		expected_num_tables = 1;
		expected_num_pages = 1;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 5 ;
		correct = 1 ;
	}
	{
		/*allocate page*/char c3[100] ;strcconcat(aup_cmd, " 0x801000", c3); execute_command(c3);
		/*allocate page*/char c4[100] ;strcconcat(aup_cmd, " 0x810000", c4); execute_command(c4);

		//Test6
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x700000, 0x700000+2*mega, &num_tables, &num_pages);
		expected_num_tables = 1;
		expected_num_pages = 3;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;

		//Test7
		num_tables = 0;
		num_pages = 0;
		calculate_allocated_space(proc_directory, 0x3FFFFF, 0x3FFFFF+1*kilo, &num_tables, &num_pages);
		expected_num_tables = 1;
		expected_num_pages = 0;
		if (num_tables != expected_num_tables)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Tables count is not correct). res=%d, expected=%d\n", num_tables, expected_num_tables);
			correct = 0;
		}
		if (num_pages != expected_num_pages)
		{
			warn("[EVAL] calculate_allocated_space: Failed (Pages count is not correct). res=%d, expected=%d\n", num_pages, expected_num_pages);
			correct = 0;
		}
		if (correct) eval += 10 ;
		correct = 1 ;
	}
	cprintf("\nCASE II: END\n") ;

	cprintf("[EVAL] calculate_allocated_space: FINISHED. Evaluation = %d\n", eval);
	if(eval == 100)
		cprintf("Congratulations!! test calculate_allocated_space completed successfully.\n");

	//return back to the kernel directory
	lcr3(phys_page_directory) ;

	return 0;
}

//===========================================================================
//===========================================================================
//===========================================================================

int CB(uint32 *ptr_dir, uint32 va, int bn)
{
	//assert(USE_KHEAP == 0) ;
	uint32 mask = 1<<bn;
	if (!(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & 1)) return 0;
	uint32 *table = (STATIC_KERNEL_VIRTUAL_ADDRESS(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & ~0xFFF));
	return ((table[((((uint32) (va)) >> 12) & 0x3FF)]&mask) == mask)? 1 : 0 ;
}
int SB(uint32 *ptr_dir, uint32 va, int bn , int v)
{
	assert(USE_KHEAP == 0) ;
	uint32 mask = 1<<bn;
	if (!(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & 1)) return 0;
	uint32 *table = (STATIC_KERNEL_VIRTUAL_ADDRESS(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & ~0xFFF));
	if (~v) table[((((uint32) (va)) >> 12) & 0x3FF)] &= ~mask ;
	else 	table[((((uint32) (va)) >> 12) & 0x3FF)] |= mask ;
	return 0;
}
int CPs(uint32 *ptr_dir, uint32 va, uint32 perms, uint32 which)
{
	assert(USE_KHEAP == 0) ;
	if (!(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & 1)) return 0;
	uint32 *table = (STATIC_KERNEL_VIRTUAL_ADDRESS(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & ~0xFFF));
	for (int i = 0 ; i < 12 ; i++)
	{
		uint32 mask = 1<<i;
		if (!(which & mask))	continue ;
		uint8 c = (table[((((uint32) (va)) >> 12) & 0x3FF)] & mask) == (perms & mask) ? 1 :  0;
		if (!c) return 0;
	}
	return 1;
}

int CA(uint32 *ptr_dir, uint32 va)
{
	assert(USE_KHEAP == 0) ;
	if (!(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & 1)) return 0;
	uint32 *table = (STATIC_KERNEL_VIRTUAL_ADDRESS(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & ~0xFFF));
	return table[((((uint32) (va)) >> 12) & 0x3FF)]&~0x00000FFF;
}

int CE(uint32 *_d, uint32 va)
{
	if (!(_d[((((uint32) (va)) >> 22) & 0x3FF)] & 1)) return 0;
	uint32 *_t = (STATIC_KERNEL_VIRTUAL_ADDRESS(_d[((((uint32) (va)) >> 22) & 0x3FF)] & ~0xFFF));
	if ((_t[((((uint32) (va)) >> 12) & 0x3FF)])!=0) return 0;
	return 1;
}

int CP(uint32* pd, uint32 va, uint32 ps, uint32 pc)
{
	assert(USE_KHEAP == 0) ;
	uint32 pd_entry = pd[((((uint32) (va)) >> 22) & 0x3FF)];
	if ( (pd_entry & 1) == 1)
	{
		uint32 *t = NULL;
		t = (STATIC_KERNEL_VIRTUAL_ADDRESS(EXTRACT_ADDRESS(pd_entry)));
		//cprintf("va =%x, ENTRY after PERM = %x, perm to set = %x, perm to clear = %x\n", va, t[PTX(va)]&0x00000FFF, ps, pc);

		if (((t[((((uint32) (va)) >> 12) & 0x3FF)]&ps) == ps)&&((~(t[((((uint32) (va)) >> 12) & 0x3FF)])&pc) == pc))
			return 1;
	}
	return 0;
}

uint32 GP(uint32 *ptr_dir, uint32 va)
{
	assert(USE_KHEAP == 0) ;
	if (!(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & 1)) return 0;
	uint32 *table = (STATIC_KERNEL_VIRTUAL_ADDRESS(ptr_dir[((((uint32) (va)) >> 22) & 0x3FF)] & ~0xFFF));
	return table[((((uint32) (va)) >> 12) & 0x3FF)] & 0xFFF;
}

void ClearUserSpace(uint32 *ptr_dir)
{
	for (int i = 0; i < PDX(USER_TOP); ++i) {
		ptr_dir[i] = 0;
	}
}

int CCP(uint32 *ptr_dir, uint32 ptr1, uint32 ptr2, uint32 size, int ref, uint32 dst_perms, uint32 dst_to_chk, uint32 src_perms, uint32 src_to_chk, uint8 chk_type)
{
	void* ptrTemp1 = (void*)(ptr1 - ptr1 % (4096));
	void* ptrTemp2 = (void*)(ptr2 - ptr2 % (4096));

	for( ;ptrTemp2 < (void*)(ptr2+size); ptrTemp2+=PAGE_SIZE)
	{
		uint32* ptr_table1;
		uint32* ptr_table2;
		struct FrameInfo * pfi1 ;
		struct FrameInfo * pfi2 ;
		if (chk_type != CHK_ALLOC)
		{
			pfi1 = get_frame_info(ptr_dir, (uint32)ptrTemp1, &ptr_table1);
			if (ptr_table1 == NULL)
			{
				warn("[EVAL] Failed. Table of address 1 = NULL\n");
				return 0;
			}
		}
		pfi2 = get_frame_info(ptr_dir, (uint32)ptrTemp2, &ptr_table2);
		if (ptr_table2 == NULL)
		{
			warn("[EVAL] Failed. Table of address 2 = NULL\n");
			return 0;
		}
		if (chk_type == CHK_SHARE)
		{
			uint32 fn1 = ptr_table1[PTX(ptrTemp1)] >> 12 ;
			uint32 fn2 = ptr_table2[PTX(ptrTemp2)] >> 12 ;

			if(fn1 != fn2)
			{
				warn("[EVAL] Failed. Frame numbers not equal in the whole range!\nva1=%x, va2=%x, fn1=%x, fn2=%x\n", ptrTemp1, ptrTemp2, fn1, fn2);
				return 0;
			}
		}
		if (ref != -1)
		{
			if (pfi2 == NULL || (*pfi2).references != ref)
			{
				warn("[EVAL] Failed. Num of frame references is not correct. MAKE SURE to use the functions of LAB5! va2=%x, ref2=%d\n", ptrTemp2, pfi2==NULL? 0 : (*pfi2).references);
				return 0;
			}
		}
		if (CPs(ptr_dir, (uint32)ptrTemp2, dst_perms, dst_to_chk) <= 0)
		{
			warn("[EVAL] Failed. one or more permission in destination is not correct\n");
			return 0;
		}
		if (chk_type != CHK_ALLOC)
		{
			if (CPs(ptr_dir, (uint32)ptrTemp1, src_perms, src_to_chk) <= 0)
			{
				warn("[EVAL] Failed. one or more permission in source is not correct\n");
				return 0;
			}
		}
		if (chk_type != CHK_ALLOC)
		{
			ptrTemp1 += PAGE_SIZE;
		}
	}

	return 1;
}
