/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <kvm-arm64/image.h>

.section ".linux_arm64_hdr","a"
	add	x13, x18, #0x16			/* code0 */
	b	qemu_virt_entry			/* code1 */
	.quad	LINUX_ARM64_HDR_LOAD_OFFS	/* text_offset */
	.quad	LINUX_ARM64_HDR_IMAGE_SIZE	/* image_size */
	.quad	LINUX_ARM64_HDR_FLAGS		/* kernel flags */
	.quad	0				/* res2 */
	.quad	0				/* res3 */
	.quad	0				/* res4 */
	.ascii	LINUX_ARM64_HDR_MAGIC		/* magic */
	.long	0x40				/* offset to PE header */
