/* See COPYRIGHT for copyright information. */
/*
KEY WORDS
==========
MACROS: 	STATIC_KERNEL_PHYSICAL_ADDRESS, STATIC_KERNEL_VIRTUAL_ADDRESS, PDX, PTX, CONSTRUCT_ENTRY, EXTRACT_ADDRESS, ROUNDUP, ROUNDDOWN, LIST_INIT, LIST_INSERT_HEAD, LIST_FIRST, LIST_REMOVE
CONSTANTS:	PAGE_SIZE, PERM_PRESENT, PERM_WRITEABLE, PERM_USER, KERNEL_STACK_TOP, KERNEL_STACK_SIZE, KERNEL_BASE, READ_ONLY_FRAMES_INFO, PHYS_IO_MEM, PHYS_EXTENDED_MEM, E_NO_MEM
VARIABLES:	ptr_free_mem, ptr_page_directory, phys_page_directory, phys_stack_bottom, Frame_Info, frames_info, free_frame_list, references, prev_next_info, size_of_extended_mem, number_of_frames, ptr_frame_info ,create, perm, va
FUNCTIONS:	to_physical_address, get_frame_info, tlb_invalidate
=====================================================================================================================================================================================================
 */

#include "memory_manager.h"

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/trap/trap.h>

#include <kern/proc/user_environment.h>
#include <kern/cpu/kclock.h>
#include <kern/cpu/sched.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"




void tlb_invalidate(uint32 *ptr_page_directory, void *virtual_address)
{
	// Flush the entry only if we're modifying the current address space.
	// For now, there is only one address space, so always invalidate.
	invlpg(virtual_address);
}

///******************************* MAPPING USER SPACE *******************************

// --------------------------------------------------------------
// Tracking of physical frames.
// The 'frames_info' array has one 'struct Frame_Info' entry per physical frame.
// frames_info are reference counted, and free frames are kept on a linked list.
// --------------------------------------------------------------

// Initialize paging structure and free_frame_list.
// After this point, ONLY use the functions below
// to allocate and deallocate physical memory via the free_frame_list,
// and NEVER use boot_allocate_space() or the related boot-time functions above.
//

extern void initialize_disk_page_file();
void initialize_paging()
{
	// The example code here marks all frames_info as free.
	// However this is not truly the case.  What memory is free?
	//  1) Mark frame 0 as in use.
	//     This way we preserve the real-mode IDT and BIOS structures
	//     in case we ever need them.  (Currently we don't, but...)
	//  2) Mark the rest of base memory as free.
	//  3) Then comes the IO hole [PHYS_IO_MEM, PHYS_EXTENDED_MEM).
	//     Mark it as in use so that it can never be allocated.
	//  4) Then extended memory [PHYS_EXTENDED_MEM, ...).
	//     Some of it is in use, some is free. Where is the kernel?
	//     Which frames are used for page tables and other data structures?
	//
	// Change the code to reflect this.
	int i;
	LIST_INIT(&free_frame_list);
	LIST_INIT(&modified_frame_list);

	frames_info[0].references = 1;
	frames_info[1].references = 1;
	frames_info[2].references = 1;
	ptr_zero_page = (uint8*) KERNEL_BASE+PAGE_SIZE;
	ptr_temp_page = (uint8*) KERNEL_BASE+2*PAGE_SIZE;
	i =0;
	for(;i<1024; i++)
	{
		ptr_zero_page[i]=0;
		ptr_temp_page[i]=0;
	}

	int range_end = ROUNDUP(PHYS_IO_MEM,PAGE_SIZE);

	for (i = 3; i < range_end/PAGE_SIZE; i++)
	{

		initialize_frame_info(&(frames_info[i]));
		//frames_info[i].references = 0;

		LIST_INSERT_HEAD(&free_frame_list, &frames_info[i]);
	}

	for (i = PHYS_IO_MEM/PAGE_SIZE ; i < PHYS_EXTENDED_MEM/PAGE_SIZE; i++)
	{
		frames_info[i].references = 1;
	}

	range_end = ROUNDUP(STATIC_KERNEL_PHYSICAL_ADDRESS(ptr_free_mem), PAGE_SIZE);

	for (i = PHYS_EXTENDED_MEM/PAGE_SIZE ; i < range_end/PAGE_SIZE; i++)
	{
		frames_info[i].references = 1;
	}

	for (i = range_end/PAGE_SIZE ; i < number_of_frames; i++)
	{
		initialize_frame_info(&(frames_info[i]));

		//frames_info[i].references = 0;
		LIST_INSERT_HEAD(&free_frame_list, &frames_info[i]);
	}

	initialize_disk_page_file();
}

