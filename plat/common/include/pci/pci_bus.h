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
 */
/*
 * Copyright(c) 2010-2015 Intel Corporation.
 * Copyright 2013-2014 6WIND S.A.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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

#ifndef __UKPLAT_COMMON_PCI_BUS_H__
#define __UKPLAT_COMMON_PCI_BUS_H__

#include <uk/bus.h>
#include <uk/alloc.h>
#include <uk/ctors.h>

/**
 * A structure describing an ID for a PCI driver. Each driver provides a
 * table of these IDs for each device that it supports.
 *  * Derived from: lib/librte_pci/rte_pci.h
 */
struct pci_device_id {
	/**< Class ID or PCI_CLASS_ANY_ID. */
	uint32_t class_id;
	/**< Vendor ID or PCI_ANY_ID. */
	uint16_t vendor_id;
	/**< Device ID or PCI_ANY_ID. */
	uint16_t device_id;
	/**< Subsystem vendor ID or PCI_ANY_ID. */
	uint16_t subsystem_vendor_id;
	/**< Subsystem device ID or PCI_ANY_ID. */
	uint16_t subsystem_device_id;
};

/** Any PCI device identifier (vendor, device, ...) */
#define PCI_ANY_ID       (0xffff)
#define PCI_CLASS_ANY_ID (0xffffff)

/**
 * Macros used to help building up tables of device IDs
 * Derived from: lib/librte_pci/rte_pci.h
 */
#define PCI_DEVICE_ID(vend, dev)           \
	.class_id = PCI_CLASS_ANY_ID,      \
	.vendor_id = (vend),               \
	.device_id = (dev),                \
	.subsystem_vendor_id = PCI_ANY_ID, \
	.subsystem_device_id = PCI_ANY_ID

#define PCI_ANY_DEVICE_ID                  \
	.class_id = PCI_CLASS_ANY_ID,      \
	.vendor_id = PCI_ANY_ID,           \
	.device_id = PCI_ANY_ID,           \
	.subsystem_vendor_id = PCI_ANY_ID, \
	.subsystem_device_id = PCI_ANY_ID

/**
 * A structure describing the location of a PCI device.
 * Derived from: lib/librte_pci/rte_pci.h
 */
struct pci_address {
	/**< Device domain */
	uint32_t domain;
	/**< Device bus */
	uint8_t bus;
	/**< Device ID */
	uint8_t devid;
	/**< Device function. */
	uint8_t function;
};

struct pci_device;

typedef int (*pci_driver_add_func_t)(struct pci_device *);
typedef int (*pci_driver_init_func_t)(struct uk_alloc *a);

struct pci_driver {
	struct uk_list_head list;
	/**< ANY-ID terminated list of device IDs that the driver handles */
	const struct pci_device_id *device_ids;
	pci_driver_init_func_t init; /* optional */
	pci_driver_add_func_t add_dev;
};

enum pci_device_state {
	PCI_DEVICE_STATE_RESET = 0,
	PCI_DEVICE_STATE_RUNNING
};

struct pci_device {
	struct uk_list_head list;
	struct pci_device_id  id;
	struct pci_address    addr;
	struct pci_driver     *drv;
	enum pci_device_state state;

	uint16_t base;
	unsigned long irq;
};


#define PCI_REGISTER_DRIVER(b)                  \
	_PCI_REGISTER_DRIVER(__LIBNAME__, b)

#define _PCI_REGFNNAME(x, y)      x##y

#define PCI_REGISTER_CTOR(ctor)				\
	UK_CTOR_PRIO(ctor, UK_PRIO_AFTER(UK_BUS_REGISTER_PRIO))

#define _PCI_REGISTER_DRIVER(libname, b)				\
	static void						\
	_PCI_REGFNNAME(libname, _pci_register_driver)(void)		\
	{								\
		_pci_register_driver((b));				\
	}								\
	PCI_REGISTER_CTOR(_PCI_REGFNNAME(libname, _pci_register_driver))

/* Do not use this function directly: */
void _pci_register_driver(struct pci_driver *drv);

struct pci_bus_handler {
	struct uk_bus b;
	struct uk_alloc *a;
	struct uk_list_head drv_list;  /**< List of PCI drivers */
	struct uk_list_head dev_list;  /**< List of PCI devices */
};
static struct pci_bus_handler ph;

#define PCI_INVALID_ID              (0xFFFF)
#define PCI_DEVICE_ID_MASK          (0xFFFF)

#define PCI_CONFIG_ADDR             (0xCF8)
#define PCI_CONFIG_DATA             (0xCFC)

/* 8 bits for bus number, 5 bits for devices, 3 for functions */
#define PCI_MAX_BUSES               (1 << 8)
#define PCI_MAX_DEVICES             (1 << 5)
#define PCI_MAX_FUNCTIONS           (1 << 3)

#define PCI_BUS_SHIFT               (16)
#define PCI_DEVICE_SHIFT            (11)
#define PCI_FUNCTION_SHIFT          (8)
#define PCI_ENABLE_BIT              (1 << 31)

#define PCI_CONF_CLASS_ID          (0x08)
#define PCI_CONF_CLASS_ID_SHFT     (16)
#define PCI_CONF_CLASS_ID_MASK     (0xFF00)

#define PCI_CONF_VENDOR_ID          (0x00)
#define PCI_CONF_VENDOR_ID_SHFT     (0)
#define PCI_CONF_VENDOR_ID_MASK     (0x0000FFFF)

#define PCI_CONF_DEVICE_ID          (0x00)
#define PCI_CONF_DEVICE_ID_SHFT     (16)
#define PCI_CONF_DEVICE_ID_MASK     (0x0000FFFF)

#define PCI_CONF_SUBSYSVEN_ID          (0x2c)
#define PCI_CONF_SUBSYSVEN_ID_SHFT     (0)
#define PCI_CONF_SUBSYSVEN_ID_MASK     (0xFFFF)

#define PCI_CONF_SUBCLASS_ID          (0x08)
#define PCI_CONF_SUBCLASS_ID_SHFT     (16)
#define PCI_CONF_SUBCLASS_ID_MASK     (0x00FF)

#define PCI_CONF_SECONDARY_BUS          (0x18)
#define PCI_CONF_SECONDARY_BUS_SHFT     (0)
#define PCI_CONF_SECONDARY_BUS_MASK     (0xFF00)

#define PCI_HEADER_TYPE_MSB_MASK   (0x80)
#define PCI_CONF_HEADER_TYPE       (0x00)
#define PCI_CONF_HEADER_TYPE_SHFT  (16)
#define PCI_CONF_HEADER_TYPE_MASK  (0xFF)

#define PCI_CONF_SUBSYS_ID          (0x2c)
#define PCI_CONF_SUBSYS_ID_SHFT     (16)
#define PCI_CONF_SUBSYS_ID_MASK     (0xFFFF)

#define PCI_CONF_IRQ                (0X3C)
#define PCI_CONF_IRQ_SHFT           (0x0)
#define PCI_CONF_IRQ_MASK           (0XFF)

#define PCI_CONF_IOBAR              (0x10)
#define PCI_CONF_IOBAR_SHFT         (0x0)
#define PCI_CONF_IOBAR_MASK         (~0x3)

struct pci_driver *pci_find_driver(struct pci_device_id *id);

#endif /* __UKPLAT_COMMON_PCI_BUS_H__ */
