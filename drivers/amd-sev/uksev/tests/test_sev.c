/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/paging.h>
#include <uk/sev.h>
#include <uk/test.h>



#ifdef CONFIG_X86_AMD64_FEAT_SEV_ES
UK_TESTCASE(uksev, test_ghcb_cpuid)
{

}
#endif


uk_testsuite_register(uksev, NULL);