//
// Initialize a Frame_Info structure.
// The result has null links and 0 references.
// Note that the corresponding physical frame is NOT initialized!
//
void initialize_frame_info(struct FrameInfo *ptr_frame_info)
{
	memset(ptr_frame_info, 0, sizeof(*ptr_frame_info));
}

//
// Allocates a physical frame.
// Does NOT set the contents of the physical frame to zero -
// the caller must do that if necessary.
//
// *ptr_frame_info -- is set to point to the Frame_Info struct of the
// newly allocated frame
//
// RETURNS
//   0 -- on success
//   If failed, it panic.
//
// Hint: use LIST_FIRST, LIST_REMOVE, and initialize_frame_info
// Hint: references should not be incremented

//extern void env_free(struct Env *e);

int allocate_frame(struct FrameInfo **ptr_frame_info)
{
	*ptr_frame_info = LIST_FIRST(&free_frame_list);
	int c = 0;
	if (*ptr_frame_info == NULL)
	{
		//TODO: [PROJECT'23.MS3 - BONUS] Free RAM when it's FULL
		panic("ERROR: Kernel run out of memory... allocate_frame cannot find a free frame.\n");
		// When allocating new frame, if there's no free frame, then you should:
		//	1-	If any process has exited (those with status ENV_EXIT), then remove one or more of these exited processes from the main memory
		//	2-	otherwise, free at least 1 frame from the user working set by applying the FIFO algorithm
	}

	LIST_REMOVE(&free_frame_list,*ptr_frame_info);

	/******************* PAGE BUFFERING CODE *******************
	 ***********************************************************/

	if((*ptr_frame_info)->isBuffered)
	{
		pt_clear_page_table_entry((*ptr_frame_info)->environment->env_page_directory,(*ptr_frame_info)->va);
		//pt_set_page_permissions((*ptr_frame_info)->environment->env_pgdir, (*ptr_frame_info)->va, 0, PERM_BUFFERED);
	}

	/**********************************************************
	 ***********************************************************/

	initialize_frame_info(*ptr_frame_info);
	return 0;
}

//
// Return a frame to the free_frame_list.
// (This function should only be called when ptr_frame_info->references reaches 0.)
//
void free_frame(struct FrameInfo *ptr_frame_info)
{
	/*2012: clear it to ensure that its members (env, isBuffered, ...) become NULL*/
	initialize_frame_info(ptr_frame_info);
	/*=============================================================================*/

	// Fill this function in
	LIST_INSERT_HEAD(&free_frame_list, ptr_frame_info);
	//LOG_STATMENT(cprintf("FN # %d FREED",to_frame_number(ptr_frame_info)));
}

//
// Decrement the reference count on a frame
// freeing it if there are no more references.
//
void decrement_references(struct FrameInfo* ptr_frame_info)
{
	if (--(ptr_frame_info->references) == 0)
		free_frame(ptr_frame_info);
}

//
// Stores address of page table entry in *ptr_page_table .
// Stores 0 if there is no such entry or on error.
//
// IT RETURNS:
//  TABLE_IN_MEMORY : if page table exists in main memory
//	TABLE_NOT_EXIST : if page table doesn't exist,
//

