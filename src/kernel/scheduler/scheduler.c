/*
 *
 * SOS Source Code
 * __________________
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#include "scheduler.h"

#include "arch/tgt.h"
#include "arch/board_support.h"

#include "kernel/kernel_assert.h"
#include "kernel/kernel_panic.h"
#include "kernel/kernel_initialise.h"
#include "kernel/process/process_manager.h"
#include "kernel/objects/object_table.h"
#include "kernel/objects/obj_semaphore.h"
#include "kernel/utils/util_memcpy.h"
#include "kernel/utils/collections/unbounded_queue.h"
#include "kernel/utils/collections/stack.h"

UNBOUNDED_QUEUE_TYPE(__thread_queue_t)
UNBOUNDED_QUEUE_INTERNAL_TYPE(__thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_INITIALISE(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_FRONT(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_PUSH(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_POP(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_REMOVE(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_REORDER_FIRST(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_SPEC_SIZE(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_INITIALISE(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_FRONT(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_PUSH(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_POP(extern, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_REMOVE(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_REORDER_FIRST(static, __thread_queue_t, __thread_t*)
UNBOUNDED_QUEUE_BODY_SIZE(static, __thread_queue_t, __thread_t*)

STACK_TYPE(__queue_stack_t)
STACK_INTERNAL_TYPE(__queue_stack_t, __thread_queue_t*)
STACK_SPEC_INITIALISE(static, __queue_stack_t, __thread_queue_t*)
STACK_SPEC_PUSH(static, __queue_stack_t, __thread_queue_t*)
STACK_SPEC_POP(static, __queue_stack_t, __thread_queue_t*)
STACK_SPEC_SIZE(static, __queue_stack_t, __thread_queue_t*)
STACK_SPEC_GET(static, __queue_stack_t, __thread_queue_t*)
STACK_SPEC_FRONT(static, __queue_stack_t, __thread_queue_t*)
STACK_SPEC_INSERT(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_INITIALISE(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_PUSH(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_SIZE(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_GET(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_POP(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_FRONT(static, __queue_stack_t, __thread_queue_t*)
STACK_BODY_INSERT(static, __queue_stack_t, __thread_queue_t*)

/**
 * The current thread executing
 */
static __thread_t * __sch_current_thread = NULL;

static __thread_queue_t * __sch_active_queue;

static __thread_queue_t __sch_thread_queues[__MAX_PRIORITY + 1];

static __priority_t __sch_current_priority;

static __queue_stack_t __sch_queue_stack;

static void __sch_priority_find_next_queue(__thread_t * const t);

static bool_t __sch_initialised = false;

void __sch_initialise_scheduler(void)
{
	if ( !__sch_initialised )
	{
		for ( uint16_t i = 0 ; i < __MAX_PRIORITY + 1 ; i++ )
		{
			__thread_queue_t_initialise(&__sch_thread_queues[i], __mem_get_default_pool());
		}

		__sch_active_queue = &(__sch_thread_queues[0]);
		__queue_stack_t_initialise( &__sch_queue_stack, __mem_get_default_pool() );
		__queue_stack_t_push(&__sch_queue_stack, __sch_active_queue);

		__sch_initialised = true;
	}
}

