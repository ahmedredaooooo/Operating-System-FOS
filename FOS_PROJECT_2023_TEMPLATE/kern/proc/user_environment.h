/* See COPYRIGHT for copyright information. */

#ifndef FOS_KERN_ENV_H
#define FOS_KERN_ENV_H

#ifndef FOS_MULTIENV
// Change this value to 1 once you're allowing multiple environments
// (for UCLA: Lab 3, Part 3; for MIT: Lab 4).
#define FOS_MULTIENV 0
#endif

#include <inc/environment_definitions.h>
#include <kern/proc/user_programs.h>
#include <inc/types.h>


//========================================================
//extern struct UserProgramInfo userPrograms[];
extern struct UserProgramInfo* ptr_UserPrograms;

//=========================================================

extern struct Env *envs;		// All environments
extern struct Env *curenv;	        // Current environment

///===================================================================================

/*2020*/ struct Env* env_create(char* user_program_name, unsigned int page_WS_size, unsigned int LRU_second_list_size, unsigned int percent_WS_pages_to_remove);
void env_run(struct Env *e) __attribute__((noreturn));
void env_free(struct Env *e);
int envid2env(int32  envid, struct Env **env_store, bool checkperm);
/*2015*/ void env_exit();
///===================================================================================
void	env_init(void);
//int	env_alloc(struct Env **e, int32  parent_id);
// The following two functions do not return
void	env_pop_tf(struct Trapframe *tf) __attribute__((noreturn));
void env_run_cmd_prmpt();

#endif // !FOS_KERN_ENV_H
