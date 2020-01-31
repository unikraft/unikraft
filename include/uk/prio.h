/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#ifndef __UK_PRIO_H__
#define __UK_PRIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Computes the priority level that has one lower priority than
 * the given priority level. This macro can be used to state
 * initialization dependencies. It is intended to be used for
 * declaring macro constants.
 * NOTE: This macro should only be used for Unikraft constructors
 * (UK_CTOR_PRIO(), UK_CTOR()) and Unikraft init table entries.
 *
 * @param x
 *  Given Unikraft priority level
 * @return
 *  Priority level that has one priority less than x
 */
#define __UK_PRIO_AFTER_0 1
#define __UK_PRIO_AFTER_1 2
#define __UK_PRIO_AFTER_2 3
#define __UK_PRIO_AFTER_3 4
#define __UK_PRIO_AFTER_4 5
#define __UK_PRIO_AFTER_5 6
#define __UK_PRIO_AFTER_6 7
#define __UK_PRIO_AFTER_7 8
#define __UK_PRIO_AFTER_8 9
#define __UK_PRIO_AFTER_9 __UK_PRIO_OUT_OF_BOUNDS
#define __UK_PRIO_AFTER(x) __UK_PRIO_AFTER_##x
#define UK_PRIO_AFTER(x)   __UK_PRIO_AFTER(x)

#define UK_PRIO_EARLIEST 0
#define UK_PRIO_LATEST   9

/* Stop compilation if priority is getting out of bounds */
#ifdef __GNUC__
#pragma GCC poison __UK_PRIO_OUT_OF_BOUNDS
#else
#error Out of bounds pragma not defined
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UK_PRIO_H__ */
