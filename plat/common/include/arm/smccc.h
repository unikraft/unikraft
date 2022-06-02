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
#ifndef __UKARCH_SMCCC_H__
#define __UKARCH_SMCCC_H__

#include <uk/config.h>

/**
 * Implements SMCCC up to v1.3 (ARM DEN 0028D)
 */

#define SMCCC_VERSION_1_0	0x10000U
#define SMCCC_VERSION_1_1	0x10001U
#define SMCCC_VERSION_1_2	0x10002U
#define SMCCC_VERSION_1_3	0x10003U
#define SMCCC_VERSION_MAX	SMCCC_VERSION_1_3

#define SMCCC_VERSION_MAJOR_MASK  0x7fffU
#define SMCCC_VERSION_MAJOR_SHIFT 16U
#define SMCCC_VERSION_MINOR_MASK  0xffffU
#define SMCCC_VERSION_MINOR_SHIFT 0U

#define	SMCCC_CONDUIT_SMC	0U
#define	SMCCC_CONDUIT_HVC	1U

#define SMCCC_TYPE_YIELDING	0U
#define SMCCC_TYPE_FAST		1U

#define SMCCC_CC_32		0U
#define SMCCC_CC_64		1U

#define SMCCC_TYPE_MASK		0x80000000U
#define SMCCC_TYPE_SHIFT	31U
#define SMCCC_CALL_CONV_MASK	0x40000000U
#define SMCCC_CALL_CONV_SHIFT	30U
#define SMCCC_SERVICE_MASK	0x3f000000U
#define SMCCC_SERVICE_SHIFT	24U
#define SMCCC_SVE_STATE_MASK	0x00010000U
#define SMCCC_SVE_STATE_SHIFT	16U
#define SMCCC_FNUM_MASK		0x0000ffffU
#define SMCCC_FNUM_SHIFT	0U

#define SMCCC_SVE_STATE_ON	0U
#define SMCCC_SVE_STATE_OFF	1U

#define SMCCC_FID(_type, _cc, _svc, _fnum)				\
	((((_type) << SMCCC_TYPE_SHIFT) & SMCCC_TYPE_MASK) |		\
	 (((_cc) << SMCCC_CALL_CONV_SHIFT) & SMCCC_CALL_CONV_MASK) |	\
	 (((_svc) << SMCCC_SERVICE_SHIFT) & SMCCC_SERVICE_MASK) |	\
	 (((_fnum) << SMCCC_FNUM_SHIFT) & SMCCC_FNUM_MASK))

/* SMC & HVC Services */
#define	SMCCC_SVC_ARCH			0U
#define	SMCCC_SVC_CPU			1U
#define	SMCCC_SVC_SIP			2U
#define	SMCCC_SVC_OEM			3U
#define	SMCCC_SVC_STD_SEC		4U
#define	SMCCC_SVC_STD_HYP		5U
#define	SMCCC_SVC_VENDOR_HYP		6U
#define	SMCCC_SVC_TRUSTED_APP		48U
#define	SMCCC_SVC_TRUSTED_APP_END	48U
#define	SMCCC_SVC_TRUSTED_OS		50U
#define	SMCCC_SVC_TRUSTED_OD_END	63U

#define SMCCC_SUCCESS		0UL
#define SMCCC_NOT_SUPPORTED	-1
#define SMCCC_NOT_REQUIRED	-2
#define SMCCC_INVALID_PARAMETER	-3

/* Arm Architecture Calls */
#define SMCCC_FID_SMCCC_VERSION		0x80000000U
#define SMCCC_FID_ARCH_FEATURES		0x80000001U
#define SMCCC_FID_ARCH_SOC_ID		0x80000002U
#define SMCCC_FID_ARCH_WORKAROUND_1	0x80008000U
#define SMCCC_FID_ARCH_WORKAROUND_2	0x80007fffU

#define SMCCC_ARCH_SOC_VERSION		0U
#define SMCCC_ARCH_SOC_REVISION		1U

/* General Service Queries */
#define	SMCCC_QUERY_CALL_COUNT	0xff00U
#define	SMCCC_QUERY_CALL_UID	0xff01U
#define	SMCCC_QUERY_REVISION	0xff03U

#ifndef __ASSEMBLY__

struct smccc_config {
	unsigned long conduit;
	unsigned long version;
};

