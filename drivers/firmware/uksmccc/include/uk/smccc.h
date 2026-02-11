/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SMCCC_H__
#define __UK_SMCCC_H__

#include <uk/config.h>
#include <uk/arch/types.h>

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

struct uk_smccc_config {
	__u64 conduit;
	__u64 version;
};

struct uk_smccc_args {
	__u64 a0;
	__u64 a1;
	__u64 a2;
	__u64 a3;
	__u64 a4;
	__u64 a5;
	__u64 a6;
	__u64 a7;
	__u64 a8;
	__u64 a9;
	__u64 a10;
	__u64 a11;
	__u64 a12;
	__u64 a13;
	__u64 a14;
	__u64 a15;
	__u64 a16;
	__u64 a17;
};

typedef void (*uk_smccc_conduit_func)(struct uk_smccc_args *args);

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
void uk_smccc_init(struct uk_smccc_config *config);

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
void uk_smccc_smc(struct uk_smccc_args *args);

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
void uk_smccc_hvc(struct uk_smccc_args *args);

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
void uk_smccc_invoke(struct uk_smccc_args *args);

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
__u64 uk_smccc_version(void);

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
__u64 uk_smccc_arch_features(__u64 fid);

/**
 * Obtains the SoC ID, as defined by the silicon provider
 *
 * @param type SMCCC_ARCH_SOC_VERSION or SMCCC_ARCH_SOC_REVISION
 * @return SoC ID on success, NOT_IMPLEMENTED or INVALID_PARAMETER on error
 */
__u64 uk_smccc_arch_soc_id(__u64 type);

/**
 * Requests the firmware to apply workaround for CVE-2017-5715
 *
 * This function should be called on every PE that requires a firmware
 * mitigation for CVE-2017-5715. Should be called only when feature
 * discovery returns a non-negative value. For more information see
 * ARM DEN0028C Sect. 7.5
 */
void uk_smccc_arch_workaround_1(void);

/**
 * Requests the firmware to apply workaround for CVE-2018-3639.
 *
 * This function should be called on every PE that requires a firmware
 * mitigation for CVE-2018-3639. Should be called only when feature
 * discovery returns a non-negative value. For more information see
 * ARM DEN0028C Sect. 7.6
 */
void uk_smccc_arch_workaround_2(void);

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
__u64 uk_smccc_svc_query(__u64 service, __u64 type);

#endif /* __ASSEMBLY__ */

#endif /* __UK_SMCCC_H__ */
