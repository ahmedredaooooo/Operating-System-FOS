#include <inc/stdio.h>
#include <kern/proc/priority_manager.h>
#include <inc/assert.h>
#include <kern/proc/user_environment.h>
#include "../disk/pagefile_manager.h"
#include "../mem/kheap.h"
#include "../mem/memory_manager.h"

void set_program_priority(struct Env* env, int priority)
{
	panic("Not implemented");
}
