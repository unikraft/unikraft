/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PS2_H__
#define __UK_PS2_H__

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPS2_SYSRESET
/**
 * Trigger a CPU reset through the PS/2 Controller.
 */
__isr __noreturn void uk_ps2_cpu_reset(void);
#endif /* CONFIG_LIBUKPS2_SYSRESET */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /*  __UK_PS2_H__ */
