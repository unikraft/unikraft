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

#include <stdlib.h>
#include <errno.h>
#include <uk/plat/time.h>
#include <uk/thread_attr.h>
#include <uk/assert.h>


int uk_thread_attr_init(uk_thread_attr_t *attr)
{
	if (attr == NULL)
		return -EINVAL;

	attr->detached = false;
	attr->prio = UK_THREAD_ATTR_PRIO_INVALID;
	attr->timeslice = UK_THREAD_ATTR_TIMESLICE_NIL;

	return 0;
}

int uk_thread_attr_fini(uk_thread_attr_t *attr)
{
	if (attr == NULL)
		return -EINVAL;

	return 0;
}

int uk_thread_attr_set_detachstate(uk_thread_attr_t *attr, int state)
{
	if (attr == NULL)
		return -EINVAL;

	if (state == UK_THREAD_ATTR_DETACHED)
		attr->detached = true;

	else if (state == UK_THREAD_ATTR_WAITABLE)
		attr->detached = false;

	else
		return -EINVAL;

	return 0;
}

int uk_thread_attr_get_detachstate(const uk_thread_attr_t *attr, int *state)
{
	if (attr == NULL || state == NULL)
		return -EINVAL;

	if (attr->detached)
		*state = UK_THREAD_ATTR_DETACHED;
	else
		*state = UK_THREAD_ATTR_WAITABLE;

	return 0;
}

int uk_thread_attr_set_prio(uk_thread_attr_t *attr, prio_t prio)
{
	int rc = -EINVAL;

	if (attr == NULL)
		return rc;

	if (prio >= UK_THREAD_ATTR_PRIO_MIN &&
		prio <= UK_THREAD_ATTR_PRIO_MAX) {
		attr->prio = prio;
		rc = 0;
	}

	return rc;
}

int uk_thread_attr_get_prio(const uk_thread_attr_t *attr, prio_t *prio)
{
	if (attr == NULL || prio == NULL)
		return -EINVAL;

	*prio = attr->prio;

	return 0;
}

int uk_thread_attr_set_timeslice(uk_thread_attr_t *attr, __nsec timeslice)
{
	if (attr == NULL)
		return -EINVAL;

	if (timeslice < UKPLAT_TIME_TICK_NSEC)
		return -EINVAL;

	attr->timeslice = timeslice;

	return 0;
}

int uk_thread_attr_get_timeslice(const uk_thread_attr_t *attr, __nsec *timeslice)
{
	if (attr == NULL || timeslice == NULL)
		return -EINVAL;

	*timeslice = attr->timeslice;

	return 0;
}
