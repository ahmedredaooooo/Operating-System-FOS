/********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 15 - SECOND CHANCE LIST = 2*/
/************************************************************/

#include <inc/lib.h>

void _main(void)
{
	cprintf("PART I: Test the Pointer Validation inside fault_handler():\n");
	cprintf("===========================================================\n");
		rsttst();
	int ID1 = sys_create_env("tia_slave1", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
	sys_run_env(ID1);

	int ID2 = sys_create_env("tia_slave2", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
	sys_run_env(ID2);

	int ID3 = sys_create_env("tia_slave3", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
	sys_run_env(ID3);
	env_sleep(10000);

	if (gettst() != 0)
		cprintf("\nPART I... Failed.\n");
	else
		cprintf("\nPART I... completed successfully\n\n");


	cprintf("PART II: PLACEMENT: Test the Invalid Access to a NON-EXIST page in Page File, Stack & Heap:\n");
	cprintf("===========================================================================================\n");

	rsttst();
	int ID4 = sys_create_env("tia_slave4", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
	sys_run_env(ID4);

	env_sleep(10000);

	if (gettst() != 0)
		cprintf("\nPART II... Failed.\n");
	else
		cprintf("\nPART II... completed successfully\n\n");

	cprintf("Congratulations... test invalid access completed successfully\n");
}

