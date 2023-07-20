/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __KVM_ARM64_IMAGE_H__
#define __KVM_ARM64_IMAGE_H__

#if defined(CONFIG_KVM_VMM_QEMU)
#define RAM_BASE_ADDR 0x40000000
#elif defined(CONFIG_KVM_VMM_FIRECRACKER)
#define RAM_BASE_ADDR 0x80000000
#endif /* CONFIG_KVM_VMM_FIRECRACKER  */

#define DTB_RESERVED_SIZE 0x100000

#define LINUX_ARM64_HDR_SIZE		64
#define LINUX_ARM64_HDR_MAGIC		"ARM\x64"
#define LINUX_ARM64_HDR_IMAGE_SIZE	_uk_image_size
#define LINUX_ARM64_HDR_LOAD_OFFS	_uk_load_offs
#define LINUX_ARM64_HDR_FLAGS_BE	0
#define LINUX_ARM64_HDR_FLAGS_LE	1
#define LINUX_ARM64_HDR_FLAGS_4K	(1 << 1)
#define LINUX_ARM64_HDR_FLAGS_16K	(2 << 1)
#define LINUX_ARM64_HDR_FLAGS_64K	(3 << 1)
#define LINUX_ARM64_HDR_FLAGS					\
	(LINUX_ARM64_HDR_FLAGS_LE | LINUX_ARM64_HDR_FLAGS_4K)

#define LINUX_ARM64_HDR_SYMBOLS					\
	_uk_image_size = _end - _base_addr;			\
	_uk_load_offs  = _base_addr - _start_ram_addr - 64;	\
	_linux_arm64_hdr_base = _start_ram_addr + DTB_RESERVED_SIZE - 64;

#define LINUX_ARM64_HDR_SECTION					\
	.linux_arm64_hdr :					\
	{							\
		KEEP(*(.linux_arm64_hdr))			\
	} :headers

#endif /*  __KVM_ARM64_IMAGE_H__ */