void __sch_notify_new_thread(__thread_t * const t)
{
	if (!__sch_initialised)
	{
		__sch_initialise_scheduler();
	}

	if (t)
	{
		const __priority_t thread_priority = __thread_get_priority(t);
		__thread_queue_t * const queue = &(__sch_thread_queues[thread_priority]);
		__thread_queue_t_push(queue, t);

#if defined(__PROCESS_DEBUGGING)
		__debug_print("scheduler: new thread (%s) with priority (%d)\n", __thread_get_name(t), thread_priority);
#endif

		if (thread_priority > __sch_current_priority)
		{
				__sch_current_priority = thread_priority;
				__sch_active_queue = &(__sch_thread_queues[__sch_current_priority]);
				__queue_stack_t_push(&__sch_queue_stack, __sch_active_queue);
		}
		else if ( thread_priority < __sch_current_priority )
		{
			/* if we add a thread that's of a lower priority then it must be added
			 * to the queue stack so when the new high priority task finishes the
			 *  new lower priority task will be executed */
			__thread_queue_t * stack_queue = NULL;
			const uint32_t stack_size = __queue_stack_t_size(&__sch_queue_stack);
			int32_t stack_index = (int32_t)(stack_size - 1);
			bool_t found = false;
			int32_t insert_index = -1;

			/*
			 * the stack queue uses the end of the queue (size) as the highest priority
			 * and the start (0) of the queue as the lowest priority
			 *
			 * --> the below code works out where to install the priority queue into
			 * --> the stack which is priority based
			 *
			 * FIXME: This is a crap linear search.
			 *
			 */
			while(stack_index >= 0 && !found && insert_index == -1)
			{
				if (__queue_stack_t_get(&__sch_queue_stack, stack_index, &stack_queue))
				{
					const uint32_t queue_size = __thread_queue_t_size(stack_queue);
					if ( queue_size )
					{
						__thread_t * firstThreadInQueue = NULL;
						const bool_t ok = __thread_queue_t_front(stack_queue, &firstThreadInQueue);
						if ( ok == true )
						{
							const __priority_t stack_thread_priority = __thread_get_priority(firstThreadInQueue);
							if ( stack_thread_priority == thread_priority )
							{
								/* it's already on the stack */
								found = true;
							}
							else if ( stack_thread_priority < thread_priority)
							{
								insert_index = stack_index + 1;
							}
						}
					}
				}
				stack_index--;
			}

			/*
			 * we need to insert the queue into the stack to ensure
			 * that at some point it'll be scheduled
			 */
			if ( !found && insert_index >= 0 )
			{
				if (!__queue_stack_t_insert(&__sch_queue_stack, insert_index, queue))
				{
					/* failed to add it to the scheduler, panic */
					__kernel_panic();
				}
			}
		}
	}
#if defined(__PROCESS_DEBUGGING)
		__debug_print("scheduler: new priority, now (%d)\n", __sch_current_priority);
#endif
}

void __sch_notify_exit_thread(__thread_t * const t)
{
	if ( t )
	{
		const __priority_t thread_priority = __thread_get_priority(t);
		__thread_queue_t * const queue = &(__sch_thread_queues[thread_priority]);
		__thread_queue_t_remove(queue, t);

#if defined(__PROCESS_DEBUGGING)
		__debug_print("scheduler: exit thread (%s) with priority (%d)\n", __thread_get_name(t), thread_priority);
#endif

		if (__thread_queue_t_size(__sch_active_queue) == 0)
		{
			/* pop the one we're using off */
			__queue_stack_t_pop(&__sch_queue_stack, &__sch_active_queue);
			/* pop the next queue off the stack - if one can't be found
			 * perform a slow search */
			if (!__queue_stack_t_front(&__sch_queue_stack, &__sch_active_queue))
			{
				__sch_priority_find_next_queue(t);
			}
			else
			{
				__thread_t * first_thread = NULL;
				if (__thread_queue_t_front(__sch_active_queue, &first_thread))
				{
					const __priority_t tp = __thread_get_priority(first_thread);
					__sch_current_priority = tp;
				}
			}
		}
	}
#if defined(__PROCESS_DEBUGGING)
		__debug_print("scheduler: exit priority, now (%d)\n", __sch_current_priority);
#endif
}

void __sch_notify_pause_thread(__thread_t * const t)
{
	__sch_notify_exit_thread(t);
}

void __sch_notify_resume_thread(__thread_t * const t)
{
	__sch_notify_new_thread(t);
}

