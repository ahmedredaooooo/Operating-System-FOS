/*
 * fault_handler.h
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#ifndef KERN_FAULT_HANDLER_H_
#define KERN_FAULT_HANDLER_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif
#include <inc/types.h>
#include <inc/environment_definitions.h>

/******************************/
/*	DATA 					  */
/******************************/
uint32 _EnableModifiedBuffer ;
uint32 _EnableBuffering ;

uint32 _PageRepAlgoType;
#define PG_REP_LRU_TIME_APPROX 0x1
#define PG_REP_LRU_LISTS_APPROX 0x2
#define PG_REP_CLOCK 0x3
#define PG_REP_FIFO 0x4
#define PG_REP_MODIFIEDCLOCK  0x5
#define PG_REP_NchanceCLOCK 0x6
#define PG_REP_DYNAMIC_LOCAL 0x7

/*2021*/ uint32 page_WS_max_sweeps;

extern uint8 bypassInstrLength ;

/******************************/
/*	FUNCTIONS				  */
/******************************/
//===============================
// REPLACEMENT STRATEGIES
//===============================
void setPageReplacmentAlgorithmLRU(int LRU_TYPE);
void setPageReplacmentAlgorithmCLOCK();
void setPageReplacmentAlgorithmFIFO();
void setPageReplacmentAlgorithmModifiedCLOCK();
/*2018*/void setPageReplacmentAlgorithmDynamicLocal();
/*2021*/void setPageReplacmentAlgorithmNchanceCLOCK();

uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE);
uint32 isPageReplacmentAlgorithmCLOCK();
uint32 isPageReplacmentAlgorithmFIFO();
uint32 isPageReplacmentAlgorithmModifiedCLOCK();
/*2018*/uint32 isPageReplacmentAlgorithmDynamicLocal();
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK();

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt);
uint8 isModifiedBufferEnabled();
void enableBuffering(uint32 enableIt);
uint8 isBufferingEnabled() ;
void setModifiedBufferLength(uint32 length) ;
uint32 getModifiedBufferLength();

//===============================
// FAULT HANDLERS
//===============================
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va);
void dyn_alloc_local_scope_method(struct Env * curenv, uint32 fault_va);
void page_fault_handler(struct Env * curenv, uint32 fault_va);
void table_fault_handler(struct Env * curenv, uint32 fault_va);

#endif /* KERN_FAULT_HANDLER_H_ */
