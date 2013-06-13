/*
 *
 * SOS Source Code
 * __________________
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#ifndef PROCESS_MANAGER_H_
#define PROCESS_MANAGER_H_

#include "../kernel_types.h"
#include "kernel/utils/collections/unbounded_list.h"
#include "kernel/utils/collections/unbounded_list_iterator.h"

UNBOUNDED_LIST_TYPE(process_list_t)
UNBOUNDED_LIST_ITERATOR_TYPE(process_list_it_t)
UNBOUNDED_LIST_ITERATOR_SPEC(extern, process_list_it_t, process_list_t, __process_t*)
UNBOUNDED_LIST_TYPE(thread_list_t)
UNBOUNDED_LIST_ITERATOR_TYPE(thread_list_it_t)
UNBOUNDED_LIST_ITERATOR_SPEC(extern, thread_list_it_t, thread_list_t, __thread_t*)
/*
 * Process Manager
 */

void __proc_initialise(void);

error_t __proc_create_process(
		const char * image,
		const char * initial_task_name,
		__thread_entry * entry_point,
		const uint8_t priority,
		const uint32_t stack,
		const uint32_t heap,
		const uint32_t flags,
		__process_t ** process);

error_t __proc_create_thread(
		__process_t * process,
		const char * name,
		__thread_entry * entry_point,
		const uint8_t priority,
		const uint32_t stack,
		const uint32_t flags,
		__object_t ** thread_object,
		__thread_t ** new_thread);

void __proc_get_object_table(__object_table_t ** t);

__process_t * __proc_get_process(const uint32_t process_id);

__thread_t * __proc_get_thread(const __process_t * proc, const uint32_t thread_id);

process_list_it_t * __proc_get_process_iterator(void);

process_list_it_t * __proc_get_new_process_iterator(void);

#endif /* PROCESS_MANAGER_H_ */
