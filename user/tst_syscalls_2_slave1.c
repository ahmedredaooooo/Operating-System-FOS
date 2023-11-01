// Test the correct validation of system call params
#include <inc/lib.h>

void
_main(void)
{
	//[1] NULL (0) address
	sys_allocate_user_mem(0,10);
	inctst();
	panic("tst system calls #2 failed: sys_allocate_user_mem is called with invalid params\nThe env must be killed and shouldn't return here.");

	sys_free_user_mem(0, 10);
	inctst();
	panic("tst system calls #2 failed: sys_free_user_mem is called with invalid params\nThe env must be killed and shouldn't return here.");
}
