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

#ifndef __UK_BUS_PCI_H__
#define __UK_BUS_PCI_H__

#include <stddef.h>
#include <uk/arch/types.h>
#include <uk/bus.h>
#include <uk/alloc.h>
#include <uk/ctors.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure describing an ID for a PCI driver. Each driver provides a
 * table of these IDs for each device that it supports.
 *  * Derived from: lib/librte_pci/rte_pci.h
 */
struct pci_device_id {
	/**< Class ID or PCI_CLASS_ANY_ID. */
	uint32_t class_id;
	/**< Sub class ID or PCI_CLASS_ANY_ID. */
	uint32_t sub_class_id;
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
	.sub_class_id = PCI_CLASS_ANY_ID,  \
	.vendor_id = (vend),               \
	.device_id = (dev),                \
	.subsystem_vendor_id = PCI_ANY_ID, \
	.subsystem_device_id = PCI_ANY_ID

#define PCI_ANY_DEVICE_ID                  \
	.class_id = PCI_CLASS_ANY_ID,      \
	.sub_class_id = PCI_CLASS_ANY_ID,  \
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

/**
 * @brief Description of a mapped BAR.
 */
struct pci_bar_memory {
	/**< Start address of memory region */
	void *start;
	/**< Size of the memory region */
	size_t size;
};

struct pci_device {
	struct uk_list_head list;
	struct pci_device_id  id;
	struct pci_address    addr;
	struct pci_driver     *drv;
	enum pci_device_state state;

