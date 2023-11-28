/********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 15 */
/************************************************************/

#include <inc/lib.h>

void _main(void)
{
	//[1] Kernel address
	uint32 *ptr = (uint32*)(KERNEL_STACK_TOP - 12) ;
	*ptr = 100 ;

	inctst();
	panic("tst invalid access failed:Attempt to access kernel location.\nThe env must be killed and shouldn't return here.");

	return;
}

