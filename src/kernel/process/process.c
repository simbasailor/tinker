/*
 *
 * SOS Source Code
 * __________________
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#include "process.h"

#include "config.h"
#include "arch/tgt.h"
#include "kernel/objects/object_table.h"
#include "kernel/objects/obj_thread.h"
#include "kernel/utils/util_strlen.h"

/**
 * The code to handle an unbounded list of threads for the process
 */
UNBOUNDED_LIST_INTERNAL_TYPE(thread_list_t, __thread_t*)
UNBOUNDED_LIST_SPEC(static, thread_list_t, __thread_t*)
UNBOUNDED_LIST_BODY(static, thread_list_t, __thread_t*)

/**
 * An iterator for the thread list
 */
UNBOUNDED_LIST_ITERATOR_INTERNAL_TYPE(thread_list_it_t, thread_list_t, __thread_t*)
UNBOUNDED_LIST_ITERATOR_BODY(extern, thread_list_it_t, thread_list_t, __thread_t*)

typedef struct __process_t
{
	uint32_t				process_id;
	thread_list_t	*		threads;
	__mem_pool_info_t * 	memory_pool;
	__object_table_t *		object_table;
	object_number_t			object_number;
	bool					kernel_process;
	tgt_mem_t				segment_info;
	const mem_section_t *	first_section;
	char					image[__MAX_PROCESS_IMAGE_LEN + 1];
} __process_internal_t;

error_t __process_create(
		__mem_pool_info_t * const mempool,
		const uint32_t pid,
		const char * const name,
		const bool is_kernel,
		const __mem_pool_info_t * pool,
		__process_t ** process)
{
	__process_t * p = (__process_t*)__mem_alloc_aligned(mempool, sizeof(__process_t), MMU_PAGE_SIZE);
	error_t ret = NO_ERROR;
	if (p)
	{
		p->threads = thread_list_t_create(mempool);
		p->process_id = pid;
		p->kernel_process = is_kernel;
		p->memory_pool = (__mem_pool_info_t *)pool;
		const uint32_t length = __util_strlen(name,__MAX_PROCESS_IMAGE_LEN);
		__util_memcpy(p->image, name, length);
		p->image[length] = '\0';
		p->object_table = __obj_table_create(p->memory_pool);

		p->first_section = __mem_sec_create(
				p->memory_pool,
				__mem_get_start_addr(p->memory_pool),
				VIRTUAL_ADDRESS_SPACE,
				__mem_get_alloc_size(p->memory_pool),
				mmu_random_access_memory,
				mmu_user_access,
				mmu_read_write);

		ret = __tgt_initialise_process(p);
		if (ret == NO_ERROR && process)
		{
			*process = p;
		}
	}
	else
	{
		ret = OUT_OF_MEMORY;
	}
	return ret;
}

uint32_t __process_get_pid(const __process_t * const process)
{
	return process->process_id;
}

object_number_t __process_get_oid(const __process_t * const process)
{
	return process->object_number;
}

void __process_set_oid(
		__process_t * const process,
		const object_number_t oid)
{
	process->object_number = oid;
}

bool __process_is_kernel(const __process_t * const process)
{
	return process->kernel_process;
}

void __process_set_kernel(__process_t * process, bool is_kernel)
{
	process->kernel_process = is_kernel;
}

__mem_pool_info_t * __process_get_mem_pool(const __process_t * const process)
{
	return process->memory_pool;
}

__object_table_t * __process_get_object_table(const __process_t * const process)
{
	return process->object_table;
}

const tgt_mem_t * __process_get_segment_info(const __process_t * const process)
{
	return &process->segment_info;
}

void __process_set_segment_info(
		__process_t * const process,
		const tgt_mem_t * const seg)
{
	__util_memcpy(
			&process->segment_info,
			seg,
			sizeof(tgt_mem_t));
}

bool __process_add_thread(
		__process_t * const process,
		__thread_t * const thread,
		object_number_t * const objno)
{
	bool ret = false;
	const uint32_t thread_count = thread_list_t_size(process->threads);
	if ( thread_count < __MAX_THREADS )
	{
		/* get the new thread id - SLOW! - need to speed up */
		__thread_t * tmp = NULL;
		uint32_t thread_id = 0;
		if ( thread_count == 0 )
		{
			thread_id = 0;
		}
		else
		{
			for ( uint32_t i = 0 ; i < __MAX_THREADS ; i++ )
			{
				if ( !thread_list_t_get(process->threads, i, &tmp) )
				{
					thread_id = i;
					break;
				}
			}
		}
		// update the thread's id
		__thread_set_tid(thread, thread_id);

		// create an object in the table
		ret = __obj_create_thread(
				process->memory_pool,
				process->object_table,
				process->process_id,
				thread_id,
				thread,
				 objno);

		__thread_set_oid(thread, *objno);

		thread_list_t_add(process->threads, thread);

		ret = true;
	}
	return ret;
}

__thread_t * _process_get_main_thread(const __process_t * process)
{
	__thread_t * t = NULL;
	thread_list_t_get(process->threads, 0, &t);
	return t;
}

thread_list_it_t * __process_get_threads(const __process_t * const process)
{
	return thread_list_it_t_create(process->threads);
}

const mem_section_t * __process_get_first_section(const __process_t * const process)
{
	return process->first_section;
}