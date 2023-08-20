/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_RSI_H__
#define __UK_RSI_H__

#include <uk/asm/rsi.h>

#ifdef __cplusplus__
extern "C"
{
#endif

extern __u64 uk_rsi_unprotected_mask;
#define PTE_RME_UNPROTECTED_BIT ((uk_rsi_unprotected_mask))

struct rsi_realm_config {
	__u64 ipa_width;	/* IPA width in bits */
	__u8 hash_algo;		/* Hash algorithm */
};

struct rsi_host_call_data {
	__u16 imm;	/* Immediate value */
	__u64 gprs[31];	/* Registers */
};

/**
 * Continue the operation to retrieve an attestation token.
 *
 * @param addr IPA of the Granule to which the token will be written
 * @param[out] size Token size in bytes
 *
 * @return Command return status
 */
__u64 uk_rsi_attestation_token_continue(__u64 addr, __sz *size);

/**
 * Initialize the operation to retrieve an attestation token.
 *
 * @param addr IPA of the Granule to which the token will be written
 * @param challenge Challenge value
 *
 * @return Command return status
 */
__u64 uk_rsi_attestation_token_init(__u64 addr, __u64 challenge[8]);

/**
 * Make a Host call.
 *
 * @param addr IPA of the Host call data structure
 *
 * @return Command return status
 */
__u64 uk_rsi_host_call(__u64 addr);

/**
 * Get RIPAS of a target page.
 *
 * @param addr IPA of target page
 * @param[out] ripas RIPAS value
 *
 * @return Command return status
 */
__u64 uk_rsi_ipa_state_get(__u64 addr, __u8 *ripas);

/**
 * Request RIPAS of a target IPA range to be changed to a specified value.
 *
 * @param base Base of target IPA region
 * @param top Top of target IPA region
 * @param ripas RIPAS value
 * @param flags Flags
 * @param[out] new_base New base of target IPA region
 *
 * @return Command return status
 */
__u64 uk_rsi_ipa_state_set(__u64 base, __u64 top, __u8 ripas, __u64 flags,
			   __u64 *new_base);

/**
 * Extend Realm Extensible Measurement (REM) value.
 *
 * @param index Measurement index
 * @param size Measurement size in bytes
 * @param value The measurement value
 *
 * @return Command return status
 */
__u64 uk_rsi_measurement_extend(__u64 index, __u64 size, __u64 value[8]);

/**
 * Read the measurement value.
 *
 * @param index Measurement index
 * @param[out] value The Realm measurement identified by “index”
 *
 * @return Command return status
 */
__u64 uk_rsi_measurement_read(__u64 index, __u64 value[8]);

/**
 * Read configuration for the current Realm.
 *
 * @param config Pointer to the configuration structure
 *
 * @return Command return status
 */
__u64 uk_rsi_realm_config(__u64 config);

/**
 * Returns RSI version.
 *
 * @return RSI version
 */
__u64 uk_rsi_version(void);

/**
 * Set the range of memory accessible by the realm. This function wraps up
 * calls to uk_rsi_ipa_state_set.
 *
 * @param base The base address of the memory range
 * @param end The end address of the memory range
 * @param ripas The RIPAS value
 *
 * @return Command return status
 */
__u64 uk_rsi_setup_memory(__u64 base, __u64 end, __u8 ripas);

/**
 * Generate an attestation token. This function wraps up calls to
 * uk_rsi_attestation_token_init and uk_rsi_attestation_token_continue.
 *
 * @param addr IPA of the Granule to which the token will be written
 * @param challenge Challenge value
 * @param[out] size Attestation token size in bytes
 *
 * @return Command return status
 */
__u64
uk_rsi_generate_attestation_token(__u64 addr, __u64 challenge[8], __u64 *size);

/**
 * Initialize the RSI interface.
 */
void uk_rsi_init(void);

/**
 * Allocate a memory region for early devices.
 *
 * @param base The base address of the device.
 * @param len The length of the device.
 * @param[out] new_base The new base address of the device.
 *
 * @return 0 on success, a non-zero error code otherwise
 */
int uk_rsi_set_early_unprotected(__u64 base, __sz len, __u64 *new_base);

/**
 * Setup memory for the realm
 *
 * @param bi Pointer to the image's `struct ukplat_bootinfo` structure.
 *
 * @return 0 on success, a non-zero error code otherwise
 */
int __check_result uk_rsi_init_memory(void);

#if CONFIG_PAGING
/**
 * Setup the device region for the realm.
 *
 *  @return 0 on success, a non-zero error code otherwise
 */
int uk_rsi_init_device(__u64 base, __sz size);

/**
 * Set the memory region as protected.
 *
 * @param addr IPA of the region
 * @param numpages Number of pages
 *
 * @return 0 for success, other for failure
 */
int uk_rsi_set_memory_protected(__u64 addr, unsigned long numpages);

/**
 * Set the memory region as unprotected.
 *
 * @param addr IPA of the region
 * @param numpages Number of pages
 *
 * @return 0 on success, a non-zero error code otherwise
 */
int uk_rsi_set_memory_shared(__u64 addr, unsigned long numpages);
#endif /* CONFIG_PAGING */

#ifdef __cplusplus__
}
#endif

#endif /* __UK_RSI_H__ */