int get_page_table(uint32 *ptr_page_directory, const uint32 virtual_address, uint32 **ptr_page_table)
{
	//	cprintf("gpt .05\n");
	uint32 page_directory_entry = ptr_page_directory[PDX(virtual_address)];

	//2022: check PERM_PRESENT of the table first before calculating its PA
	if ( (page_directory_entry & PERM_PRESENT) == PERM_PRESENT)
	{
		//	cprintf("gpt .07, page_directory_entry= %x \n",page_directory_entry);
		if(USE_KHEAP && !CHECK_IF_KERNEL_ADDRESS(virtual_address))
		{
			*ptr_page_table = (void *)kheap_virtual_address(EXTRACT_ADDRESS(page_directory_entry)) ;
			//cprintf("===>get_page_table: page_dir_entry = %x ptr_page_table = %x\n", page_directory_entry,*ptr_page_table);
		}
		else
		{
			*ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(EXTRACT_ADDRESS(page_directory_entry)) ;
		}
		return TABLE_IN_MEMORY;
	}
	else if (page_directory_entry != 0) //the table exists but not in main mem, so it must be in sec mem
	{
		// Put the faulted address in CR2 and then
		// Call the fault_handler() to load the table in memory for us ...
		//		cprintf("gpt .1\n, %x page_directory_entry\n", page_directory_entry);
		lcr2((uint32)virtual_address) ;

		//		cprintf("gpt .12\n");
		fault_handler(NULL);

		//		cprintf("gpt .15\n");
		// now the page_fault_handler() should have returned successfully and updated the
		// directory with the new table frame number in memory
		page_directory_entry = ptr_page_directory[PDX(virtual_address)];
		if(USE_KHEAP && !CHECK_IF_KERNEL_ADDRESS(virtual_address))
		{
			*ptr_page_table = (void *)kheap_virtual_address(EXTRACT_ADDRESS(page_directory_entry)) ;
		}
		else
		{
			*ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(EXTRACT_ADDRESS(page_directory_entry)) ;
		}

		return TABLE_IN_MEMORY;
	}
	else // there is no table for this va anywhere. This is a new table required, so check if the user want creation
	{
		//		cprintf("gpt .2\n");
		*ptr_page_table = 0;
		return TABLE_NOT_EXIST;
	}
}

void * create_page_table(uint32 *ptr_directory, const uint32 virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #0 GIVENS] [PROGRAM LOAD] create_page_table()

	//Use kmalloc() to create a new page TABLE for the given virtual address,
	//link it to the given directory and return the address of the created table
	//REMEMBER TO:
	//	a.	clear all entries (as it may contain garbage data)
	//	b.	clear the TLB cache (using "tlbflush()")

	//change this "return" according to your answer

#if USE_KHEAP
	uint32 * ptr_page_table = kmalloc(PAGE_SIZE);
	if(ptr_page_table == NULL)
	{
		panic("NOT ENOUGH KERNEL HEAP SPACE");
	}
	ptr_directory[PDX(virtual_address)] = CONSTRUCT_ENTRY(
			kheap_physical_address((unsigned int)ptr_page_table)
			, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);

	//================
	memset(ptr_page_table , 0, PAGE_SIZE);
	tlbflush();

#else
	uint32 * ptr_page_table ;
	__static_cpt(ptr_directory, virtual_address, &ptr_page_table) ;
#endif

	return ptr_page_table;
}

