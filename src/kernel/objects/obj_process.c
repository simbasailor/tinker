/*
 *
 * TINKER Source Code
 * 
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#include "obj_process.h"
#include "object.h"
#include "object_private.h"
#include "object_table.h"
#include "kernel/kernel_assert.h"
#include "kernel/console/print_out.h"
#include "kernel/process/thread.h"
#include "kernel/process/process_manager.h"
#include "kernel/memory/memory_manager.h"

typedef struct object_process_t
{
	object_internal_t object;
	uint32_t pid;
	mem_pool_info_t * pool;
	process_t * process;
} object_process_internal_t;

object_process_t * obj_cast_process(object_t * o)
{
	object_process_t * result = NULL;
	if(o)
	{
		const object_process_t * const tmp = (const object_process_t*)o;
		if (tmp->object.type == PROCESS_OBJ)
		{
			result = (object_process_t*)tmp;
		}
	}
	return result;
}

error_t obj_create_process(
		mem_pool_info_t * const pool,
		object_table_t * const table,
		const uint32_t process_id,
		process_t * const process,
		object_t ** object)
{
	object_process_t * no = NULL;
	error_t result = NO_ERROR;

	if (object)
	{
		if (table)
		{
			no = (object_process_t*)mem_alloc(pool, sizeof(object_process_t));
#if defined (PROCESS_DEBUGGING)
			debug_print("Objects: Creating process, address is %x\n", no);
#endif
			if (no)
			{
				object_number_t objno;
				result = obj_add_object(table, (object_t*)no, &objno);
				if (result == NO_ERROR)
				{
#if defined (PROCESS_DEBUGGING)
					debug_print("Objects: Process added to object table\n");
#endif
					obj_initialise_object(&no->object, objno, PROCESS_OBJ);
					no->pid = process_id;
					no->pool = pool;
					no->process = process;
					*object = (object_t*)no;
				}
				else
				{
					mem_free(pool, no);
				}
			}
			else
			{
				result = OUT_OF_MEMORY;
			}
		}
		else
		{
			result = PARAMETERS_OUT_OF_RANGE;
		}
	}
	else
	{
		result = PARAMETERS_NULL;
	}

	return result;
}

error_t obj_process_thread_exit(
		object_process_t * const o,
		object_thread_t * const thread)
{
	error_t ret;
	kernel_assert("obj_process_thread - check process object exists\n", o != NULL);
	kernel_assert("obj_process_thread - check thread object exists\n", thread != NULL);

#if defined(PROCESS_DEBUGGING)
		debug_print("Objects: Process %d (%s) Thread %s is exiting\n", o->pid, process_get_image(o->process), thread_get_name(obj_get_thread(thread)));
#endif

	process_thread_exit(o->process, obj_get_thread(thread));
	obj_remove_object(
			process_get_object_table(o->process),
			obj_thread_get_oid(thread));

	ret = obj_exit_thread(thread);

	const uint32_t thread_count = process_get_thread_count(o->process);
	if (!thread_count)
	{
		obj_process_exit(o);
	}

	return ret;
}

void obj_process_exit(object_process_t * const o)
{
	kernel_assert("obj_process_exit - check process object exists\n", o != NULL);
	obj_remove_object(
			process_get_object_table(o->process),
			o->object.object_number);
#if defined(PROCESS_DEBUGGING)
	debug_print("Objects: Process %d exit\n", o->pid);
#endif
	process_t * const proc = o->process;
	obj_delete_process(o);
	proc_delete_proc(proc);
	process_exit(proc);
}

void obj_delete_process(object_process_t * const o)
{
	mem_free(o->pool, o);
}

object_number_t obj_process_get_oid
	(const object_process_t * const o)
{
	object_number_t oid = INVALID_OBJECT_ID;
	if (o)
	{
		oid = o->object.object_number;
	}
	return oid;
}

uint32_t obj_process_pid(const object_process_t * const o)
{
	return o->pid;
}
