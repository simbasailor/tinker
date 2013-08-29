/*
 *
 * SOS Source Code
 * __________________
 *
 *  [2009] - [2013] Samuel Steven Truscott
 *  All Rights Reserved.
 */
#ifndef KERNEL_ASSERT_H_
#define KERNEL_ASSERT_H_

#include "arch/tgt_types.h"

void __kernel_assert_cond(const char * message, bool_t var, const bool_t cond);

void __kernel_assert(const char * message, const bool_t var);

#endif /* KERNEL_ASSERT_H_ */
