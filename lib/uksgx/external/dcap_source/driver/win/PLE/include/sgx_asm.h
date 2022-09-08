// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
// Copyright(c) 2016-17 Intel Corporation.
//
// Authors:
//
// Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>

#ifndef SGX_ASM_H
#define SGX_ASM_H

.macro ENCLU
.byte 0x0f, 0x01, 0xd7
.endm

#endif /* SGX_ASM_H */
