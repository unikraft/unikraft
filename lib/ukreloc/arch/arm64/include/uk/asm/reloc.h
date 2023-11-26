/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RELOC_H__
#error Do not include this header directly
#endif

#ifdef __ASSEMBLY__

/*
 * Macro to replace the non-relocation-friendly `ldr` instruction. The usage
 * is the same.
 *
 * @param req The register into which to load the value of the symbol
 * @param sym The symbol whose value to place into the register
 */
.macro	ur_ldr	reg:req, sym:req
#if CONFIG_LIBUKRELOC
	adrp	\reg, \sym
	add	\reg, \reg, :lo12:\sym
#else /* CONFIG_LIBUKRELOC */
	ldr	\reg, =\sym
#endif /* !CONFIG_LIBUKRELOC */
.endm

#endif /* __ASSEMBLY__ */
