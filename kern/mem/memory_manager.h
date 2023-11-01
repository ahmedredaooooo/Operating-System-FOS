/* See COPYRIGHT for copyright information. */

#ifndef FOS_KERN_MEM_MAN_H
#define FOS_KERN_MEM_MAN_H

#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/error.h>
#include <inc/string.h>
//#include <inc/environment_definitions.h>
#include <inc/uheap.h>
#include "../tests/utilities.h"
#include <kern/cpu/kclock.h>

#include "boot_memory_manager.h"
#include "paging_helpers.h"
#include "working_set_manager.h"
#include "chunk_operations.h"


#define TABLE_IN_MEMORY 0
#define TABLE_NOT_EXIST 1

//***********************************
/*2015*/ //USER HEAP STRATEGIES
uint32 _UHeapPlacementStrategy;

static inline void setUHeapPlacementStrategyFIRSTFIT(){_UHeapPlacementStrategy = UHP_PLACE_FIRSTFIT;}
static inline void setUHeapPlacementStrategyBESTFIT(){_UHeapPlacementStrategy = UHP_PLACE_BESTFIT;}
static inline void setUHeapPlacementStrategyNEXTFIT(){_UHeapPlacementStrategy = UHP_PLACE_NEXTFIT;}
static inline void setUHeapPlacementStrategyWORSTFIT(){_UHeapPlacementStrategy = UHP_PLACE_WORSTFIT;}

static inline uint8 isUHeapPlacementStrategyFIRSTFIT(){if(_UHeapPlacementStrategy == UHP_PLACE_FIRSTFIT) return 1; return 0;}
static inline uint8 isUHeapPlacementStrategyBESTFIT(){if(_UHeapPlacementStrategy == UHP_PLACE_BESTFIT) return 1; return 0;}
static inline uint8 isUHeapPlacementStrategyNEXTFIT(){if(_UHeapPlacementStrategy == UHP_PLACE_NEXTFIT) return 1; return 0;}
static inline uint8 isUHeapPlacementStrategyWORSTFIT(){if(_UHeapPlacementStrategy == UHP_PLACE_WORSTFIT) return 1; return 0;}

//***********************************
//2018 Memory Threshold
uint32 memory_scarce_threshold_percentage;	// Memory remains plentiful till the % of free frames gets below this threshold percentage
#define DEFAULT_MEM_SCARCE_PERCENTAGE 25	// Default threshold % of free memory to indicate scarce MEM
//***********************************

//***********************************
/*DATA*/
struct freeFramesCounters
{
	int freeBuffered, freeNotBuffered, modified;
};


//***********************************
/*FUNCTIONS*/

//RUN TIME [USER SPACE]
int allocate_frame(struct FrameInfo **ptr_frame_info);
void free_frame(struct FrameInfo *ptr_frame_info);
int	map_frame(uint32 *ptr_page_directory, struct FrameInfo *ptr_frame_info, uint32 virtual_address, int perm);
void unmap_frame(uint32 *pgdir, uint32 virtual_address);
int get_page_table(uint32 *ptr_page_directory, const uint32 virtual_address, uint32 **ptr_page_table);
/*2016*/ void * create_page_table(uint32 *ptr_page_directory, const uint32 virtual_address);
struct FrameInfo *get_frame_info(uint32 *ptr_page_directory, uint32 virtual_address, uint32 **ptr_page_table);
void decrement_references(struct FrameInfo* ptr_frame_info);
void initialize_frame_info(struct FrameInfo *ptr_frame_info);

static inline uint32 to_frame_number(struct FrameInfo *ptr_frame_info)
{
	return ptr_frame_info - frames_info;
}

static inline uint32 to_physical_address(struct FrameInfo *ptr_frame_info)
{
	return to_frame_number(ptr_frame_info) << PGSHIFT;
}

static inline struct FrameInfo* to_frame_info(uint32 physical_address)
{
	if (PPN(physical_address) >= number_of_frames)
		panic("to_frame_info called with invalid pa");
	return &frames_info[PPN(physical_address)];
}

void	tlb_invalidate(uint32 *pgdir, void *ptr);

struct freeFramesCounters calculate_available_frames();

void __static_cpt(uint32 *ptr_directory, const uint32 virtual_address, uint32 **ptr_page_table);
int loadtime_map_frame(uint32 *ptr_page_directory, struct FrameInfo *ptr_frame_info, uint32 virtual_address, int perm);

/************************************/
/*MACROS*/
/* This macro takes a user supplied address and turns it into
 * something that will cause a fault if it is a kernel address.  ULIM
 * itself is guaranteed never to contain a valid page.
 */
#define TRUP(_p)   						\
({								\
	register typeof((_p)) __m_p = (_p);			\
	(uint32) __m_p > ULIM ? (typeof(_p)) ULIM : __m_p;	\
})

/* This macro takes a kernel virtual address -- an address that points above
 * KERNEL_BASE, where the machine's maximum 256MB of physical memory is mapped --
 * and returns the corresponding physical address.  It panics if you pass it a
 * non-kernel virtual address.
 */
#define STATIC_KERNEL_PHYSICAL_ADDRESS(kva)						\
({								\
	uint32 __m_kva = (uint32) (kva);		\
	if (__m_kva < KERNEL_BASE)					\
		panic("K_PHYSICAL_ADDRESS called with invalid kva %08lx", __m_kva);\
	__m_kva - KERNEL_BASE;					\
})

/* This macro takes a physical address and returns the corresponding kernel
 * virtual address.  It panics if you pass an invalid physical address. */
#define STATIC_KERNEL_VIRTUAL_ADDRESS(pa)						\
({								\
	uint32 __m_pa = (pa);				\
	uint32 __m_ppn = PPN(__m_pa);				\
	if (__m_ppn >= number_of_frames)					\
		panic("K_VIRTUAL_ADDRESS called with invalid pa %08lx", __m_pa);\
	(void*) (__m_pa + KERNEL_BASE);				\
})

/* This Macro creates a page table/directory entry (32 bits)
* according to the format of Intel page table/directory entry
*/
#define CONSTRUCT_ENTRY(phys_frame_address, permissions) \
( \
	phys_frame_address | permissions \
)

//2016
#define CHECK_IF_KERNEL_ADDRESS(virtual_address) ( (uint32)virtual_address >= (uint32)USER_TOP && (uint32)virtual_address <= (uint32)0xFFFFFFFF)

#endif /* !FOS_KERN_MEM_MAN_H */
