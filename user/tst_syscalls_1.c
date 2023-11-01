// Test the correct implementation of system calls
#include <inc/lib.h>

void
_main(void)
{
	rsttst();
	void * ret = sys_sbrk(10);
	if (ret != (void*)-1)
		panic("tst system calls #1 failed: sys_sbrk is not handled correctly");
	sys_allocate_user_mem(100,10);
	sys_free_user_mem(100, 10);
	int ret2 = gettst();
	if (ret2 != 2)
		panic("tst system calls #1 failed: sys_allocate_user_mem and/or sys_free_user_mem are not handled correctly");
	cprintf("Congratulations... tst system calls #1 completed successfully");
}
