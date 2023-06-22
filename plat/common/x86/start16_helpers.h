/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __START16_HELPERS_H__
#define __START16_HELPERS_H__

extern __vaddr_t x86_start16_addr; /* target address */
extern void *x86_start16_begin[];
extern void *x86_start16_end[];

#define X86_START16_SIZE						\
	((__uptr)x86_start16_end - (__uptr)x86_start16_begin)

#define START16_UKRELOC_MOV_SYM(sym, sz)				\
	sym##_ukreloc_imm##sz##_start16

#define START16_UKRELOC_DATA_SYM(sym, sz)				\
	sym##_ukreloc_data##sz##_start16

#define IMPORT_START16_UKRELOC_SYM(sym, sz, type)			\
	extern void *sym[];						\
	extern void *START16_UKRELOC_##type##_SYM(sym, sz)[]

#define START16_UKRELOC_MOV_OFF(sym, sz)				\
	((void *)START16_UKRELOC_MOV_SYM(sym, sz) -			\
	(void *)x86_start16_begin)

#define START16_UKRELOC_DATA_OFF(sym, sz)				\
	((void *)START16_UKRELOC_DATA_SYM(sym, sz) -			\
	(void *)x86_start16_begin)

#define START16_UKRELOC_ENTRY(sym, sz, type)				\
	UKRELOC_ENTRY(START16_UKRELOC_##type##_OFF(sym, sz),		\
		       (void *)sym - (void *)x86_start16_begin,		\
		       sz, UKRELOC_FLAGS_PHYS_REL)

#endif  /* __START16_HELPERS_H__ */
