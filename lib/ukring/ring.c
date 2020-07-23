/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2007, 2008 Kip Macy <kmacy@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <uk/ring.h>
#include <uk/assert.h>
#include <uk/alloc.h>
#include <uk/mutex.h>
#include <uk/config.h>
#include <uk/print.h>
#include <uk/essentials.h>

struct uk_ring *
uk_ring_alloc(int count, struct uk_alloc *a
#ifdef DEBUG_BUFRING
		, struct uk_mutex *lock
#endif
)
{
	struct uk_ring *br;

	/* buf ring must be size power of 2 */
	UK_ASSERT(POWER_OF_2(count));

	br = uk_malloc(a, sizeof(struct uk_ring) + count * sizeof(caddr_t));
	if (br == NULL)
		return NULL;
#ifdef DEBUG_BUFRING
	br->br_lock = lock;
#endif
	br->br_prod_size = br->br_cons_size = count;
	br->br_prod_mask = br->br_cons_mask = count - 1;
	br->br_prod_head = br->br_cons_head = 0;
	br->br_prod_tail = br->br_cons_tail = 0;

	return br;
}

void
uk_ring_free(struct uk_ring *br, struct uk_alloc *a)
{
	uk_free(a, br);
}
