/*
 * boot_time_memory.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/proc/user_environment.h>
#include <kern/disk/pagefile_manager.h>
#include "memory_manager.h"
// Global descriptor table.
//
// The kernel and user segments are identical(except for the DPL).
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
struct Segdesc gdt[] =
{
		// 0x0 - unused (always faults -- for trapping NULL far pointers)
		SEG_NULL,

		// 0x8 - kernel code segment
		[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

		// 0x10 - kernel data segment
		[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

		// 0x18 - user code segment
		[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

		// 0x20 - user data segment
		[GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

		// 0x28 - tss, initialized in idt_init()
		[GD_TSS >> 3] = SEG_NULL
};

struct Pseudodesc gdt_pd =
{
		sizeof(gdt) - 1, (unsigned long) gdt
};


///**************************** MAPPING KERNEL SPACE *******************************

// Set up a two-level page table:
//    ptr_page_directory is the virtual address of the page directory
//    phys_page_directory is the physical adresss of the page directory
// Then turn on paging.  Then effectively turn off segmentation.
// (i.e., the segment base addrs are set to zero).
//
// This function only sets up the kernel part of the address space
// (ie. addresses >= USER_TOP).  The user part of the address space
// will be setup later.
//
// From USER_TOP to USER_LIMIT, the user is allowed to read but not write.
// Above USER_LIMIT the user cannot read (or write).

void initialize_kernel_VM()
{
	// Remove this line when you're ready to test this function.
	//panic("initialize_kernel_VM: This function is not finished\n");

	//////////////////////////////////////////////////////////////////////
	// create initial page directory.

	ptr_page_directory = boot_allocate_space(PAGE_SIZE, PAGE_SIZE);
	/*2023: this line is moved to the boot_allocate_space()*/ //memset(ptr_page_directory, 0, PAGE_SIZE);
	phys_page_directory = STATIC_KERNEL_PHYSICAL_ADDRESS(ptr_page_directory);

	//////////////////////////////////////////////////////////////////////
	// Map the kernel stack with VA range :
	//  [KERNEL_STACK_TOP-KERNEL_STACK_SIZE, KERNEL_STACK_TOP),
	// to physical address : "phys_stack_bottom".
	//     Permissions: kernel RW, user NONE
	// Your code goes here:
	boot_map_range(ptr_page_directory, KERNEL_STACK_TOP - KERNEL_STACK_SIZE, KERNEL_STACK_SIZE, STATIC_KERNEL_PHYSICAL_ADDRESS(ptr_stack_bottom), PERM_WRITEABLE) ;

	//////////////////////////////////////////////////////////////////////
	// Map all of physical memory at KERNEL_BASE.
	// i.e.  the VA range [KERNEL_BASE, 2^32) should map to
	//      the PA range [0, 2^32 - KERNEL_BASE)
	// We might not have 2^32 - KERNEL_BASE bytes of physical memory, but
	// we just set up the mapping anyway.
	// Permissions: kernel RW, user NONE
	// Your code goes here:

	//2016:
	//boot tables
	unsigned long long sva = KERNEL_BASE;
	unsigned int nTables=0;
	for (;sva < 0xFFFFFFFF;  sva += PTSIZE)
	{
		++nTables;
		boot_get_page_table(ptr_page_directory, (uint32)sva, 1);
	}
	//cprintf("nTables = %d\n", nTables);

	//////////////////////////////////////////////////////////////////////
	// Make 'frames_info' point to an array of size 'number_of_frames' of 'struct Frame_Info'.
	// The kernel uses this structure to keep track of physical frames;
	// 'number_of_frames' equals the number of physical frames in memory.  User-level
	// programs get read-only access to the array as well.
	// You must allocate the array yourself.
	// ************************************************************************************
	// /*2016: READ_ONLY_FRAMES_INFO not valid any more since it can't fit in 4 MB space*/
	//	Map this array read-only by the user at virtual address READ_ONLY_FRAMES_INFO
	// (ie. perm = PERM_USER | PERM_PRESENT)
	// ************************************************************************************
	// Permissions:
	//    - frames_info -- kernel RW, user NONE
	//    - the image mapped at READ_ONLY_FRAMES_INFO  -- kernel R, user R
	// Your code goes here:
	//cprintf("size of WorkingSetPage = %d\n",sizeof(struct WorkingSetPage));
	uint32 array_size;
	array_size = number_of_frames * sizeof(struct FrameInfo) ;
	frames_info = boot_allocate_space(array_size, PAGE_SIZE);
	/*2023: this line is moved to the boot_allocate_space()*/ //memset(frames_info, 0, array_size);

	//2016: Not valid any more since the RAM size exceed the 64 MB limit. This lead to the
	// 		size of "frames_info" can exceed the 4 MB space for "READ_ONLY_FRAMES_INFO"
	//boot_map_range(ptr_page_directory, READ_ONLY_FRAMES_INFO, array_size, STATIC_KERNEL_PHYSICAL_ADDRESS(frames_info),PERM_USER) ;


	uint32 disk_array_size = PAGES_PER_FILE * sizeof(struct FrameInfo);
	disk_frames_info = boot_allocate_space(disk_array_size , PAGE_SIZE);
	/*2023: this line is moved to the boot_allocate_space()*/ //memset(disk_frames_info , 0, disk_array_size);

	// This allows the kernel & user to access any page table entry using a
	// specified VA for each: VPT for kernel and UVPT for User.
	setup_listing_to_all_page_tables_entries();

	//////////////////////////////////////////////////////////////////////
	// Make 'envs' point to an array of size 'NENV' of 'struct Env'.
	// Map this array read-only by the user at linear address UENVS
	// (ie. perm = PTE_U | PTE_P).
	// Permissions:
	//    - envs itself -- kernel RW, user NONE
	//    - the image of envs mapped at UENVS  -- kernel R, user R

	// LAB 3: Your code here.
	cprintf("Max Envs = %d, Nearest Pow of 2 = %d\n",NENV, NEARPOW2NENV);
	int envs_size = NENV * sizeof(struct Env) ;

	//allocate space for "envs" array aligned on 4KB boundary
	envs = boot_allocate_space(envs_size, PAGE_SIZE);
	/*2023: this line is moved to the boot_allocate_space()*/ //memset(envs , 0, envs_size);

	//make the user to access this array by mapping it to UPAGES linear address (UPAGES is in User/Kernel space)
	boot_map_range(ptr_page_directory, UENVS, envs_size, STATIC_KERNEL_PHYSICAL_ADDRESS(envs), PERM_USER) ;

	//update permissions of the corresponding entry in page directory to make it USER with PERMISSION read only
	ptr_page_directory[PDX(UENVS)] = ptr_page_directory[PDX(UENVS)]|(PERM_USER|(PERM_PRESENT & (~PERM_WRITEABLE)));


