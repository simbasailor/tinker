/*
 *
 * TINKER Source Code
 *
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */

#ifndef KERNEL_DEVICE_H_
#define KERNEL_DEVICE_H_

#include "arch/tgt_types.h"
#include "tinker_api_errors.h"

struct kernel_device;

typedef error_t (kernel_device_initialise)(
		struct kernel_device * device_info,
		void * param,
		const uint32_t param_size);

typedef error_t (kernel_device_control)(
		void * usr_data, uint32_t code);

typedef error_t (kernel_device_write_register)(
		const void * const usr_data,
		const uint32_t id,
		const uint32_t val);

typedef error_t (kernel_device_read_register)(
		const void * const usr_data, const uint32_t id, uint32_t * const val);

typedef error_t (kernel_device_write_buffer)(
		const void * const usr_data,
		const uint32_t dst,
		const void * const src,
		const uint32_t src_size);

typedef error_t (kernel_device_read_buffer)(
		const void * const usr_data,
		const uint32_t src,
		void * const dst,
		const uint32_t dst_size);

typedef error_t (kernel_device_isr)(
		const void * const usr_data,
		const uint32_t vector);

typedef struct kernel_device
{
	kernel_device_initialise * 	initialise;
	kernel_device_control	*		control;
	kernel_device_write_register *write_register;
	kernel_device_read_register *	read_register;
	kernel_device_write_buffer *	write_buffer;
	kernel_device_read_buffer *	read_buffer;
	kernel_device_isr * 			isr;
	const void * user_data;
} kernel_device_t;

#endif /* KERNEL_DEVICE_H_ */
