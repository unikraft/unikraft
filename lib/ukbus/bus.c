/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/bus.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/init.h>

UK_LIST_HEAD(uk_bus_list);
static unsigned int bus_count;

static int uk_bus_init(struct uk_bus *b, struct uk_alloc *a);
static int uk_bus_probe(struct uk_bus *b);
static unsigned int uk_bus_init_all(struct uk_alloc *a);
static unsigned int uk_bus_probe_all(void);
static int uk_bus_lib_init(void);

void _uk_bus_register(struct uk_bus *b)
{
	UK_ASSERT(b != NULL);
	UK_ASSERT(b->probe != NULL);

	uk_pr_debug("Register bus handler: %p\n", b);
	uk_list_add_tail(&b->list, &uk_bus_list);
	++bus_count;
}

void _uk_bus_unregister(struct uk_bus *b)
{
	UK_ASSERT(b != NULL);
	UK_ASSERT(bus_count > 0);

	uk_pr_debug("Unregister bus handler: %p\n", b);
	uk_list_del_init(&b->list);
	bus_count--;
}

unsigned int uk_bus_count(void)
{
	return bus_count;
}

static int uk_bus_init(struct uk_bus *b, struct uk_alloc *a)
{
	UK_ASSERT(b != NULL);

	uk_pr_debug("Initialize bus handler %p...\n", b);
	if (!b->init)
		return 0;
	return b->init(a);
}


static int uk_bus_probe(struct uk_bus *b)
{
	UK_ASSERT(b != NULL);
	UK_ASSERT(b->probe != NULL);

	uk_pr_debug("Probe bus %p...\n", b);
	return b->probe();
}

/* Returns the number of successfully initialized device buses */
static unsigned int uk_bus_init_all(struct uk_alloc *a)
{
	struct uk_bus *b, *b_next;
	unsigned int ret = 0;
	int status = 0;

	if (uk_bus_count() == 0)
		return 0;

	uk_list_for_each_entry_safe(b, b_next, &uk_bus_list, list) {
		if ((status = uk_bus_init(b, a)) >= 0) {
			++ret;
		} else {
			uk_pr_err("Failed to initialize bus driver %p: %d\n",
				  b, status);

			/* Remove the failed driver from the list */
			_uk_bus_unregister(b);
		}
	}
	return ret;
}

/* Returns the number of successfully probed device buses */
static unsigned int uk_bus_probe_all(void)
{
	struct uk_bus *b;
	unsigned int ret = 0;

	if (uk_bus_count() == 0)
		return 0;

	uk_list_for_each_entry(b, &uk_bus_list, list) {
		if (uk_bus_probe(b) >= 0)
			++ret;
	}
	return ret;
}

static int uk_bus_lib_init(void)
{
	uk_pr_info("Initialize bus handlers...\n");
	uk_bus_init_all(uk_alloc_get_default());
	uk_pr_info("Probe buses...\n");
	uk_bus_probe_all();
	return 0;
}
uk_early_initcall_prio(uk_bus_lib_init, 0);
