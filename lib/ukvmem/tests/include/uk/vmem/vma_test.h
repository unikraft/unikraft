/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Utilities for implementing VMA unittests */

#ifndef __UK_VMEM_VMA_TEST_H__
#define __UK_VMEM_VMA_TEST_H__

#include <uk/nofault.h>
#include <uk/vmem.h>

#define uk_vmem_test_bug_on(cond)					\
	do {								\
		if (unlikely(cond))					\
			UK_CRASH("'%s' during test execution.\n",	\
				 STRINGIFY(cond));			\
	} while (0)

/* Setup & teardown of test VAS */
struct uk_vas *uk_vmem_test_vas_init(void);
void uk_vmem_test_vas_clean(struct uk_vas *vas);

/* Expected contents of VMA; checked during test */
struct uk_vmem_test_vma {
	__vaddr_t start;
	__vaddr_t end;
	unsigned long attr;
};

/**
 * Check the state of `vas` against expected VMAs in `vmas[num]`.
 *
 * @return 0 on success, < 0 on failure.
 */
int uk_vmem_test_check_vas(struct uk_vas *vas,
			   struct uk_vmem_test_vma *vmas, __sz num);

/* Convenience functions for probing memory without faulting. */

static inline __sz uk_vmem_test_probe_r(__vaddr_t vaddr, __sz len)
{
	return uk_nofault_probe_r(vaddr, len, UK_NOFAULTF_CONTINUE);
}

static inline __sz uk_vmem_test_probe_rw(__vaddr_t vaddr, __sz len)
{
	return uk_nofault_probe_rw(vaddr, len, UK_NOFAULTF_CONTINUE);
}

static inline __sz uk_vmem_test_probe_r_nopage(__vaddr_t vaddr, __sz len)
{
	return uk_nofault_probe_r(vaddr, len, UK_NOFAULTF_CONTINUE |
					      UK_NOFAULTF_NOPAGING);
}

static inline __sz uk_vmem_test_probe_rw_nopage(__vaddr_t vaddr, __sz len)
{
	return uk_nofault_probe_rw(vaddr, len, UK_NOFAULTF_CONTINUE |
					       UK_NOFAULTF_NOPAGING);
}

#endif /* __UK_VMEM_VMA_TEST_H__ */
