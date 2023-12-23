#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>

//void on_clock_update_WS_time_stamps();
extern void cleanup_buffers(struct Env* e);
//================

//=================================================================================//
//============================== QUEUE FUNCTIONS ==================================//
//=================================================================================//

//================================
// [1] Initialize the given queue:
//================================
void init_queue(struct Env_Queue* queue)
{
	if(queue != NULL)
	{
		LIST_INIT(queue);
	}
}

//================================
// [2] Get queue size:
//================================
int queue_size(struct Env_Queue* queue)
{
	if(queue != NULL)
	{
		return LIST_SIZE(queue);
	}
	else
	{
		return 0;
	}
}

//====================================
// [3] Enqueue env in the given queue:
//====================================
void enqueue(struct Env_Queue* queue, struct Env* env)
{
	assert(queue != NULL)	;
	if(env != NULL)
	{
		LIST_INSERT_HEAD(queue, env);
	}
}

//======================================
// [4] Dequeue env from the given queue:
//======================================
struct Env* dequeue(struct Env_Queue* queue)
{
	if (queue == NULL) return NULL;
	struct Env* envItem = LIST_LAST(queue);
	if (envItem != NULL)
	{
		LIST_REMOVE(queue, envItem);
	}
	return envItem;
}

//====================================
// [5] Remove env from the given queue:
//====================================
void remove_from_queue(struct Env_Queue* queue, struct Env* e)
{
	assert(queue != NULL)	;

	if (e != NULL)
	{
		LIST_REMOVE(queue, e);
	}
}

//========================================
// [6] Search by envID in the given queue:
//========================================
struct Env* find_env_in_queue(struct Env_Queue* queue, uint32 envID)
{
	if (queue == NULL) return NULL;

	struct Env * ptr_env=NULL;
	LIST_FOREACH(ptr_env, queue)
	{
		if(ptr_env->env_id == envID)
		{
			return ptr_env;
		}
	}
	return NULL;
}

//=====================================================================================//
//============================== SCHED Q'S FUNCTIONS ==================================//
//=====================================================================================//

//========================================
// [1] Delete all ready queues:
//========================================
void sched_delete_ready_queues()
{
#if USE_KHEAP
	if (env_ready_queues != NULL)
		kfree(env_ready_queues);
	if (quantums != NULL)
		kfree(quantums);
#endif
}

//=================================================
// [2] Insert the given Env in the 1st Ready Queue:
//=================================================
void sched_insert_ready0(struct Env* env)
{
	if(env != NULL)
	{
		env->env_status = ENV_READY ;
		enqueue(&(env_ready_queues[0]), env);
	}
}

//=================================================
// [3] Remove the given Env from the Ready Queue(s):
//=================================================
void sched_remove_ready(struct Env* env)
{
	if(env != NULL)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			struct Env * ptr_env = find_env_in_queue(&(env_ready_queues[i]), env->env_id);
			if (ptr_env != NULL)
			{
				LIST_REMOVE(&(env_ready_queues[i]), env);
				env->env_status = ENV_UNKNOWN;
				return;
			}
		}
	}
}

//=================================================
// [4] Insert the given Env in NEW Queue:
//=================================================
void sched_insert_new(struct Env* env)
{
	if(env != NULL)
	{
		env->env_status = ENV_NEW ;
		enqueue(&env_new_queue, env);
	}
}

//=================================================
// [5] Remove the given Env from NEW Queue:
//=================================================
void sched_remove_new(struct Env* env)
{
	if(env != NULL)
	{
		LIST_REMOVE(&env_new_queue, env) ;
		env->env_status = ENV_UNKNOWN;
	}
}

//=================================================
// [6] Insert the given Env in EXIT Queue:
//=================================================
void sched_insert_exit(struct Env* env)
{
	if(env != NULL)
	{
		if(isBufferingEnabled()) {cleanup_buffers(env);}
		env->env_status = ENV_EXIT ;
		enqueue(&env_exit_queue, env);
	}
}
//=================================================
// [7] Remove the given Env from EXIT Queue:
//=================================================
void sched_remove_exit(struct Env* env)
{
	if(env != NULL)
	{
		LIST_REMOVE(&env_exit_queue, env) ;
		env->env_status = ENV_UNKNOWN;
	}
}

//=================================================
// [8] Sched the given Env in NEW Queue:
//=================================================
void sched_new_env(struct Env* e)
{
	//add the given env to the scheduler NEW queue
	if (e!=NULL)
	{
		sched_insert_new(e);
	}
}


//=================================================
// [9] Run the given EnvID:
//=================================================
void sched_run_env(uint32 envId)
{
	struct Env* ptr_env=NULL;
	LIST_FOREACH(ptr_env, &env_new_queue)
	{
		if(ptr_env->env_id == envId)
		{
			sched_remove_new(ptr_env);
			sched_insert_ready0(ptr_env);

			/*2015*///if scheduler not run yet, then invoke it!
			if (scheduler_status == SCH_STOPPED)
			{
				fos_scheduler();
			}
			break;
		}
	}
	//	cprintf("ready queue:\n");
	//	LIST_FOREACH(ptr_env, &env_ready_queue)
	//	{
	//		cprintf("%s - %d\n", ptr_env->prog_name, ptr_env->env_id);
	//	}

}