void __static_cpt(uint32 *ptr_directory, const uint32 virtual_address, uint32 **ptr_page_table)
{
	struct FrameInfo* ptr_new_frame_info;
	int err = allocate_frame(&ptr_new_frame_info) ;

	uint32 phys_page_table = to_physical_address(ptr_new_frame_info);
	*ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(phys_page_table) ;
	ptr_new_frame_info->references = 1;
	ptr_directory[PDX(virtual_address)] = CONSTRUCT_ENTRY(phys_page_table, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
	//initialize new page table by 0's
	memset(*ptr_page_table , 0, PAGE_SIZE);
	tlbflush();
}
//
// Map the physical frame 'ptr_frame_info' at 'virtual_address'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm|PERM_PRESENT'.
//
// Details
//   - If there is already a frame mapped at 'virtual_address', it should be unmaped
// using unmap_frame().
//   - If necessary, on demand, allocates a page table and inserts it into 'ptr_page_directory'.
//   - ptr_frame_info->references should be incremented if the insertion succeeds
//
// RETURNS:
//   0 on success
//
// Hint: implement using get_page_table() and unmap_frame().
//
int map_frame(uint32 *ptr_page_directory, struct FrameInfo *ptr_frame_info, uint32 virtual_address, int perm)
{
	// Fill this function in
	uint32 physical_address = to_physical_address(ptr_frame_info);
	uint32 *ptr_page_table;
	if( get_page_table(ptr_page_directory, virtual_address, &ptr_page_table) == TABLE_NOT_EXIST)
	{
		/*==========================================================================================
		// OLD WRONG SOLUTION
		//=====================
		//// initiate a read instruction for an address inside the wanted table.
		//// this will generate a page fault, that will cause page_fault_handler() to
		//// create the table in memory for us ...
		//char dummy_char = *((char*)virtual_address) ;
		//// a page fault is created now and page_fault_handler() should start handling the fault ...

		//// now the page_fault_handler() should have returned successfully and updated the
		//// directory with the new table frame number in memory
		//uint32 page_directory_entry;
		//page_directory_entry = ptr_page_directory[PDX(virtual_address)];
		//ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(EXTRACT_ADDRESS(page_directory_entry)) ;
		=============================================================================================*/
#if USE_KHEAP
		{
			ptr_page_table = create_page_table(ptr_page_directory, (uint32)virtual_address);
			//cprintf("======>page table created using kheap for VA %x at dir = %x PT = %x\n", virtual_address, ptr_page_directory[PDX(virtual_address)], ptr_page_table);
			uint32* ptr_page_table2 =NULL;
			//cprintf("======> After the table created at %x\n\n", get_page_table(ptr_page_directory, virtual_address,&ptr_page_table2));
		}
#else
		{
			__static_cpt(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
		}
#endif

	}

	//cprintf("NOW .. map add = %x ptr_page_table = %x PTX(virtual_address) = %d\n", virtual_address, ptr_page_table,PTX(virtual_address));
	uint32 page_table_entry = ptr_page_table[PTX(virtual_address)];

	/*OLD WRONG SOLUTION
	if( EXTRACT_ADDRESS(page_table_entry) != physical_address)
	{
		if( page_table_entry != 0)
		{
			unmap_frame(ptr_page_directory , virtual_address);
		}
		ptr_frame_info->references++;
		ptr_page_table[PTX(virtual_address)] = CONSTRUCT_ENTRY(physical_address , perm | PERM_PRESENT);

	}*/

	/*NEW'15 CORRECT SOLUTION*/
	//If already mapped
	if ((page_table_entry & PERM_PRESENT) == PERM_PRESENT)
	{
		//on this pa, then do nothing
		if (EXTRACT_ADDRESS(page_table_entry) == physical_address)
			return 0;
		//on another pa, then unmap it
		else
			unmap_frame(ptr_page_directory , virtual_address);
	}
	ptr_frame_info->references++;
	ptr_page_table[PTX(virtual_address)] = CONSTRUCT_ENTRY(physical_address , perm | PERM_PRESENT);

	return 0;
}

//
// Return the frame mapped at 'virtual_address'.
// If the page table entry corresponding to 'virtual_address' exists, then we store a pointer to the table in 'ptr_page_table'
// This is used by 'unmap_frame()'
// but should not be used by other callers.
//
// Return 0 if there is no frame mapped at virtual_address.
//
// Hint: implement using get_page_table() and get_frame_info().
//
struct FrameInfo * get_frame_info(uint32 *ptr_page_directory, uint32 virtual_address, uint32 **ptr_page_table)
{
	// Fill this function in
	//cprintf(".gfi .1\n %x, %x, %x, \n", ptr_page_directory, virtual_address, ptr_page_table);
	uint32 ret =  get_page_table(ptr_page_directory, virtual_address, ptr_page_table) ;
	//cprintf(".gfi .15\n");
	if((*ptr_page_table) != 0)
	{
		uint32 index_page_table = PTX(virtual_address);
		//cprintf(".gfi .2\n");
		uint32 page_table_entry = (*ptr_page_table)[index_page_table];
		if( page_table_entry != 0)
		{
			//cprintf(".gfi .3\n");
			return to_frame_info( EXTRACT_ADDRESS ( page_table_entry ) );
		}
		return 0;
	}
	return 0;
}

//
// Unmaps the physical frame at 'virtual_address'.
//
// Details:
//   - The references count on the physical frame should decrement.
//   - The physical frame should be freed if the 'references' reaches 0.
//   - The page table entry corresponding to 'virtual_address' should be set to 0.
//     (if such a page table exists)
//   - The TLB must be invalidated if you remove an entry from
//	   the page directory/page table.
//
// Hint: implement using get_frame_info(),
// 	tlb_invalidate(), and decrement_references().
//
void unmap_frame(uint32 *ptr_page_directory, uint32 virtual_address)
{
	// Fill this function in
	uint32 *ptr_page_table;
	struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, virtual_address, &ptr_page_table);
	if( ptr_frame_info != 0 )
	{
		if (ptr_frame_info->isBuffered && !CHECK_IF_KERNEL_ADDRESS((uint32)virtual_address))
			cprintf("Freeing BUFFERED frame at va %x!!!\n", virtual_address) ;
		decrement_references(ptr_frame_info);
		ptr_page_table[PTX(virtual_address)] = 0;
		tlb_invalidate(ptr_page_directory, (void *)virtual_address);
	}
}


/*/this function should be called only in the env_create() for creating the page table if not exist
 * (without causing page fault as the normal map_frame())*/
// Map the physical frame 'ptr_frame_info' at 'virtual_address'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm|PERM_PRESENT'.
//
// Details
//   - If there is already a frame mapped at 'virtual_address', it should be unmaped
// using unmap_frame().
//   - If necessary, on demand, allocates a page table and inserts it into 'ptr_page_directory'.
//   - ptr_frame_info->references should be incremented if the insertion succeeds
//
// RETURNS:
//   0 on success
//
//
int loadtime_map_frame(uint32 *ptr_page_directory, struct FrameInfo *ptr_frame_info, uint32 virtual_address, int perm)
{
	uint32 physical_address = to_physical_address(ptr_frame_info);
	uint32 *ptr_page_table;

	uint32 page_directory_entry = ptr_page_directory[PDX(virtual_address)];

	if(USE_KHEAP && !CHECK_IF_KERNEL_ADDRESS(virtual_address))
	{
		ptr_page_table = (uint32*)kheap_virtual_address(EXTRACT_ADDRESS(page_directory_entry)) ;
	}
	else
	{
		ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(EXTRACT_ADDRESS(page_directory_entry)) ;
	}

	//if page table not exist, create it in memory and link it with the directory
	if (page_directory_entry == 0)
	{
#if USE_KHEAP
		{
			ptr_page_table = create_page_table(ptr_page_directory, virtual_address);
		}
#else
		{
			__static_cpt(ptr_page_directory, virtual_address, &ptr_page_table);
		}
#endif
	}

	ptr_frame_info->references++;
	ptr_page_table[PTX(virtual_address)] = CONSTRUCT_ENTRY(physical_address , perm | PERM_PRESENT);

	return 0;
}


///****************************************************************************************///
///******************************* END OF MAPPING USER SPACE ******************************///
///****************************************************************************************///


//==================================================================================================
//==================================================================================================
//==================================================================================================



// calculate_available_frames:
struct freeFramesCounters calculate_available_frames()
{

	//calculate the free frames from the free frame list
	struct FrameInfo *ptr;
	uint32 totalFreeUnBuffered = 0 ;
	uint32 totalFreeBuffered = 0 ;
	uint32 totalModified = 0 ;

	LIST_FOREACH(ptr, &free_frame_list)
	{
		if (ptr->isBuffered)
			totalFreeBuffered++ ;
		else
			totalFreeUnBuffered++ ;
	}

	LIST_FOREACH(ptr, &modified_frame_list)
	{
		totalModified++ ;
	}

	struct freeFramesCounters counters ;
	counters.freeBuffered = totalFreeBuffered ;
	counters.freeNotBuffered = totalFreeUnBuffered ;
	counters.modified = totalModified;
	return counters;
}

///============================================================================================