struct smccc_args {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
#ifdef CONFIG_ARCH_ARM_64
	unsigned long a8;
	unsigned long a9;
	unsigned long a10;
	unsigned long a11;
	unsigned long a12;
	unsigned long a13;
	unsigned long a14;
	unsigned long a15;
	unsigned long a16;
	unsigned long a17;
#endif
};

typedef void (*smccc_conduit_fn_t)(struct smccc_args *args);

/**
 * Sets the conduit and version to use when issuing SMCCC calls.
 *
 * Platforms must derive the information using platform-specific
 * methods (device tree / ACPI / other). For version discovery
 * see also smccc_version().
 *
 * If this function is not called, the default values used are:
 * - conduit: SMC
 * - version: SMCCC_VERSION_1_2
 *
 * @param config Pointer to config struct
 */
void smccc_init(struct smccc_config *config);

/**
 * Issues an SMC.
 *
 * Upon completion args are updated with return values
 * depending on the Function ID and the SMCCC version
 * implemented.
 *
 * @param args Arguments to be passed to the SMC call.
 *             Updated with return values upon completion.
 */
void smccc_smc(struct smccc_args *args);

/**
 * Issues an HVC
 *
 * Upon completion args are updated with return values
 * depending on the Function ID and the SMCCC version
 * implemented.
 *
 * @param args Arguments to be passed to the HVC call.
 *             Updated with return values upon completion.
 */
void smccc_hvc(struct smccc_args *args);

/**
 * Issue an SMCCC call using the selected conduit
 *
 * Upon completion args are updated with return values
 * depending on the Function ID and the SMCCC version
 * implemented.
 *
 * @param args Arguments to be passed to the conduit call.
 *             Updated with return values upon completion.
 */
void smccc_invoke(struct smccc_args *args);

/**
 * Requests the implemented SMCCC version
 *
 * This function is optional on SMCCC v1.0 and mandatory
 * from SMCCC v1.1.
 *
 * Notice: Before calling this function one must make sure
 * that it is safe to do so. For methods to obtain this information
 * see Appendix B of ARM-DEN0028C.
 *
 * @return 32-bit version or SMCCC_NOT_SUPPORTED
 */
unsigned long smccc_version(void);

/**
 * Requests information on the implementation of an
 * Architecture Service Function
 *
 * This function is optional on SMCCC v1.0 and mandatory
 * from SMCCC v1.1.
 *
 * @param fid Function ID of feature to query
 * @return SMCCC_SUCCESS indicates implemented.
 *          Other positive values indicated implemented and contain
 *          additional feature-specific flags.
 *          A negative value indicates not implemented.
 */
unsigned long smccc_arch_features(unsigned long fid);

/**
 * Obtains the SoC ID, as defined by the silicon provider
 *
 * @param type SMCCC_ARCH_SOC_VERSION or SMCCC_ARCH_SOC_REVISION
 * @return SoC ID on success, NOT_IMPLEMENTED or INVALID_PARAMETER on error
 */
unsigned long smccc_arch_soc_id(unsigned long type);

/**
 * Requests the firmware to apply workaround for CVE-2017-5715
 *
 * This function should be called on every PE that requires a firmware
 * mitigation for CVE-2017-5715. Should be called only when feature
 * discovery returns a non-negative value. For more information see
 * ARM DEN0028C Sect. 7.5
 */
void smccc_arch_workaround_1(void);

/**
 * Requests the firmware to apply workaround for CVE-2018-3639.
 *
 * This function should be called on every PE that requires a firmware
 * mitigation for CVE-2018-3639. Should be called only when feature
 * discovery returns a non-negative value. For more information see
 * ARM DEN0028C Sect. 7.6
 */
void smccc_arch_workaround_2(void);

/**
 * Performs a general service query
 *
 * @param service Valid services:
 * - SMCCC_SVC_ARCH
 * - SMCCC_SVC_CPU
 * - SMCCC_SVC_SIP
 * - SMCCC_SVC_OEM
 * - SMCCC_SVC_STD_SEC
 * - SMCCC_SVC_STD_HYP
 * - SMCCC_SVC_VENDOR_HYP
 * - SMCCC_SVC_TRUSTED_OS_END
 * @param type Valid query types:
 * - SMCCC_QUERY_CALL_COUNT (deprecated from SMCCC v1.2)
 * - SMCCC_QUERY_CALL_UID
 * - SMCCC_QUERY_REVISION
 * @return Query-specific value
 */
unsigned long smccc_svc_query(unsigned long service, unsigned long type);

#endif /* __ASSEMBLY__ */

#endif /* __UKARCH_SMCCC_H__ */
