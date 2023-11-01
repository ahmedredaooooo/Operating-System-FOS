/********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 15 */
/************************************************************/

#include <inc/lib.h>

void _main(void)
{
	//[4] Not in Page File, Not Stack & Not Heap
	uint32 kilo = 1024;
	{
		uint32 size = 4*kilo;

		unsigned char *x = (unsigned char *)(0x00200000-PAGE_SIZE);

		int i=0;
		for(;i< size+20;i++)
		{
			x[i]=-1;
		}
	}

	inctst();
	panic("tst invalid access failed: Attempt to access page that's not exist in page file, neither stack or heap.\nThe env must be killed and shouldn't return here.");

	return;
}

