#ifndef FOS_KERN_UTILITIES_H
#define FOS_KERN_UTILITIES_H
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/x86.h>
#include <inc/timerreg.h>
#include <inc/memlayout.h>

inline unsigned int nearest_pow2_ceil(unsigned int x) ;
inline unsigned int log2_ceil(unsigned int x) ;

void scarce_memory();

void	check_boot_pgdir();

void detect_loop_in_FrameInfo_list(struct FrameInfo_List* fi_list);
uint32 calc_no_pages_tobe_removed_from_ready_exit_queues();

struct Env *__pe, *__ne ;
uint8 __pl, __nl, __chkstatus ;
void chksch(uint8 onoff);
void chk1();
void chk2(struct Env* __se);

uint32 tstcnt;
void rsttst();
void inctst();
uint32 gettst();
void tst(uint32 n, uint32 v1, uint32 v2, char c, int inv);
void chktst(uint32 n);

#endif /* !FOS_KERN_UTILITIES_H */
