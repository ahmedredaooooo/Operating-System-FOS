/********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 15 */
/************************************************************/

#include <inc/lib.h>

void _main(void)
{
	//[1] User address but READ-ONLY
	uint32 *ptr = (uint32*)USER_TOP;
	*ptr = 100 ;

	inctst();
	panic("tst invalid access failed: Attempt to write on a READ-ONLY user page.\nThe env must be killed and shouldn't return here.");

	return;
}