//=================================================
// [10] Exit the given EnvID:
//=================================================
void sched_exit_env(uint32 envId)
{
	struct Env* ptr_env=NULL;
	int found = 0;
	if (!found)
	{
		LIST_FOREACH(ptr_env, &env_new_queue)
		{
			if(ptr_env->env_id == envId)
			{
				sched_remove_new(ptr_env);
				found = 1;
				//			return;
			}
		}
	}
	if (!found)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			if (!LIST_EMPTY(&(env_ready_queues[i])))
			{
				ptr_env=NULL;
				LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
				{
					if(ptr_env->env_id == envId)
					{
						LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
						found = 1;
						break;
					}
				}
			}
			if (found)
				break;
		}
	}
	if (!found)
	{
		if (curenv->env_id == envId)
		{
			ptr_env = curenv;
			found = 1;
		}
	}

	if (found)
	{
		sched_insert_exit(ptr_env);

		//If it's the curenv, then reinvoke the scheduler as there's no meaning to return back to an exited env
		if (curenv->env_id == envId)
		{
			curenv = NULL;
			fos_scheduler();
		}
	}
}


/*2015*/
//=================================================
// [11] KILL the given EnvID:
//=================================================
void sched_kill_env(uint32 envId)
{
	struct Env* ptr_env=NULL;
	int found = 0;
	if (!found)
	{
		LIST_FOREACH(ptr_env, &env_new_queue)
															{
			if(ptr_env->env_id == envId)
			{
				cprintf("killing[%d] %s from the NEW queue...", ptr_env->env_id, ptr_env->prog_name);
				sched_remove_new(ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
				found = 1;
				//			return;
			}
															}
	}
	if (!found)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			if (!LIST_EMPTY(&(env_ready_queues[i])))
			{
				ptr_env=NULL;
				LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
				{
					if(ptr_env->env_id == envId)
					{
						cprintf("killing[%d] %s from the READY queue #%d...", ptr_env->env_id, ptr_env->prog_name, i);
						LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
						env_free(ptr_env);
						cprintf("DONE\n");
						found = 1;
						break;
						//return;
					}
				}
			}
			if (found)
				break;
		}
	}
	if (!found)
	{
		ptr_env=NULL;
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			if(ptr_env->env_id == envId)
			{
				cprintf("killing[%d] %s from the EXIT queue...", ptr_env->env_id, ptr_env->prog_name);
				sched_remove_exit(ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
				found = 1;
				//return;
			}
		}
	}

	if (!found)
	{
		if (curenv->env_id == envId)
		{
			ptr_env = curenv;
			assert(ptr_env->env_status == ENV_RUNNABLE);
			cprintf("killing a RUNNABLE environment [%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			env_free(ptr_env);
			cprintf("DONE\n");
			found = 1;
			//If it's the curenv, then reset it and reinvoke the scheduler
			//as there's no meaning to return back to a killed env
			//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
			lcr3(phys_page_directory);
			curenv = NULL;
			fos_scheduler();
		}
	}
}

//=================================================
// [12] PRINT ALL Envs from all queues:
//=================================================
void sched_print_all()
{
	struct Env* ptr_env ;
	if (!LIST_EMPTY(&env_new_queue))
	{
		cprintf("\nThe processes in NEW queue are:\n");
		LIST_FOREACH(ptr_env, &env_new_queue)
		{
			cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
		}
	}
	else
	{
		cprintf("\nNo processes in NEW queue\n");
	}
	cprintf("================================================\n");
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(env_ready_queues[i])))
		{
			cprintf("The processes in READY queue #%d are:\n", i);
			LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
			{
				cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
			}
		}
		else
		{
			cprintf("No processes in READY queue #%d\n", i);
		}
		cprintf("================================================\n");
	}
	if (!LIST_EMPTY(&env_exit_queue))
	{
		cprintf("The processes in EXIT queue are:\n");
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
		}
	}
	else
	{
		cprintf("No processes in EXIT queue\n");
	}
}

//=================================================
// [13] MOVE ALL NEW Envs into READY Q:
//=================================================
void sched_run_all()
{
	struct Env* ptr_env=NULL;
	LIST_FOREACH(ptr_env, &env_new_queue)
	{
		sched_remove_new(ptr_env);
		sched_insert_ready0(ptr_env);
	}
	/*2015*///if scheduler not run yet, then invoke it!
	if (scheduler_status == SCH_STOPPED)
		fos_scheduler();
}

