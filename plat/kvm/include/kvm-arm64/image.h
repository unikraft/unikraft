/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __KVM_ARM64_IMAGE_H__
#define __KVM_ARM64_IMAGE_H__

#if CONFIG_KVM_VMM_QEMU
#define RAM_BASE_ADDR 0x40000000
#elif CONFIG_KVM_VMM_FIRECRACKER
#define RAM_BASE_ADDR 0x80000000
#endif /* CONFIG_KVM_VMM_FIRECRACKER  */

#define DTB_RESERVED_SIZE 0x100000

#endif /*  __KVM_ARM64_IMAGE_H__ */
