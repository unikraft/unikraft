/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UK_BUS__
#define __UK_BUS__

#include <stddef.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/plat/ctors.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_bus;
UK_TAILQ_HEAD(uk_bus_list, struct uk_bus);
extern struct uk_bus_list uk_bus_list;

typedef int (*uk_bus_init_func_t)(struct uk_alloc *a);
typedef int (*uk_bus_probe_func_t)(void);

struct uk_bus {
	UK_TAILQ_ENTRY(struct uk_bus) next;
	uk_bus_init_func_t init; /**< Initialize bus handler (optional) */
	uk_bus_probe_func_t probe; /**< Probe for devices attached to the bus */
};

#define UK_BUS_LIST_FOREACH(b)			\
	UK_TAILQ_FOREACH(b, &uk_bus_list, next)

#define UK_BUS_LIST_FOREACH_SAFE(b, b_next)	\
	UK_TAILQ_FOREACH_SAFE(b, &uk_bus_list, next, b_next)

/* Returns the number of registered buses */
unsigned int uk_bus_count(void);

/* Do not use this function directly: */
void _uk_bus_register(struct uk_bus *b);
/* Do not use this function directly: */
void _uk_bus_unregister(struct uk_bus *b);

/* Initializes a bus driver */
int uk_bus_init(struct uk_bus *b, struct uk_alloc *a);

/* Scan for devices on the bus */
int uk_bus_probe(struct uk_bus *b);

/* Returns the number of successfully initialized device buses */
static inline unsigned int uk_bus_init_all(struct uk_alloc *a)
{
	struct uk_bus *b, *b_next;
	unsigned int ret = 0;
	int status = 0;

	if (uk_bus_count() == 0)
		return 0;

	UK_BUS_LIST_FOREACH_SAFE(b, b_next) {
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
static inline unsigned int uk_bus_probe_all(void)
{
	struct uk_bus *b;
	unsigned int ret = 0;

	if (uk_bus_count() == 0)
		return 0;

	UK_BUS_LIST_FOREACH(b) {
		if (uk_bus_probe(b) >= 0)
			++ret;
	}
	return ret;
}

/* registers a bus driver to the bus system */
#define UK_BUS_REGISTER(b) \
	_UK_BUS_REGISTER(__LIBNAME__, b)

#define _UK_BUS_REGFNNAME(x, y)      x##y

#define _UK_BUS_REGISTER(libname, b)				\
	static void __constructor_prio(102)			\
	_UK_BUS_REGFNNAME(libname, _uk_bus_register)(void)	\
	{							\
		_uk_bus_register((b));				\
	}

#ifdef __cplusplus
}
#endif

#endif /* __UK_BUS__ */
