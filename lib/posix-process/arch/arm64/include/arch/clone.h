/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PROCESS_H__
#error Do not include this header directly
#endif

#include <uk/syscall.h>
#include <uk/thread.h>

#if !__ASSEMBLY__

void clone_setup_child_ctx(struct ukarch_execenv *pexecenv,
			   struct uk_thread *child, __uptr sp);

#endif /* !__ASSEMBLY__ */
