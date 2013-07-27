/*
 *
 * SOS Source Code
 * __________________
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#ifndef OPIC_TIMER_H_
#define OPIC_TIMER_H_

#include "kernel/kernel_types.h"
#include "kernel/time/time.h"

void opic_tmr_get_timer(uint32_t * base_address, __timer_t * timer);

#endif /* OPIC_TIMER_H_ */
