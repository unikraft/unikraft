/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __KVM_X86_SYSCALL_H__
#define __KVM_X86_SYSCALL_H__

#include <uk/config.h>

#if CONFIG_HAVE_SYSCALL && !__ASSEMBLY__
int ukplat_x86_64_syscall_init_cpu(void);
#endif /* CONFIG_HAVE_SYSCALL && !__ASSEMBLY__ */

#endif /* __KVM_X86_SYSCALL_H__ */
