/*
 * test_working_set.c
 *
 *  Created on: Oct 13, 2023
 *      Author: HP
 */

#include <kern/tests/test_working_set.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/working_set_manager.h>

//2020
int sys_check_LRU_lists(uint32* active_list_content, uint32* second_list_content, int actual_active_list_size, int actual_second_list_size)
{
	struct Env* env = curenv;
	int active_list_validation = 1;
	int second_list_validation = 1;
	struct WorkingSetElement* ptr_WS_element;

	//1- Check active list content if not null
	if(active_list_content != NULL)
	{
		int idx_active_list = 0;
		LIST_FOREACH(ptr_WS_element, &(env->ActiveList))
		{
			if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) != ROUNDDOWN(active_list_content[idx_active_list], PAGE_SIZE))
			{
				active_list_validation = 0;
				break;
			}
			idx_active_list++;
		}
		if(LIST_SIZE(&env->ActiveList) != actual_active_list_size)
		{
			active_list_validation = 0;

		}
	}

	//2- Check second chance list content if not null
	if(second_list_content != NULL)
	{
		int idx_second_list = 0;
		LIST_FOREACH(ptr_WS_element, &(env->SecondList))
		{
			if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) != ROUNDDOWN(second_list_content[idx_second_list], PAGE_SIZE))
			{
				second_list_validation = 0;
				break;
			}
			idx_second_list++;
		}
		if(LIST_SIZE(&env->SecondList) != actual_second_list_size)
			second_list_validation = 0;
	}
	return active_list_validation&second_list_validation;
}


//2020
int sys_check_LRU_lists_free(uint32* list_content, int list_size)
{
	struct Env* env = curenv;
	int list_validation_count = 0;
	struct WorkingSetElement* ptr_WS_element;

	LIST_FOREACH(ptr_WS_element, &(env->ActiveList))
	{
		for(int var = 0; var < list_size; var++)
		{
			if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) == ROUNDDOWN(list_content[var], PAGE_SIZE))
			{
				list_validation_count++;
				break;
			}
		}
		if(list_validation_count > 0)
			return list_validation_count;
	}


	LIST_FOREACH(ptr_WS_element, &(env->SecondList))
	{
		for(int var = 0; var < list_size; var++)
		{
			if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) == ROUNDDOWN(list_content[var], PAGE_SIZE))
			{
				list_validation_count++;
				break;
			}
		}
		if(list_validation_count > 0)
			return list_validation_count;

	}


	return list_validation_count;
}

//2023
/*chk_status:
 * = 0: check entire list (order is not important)
 * = 1: check entire list (order is important)
 * = 2: check only the existence of the given set of elements
 * = 3: check only the NOT existence of the given set of elements
 */
int sys_check_WS_list(uint32* WS_list_content, int actual_WS_list_size, uint32 last_WS_element_content, bool chk_status)
{
#if USE_KHEAP
	cprintf("CURRENT WS CONTENT BEFORE CHECKING:\n");
	env_page_ws_print(curenv);
	struct Env* env = curenv;
	int WS_list_validation = 1;
	struct WorkingSetElement* ptr_WS_element;

	if (chk_status == 0 || chk_status == 1)
	{
		if(LIST_SIZE(&(env->page_WS_list)) != actual_WS_list_size)
		{
			return WS_list_validation = 0;
		}
	}
	//if it's required to check the last_WS_element
	if (last_WS_element_content != 0)
	{
		if (ROUNDDOWN(env->page_last_WS_element->virtual_address, PAGE_SIZE) != ROUNDDOWN(last_WS_element_content, PAGE_SIZE))
		{
			return WS_list_validation = 0;
		}
	}
	//if the order of the content is important to check
	if (chk_status == 1)
	{
		int idx_WS_list = 0;
		LIST_FOREACH(ptr_WS_element, &(env->page_WS_list))
		{
			if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) != ROUNDDOWN(WS_list_content[idx_WS_list], PAGE_SIZE))
			{
				WS_list_validation = 0;
				break;
			}
			idx_WS_list++;
		}
	}
	else if (chk_status == 0 || chk_status == 2)
	{
		for (int idx_expected_list = 0; idx_expected_list < actual_WS_list_size; ++idx_expected_list)
		{
			bool found = 0;
			LIST_FOREACH(ptr_WS_element, &(env->page_WS_list))
			{
				if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) == ROUNDDOWN(WS_list_content[idx_expected_list], PAGE_SIZE))
				{
					found = 1;
					break;
				}
			}
			if (!found)
			{
				WS_list_validation = 0;
				break;
			}
		}
	}
	//Check NON-EXITENCE of the Given Addresses
	else if (chk_status == 3)
	{
		for (int idx_expected_list = 0; idx_expected_list < actual_WS_list_size; ++idx_expected_list)
		{
			bool found = 0;
			LIST_FOREACH(ptr_WS_element, &(env->page_WS_list))
			{
				if (ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE) == ROUNDDOWN(WS_list_content[idx_expected_list], PAGE_SIZE))
				{
					found = 1;
					break;
				}
			}
			if (found)
			{
				WS_list_validation = 0;
				break;
			}
		}
	}

	return WS_list_validation;
#else
	panic("sys_check_WS_list: this function is intended to be used when USE_KHEAP = 1");
	return 0;
#endif
}
