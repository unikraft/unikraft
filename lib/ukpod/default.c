/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Initialization of system default PoD */

#include <uk/pod/default.h>

#if CONFIG_LIBUKPOD_DEFAULT_EAGER

struct uk_alloc *uk_pod_eager_init_ctx = __NULL;

int uk_pod_default_init(void)
{
	struct uk_alloc *expected = __NULL;

	if (unlikely(!UK_POD_DEFAULT_CTX))
		__atomic_compare_exchange_n(&uk_pod_eager_init_ctx, &expected,
					    uk_alloc_get_default(), 0,
					    __ATOMIC_RELEASE, __ATOMIC_RELAXED);
	return 0;
}

#endif /* CONFIG_LIBUKPOD_EAGER */