void __sch_notify_change_priority(
		__thread_t * const t,
		const __priority_t original_priority)
{
	if ( t )
	{
		const __priority_t thread_priority = __thread_get_priority(t);

#if defined(__PROCESS_DEBUGGING)
		__debug_print("scheduler: change thread (%s) with priority (%d)\n", __thread_get_name(t), thread_priority);
#endif

		/* remove it from the old list */
		__thread_queue_t * queue = &(__sch_thread_queues[original_priority]);
		__thread_queue_t_remove(queue, t);

		/* add it to the new list */
		queue = &(__sch_thread_queues[thread_priority]);
		__thread_queue_t_push(queue, t);

		if (!__thread_queue_t_size(__sch_active_queue))
		{
			__sch_current_priority = thread_priority;

			/* pop the one we're using off */
			__queue_stack_t_pop(&__sch_queue_stack, &__sch_active_queue);
			/* pop the next queue off the stack - if one can't be found
			 * perform a slow search */
			if ( !__queue_stack_t_front(&__sch_queue_stack, &__sch_active_queue) )
			{
				__sch_priority_find_next_queue(t);
			}
		}
	}
#if defined(__PROCESS_DEBUGGING)
		__debug_print("scheduler: change priority, now (%d)\n", __sch_current_priority);
#endif
}

void __sch_terminate_current_thread(
		const __tgt_context_t * const context)
{
	__thread_t * thread = __sch_get_current_thread();

	if ( thread )
	{
		__thread_set_state(thread, THREAD_TERMINATED);
		__thread_save_context(thread, context);
		__sch_notify_exit_thread(thread);
	}
}

__thread_t * __sch_get_current_thread(void)
{
	return __sch_current_thread;
}

void __sch_set_current_thread(__thread_t * const thread)
{
	if ( thread )
	{
		__sch_current_thread = thread;
	}
}

static void __sch_priority_find_next_queue(__thread_t * const t)
{
	uint32_t size;
	__priority_t p = __thread_get_priority(t);
	__thread_queue_t * queue;

	/* there will always be 1 - the idle thread */
	do
	{
		queue = &(__sch_thread_queues[--p]);
		size = __thread_queue_t_size(queue);
		if ( size )
		{
			break;
		}
	} while ( !size && p );

	__sch_current_priority = p;
	__sch_active_queue = queue;
	__queue_stack_t_push(&__sch_queue_stack, __sch_active_queue);
}

void __sch_set_context_for_next_thread(
		__tgt_context_t * const context)
{
	__thread_t * const current_thread = __sch_current_thread;

	if ( __sch_active_queue )
	{
		__thread_queue_t_front(__sch_active_queue, &__sch_current_thread);
	}

	/* once we're either back at the start or we've
	 * selected a new thread check its running and
	 * then use it */
	if (__sch_current_thread)
	{
		const bool_t reorder_ok = __thread_queue_t_reorder_first(__sch_active_queue);
		__kernel_assert("re-ordering of priority queue failed", reorder_ok);
		const __thread_state_t state = __thread_get_state(__sch_current_thread);
		if (state == THREADY_READY)
		{
			__thread_set_state(__sch_current_thread, THREAD_RUNNING);
		}
		else
		{
			__sch_current_thread = NULL;
		}
	}

	if (__sch_current_thread == NULL)
	{
		__sch_current_thread = __kernel_get_idle_thread();
	}

	// the thread changed so save the state of the previous thread
	if (current_thread != __sch_current_thread)
	{
		// Move the thread from the running state to the ready state
		const __thread_state_t state = __thread_get_state(current_thread);
		if (state == THREAD_RUNNING)
		{
			__thread_set_state(current_thread, THREADY_READY);
		}
		__thread_save_context(current_thread, context);
		// load in the state of the new thread
		__thread_load_context(__sch_current_thread, context);
		__tgt_prepare_context(context, __sch_current_thread);
	}

	__kernel_assert("Scheduler couldn't get next thread", __sch_current_thread != NULL);
}
