/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_XEN_START_H__
#define __UK_PLAT_XEN_START_H__

/* No secondary core start support; fake symbols will error out if used */

#define UK_PLAT_XEN_SENTRY_SYM		uk_plat_xen_sentry
#define UK_PLAT_XEN_SSTACKP_SYM		uk_plat_xen_sstackp
#define UK_PLAT_XEN_SARG_SYM		uk_plat_xen_sarg

#endif /* __UK_PLAT_XEN_START_H__ */
