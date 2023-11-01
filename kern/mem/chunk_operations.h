/*
 * chunk_operations.h
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#ifndef KERN_MEM_CHUNK_OPERATIONS_H_
#define KERN_MEM_CHUNK_OPERATIONS_H_

#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages) ;
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size);
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms) ;
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms) ;
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages) ;
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size);

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size);
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size);
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size);
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size);

#endif /* KERN_MEM_CHUNK_OPERATIONS_H_ */
