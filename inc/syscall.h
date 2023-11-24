#ifndef FOS_INC_SYSCALL_H
#define FOS_INC_SYSCALL_H

/* system call numbers */
enum
{
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_getenvindex,
	SYS_getparentenvid,
	SYS_allocate_page,
	SYS_map_frame,
	SYS_unmap_frame,
	SYS_calc_req_frames,
	SYS_calc_free_frames,
	SYS_calc_modified_frames,
	SYS_calc_notmod_frames,
	SYS_pf_calc_allocated_pages,
	SYS_calculate_pages_tobe_removed_ready_exit,
	SYS_scarce_memory,
	SYS_allocate_chunk_in_mem,
	SYS_move_user_mem,
	SYS_disableINTR,
	SYS_enableINTR,
	SYS_cputc,
	SYS_clearFFL,
	SYS_create_semaphore,
	SYS_wait_semaphore,
	SYS_signal_semaphore,
	SYS_get_semaphore_value,
	SYS_create_shared_object,
	SYS_get_size_of_shared_object,
	SYS_get_shared_object,
	SYS_free_shared_object,
	SYS_get_max_shares,
	SYS_create_env,
	SYS_run_env,
	SYS_destroy_env,
	SYS_exit_env,
	SYS_get_virtual_time,
	SYS_rcr2,
	SYS_bypassPageFault,
	SYS_testNum,
	SYS_rsttst,
	SYS_chktst,
	SYS_inctst,
	SYS_gettst,
	SYS_get_heap_strategy,
	SYS_set_heap_strategy,
	SYS_check_LRU_lists,
	SYS_check_LRU_lists_free,
	SYS_check_WS_list,
	/*2024*/
	//TODO: [PROJECT'23.MS1 - #3] [2] SYSTEM CALLS - Add suitable code here
	SYS_sbrk,
	SYS_free_user_mem,
	SYS_allocate_user_mem,
	//=====================================================================
	NSYSCALLS
};

#endif /* !FOS_INC_SYSCALL_H */
