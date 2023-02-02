/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __PLAT_CMN_BOOTINFO_H__
#define __PLAT_CMN_BOOTINFO_H__

#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/plat/common/memory.h>

/** Unikraft boot info */
struct ukplat_bootinfo {
	/** Magic value to recognize Unikraft boot info */
#define UKPLAT_BOOTINFO_MAGIC			0xb007b0b0 /* Boot Bobo */
	__u32 magic;

	/** Version of the boot info */
#define UKPLAT_BOOTINFO_VERSION			0x01
	__u8 version;
	__u8 _pad0[3];

	/** Null-terminated boot loader identifier */
	char bootloader[16];

	/** Null-terminated boot protocol identifier */
	char bootprotocol[16];

	/** Address of null-terminated kernel command line */
	__u64 cmdline;

	/**
	 * List of memory regions. Must be the last member as the
	 * memory regions directly follow this boot information structure
	 */
	struct ukplat_memregion_list mrds;
} __packed __align(__SIZEOF_LONG__);

UK_CTASSERT(sizeof(struct ukplat_bootinfo) == 56);

#ifdef CONFIG_UKPLAT_MEMRNAME
#if __SIZEOF_LONG__ == 8
UK_CTASSERT(sizeof(struct ukplat_memregion_desc) == 64);
#else /* __SIZEOF_LONG__ == 8 */
UK_CTASSERT(sizeof(struct ukplat_memregion_desc) == 52);
#endif /* __SIZEOF_LONG__ != 8 */
#else /* CONFIG_UKPLAT_MEMRNAME */
#if __SIZEOF_LONG__ == 8
UK_CTASSERT(sizeof(struct ukplat_memregion_desc) == 32);
#else /* __SIZEOF_LONG__ == 8 */
UK_CTASSERT(sizeof(struct ukplat_memregion_desc) == 16);
#endif /* __SIZEOF_LONG__ != 8 */
#endif /* !CONFIG_UKPLAT_MEMRNAME */

#define UKPLAT_BOOTINFO_SIZE(mrd_capacity)				\
	(sizeof(struct ukplat_bootinfo) +				\
	 mrd_capacity * sizeof(struct ukplat_memregion_desc))

/**
 * Returns a pointer to the boot information
 */
struct ukplat_bootinfo *ukplat_bootinfo_get(void);

/**
 * Sets new boot information
 *
 * @param bi
 *   Pointer to new boot information. The structure is not copied and
 *   must remain accessible after the call.
 */
void ukplat_bootinfo_set(struct ukplat_bootinfo *bi);

/**
 * Prints the boot information to the kernel console using informational level
 */
void ukplat_bootinfo_print(void);

#endif /* __PLAT_CMN_BOOTINFO_H__ */
