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
#include <uk/ctors.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_bus;
extern struct uk_list_head uk_bus_list;

typedef int (*uk_bus_init_func_t)(struct uk_alloc *a);
typedef int (*uk_bus_probe_func_t)(void);

struct uk_bus {
	struct uk_list_head list;
	uk_bus_init_func_t init; /**< Initialize bus handler (optional) */
	uk_bus_probe_func_t probe; /**< Probe for devices attached to the bus */
};

/* Returns the number of registered buses */
unsigned int uk_bus_count(void);

/* Do not use this function directly: */
void _uk_bus_register(struct uk_bus *b);
/* Do not use this function directly: */
void _uk_bus_unregister(struct uk_bus *b);

/* registers a bus driver to the bus system */
#define UK_BUS_REGISTER(b) \
	_UK_BUS_REGISTER(__LIBNAME__, b)

#define _UK_BUS_REGFNNAME(x, y)      x##y

#define _UK_BUS_REGISTER_CTOR(CTOR)  \
	UK_CTOR_FUNC(0, CTOR)


#define _UK_BUS_REGISTER(libname, b)				\
	static void						\
	_UK_BUS_REGFNNAME(libname, _uk_bus_register)(void)	\
	{							\
		_uk_bus_register((b));				\
	}							\
	_UK_BUS_REGISTER_CTOR(_UK_BUS_REGFNNAME(libname, _uk_bus_register))

#ifdef __cplusplus
}
#endif

#endif /* __UK_BUS__ */
