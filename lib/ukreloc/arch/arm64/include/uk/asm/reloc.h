/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RELOC_H__
#error Do not include this header directly
#endif

#ifdef __ASSEMBLY__

/**
 * Load symbol's value into register
 *
 * Notice: "symbol value" refers to the contents in the symbol table, i.e.
 *         an address or a constant, depending on the symbol type. To obtain
 *         the value at the symbol's address, use ur_ldr instead.
 *
 * If the symbol is an absolute symbol, the value is loaded directly.
 * Otherwise:
 * - If relocation is enabled, uses PC-relative addressing (±4GiB for 4KiB pages)
 * - If relocation is disabled uses literal load (no range constraints).
 *
 * @param req Target register
 * @param sym Symbol whose address to load
 */
.macro	ur_addr	reg:req, sym:req
#if CONFIG_LIBUKRELOC
	adrp	\reg, \sym
	add	\reg, \reg, :lo12:\sym
#else /* CONFIG_LIBUKRELOC */
	ldr	\reg, =\sym
#endif /* !CONFIG_LIBUKRELOC */
.endm

/**
 * Dereference a symbol and load the resulting value into register
 *
 * Uses PC-relative addressing (±4GiB for 4KiB pages).
 *
 * @param req Target register
 * @param sym Symbol to dereference
 */
.macro ur_ldr reg:req, sym:req
	adrp	\reg, \sym
	ldr	\reg, [\reg, :lo12:\sym]
.endm

#endif /* __ASSEMBLY__ */
