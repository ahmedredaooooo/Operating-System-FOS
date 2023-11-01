/*
 * boot_time_memory.h
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#ifndef KERN_MEM_BOOT_MEMORY_MANAGER_H_
#define KERN_MEM_BOOT_MEMORY_MANAGER_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif


uint32 number_of_frames;	// Amount of physical memory (in frames_info)
uint32 size_of_base_mem;		// Amount of base memory (in bytes)
uint32 size_of_extended_mem;		// Amount of extended memory (in bytes)
extern char ptr_stack_top[], ptr_stack_bottom[];

// These variables are set in initialize_kernel_VM()
uint32* ptr_page_directory;		// Virtual address of boot time page directory
uint8* ptr_zero_page;		// Virtual address of zero page used by program loader to initialize extra segment zero memory (bss section) it to zero
uint8* ptr_temp_page;		// Virtual address of a page used by program loader to initialize segment last page fraction
uint32 phys_page_directory;		// Physical address of boot time page directory
char* ptr_free_mem;	// Pointer to next byte of free mem

struct FrameInfo* frames_info;		// Virtual address of physical frames_info array
struct FrameInfo* disk_frames_info;		// Virtual address of physical frames_info array
struct FrameInfo_List free_frame_list;	// Free list of physical frames_info
struct FrameInfo_List modified_frame_list;

//BOOT TIME [KERNEL SPACE]
void boot_map_range(uint32 *ptr_page_directory, uint32 virtual_address, uint32 size, uint32 physical_address, int perm);
uint32* boot_get_page_table(uint32 *ptr_page_directory, uint32 virtual_address, int create);
void* boot_allocate_space(uint32 size, uint32 align);
void initialize_kernel_VM();
void initialize_paging();
void	detect_memory();
void 	turn_on_paging();
void	setup_listing_to_all_page_tables_entries();


#endif /* KERN_MEM_BOOT_MEMORY_MANAGER_H_ */
