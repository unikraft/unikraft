/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Michalis Pappas <michalis.pappas@opensynergy.com>
 *
 * Copyright (c) 2021, OpenSynergy GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <arm/smccc.h>

static smccc_conduit_fn_t conduit = smccc_smc;
static unsigned long _version = SMCCC_VERSION_1_2;

unsigned long smccc_version(void)
{
	struct smccc_args args = {0};

	args.a0 = SMCCC_FID_SMCCC_VERSION;

	smccc_invoke(&args);

	return args.a0;
}

unsigned long smccc_arch_features(unsigned long fid)
{
	struct smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_FEATURES;
	args.a1 = fid;

	smccc_invoke(&args);

	return args.a0;
}

unsigned long smccc_arch_soc_id(unsigned long type)
{
	struct smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_SOC_ID;
	args.a1 = type;

	smccc_invoke(&args);

	return args.a0;
}

void smccc_arch_workaround_1(void)
{
	struct smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_WORKAROUND_1;

	smccc_invoke(&args);
}

void smccc_arch_workaround_2(void)
{
	struct smccc_args args = {0};

	args.a0 = SMCCC_FID_ARCH_WORKAROUND_2;

	smccc_invoke(&args);
}

unsigned long smccc_svc_query(unsigned long service, unsigned long type)
{
	struct smccc_args args = {0};

	if (_version >= SMCCC_VERSION_1_2 && type == SMCCC_QUERY_CALL_COUNT)
		return SMCCC_NOT_SUPPORTED;

	if (_version >= SMCCC_VERSION_1_2 && service == SMCCC_SVC_ARCH)
		return SMCCC_NOT_SUPPORTED;

	args.a0 = SMCCC_FID(SMCCC_TYPE_FAST, SMCCC_CC_32,
			    service, type);

	smccc_invoke(&args);

	return args.a0;
}

void smccc_init(struct smccc_config *config)
{
	if (config->conduit == SMCCC_CONDUIT_HVC)
		conduit = smccc_hvc;
	else
		conduit = smccc_smc;

	_version = config->version;
}

void smccc_invoke(struct smccc_args *args)
{
#ifndef CONFIG_FPSIMD
	if (_version >= SMCCC_VERSION_1_3) {
		/* Don't preserve the SVE state when FPSIMD is disabled.
		 * Since UK doesn't have tasks we consider SVE registers
		 * to contain live state when FPSIMD is enabled.
		 * See ARM DEN0028D Sect. 2.5 & ARM DEN0091.
		 */
		args->a0 |= (SMCCC_SVE_STATE_OFF << SMCCC_SVE_STATE_SHIFT);
	}
#endif
	conduit(args);
}