#if USE_KHEAP
	{
		// MAKE SURE THAT THIS MAPPING HAPPENS AFTER ALL BOOT ALLOCATIONS (boot_allocate_space)
		// calls are fininshed, and no remaining data to be allocated for the kernel
		// map all used pages so far for the kernel
		boot_map_range(ptr_page_directory, KERNEL_BASE, (uint32)ptr_free_mem - KERNEL_BASE, 0, PERM_WRITEABLE) ;
	}
#else
	{
		boot_map_range(ptr_page_directory, KERNEL_BASE, 0xFFFFFFFF - KERNEL_BASE, 0, PERM_WRITEABLE) ;
	}
#endif
	// Check that the initial page directory has been set up correctly.
	check_boot_pgdir();

	memory_scarce_threshold_percentage = DEFAULT_MEM_SCARCE_PERCENTAGE;	// Memory remains plentiful till % of free frames gets below 25% of the memory space

	/*
	NOW: Turn off the segmentation by setting the segments' base to 0, and
	turn on the paging by setting the corresponding flags in control register 0 (cr0)
	 */
	turn_on_paging() ;
}

//
// Allocate "size" bytes of physical memory aligned on an
// "align"-byte boundary.  Align must be a power of two.
// Return the start kernel virtual address of the allocated space.
// Returned memory is uninitialized.
//
// If we're out of memory, boot_allocate_space should panic.
// It's too early to run out of memory.
// This function may ONLY be used during boot time,
// before the free_frame_list has been set up.
//
void* boot_allocate_space(uint32 size, uint32 align)
{
	extern char end_of_kernel[];

	// Initialize ptr_free_mem if this is the first time.
	// 'end_of_kernel' is a symbol automatically generated by the linker,
	// which points to the end of the kernel-
	// i.e., the first virtual address that the linker
	// did not assign to any kernel code or global variables.
	if (ptr_free_mem == 0)
		ptr_free_mem = end_of_kernel;

	// Your code here:
	//	Step 1: round ptr_free_mem up to be aligned properly
	ptr_free_mem = ROUNDUP(ptr_free_mem, align) ;

	//	Step 2: save current value of ptr_free_mem as allocated space
	void *ptr_allocated_mem;
	ptr_allocated_mem = ptr_free_mem ;

	//	Step 3: increase ptr_free_mem to record allocation
	ptr_free_mem += size ;

	//// 2016: Step 3.5: initialize allocated space by ZEROOOOOOOOOOOOOO
	/*2023*/ /*THIS LINE IS UNCOMMENTED To Ensure that any boot allocations ARE SET TO ZERO
	 * This is mainly to ensure that any restart will be fresh and no grabage data will be exist
	 */
	memset(ptr_allocated_mem, 0, size);

	//	Step 4: return allocated space
	return ptr_allocated_mem ;

}


