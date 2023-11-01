
#include <inc/lib.h>

void
destroy(void)
{
	sys_destroy_env(0);
}

void
exit(void)
{
	sys_exit_env();
}
