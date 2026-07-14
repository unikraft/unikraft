/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

__uk_pcpuvar __uptr uk_plat_native_auxsp;

void uk_plat_native_set_auxsp(__uptr auxsp)
{
	uk_pcpuvar_current_set(uk_plat_native_auxsp, auxsp);
}