//
// Map [virtual_address, virtual_address+size) of virtual address space to
// physical [physical_address, physical_address+size)
// in the page table rooted at ptr_page_directory.
// "size" is a multiple of PAGE_SIZE.
// Use permission bits perm|PERM_PRESENT for the entries.
//
// This function may ONLY be used during boot time,
// before the free_frame_list has been set up.
//
void boot_map_range(uint32 *ptr_page_directory, uint32 virtual_address, uint32 size, uint32 physical_address, int perm)
{
	int i = 0 ;
	//physical_address = ROUNDUP(physical_address, PAGE_SIZE) ;
	///we assume here that all addresses are given divisible by 4 KB, look at boot_allocate_space ...

	for (i = 0 ; i < size ; i += PAGE_SIZE)
	{
		uint32 *ptr_page_table = boot_get_page_table(ptr_page_directory, virtual_address, 1) ;
		uint32 index_page_table = PTX(virtual_address);
		//LOG_VARS("\nCONSTRUCT_ENTRY = %x",physical_address);
		ptr_page_table[index_page_table] = CONSTRUCT_ENTRY(physical_address, perm | PERM_PRESENT) ;

		physical_address += PAGE_SIZE ;
		virtual_address += PAGE_SIZE ;
	}
}

//
// Given ptr_page_directory, a pointer to a page directory,
// traverse the 2-level page table structure to find
// the page table for "virtual_address".
// Return a pointer to the table.
//
// If the relevant page table doesn't exist in the page directory:
//	- If create == 0, return 0.
//	- Otherwise allocate a new page table, install it into ptr_page_directory,
//	  and return a pointer into it.
//        (Questions: What data should the new page table contain?
//	  And what permissions should the new ptr_page_directory entry have?)
//
// This function allocates new page tables as needed.
//
// boot_get_page_table cannot fail.  It's too early to fail.
// This function may ONLY be used during boot time,
// before the free_frame_list has been set up.
//
uint32* boot_get_page_table(uint32 *ptr_page_directory, uint32 virtual_address, int create)
{
	uint32 index_page_directory = PDX(virtual_address);
	uint32 page_directory_entry = ptr_page_directory[index_page_directory];

	//cprintf("boot d ind = %d, entry = %x\n",index_page_directory, page_directory_entry);
	uint32 phys_page_table = EXTRACT_ADDRESS(page_directory_entry);
	uint32 *ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(phys_page_table);
	if (phys_page_table == 0)
	{
		if (create)
		{
			ptr_page_table = boot_allocate_space(PAGE_SIZE, PAGE_SIZE) ;
			phys_page_table = STATIC_KERNEL_PHYSICAL_ADDRESS(ptr_page_table);
			ptr_page_directory[index_page_directory] = CONSTRUCT_ENTRY(phys_page_table, PERM_PRESENT | PERM_WRITEABLE);
			return ptr_page_table ;
		}
		else
			return 0 ;
	}
	return ptr_page_table ;
}


int nvram_read(int r)
{
	return mc146818_read(r) | (mc146818_read(r + 1) << 8);
}

