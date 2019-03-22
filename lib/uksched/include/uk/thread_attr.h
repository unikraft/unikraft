/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UK_SCHED_THREAD_ATTR_H__
#define __UK_SCHED_THREAD_ATTR_H__

#include <stdbool.h>
#include <uk/arch/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_THREAD_ATTR_WAITABLE         0
#define UK_THREAD_ATTR_DETACHED         1

#define UK_THREAD_ATTR_PRIO_INVALID     (-1)
#define UK_THREAD_ATTR_PRIO_MIN         0
#define UK_THREAD_ATTR_PRIO_MAX         255
#define UK_THREAD_ATTR_PRIO_DEFAULT     127

#define UK_THREAD_ATTR_TIMESLICE_NIL    0

typedef int prio_t;

typedef struct uk_thread_attr {
	/* True if thread should detach */
	bool detached;
	/* Priority */
	prio_t prio;
	/* Time slice in nanoseconds */
	__nsec timeslice;
} uk_thread_attr_t;

int uk_thread_attr_init(uk_thread_attr_t *attr);
int uk_thread_attr_fini(uk_thread_attr_t *attr);

int uk_thread_attr_set_detachstate(uk_thread_attr_t *attr, int state);
int uk_thread_attr_get_detachstate(const uk_thread_attr_t *attr, int *state);

int uk_thread_attr_set_prio(uk_thread_attr_t *attr, prio_t prio);
int uk_thread_attr_get_prio(const uk_thread_attr_t *attr, prio_t *prio);

int uk_thread_attr_set_timeslice(uk_thread_attr_t *attr, __nsec timeslice);
int uk_thread_attr_get_timeslice(const uk_thread_attr_t *attr, __nsec *timeslice);

#ifdef __cplusplus
}
#endif

#endif /* __UK_SCHED_THREAD_ATTR_H__ */