//=================================================
// [14] KILL ALL Envs in the System:
//=================================================
void sched_kill_all()
{
	struct Env* ptr_env ;
	if (!LIST_EMPTY(&env_new_queue))
	{
		cprintf("\nKILLING the processes in the NEW queue...\n");
		LIST_FOREACH(ptr_env, &env_new_queue)
		{
			cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			sched_remove_new(ptr_env);
			env_free(ptr_env);
			cprintf("DONE\n");
		}
	}
	else
	{
		cprintf("No processes in NEW queue\n");
	}
	cprintf("================================================\n");
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(env_ready_queues[i])))
		{
			cprintf("KILLING the processes in the READY queue #%d...\n", i);
			LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
			{
				cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
				LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
			}
		}
		else
		{
			cprintf("No processes in READY queue #%d\n",i);
		}
		cprintf("================================================\n");
	}

	if (!LIST_EMPTY(&env_exit_queue))
	{
		cprintf("KILLING the processes in the EXIT queue...\n");
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			sched_remove_exit(ptr_env);
			env_free(ptr_env);
			cprintf("DONE\n");
		}
	}
	else
	{
		cprintf("No processes in EXIT queue\n");
	}

	//reinvoke the scheduler since there're no env to return back to it
	curenv = NULL;
	fos_scheduler();
}

/*2018*/
//=================================================
// [14] EXIT ALL Ready Envs:
//=================================================
void sched_exit_all_ready_envs()
{
	struct Env* ptr_env=NULL;
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(env_ready_queues[i])))
		{
			ptr_env=NULL;
			LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
			{
				LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
				sched_insert_exit(ptr_env);
			}
		}
	}
}

/*2023*/
/********* for BSD Priority Scheduler *************/
int64 timer_ticks()
{
	return ticks;
}
int env_get_nice(struct Env* e)
{
	//TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - env_get_nice
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	//return 0;
	return e->nice_value;
}
void env_set_nice(struct Env* e, int nice_value)
{
	//TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - env_set_nice
	//Your code is here
	//Comment the following line
    //panic("Not implemented yet");

	//[1] Update nice value of the given enviroment

	//[2] If its status is NOT NEW, just update its priority without changing ready queues
	//    Else, do nothing

	e->nice_value = nice_value;

	if(e->env_status != ENV_NEW)
		update_priority(e);

}
int env_get_recent_cpu(struct Env* e)
{
	//TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - env_get_recent_cpu
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	//return 0;
	return fix_round(fix_scale(e->recent_cpu, 100));
}
int get_load_average()
{
	//TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - get_load_average
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	//return 0;
	return fix_round(fix_scale(load_avg, 100));
}
/********* for BSD Priority Scheduler *************/

/********* OUR Helper Functions ******************/

int update_priority(struct Env* e)
{
	// priority = PRI_MAX - recent_cpu/4 - 2*nice;

	fixed_point_t nice2 = fix_int(2 * e->nice_value);
	fixed_point_t recent4 = fix_div(e->recent_cpu, fix_int(4));
	fixed_point_t first_minus = fix_sub(fix_int(num_of_ready_queues - 1), recent4);
	fixed_point_t second_minus = fix_sub(first_minus, nice2);
	e->priority = fix_trunc(second_minus);
	if(e->priority > num_of_ready_queues - 1)
		e->priority = num_of_ready_queues - 1;
	else if(e->priority < PRI_MIN)
		e->priority = PRI_MIN;
	return e->priority;
}

fixed_point_t update_recent_cpu(struct Env* e, fixed_point_t recent_cpu_t_minus_1)
{
	//recent_cpu(t) = 2*load_avg/(2*load_avg + 1) * recent_cpu(t-1) + nice

	fixed_point_t load_avg_2 = fix_mul(load_avg, fix_int(2));
	fixed_point_t load_avg_2_plus_1 = fix_add(load_avg_2, fix_int(1));
	fixed_point_t load_avg_fraction = fix_div(load_avg_2, load_avg_2_plus_1);
	fixed_point_t t_recent_cpu = fix_mul(load_avg_fraction, recent_cpu_t_minus_1);
	fixed_point_t t_nice = fix_int(e->nice_value);
	e->recent_cpu = fix_add(t_recent_cpu, t_nice);
	return e->recent_cpu;
}

fixed_point_t update_load_average(fixed_point_t load_avg_t_minus_1)
{
	// load_avg(t) = 59/60 * load_avg(t-1) + 1/60 * ready_process(t);
	num_of_ready_process = 0;
	for(int i = 0; i < num_of_ready_queues; i++)
		num_of_ready_process += queue_size(&env_ready_queues[i]);

	if(curenv)
		num_of_ready_process++;

	fixed_point_t fraction59 = fix_div(fix_int(59), fix_int(60));
	fixed_point_t old_load   = fix_mul(fraction59, load_avg_t_minus_1);
	fixed_point_t fraction1  = fix_div(fix_int(1), fix_int(60));
	fixed_point_t t_num_of_ready_process = fix_mul(fraction1, fix_int(num_of_ready_process));
	load_avg = fix_add(old_load, t_num_of_ready_process);
	return load_avg;
}

/*************************************************/
//==================================================================================//
