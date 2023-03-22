/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __ARM_64_UK_RELOC_H__
#define __ARM_64_UK_RELOC_H__

#include "../../../include/uk/reloc.h"

#ifdef __ASSEMBLY__

/*
 * Macro to replaced the non-relocation fiendly `ldr` instruction. The usage
 * is the same.
 *
 * @param req The register into which to load the value of the symbol
 * @param sym The symbol whose value to place into the register
 */
.macro ur_ldr reg:req, sym:req
#ifdef CONFIG_OPTIMIZE_PIE
	adrp	\reg, \sym
	add	\reg, \reg, :lo12:\sym
#else
	ldr	\reg, =\sym
#endif
.endm

#endif /* __ASSEMBLY__ */

#endif /* __ARM_64_UK_RELOC_H__ */