void detect_memory()
{
	uint32 maxpa;	// Maximum physical address
	uint32 size_of_base_mem;		// Amount of base memory (in bytes)
	uint32 size_of_extended_mem;		// Amount of extended memory (in bytes)

	// CMOS tells us how many kilobytes there are
	size_of_base_mem = ROUNDDOWN(nvram_read(NVRAM_BASELO)*1024, PAGE_SIZE);
	size_of_extended_mem = ROUNDDOWN(nvram_read(NVRAM_EXTLO)*1024, PAGE_SIZE);

	//2016
	//For physical memory larger than 16MB, we needed to read total memory size
	// from a different register of the MC chip, see here:
	// http://bochs.sourceforge.net/techspec/CMOS-reference.txt
	// "CMOS 34h - AMI -"
	uint32 size_of_other_mem = ROUNDDOWN(nvram_read(0x34)*1024*64, PAGE_SIZE);
	//cprintf("other mem = %dK\n", size_of_other_mem/1024);

	// Calculate the maximum physical address based on whether
	// or not there is any extended memory.  See comment in ../inc/mmu.h.
	//2016
	if(size_of_other_mem > 0)
	{
		maxpa = size_of_other_mem + 16*1024*1024;
		size_of_extended_mem = maxpa - PHYS_EXTENDED_MEM;
	}
	else
	{
		if (size_of_extended_mem)
			maxpa = PHYS_EXTENDED_MEM + size_of_extended_mem;
		else
			maxpa = size_of_extended_mem;
	}

	uint32 kernel_virtual_area = ((0xFFFFFFFF-KERNEL_BASE)+1);
	if(USE_KHEAP == 0 && maxpa > kernel_virtual_area)
	{
		cprintf("Error!: Physical memory = %dK larger than kernel virtual area (%dK)\n", maxpa/1024, kernel_virtual_area/1024);
		cprintf("Cannot use physical memory larger than kernel virtual area\nTo enable physical memory larger than virtual kernel area, set USE_KHEAP = 1 in FOS code");
		while(1);
	}
	number_of_frames = maxpa / PAGE_SIZE;

	cprintf("Physical memory: %dK available, ", (int)(maxpa/1024));
	cprintf("base = %dK, extended = %dK\n", (int)(size_of_base_mem/1024), (int)(size_of_extended_mem/1024));
}
// --------------------------------------------------------------
// Set up initial memory mappings and turn on MMU.
// --------------------------------------------------------------

void turn_on_paging()
{
	//////////////////////////////////////////////////////////////////////
	// On x86, segmentation maps a VA to a LA (linear addr) and
	// paging maps the LA to a PA.  I.e. VA => LA => PA.  If paging is
	// turned off the LA is used as the PA.  Note: there is no way to
	// turn off segmentation.  The closest thing is to set the base
	// address to 0, so the VA => LA mapping is the identity.

	// Current mapping: VA (KERNEL_BASE+x) => PA (x).
	//     (segmentation base = -KERNEL_BASE and paging is off)

	// From here on down we must maintain this VA (KERNEL_BASE + x) => PA (x)
	// mapping, even though we are turning on paging and reconfiguring
	// segmentation.

	// Map VA 0:4MB same as VA (KERNEL_BASE), i.e. to PA 0:4MB.
	// (Limits our kernel to <4MB)

	//2016
	//ptr_page_directory[0] = ptr_page_directory[PDX(KERNEL_BASE)];
	{
		int i = PDX(KERNEL_BASE);
		int j = 0;
		for(; i< PDX((uint32)ptr_free_mem); ++i, ++j)
		{
			ptr_page_directory[j] = ptr_page_directory[i];
		}
	}

	// Install page table.
	lcr3(phys_page_directory);

	// Turn on paging.
	uint32 cr0;
	cr0 = rcr0();
	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP;
	cr0 &= ~(CR0_TS|CR0_EM);
	lcr0(cr0);

	// Current mapping: KERNEL_BASE+x => x => x.
	// (x < 4MB so uses paging ptr_page_directory[0])

	// Reload all segment registers.
	asm volatile("lgdt gdt_pd");
	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));  // reload cs
	asm volatile("lldt %%ax" :: "a" (0));

	// Final mapping: KERNEL_BASE + x => KERNEL_BASE + x => x.

	// This mapping was only used after paging was turned on but
	// before the segment registers were reloaded.
	//2016
	//ptr_page_directory[0] = 0;
	{
		int i = PDX(KERNEL_BASE);
		int j = 0;
		for(; i< PDX((uint32)ptr_free_mem); ++i, ++j)
		{
			ptr_page_directory[j] = 0;
		}
	}
	// Flush the TLB for good measure, to kill the ptr_page_directory[0] mapping.
	lcr3(phys_page_directory);
}

void setup_listing_to_all_page_tables_entries()
{
	//////////////////////////////////////////////////////////////////////
	// Recursively insert PD in itself as a page table, to form
	// a virtual page table at virtual address VPT.

	// Permissions: kernel RW, user NONE
	uint32 phys_frame_address = STATIC_KERNEL_PHYSICAL_ADDRESS(ptr_page_directory);
	ptr_page_directory[PDX(VPT)] = CONSTRUCT_ENTRY(phys_frame_address , PERM_PRESENT | PERM_WRITEABLE);

	// same for UVPT
	//Permissions: kernel R, user R
	ptr_page_directory[PDX(UVPT)] = STATIC_KERNEL_PHYSICAL_ADDRESS(ptr_page_directory)|PERM_USER|PERM_PRESENT;

}

///******************************* END of MAPPING KERNEL SPACE *******************************

