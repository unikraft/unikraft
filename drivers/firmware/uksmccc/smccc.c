/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/smccc.h>

static uk_smccc_conduit_func conduit = uk_smccc_smc;
static __u64 _version = SMCCC_VERSION_1_2;

__u64 uk_smccc_version(void)
{
	struct uk_smccc_args args = {0};

	args.a0 = SMCCC_FID_SMCCC_VERSION;

	uk_smccc_invoke(&args);

	return args.a0;
}

__u64 uk_smccc_arch_features(__u64 fid)
{
	struct uk_smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_FEATURES;
	args.a1 = fid;

	uk_smccc_invoke(&args);

	return args.a0;
}

__u64 uk_smccc_arch_soc_id(__u64 type)
{
	struct uk_smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_SOC_ID;
	args.a1 = type;

	uk_smccc_invoke(&args);

	return args.a0;
}

void uk_smccc_arch_workaround_1(void)
{
	struct uk_smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_WORKAROUND_1;

	uk_smccc_invoke(&args);
}

void uk_smccc_arch_workaround_2(void)
{
	struct uk_smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_WORKAROUND_2;

	uk_smccc_invoke(&args);
}

__u64 uk_smccc_svc_query(__u64 service, __u64 type)
{
	struct uk_smccc_args args = {0};

	if (_version >= SMCCC_VERSION_1_2 && type == SMCCC_QUERY_CALL_COUNT)
		return SMCCC_NOT_SUPPORTED;

	if (_version >= SMCCC_VERSION_1_2 && service == SMCCC_SVC_ARCH)
		return SMCCC_NOT_SUPPORTED;

	args.a0 = SMCCC_FID(SMCCC_TYPE_FAST, SMCCC_CC_32,
			    service, type);

	uk_smccc_invoke(&args);

	return args.a0;
}

void uk_smccc_init(struct uk_smccc_config *config)
{
	if (config->conduit == SMCCC_CONDUIT_HVC)
		conduit = uk_smccc_hvc;
	else
		conduit = uk_smccc_smc;

	_version = config->version;
}

void uk_smccc_invoke(struct uk_smccc_args *args)
{
#if !CONFIG_FPSIMD
	if (_version >= SMCCC_VERSION_1_3) {
		/* Don't preserve the SVE state when FPSIMD is disabled.
		 * Since UK doesn't have tasks we consider SVE registers
		 * to contain live state when FPSIMD is enabled.
		 * See ARM DEN0028D Sect. 2.5 & ARM DEN0091.
		 */
		args->a0 |= (SMCCC_SVE_STATE_OFF << SMCCC_SVE_STATE_SHIFT);
	}
#endif /* !CONFIG_FPSIMD */
	conduit(args);
}
