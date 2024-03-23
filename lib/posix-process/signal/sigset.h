/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors:  Mihai Pogonaru <pogonarumihai@gmail.com>
 *		     Teodora Serbanescu <teo.serbanescu16@gmail.com>
 *		     Felipe Huici <felipe.huici@neclab.eu>
 *		     Bernard Rizzo <b.rizzo@student.uliege.be>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest.
 * All rights reserved.
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
 */
#ifndef __UK_SIGSET_H__
#define __UK_SIGSET_H__

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Kernel-side definition of sigset_t */
typedef unsigned long uk_sigset_t;

#define uk_sigemptyset(_ptr)		(*(_ptr) = 0)
#define uk_sigfillset(_ptr)		(*(_ptr) = ~((uk_sigset_t)0))
#define uk_sigaddset(_ptr, _signo)	(*(_ptr) |= (1UL << ((_signo) - 1)))
#define uk_sigdelset(_ptr, _signo)	(*(_ptr) &= ~(1UL << ((_signo) - 1)))
#define uk_sigcopyset(_ptr1, _ptr2)	(*(_ptr1) = *(_ptr2))
#define uk_sigandset(_ptr1, _ptr2)	(*(_ptr1) &= *(_ptr2))
#define uk_sigorset(_ptr1, _ptr2)	(*(_ptr1) |= *(_ptr2))
#define uk_sigreverseset(_ptr)		(*(_ptr) = ~(*(_ptr)))
#define uk_sigismember(_ptr, _signo)	(*(_ptr) & (1UL << ((_signo) - 1)))
#define uk_sigisempty(_ptr)		(*(_ptr) == 0)

#ifdef __cplusplus
}
#endif

#endif /* __UK_SIGSET_H__ */
