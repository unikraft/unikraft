/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Scratch-top layout the asm fault handlers share with the C code, kept
 * apart from mem.h because it is assembler-safe.
 */

#ifndef __HYPERLIGHT_SCRATCH_TOP_H__
#define __HYPERLIGHT_SCRATCH_TOP_H__

/* Pages at the top of scratch that hold the exception stack and the host's
 * metadata, which no allocator may hand out: SCRATCH_TOP_RESERVED_PAGES of
 * the hyperlight this kernel is built for.
 */
#define HL_SCRATCH_RESERVED_PAGES	12

/* Free pages between the transport arena and the reserved pages:
 * SCRATCH_TRANSPORT_GAP_PAGES of the hyperlight this kernel is built for.
 */
#define HL_SCRATCH_TRANSPORT_GAP_PAGES	1

/* Offset down from the scratch top of the metadata slot that holds the
 * transport arena's GPA (the arena itself is much further down).
 */
#define HL_SCRATCH_ARENA_GPA_OFF	0x38

#endif /* __HYPERLIGHT_SCRATCH_TOP_H__ */
