/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/limits.h>
#include <uk/nofault.h>
#include <uk/arch/types.h>
#include <uk/arch/traps.h>
#include <uk/arch/paging.h>
#include <uk/event.h>
#include <uk/config.h>

#include "arch/maccess.h"

#ifdef CONFIG_LIBUKVMEM
#include <uk/vmem.h>
typedef unsigned long nf_paging_state_t;

static inline void nf_disable_paging(nf_paging_state_t *state)
{
	struct uk_vas *vas = uk_vas_get_active();

	if (!vas)
		return;

	*state = uk_vas_paging_savef(vas);
}

static inline void nf_enable_paging(nf_paging_state_t *state)
{
	struct uk_vas *vas = uk_vas_get_active();

	if (!vas)
		return;

	uk_vas_paging_restoref(vas, *state);
}
#else /* CONFIG_LIBUKVMEM */
typedef int nf_paging_state_t;
#define nf_disable_paging(state) (void)state
#define nf_enable_paging(state)  (void)state
#endif /* !CONFIG_LIBUKVMEM */

/* Must be a library global symbol so that the linker can resolve it */
int nf_mf_handler(const struct nf_excpttab_entry *e,
		  struct ukarch_trap_ctx *ctx)
{
	/* Just continue execution at the fault label */
	nf_regs_ip(ctx->regs) = nf_excpttab_get_cont_ip(e);

	return UK_EVENT_HANDLED;
}

static int nf_mem_fault_handler(void *data)
{
	struct ukarch_trap_ctx *ctx = (struct ukarch_trap_ctx *)data;

	return nf_handle_trap(nf_regs_ip(ctx->regs), ctx);
}

UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_PAGE_FAULT, nf_mem_fault_handler,
		      CONFIG_LIBUKNOFAULT_FAULT_PRIO);
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_BUS_ERROR,  nf_mem_fault_handler,
		      CONFIG_LIBUKNOFAULT_FAULT_PRIO);
#ifdef CONFIG_ARCH_X86_64
UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_X86_GP,     nf_mem_fault_handler,
		      CONFIG_LIBUKNOFAULT_FAULT_PRIO);
#endif /* CONFIG_ARCH_X86_64 */

#define nf_probe_r_loop(addr, len, type, cont_label)			\
	while (len >= sizeof(type)) {					\
		nf_probe_r((type *)(addr), type, cont_label);		\
		addr += sizeof(type);					\
		len  -= sizeof(type);					\
	}

#define nf_memcpy_loop(dst, src, len, type, cont_label)			\
	while (len >= sizeof(type)) {					\
		nf_memcpy((type *)(dst), (type *)(src),			\
			  type, cont_label);				\
		dst += sizeof(type);					\
		src += sizeof(type);					\
		len -= sizeof(type);					\
	}

__sz uk_nofault_probe_r(__vaddr_t vaddr, __sz len, unsigned long flags)
{
	nf_paging_state_t ps = 0;
	__sz l = len;
	__sz bytes = 0;

	if (flags & UK_NOFAULTF_NOPAGING)
		nf_disable_paging(&ps);

	do {
		nf_probe_r_loop(vaddr, l, __u64, FAULT_1);
		nf_probe_r_loop(vaddr, l, __u32, FAULT_1);
		nf_probe_r_loop(vaddr, l, __u16, FAULT_1);
FAULT_1:
		nf_probe_r_loop(vaddr, l, __u8, FAULT_2);
		UK_ASSERT(l == 0);
		break;
FAULT_2:
		if (!(flags & UK_NOFAULTF_CONTINUE))
			break;

		UK_ASSERT(vaddr <= __VADDR_MAX - PAGE_SIZE);
		UK_ASSERT(len >= l);

		bytes += len - l;
		vaddr += PAGE_SIZE;
		l     -= MIN(PAGE_SIZE, l);
		len    = l;
	} while (l > 0);

	if (flags & UK_NOFAULTF_NOPAGING)
		nf_enable_paging(&ps);

	UK_ASSERT(len >= l);
	bytes += len - l;

	return bytes;
}

__sz uk_nofault_probe_rw(__vaddr_t vaddr, __sz len, unsigned long flags)
{
	/* Ok to use memcpy-style function despite overlap because source and
	 * destination addresses are identical and we control the implementation
	 */
	return uk_nofault_memcpy((char *)vaddr, (const char *)vaddr, len,
				 flags);
}

__sz uk_nofault_memcpy(char *dst, const char *src, __sz len,
		       unsigned long flags)
{
	nf_paging_state_t ps = 0;
	__sz l = len;
	__sz bytes = 0;

	if (flags & UK_NOFAULTF_NOPAGING)
		nf_disable_paging(&ps);

	do {
		nf_memcpy_loop(dst, src, l, __u64, FAULT_1);
		nf_memcpy_loop(dst, src, l, __u32, FAULT_1);
		nf_memcpy_loop(dst, src, l, __u16, FAULT_1);
FAULT_1:
		nf_memcpy_loop(dst, src, l, __u8, FAULT_2);
		UK_ASSERT(l == 0);
		break;
FAULT_2:
		if (!(flags & UK_NOFAULTF_CONTINUE))
			break;

		UK_ASSERT((__vaddr_t)dst <= __VADDR_MAX - PAGE_SIZE);
		UK_ASSERT((__vaddr_t)src <= __VADDR_MAX - PAGE_SIZE);
		UK_ASSERT(len >= l);

		bytes += len - l;
		dst   += PAGE_SIZE;
		src   += PAGE_SIZE;
		l     -= MIN(PAGE_SIZE, l);
		len    = l;
	} while (l > 0);

	if (flags & UK_NOFAULTF_NOPAGING)
		nf_enable_paging(&ps);

	UK_ASSERT(len >= l);
	bytes += len - l;

	return bytes;
}
