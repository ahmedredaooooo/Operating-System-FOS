/********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 15 */
/************************************************************/

#include <inc/lib.h>

void _main(void)
{
	//[1] Non=reserved User Heap
	uint32 *ptr = (uint32*)(USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE);
	*ptr = 100 ;

	inctst();
	panic("tst invalid access failed: Attempt to access a non-reserved (unmarked) user heap page.\nThe env must be killed and shouldn't return here.");

	return;
}

