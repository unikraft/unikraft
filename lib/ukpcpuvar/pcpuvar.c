/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/pcpuvar.h>

/** Hardware CPU ID for the current CPU */
__uk_pcpuvar __u64 uk_pcpuvar_cpu_id;

/** Linear index of the current CPU in the per-CPU array */
__uk_pcpuvar __u64 uk_pcpuvar_cpu_idx;

char *const volatile _uk_pcpuvar_tmpl_size_ptr = _uk_pcpuvar_tmpl_size;
