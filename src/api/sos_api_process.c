/*
 *
 * SOS Source Code
 * __________________
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#include "sos_api_process.h"
#include "sos_api.h"
#include "sos_api_kernel_interface.h"

/**
 * Create a new Process
 * @param image_name The name of the process
 * @param initial_thread_name The name of the initial thread/task
 * @param entry The entry point of the initial thread/task
 * @param priority The priority of the initial thread/task
 * @param heap The amount of heap for the process. This will be taken from the
 * parent process. If the parent process does not have enough spare heap for
 * this new process to be created then it will fail.
 * @param stack The amount of stack for th einitial threa/task.
 * @param process A pointer to the created Process.
 * @param thread A pointer to the created Thread.
 * @return
 */
error_t sos_create_process(
		const char * image_name,
		thread_entry_point * entry,
		uint8_t priority,
		const sos_meminfo_t * const meminfo,
		uint32_t flags,
		sos_process_t * process)
{
	return SOS_API_CALL_6(
			SYSCALL_CREATE_PROCESS,
			(uint32_t)image_name,
			(uint32_t)entry,
			(uint32_t)priority,
			(uint32_t)meminfo,
			flags,
			(uint32_t)process);
}

error_t sos_create_thread(
		const char * thread_name,
		thread_entry_point * entry,
		uint8_t priority,
		uint32_t stack,
		uint32_t flags,
		sos_thread_t * thread)
{
	return SOS_API_CALL_6(
			SYSCALL_CREATE_THREAD,
			(uint32_t)thread_name,
			(uint32_t)entry,
			(uint32_t)priority,
			stack,
			flags,
			(uint32_t)thread);
}

error_t sos_get_thread_object(sos_thread_t * thread)
{
	return SOS_API_CALL_1(
			SYSCALL_THREAD_OBJECT,
			(uint32_t)thread);
}

error_t	sos_get_thread_priority(
		sos_thread_t thread,
		uint8_t * priority)
{
	return SOS_API_CALL_2(
			SYSCALL_THREAD_PRIORITY,
			(uint32_t)thread,
			(uint32_t)priority);
}

error_t sos_exit_thread(void)
{
	return SOS_API_CALL_0(SYSCALL_EXIT_THREAD);
}

void sos_debug(const char * const str)
{
	SOS_API_CALL_1(SYSCALL_DEBUG, (uint32_t)str);
}
