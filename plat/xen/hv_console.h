/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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

#ifndef __XEN_HV_CONSOLE_H__
#define __XEN_HV_CONSOLE_H__

#include <errno.h>

#ifndef __XEN_CONSOLE_IMPL__
#error Do not include this header directly, use <common/console.h> instead
#endif /* !__XEN_CONSOLE_IMPL__*/

#if (CONFIG_XEN_KERNEL_HV_CONSOLE || CONFIG_XEN_DEBUG_HV_CONSOLE)
void hv_console_prepare(void);
void hv_console_init(void);
int hv_console_output(const char *str, unsigned int len);
void hv_console_flush(void);
int hv_console_input(char *str, unsigned int maxlen);
#else /* (CONFIG_XEN_KERNEL_HV_CONSOLE || CONFIG_XEN_DEBUG_HV_CONSOLE) */
#define hv_console_prepare()			\
	do {} while (0)
#define hv_console_init() \
	do {} while (0)
#define hv_console_flush() \
	do {} while (0)
#define hv_console_input(str, maxlen) \
	(-ENOTSUP)
#endif /* (CONFIG_XEN_KERNEL_HV_CONSOLE || CONFIG_XEN_DEBUG_HV_CONSOLE) */

#if CONFIG_XEN_KERNEL_HV_CONSOLE
#define hv_console_output_k(str, len) \
	hv_console_output((str), (len))
#else
#define hv_console_output_k(str, len) \
	(-ENOTSUP)
#endif /* CONFIG_XEN_KERNEL_HV_CONSOLE */

#if CONFIG_XEN_DEBUG_HV_CONSOLE
#define hv_console_output_d(str, len) \
	hv_console_output((str), (len))
#else
#define hv_console_output_d(str, len) \
	(-ENOTSUP)
#endif /* CONFIG_XEN_DEBUG_HV_CONSOLE */

#endif /* __XEN_HV_CONSOLE_H__ */
