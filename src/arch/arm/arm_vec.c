/*
 *
 * TINKER Source Code
 *
 *
 *  [2009] - [2014] Samuel Steven Truscott
 *  All Rights Reserved.
 */

#include "arm_vec.h"
#include "kernel/console/print_out.h"

#define EXCEPTION_START_SYSCALL \
	uint32_t context; \
	asm("stmfd sp!,{r0-r12,lr}");	/* store all the registers */ \
	asm("mrs r0, spsr"); 			/* get the spsr */ \
	asm("mov r1, sp"); 				/* get the new sp */ \
	asm("push {r0, r1}"); 			/* store the spsr and sp */ \
	asm("mov %[ps], sp" : [ps]"=r" (context)); /* move the sp into context var */

#define EXCEPTION_START \
	uint32_t context; \
	asm("sub lr, lr, #4"); 			/* update return addr */ \
	asm("stmfd sp!,{r0-r12,lr}");	/* store all the registers */ \
	asm("mrs r0, spsr"); 			/* get the spsr */ \
	asm("mov r1, sp"); 				/* get the new sp */ \
	asm("push {r0, r1}"); 			/* store the spsr and sp */ \
	asm("mov %[ps], sp" : [ps]"=r" (context)); /* move the sp into context var */

#define EXCEPTION_END \
	asm("nop"); \
	asm("mrs r3, cpsr"); 			/* backup cpsr */ \
	asm("msr cpsr, #0x12");			/* enter irq mode */ \
	asm("ldr sp, =__ivtse");		/* setup irq stack */ \
	asm("msr cpsr, r3");			/* restore old cpsr */ \
	asm("pop {r0, r1}"); 			/* get the spsr back */ \
	asm("msr SPSR_cxsf, r0"); 		/* restore spsr */ \
	asm("ldm sp!, {r0-r12,pc}^")	/* return! */

static arm_vec_handler_t * vector_table[8];

static void __attribute__((naked)) arm_vector_reset()
{
	EXCEPTION_START;
	vector_table[VECTOR_RESET](VECTOR_RESET, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_undefined()
{
	EXCEPTION_START;
	vector_table[VECTOR_UNDEFINED](VECTOR_UNDEFINED, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_prefetch_abort()
{
	EXCEPTION_START;
	vector_table[VECTOR_PRETECH_ABORT](VECTOR_PRETECH_ABORT, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_data_abort()
{
	EXCEPTION_START;
	vector_table[VECTOR_DATA_ABORT](VECTOR_DATA_ABORT, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_reserved()
{
	EXCEPTION_START;
	vector_table[VECTOR_RESERVED](VECTOR_RESERVED, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_irq()
{
	EXCEPTION_START;
	vector_table[VECTOR_IRQ](VECTOR_IRQ, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_fiq()
{
	EXCEPTION_START;
	vector_table[VECTOR_FIQ](VECTOR_FIQ, context);
	EXCEPTION_END;
}

static void __attribute__((naked)) arm_vector_system_call()
{
	EXCEPTION_START_SYSCALL;
	vector_table[VECTOR_SYSTEM_CALL](VECTOR_SYSTEM_CALL, context);
	EXCEPTION_END;
}

#define OFFSET_FOR_PREFETCH 8
#define ALIGNMENT_SHIFT 2
#define BRANCH_INSTRUCTION 0xEA000000
#define GET_BRANCH_FOR_VECTOR(v, i) \
	((((uint32_t)v - (i*4) - (OFFSET_FOR_PREFETCH)) >> ALIGNMENT_SHIFT) | BRANCH_INSTRUCTION)

void arm_vec_install(arm_vec_t vector, arm_vec_handler_t * const handler)
{
	volatile uint32_t * const vector_root = (uint32_t*)0x0;
	vector_table[vector] = handler;
	switch(vector)
	{
	case VECTOR_RESET:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_reset, vector);
		break;
	case VECTOR_UNDEFINED:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_undefined, vector);
		break;
	case VECTOR_PRETECH_ABORT:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_prefetch_abort, vector);
		break;
	case VECTOR_DATA_ABORT:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_data_abort, vector);
		break;
	case VECTOR_RESERVED:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_reserved, vector);
		break;
	case VECTOR_IRQ:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_irq, vector);
		break;
	case VECTOR_FIQ:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_fiq, vector);
		break;
	case VECTOR_SYSTEM_CALL:
		vector_root[vector] = GET_BRANCH_FOR_VECTOR(&arm_vector_system_call, vector);
		break;
	}
}
