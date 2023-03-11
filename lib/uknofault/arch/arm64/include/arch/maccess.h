/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKNOFAULT_ARCH_MACCESS_H__
#define __UKNOFAULT_ARCH_MACCESS_H__

#include "excpttab.h"

#define nf_do_probe_r(addr, isuffix, reg, cont_label)			\
	__asm__ goto(							\
		"1: ldr"isuffix" "reg", [%0]\n"				\
		NF_EXCPTTAB_ENTRY_L(1b, cont_label, nf_mf_handler)	\
		: /* no outputs in goto asm allowed */			\
		: "r"(addr)						\
		: reg : cont_label)

#define nf_probe_r(addr, type, cont_label)				\
	do {								\
		switch (sizeof(type)) {					\
		case 1:							\
			nf_do_probe_r(addr, "b", "w0", cont_label);	\
			break;						\
		case 2:							\
			nf_do_probe_r(addr, "h", "w0", cont_label);	\
			break;						\
		case 4:							\
			nf_do_probe_r(addr, "", "w0", cont_label);	\
			break;						\
		case 8:							\
			nf_do_probe_r(addr, "", "x0", cont_label);	\
			break;						\
		}							\
	} while (0)

#define nf_do_memcpy(dst, src, isuffix, reg, cont_label)		\
	__asm__ goto(							\
		"1: ldr"isuffix" "reg", [%1]\n"				\
		"2: str"isuffix" "reg", [%0]\n"				\
		NF_EXCPTTAB_ENTRY_L(1b, cont_label, nf_mf_handler)	\
		NF_EXCPTTAB_ENTRY_L(2b, cont_label, nf_mf_handler)	\
		: /* no outputs in goto asm allowed */			\
		: "r"(dst), "r"(src)					\
		: reg : cont_label)

#define nf_memcpy(dst, src, type, cont_label)				\
	do {								\
		switch (sizeof(type)) {					\
		case 1:							\
			nf_do_memcpy(dst, src, "b", "w0", cont_label);	\
			break;						\
		case 2:							\
			nf_do_memcpy(dst, src, "h", "w0", cont_label);	\
			break;						\
		case 4:							\
			nf_do_memcpy(dst, src, "", "w0", cont_label);	\
			break;						\
		case 8:							\
			nf_do_memcpy(dst, src, "", "x0", cont_label);	\
			break;						\
		}							\
	} while (0)

#define nf_regs_ip(regs) ((regs)->elr_el1)

#endif /* __UKNOFAULT_ARCH_MACCESS_H__ */
