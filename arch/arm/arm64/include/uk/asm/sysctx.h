/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_CTX_H__
#error Do not include this header directly
#endif

#define UKARCH_SYSCTX_SIZE			16

#if !__ASSEMBLY__

#include <uk/essentials.h>

/* Architecture specific system context */
struct ukarch_sysctx {
	/* AAPCS's TLS pointer register */
	__uptr tpidr_el0;

	__u8 pad[8];	/* Make sure we are a multiple of 16 bytes */
};

UK_CTASSERT(sizeof(struct ukarch_sysctx) == UKARCH_SYSCTX_SIZE);

/**
 * Get the TLS pointer from system register context given as argument
 *
 * @param sysctx
 *   The system register context whose TLS pointer to get.
 * @return
 *   The TLS pointer stored in the system register context.
 */
__uptr ukarch_sysctx_get_tlsp(struct ukarch_sysctx *sysctx);

/**
 * Set the TLS pointer from system register context given as argument to the
 * pointer given as argument.
 *
 * @param sysctx
 *   The system register context whose TLS pointer to set.
 * @param tlsp
 *   The TLS pointer to store in the system register context.
 */
void ukarch_sysctx_set_tlsp(struct ukarch_sysctx *sysctx, __uptr tlsp);

/**
 * Store the current system context register state into the system context
 * given as argument. Stores TLS pointer (TPIDR_EL0).
 *
 * @param sysctx
 *   The system register context to store state in.
 */
void ukarch_sysctx_store(struct ukarch_sysctx *sysctx);

/**
 * Load the system context register state from the system context
 * given as argument. Loads TLS pointer (MSR_FS_BASE) into its respective
 * system register.
 *
 * @param sysctx
 *   The system register context to load state from.
 */
void ukarch_sysctx_load(struct ukarch_sysctx *sysctx);

#endif /* !__ASSEMBLY__ */
