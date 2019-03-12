/* SPDX-License-Identifier: ISC */
/*
 * Authors: Martin Lucina
 *
 * Copyright (c) 2016-2017 Docker, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <x86/traps.h>

/*
 * GDT layout
 *
 * This must be kept consistent with the layout used by the kvm entry code (as
 * defined in plat/kvm/x86/entry64.S)
 */
#define GDT_DESC_NULL           0
#define GDT_DESC_CODE           1
#define GDT_DESC_DATA           2
#define GDT_DESC_TSS_LO         3
#define GDT_DESC_TSS_HI         4
#define GDT_DESC_TSS            GDT_DESC_TSS_LO

#define GDT_DESC_OFFSET(n)      ((n) * 0x8)
#define GDT_NUM_ENTRIES         5

#define GDT_DESC_CODE_VAL       0x00af99000000ffff
#define GDT_DESC_CODE32_VAL     0x00cf9b000000ffff
#define GDT_DESC_DATA_VAL       0x00cf93000000ffff


#define IDT_NUM_ENTRIES         48
