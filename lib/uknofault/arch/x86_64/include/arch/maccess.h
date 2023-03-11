/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKNOFAULT_ARCH_MACCESS_H__
#define __UKNOFAULT_ARCH_MACCESS_H__

#include "excpttab.h"

#define nf_do_probe_r(addr, reg, cont_label)				\
	__asm__ goto(							\
		"1: mov (%0), %%"reg"\n"				\
		NF_EXCPTTAB_ENTRY_L(1b, cont_label, nf_mf_handler)	\
		: /* no outputs in goto asm allowed */			\
		: "r"(addr)						\
		: reg : cont_label)

#define nf_probe_r(addr, type, cont_label)				\
	do {								\
		switch (sizeof(type)) {					\
		case 1:							\
			nf_do_probe_r(addr, "al", cont_label);		\
			break;						\
		case 2:							\
			nf_do_probe_r(addr, "ax", cont_label);		\
			break;						\
		case 4:							\
			nf_do_probe_r(addr, "eax", cont_label);		\
			break;						\
		case 8:							\
			nf_do_probe_r(addr, "rax", cont_label);		\
			break;						\
		}							\
	} while (0)

#define nf_do_memcpy(dst, src, reg, cont_label)				\
	__asm__ goto(							\
		"1: mov (%1), %%"reg"\n"				\
		"2: mov %%"reg", (%0)\n"				\
		NF_EXCPTTAB_ENTRY_L(1b, cont_label, nf_mf_handler)	\
		NF_EXCPTTAB_ENTRY_L(2b, cont_label, nf_mf_handler)	\
		: /* no outputs in goto asm allowed */			\
		: "r"(dst), "r"(src)					\
		: reg : cont_label)

#define nf_memcpy(dst, src, type, cont_label)				\
	do {								\
		switch (sizeof(type)) {					\
		case 1:							\
			nf_do_memcpy(dst, src, "al", cont_label);	\
			break;						\
		case 2:							\
			nf_do_memcpy(dst, src, "ax", cont_label);	\
			break;						\
		case 4:							\
			nf_do_memcpy(dst, src, "eax", cont_label);	\
			break;						\
		case 8:							\
			nf_do_memcpy(dst, src, "rax", cont_label);	\
			break;						\
		}							\
	} while (0)

#define nf_regs_ip(regs) ((regs)->rip)

#endif /* __UKNOFAULT_ARCH_MACCESS_H__ */
