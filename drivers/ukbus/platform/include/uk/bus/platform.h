/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Jia He <justin.he@arm.com>
 *
 * Copyright (c) 2020, Arm Ltd. All rights reserved.
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
 */

#ifndef __UK_BUS_PLATFORM_H__
#define __UK_BUS_PLATFORM_H__

#include <stdint.h>
#include <uk/bus.h>
#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure describing an ID for a Platform driver. Each driver provides a
 * table of these IDs for each device that it supports.
 */
#define PLATFORM_DEVICE_ID_START (0x100)
#define VIRTIO_MMIO_ID		(PLATFORM_DEVICE_ID_START)
#define GEN_PCI_ID		(PLATFORM_DEVICE_ID_START + 1)

#define PLATFORM_DEVICE_ID_END (GEN_PCI_ID + 1)

#define UK_MAX_VIRTIO_MMIO_DEVICE (0x2)

struct pf_device_id {
	uint16_t device_id;
};

struct device_match_table {
	const char		*compatible;
	struct pf_device_id	*id;
};

struct pf_device;

typedef int (*pf_driver_init_func_t)(struct uk_alloc *a);
typedef int (*pf_driver_add_func_t)(struct pf_device *);
typedef int (*pf_driver_probe_func_t)(struct pf_device *);
typedef int (*pf_driver_match_func_t)(const char *);

struct pf_driver {
	UK_TAILQ_ENTRY(struct pf_driver) next;
	const struct pf_device_id *device_ids;
	pf_driver_init_func_t init; /* optional */
	pf_driver_probe_func_t probe;
	pf_driver_add_func_t add_dev;
	pf_driver_match_func_t match;
};
UK_TAILQ_HEAD(pf_driver_list, struct pf_driver);

enum pf_device_state {
	PF_DEVICE_STATE_RESET = 0,
	PF_DEVICE_STATE_RUNNING
};

struct pf_device {
	UK_TAILQ_ENTRY(struct pf_device) next; /**< used by pf_bus_handler */
	struct pf_device_id  id;
	struct pf_driver     *drv;
	enum pf_device_state state;

	int fdt_offset;	/* The start offset of fdt node for device */
	uint64_t base;
	size_t size;
	unsigned long irq;
};
UK_TAILQ_HEAD(pf_device_list, struct pf_device);


#define PF_REGISTER_DRIVER(b)                  \
	_PF_REGISTER_DRIVER(__LIBNAME__, b)

#define _PF_REGFNNAME(x, y)      x##y

#define PF_REGISTER_CTOR(CTOR)				\
		UK_CTOR_FUNC(1, CTOR)

#define _PF_REGISTER_DRIVER(libname, b)				\
	static void						\
	_PF_REGFNNAME(libname, _pf_register_driver)(void)		\
	{								\
		_pf_register_driver((b));				\
	}								\
	PF_REGISTER_CTOR(_PF_REGFNNAME(libname, _pf_register_driver))

/* Do not use this function directly: */
void _pf_register_driver(struct pf_driver *drv);

#ifdef __cplusplus
}
#endif

#endif /* __UK_BUS_PLATFORM_H__*/