	unsigned long base;
	unsigned long irq;
	/* Memory address of the device configuration space */
	uint32_t config_addr;
};

int arch_pci_find_cap(struct pci_device *pci_dev, uint16_t vndr_id,
		      uint8_t *cap_offset);
int arch_pci_find_next_cap(struct pci_device *pci_dev, uint16_t vndr_id,
			   uint8_t curr_cap_offset,
			   uint8_t *next_cap_offset);

/**
 * @brief read a register in the configuration space header, located at @p a.
 * Reads a register, identified by @p s, and writes its contents into the @ret.
 * Type determines, how much of the data should be returned
 *
 * @param a configuration space header address
 * @param s macro suffix for the register-specific offset, mask and shift
 * @param[out] ret return variable
 * @param type return type
 *
 */
#define PCI_CONF_READ_HEADER(type, ret, a, s)					\
	do {								\
		uint32_t _conf_data;					\
		outl(PCI_CONFIG_ADDR, (a) | PCI_CONF_##s);		\
		_conf_data = ((inl(PCI_CONFIG_DATA) >> PCI_CONF_##s##_SHFT) \
			      & PCI_CONF_##s##_MASK);			\
		*(ret) = (type) _conf_data;				\
	} while (0)

/**
 * @brief Same as PCI_CONF_READ but allows for register offset, shift and mask
 * values to be set on runtime, instead of by a macro.
 *
 * @param a configuration space header address
 * @param[out] ret return variable
 * @param type return type
 * @param offset offset in bytes, from the beginning of the configuration space.
 * 		 Configuration space is 256 bytes large.
 */
#define PCI_CONF_READ_OFFSET(type, ret, a, offset, shift, mask)					\
	do {								\
		uint32_t _conf_data;					\
		outl(PCI_CONFIG_ADDR, (a) | (offset));		\
		_conf_data = ((inl(PCI_CONFIG_DATA) >> (shift)) \
			      & (mask));			\
		*(ret) = (type) _conf_data;				\
	} while (0)

/**
 * @brief write a register in the configuration space header, located at @p a.
 * Writes a register, identified by @p s.
 *
 * @param a configuration space header address
 * @param s macro suffix for the register-specific offset, mask and shift
 * @param value the value to write
 * @param type register type
 *
 */
#define PCI_CONF_WRITE_HEADER(type, a, s, value)				\
	do {									\
		uint32_t _conf_data;						\
		outl(PCI_CONFIG_ADDR, (a) | PCI_CONF_##s);			\
		_conf_data = inl(PCI_CONFIG_DATA);				\
										\
                _conf_data &= ~(PCI_CONF_##s##_MASK << PCI_CONF_##s##_SHFT);	\
                _conf_data |=							\
		    ((value) & PCI_CONF_##s##_MASK) << PCI_CONF_##s##_SHFT;	\
		outl(PCI_CONFIG_ADDR, (a) | PCI_CONF_##s);			\
		outl(PCI_CONFIG_DATA, _conf_data);				\
	} while (0)

/**
 * @brief Same as PCI_CONF_WRITE_HEADER but allows for register offset, shift
 * and mask values to be set on runtime, instead of by a macro.
 *
 * @param a configuration space header address
 * @param offset the offset of the register in the configuration space
 * @param shift the bit position of the value
 * @param mask the mask used to select the bits after the shift
 * @param value the value to write
 * @param type register type
 */
#define PCI_CONF_WRITE_OFFSET(type, a, offset, shift, mask, value)		\
	do {									\
		uint32_t _conf_data;						\
		outl(PCI_CONFIG_ADDR, (a) | (offset));				\
		_conf_data = inl(PCI_CONFIG_DATA);				\
										\
                _conf_data &= ~((mask) << (shift));				\
                _conf_data |=							\
		    ((value) & (mask)) << (shift);				\
		outl(PCI_CONFIG_ADDR, (a) | (offset));				\
		outl(PCI_CONFIG_DATA, _conf_data);				\
	} while (0)

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
static struct pci_bus_handler ph __unused;

#define PCI_INVALID_ID              (0xFFFF)
#define PCI_DEVICE_ID_MASK          (0xFFFF)

#define PCI_CONFIG_ADDR             (0xCF8)
#define PCI_CONFIG_DATA             (0xCFC)

/* 8 bits for bus number, 5 bits for devices, 3 for functions */
#define PCI_MAX_BUSES               (1 << 8)
#define PCI_MAX_DEVICES             (1 << 5)
#define PCI_MAX_FUNCTIONS           (1 << 3)

#define PCI_BUS_BIT_NBR			(8)
#define PCI_DEV_BIT_NBR			(5)
#define PCI_FN_BIT_NBR			(3)

#define PCI_BUS_SHIFT               (16)
#define PCI_DEVICE_SHIFT            (11)
#define PCI_FUNCTION_SHIFT          (8)
#define PCI_ENABLE_BIT              (1u << 31)

/* Offsets, masks and shifts for reading different registers inside a PCI
 * configuration space header.
 */
#define PCI_CONF_STATUS            (0x04)
#define PCI_CONF_STATUS_SHFT       (16)
#define PCI_CONF_STATUS_MASK       (0x0000FFFF)

#define PCI_CONF_CAP_STATUS_BIT (0x10) /* set if PCI capabilites are enabled */

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

#define PCI_HEADER_TYPE_MSB_MASK	(0x80)
#define PCI_CONF_HEADER_TYPE		(0x0C)
#define PCI_CONF_HEADER_TYPE_SHFT	(16)
#define PCI_CONF_HEADER_TYPE_MASK	(0xFF)
/*Bit 7 identifies a multi-function device*/
#define PCI_CONF_HEADER_TYPE_HEADER_TYPE_MASK (0x7F)
#define PCI_CONF_HEADER_TYPE_STANDARD	(0x0)
#define PCI_CONF_HEADER_TYPE_PCI_TO_PCI	(0x1)
#define PCI_CONF_HEADER_TYPE_CARDBUS_BRIDGE (0x2)
#define PCI_CONF_CARDBUS_BRIDGE_CAP_LIST_PTR (0x14)

#define PCI_CONF_SUBSYS_ID          (0x2c)
#define PCI_CONF_SUBSYS_ID_SHFT     (16)
#define PCI_CONF_SUBSYS_ID_MASK     (0xFFFF)

#define PCI_CONF_IRQ                (0X3C)
#define PCI_CONF_IRQ_SHFT           (0x0)
#define PCI_CONF_IRQ_MASK           (0XFF)

#define PCI_CONF_CAP_POINTER (0x34)
#define PCI_CONF_CAP_POINTER_SHFT (0x0)
/**
 * The bottom two bits are reserved and should be masked before
 * the Pointer is used to access the Configuration Space.
 */
#define PCI_CONF_CAP_POINTER_MASK (0xFC)

#define PCI_CONF_IOBAR              (0x10)
#define PCI_CONF_IOBAR_SHFT         (0x0)
#define PCI_CONF_IOBAR_MASK         (~0x3)

#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */
#define PCI_BASE_ADDRESS_1	0x14	/* 32 bits */
#define PCI_BASE_ADDRESS_2	0x18	/* 32 bits */
#define PCI_BASE_ADDRESS_3	0x1c	/* 32 bits */
#define PCI_BASE_ADDRESS_4	0x20	/* 32 bits */
#define PCI_BASE_ADDRESS_5	0x24	/* 32 bits */

#define PCI_VENDOR_ID		0x0
#define PCI_DEV_ID			0x02

#define PCI_BUS_OFFSET       16
#define PCI_SLOT_OFFSET      11
#define PCI_FUNC_OFFSET      8
#define PCI_CONFIG_ADDRESS_ENABLE   0x80000000
#define PCI_COMMAND_OFFSET   0x4
#define PCI_BUS_MASTER_BIT   0x2
#define PCI_STATUS_OFFSET    0x6
#define PCI_CLASS_REVISION   0x8
#define PCI_CLASS_OFFSET     0xb
#define PCI_SUBCLASS_OFFSET	 0xa
#define PCI_HEADER_TYPE      0xe
#define PCI_SUBSYSTEM_ID     0x2e
#define PCI_SUBSYSTEM_VID    0x2c
#define PCI_HEADER_MULTI_FUNC   0x80
#define PCI_BAR0_ADDR        0x10
#define PCI_CONFIG_SECONDARY_BUS   0x19
#define PCI_CAPABILITIES_PTR   0x34

/* Offsets inside a capability register */
#define PCI_CAP_VENDOR_ID_SHIFT	0
#define PCI_CAP_VENDOR_ID_MASK	0x000000FF
#define PCI_CAP_NEXT_SHIFT	8
#define PCI_CAP_NEXT_MASK	0x000000FC /*last two bits are reserved*/

#define PCI_HEADER_END      0xFF

#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define  PCI_COMMAND_MASTER	0x4	/* Enable bus mastering */
#define  PCI_COMMAND_SPECIAL	0x8	/* Enable response to special cycles */
#define  PCI_COMMAND_INVALIDATE	0x10	/* Use memory write and invalidate */
#define  PCI_COMMAND_VGA_PALETTE 0x20	/* Enable palette snooping */
#define  PCI_COMMAND_PARITY	0x40	/* Enable parity checking */
#define  PCI_COMMAND_WAIT	0x80	/* Enable address/data stepping */
#define  PCI_COMMAND_SERR	0x100	/* Enable SERR */
#define  PCI_COMMAND_FAST_BACK	0x200	/* Enable back-to-back writes */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx Emulation Disable */
#define PCI_COMMAND_DECODE_ENABLE	(PCI_COMMAND_MEMORY | PCI_COMMAND_IO)

/* 0x35-0x3b are reserved */
#define PCI_INTERRUPT_LINE	0x3c	/* 8 bits */
#define PCI_INTERRUPT_PIN	0x3d	/* 8 bits */
#define PCI_MIN_GNT		0x3e	/* 8 bits */
#define PCI_MAX_LAT		0x3f	/* 8 bits */

/* PCI capabilities */
#define PCI_CAP_MSIX		0x11

struct pci_driver *pci_find_driver(struct pci_device_id *id);

int pci_generic_config_read(__u8 bus, __u8 devfn,
			    int where, int size, void *val);

int pci_generic_config_write(__u8 bus, __u8 devfn,
			     int where, int size, __u32 val);
/**
 * @brief Create a memory mapping for a memory BAR
 * @param dev the device to act on
 * @param idx the index of the BAR register
 * @param mem[out] description of the mapped memory
 * @param flags the page attributes to use for mapping the memory. See the
 * 		PAGE_ATTR_PROT_* constants.
 * @return 0 if the memory was mapped successfully, a non-zero value if an error
 *   occurred.
 */
int pci_map_bar(struct pci_device *dev, __u8 idx, int attr,
		struct pci_bar_memory *mem);

/**
 * @brief Unmap a previously mapped memory BAR
 * @param dev the device to act on
 * @param idx the index of the BAR register
 * @param mem[in] description of the mapped memory
 * @return 0 if the memory was unmapped successfully, a non-zero value if an
 *   error occurred.
 */
int pci_unmap_bar(struct pci_device *dev, __u8 idx, struct pci_bar_memory *mem);

#ifdef __cplusplus
}
#endif

#endif /* __UK_BUS_PCI_H__ */
