/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/smccc.h>
#include <errno.h>
#include <uk/essentials.h>
#include <uk/arch/paging.h>
#include <uk/plat/io.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/sections.h>
#include <uk/rsi.h>

#ifdef CONFIG_PAGING
#include <uk/arch/lcpu.h>
#include <uk/plat/paging.h>
#include <kvm-arm64/image.h>
#endif

__u64 uk_rsi_unprotected_mask;

__u64 uk_rsi_attestation_token_continue(__u64 addr, __sz *size)
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_ATTESTATION_TOKEN_CONTINUE;
	args.a1 = addr;

	smccc_invoke(&args);
	*size = args.a1;

	return args.a0;
}

__u64 uk_rsi_attestation_token_init(__u64 addr, __u64 challenge[8])
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_ATTESTATION_TOKEN_INIT;
	args.a1 = addr;
	args.a2 = challenge[0];
	args.a3 = challenge[1];
	args.a4 = challenge[2];
	args.a5 = challenge[3];
	args.a6 = challenge[4];
	args.a7 = challenge[5];
	args.a8 = challenge[6];
	args.a9 = challenge[7];

	smccc_invoke(&args);

	return args.a0;
}

__u64 uk_rsi_host_call(__u64 addr)
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_HOST_CALL;
	args.a1 = addr;

	smccc_invoke(&args);

	return args.a0;
}

__u64 uk_rsi_ipa_state_get(__u64 addr, __u8 *ripas)
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_IPA_STATE_GET;
	args.a1 = addr;

	smccc_invoke(&args);
	*ripas = args.a1;

	return args.a0;
}

__u64 uk_rsi_ipa_state_set(__u64 base, __u64 top, __u8 ripas, __u64 flags,
			   __u64 *new_base)
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_IPA_STATE_SET;
	args.a1 = base;
	args.a2 = (top - base);
	args.a3 = ripas;
	args.a4 = flags;

	smccc_invoke(&args);

	*new_base = args.a1;

	return args.a0;
}

__u64 uk_rsi_measurement_extend(__u64 index, __u64 size, __u64 value[8])
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_MEASUREMENT_EXTEND;
	args.a1 = index;
	args.a2 = size;
	args.a3 = value[0];
	args.a4 = value[1];
	args.a5 = value[2];
	args.a6 = value[3];
	args.a7 = value[4];
	args.a8 = value[5];
	args.a9 = value[6];
	args.a10 = value[7];

	smccc_invoke(&args);

	return args.a0;
}

__u64 uk_rsi_measurement_read(__u64 index, __u64 value[8])
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_MEASUREMENT_READ;
	args.a1 = index;

	smccc_invoke(&args);

	value[0] = args.a1;
	value[1] = args.a2;
	value[2] = args.a3;
	value[3] = args.a4;
	value[4] = args.a5;
	value[5] = args.a6;
	value[6] = args.a7;
	value[7] = args.a8;

	return args.a0;
}

__u64 uk_rsi_realm_config(__u64 config)
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_REALM_CONFIG;
	args.a1 = config;

	smccc_invoke(&args);

	return args.a0;
}

__u64 uk_rsi_version(void)
{
	struct smccc_args args = {0};

	args.a0 = RSI_CMD_VERSION;

	smccc_invoke(&args);

	return args.a0;
}

__u64 uk_rsi_setup_memory(__u64 base, __u64 end, __u8 ripas)
{
	__u64 new_base, ret;

	/* Iterate over the memory space to set RIPAS */
	while (base != end) {
		ret = uk_rsi_ipa_state_set(base, end, ripas, 0, &new_base);
		base = new_base;
		if (ret != RSI_SUCCESS)
			break;
	}
	return ret;
}

__u64 uk_rsi_generate_attestation_token(__u64 addr, __u64 challenge[8],
					__u64 *size)
{
	__u64 ret;

	ret = uk_rsi_attestation_token_init(addr, challenge);
	if (ret != RSI_SUCCESS)
		return ret;

	do {
		ret = uk_rsi_attestation_token_continue(addr, size);
	} while (ret == RSI_INCOMPLETE);

	return ret;
}

void uk_rsi_init(void)
{
	struct rsi_realm_config config __align(PAGE_SIZE);
	__u64 ret = uk_rsi_realm_config((__u64)&config);

	if (ret != RSI_SUCCESS)
		UK_CRASH("Could not initialize RSI\n");

	/* set the mask of the unprotected bit */
	uk_rsi_unprotected_mask = (1UL) << (config.ipa_width - 1);
}

int uk_rsi_set_early_unprotected(__u64 base, __sz len, __u64 *new_base)
{
	struct ukplat_bootinfo *bi;
	struct ukplat_memregion_desc mrd = {0};
	__u64 aliased_IPA;
	int rc;

	bi = ukplat_bootinfo_get();
	aliased_IPA = base | uk_rsi_unprotected_mask;

	/* Add memory region for the unprotected IPA alias of early devices. */
	mrd.vbase = (__vaddr_t)ALIGN_DOWN(aliased_IPA, __PAGE_SIZE);
	mrd.pbase = (__paddr_t)ALIGN_DOWN(aliased_IPA, __PAGE_SIZE);
	mrd.len   = ALIGN_UP(len, __PAGE_SIZE);
	mrd.type  = UKPLAT_MEMRT_REALM;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE | UKPLAT_MEMRF_MAP;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to add memory region for early device\n");
		return rc;
	}

	rc = ukplat_memregion_list_coalesce(&bi->mrds);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to coalesce memory regions\n");
		return rc;
	}

	*new_base = aliased_IPA;

	return 0;
}

int __check_result uk_rsi_init_memory(void)
{
	struct ukplat_bootinfo *bi;
	const struct ukplat_memregion_desc *mrd;
	__paddr_t pstart, pend, len;
	__u64 ret;
	__u32 i;

	bi = ukplat_bootinfo_get();

	for (i = 0; i < bi->mrds.count; i++) {
		mrd = &bi->mrds.mrds[i];

		if (mrd->type == UKPLAT_MEMRT_REALM)
			continue;

		pstart = ALIGN_DOWN(mrd->pbase, __PAGE_SIZE);
		len = ALIGN_UP(mrd->len, __PAGE_SIZE);
		pend = pstart + len;

		uk_pr_info("Setting up memory: 0x%lx - 0x%lx\n", pstart, pend);

		ret = uk_rsi_setup_memory(pstart, pend, RSI_RIPAS_RAM);
		if (ret != RSI_SUCCESS)
			return -ENOTSUP;
	}

	return 0;
}

#if CONFIG_PAGING
int uk_rsi_init_device(__u64 base, __sz size)
{
	unsigned long pages, prot;

	/* set up device memory region */
	pages = DIV_ROUND_UP(size, PAGE_SIZE);
	prot = PAGE_ATTR_PROT_RW | PAGE_ATTR_RME_UNPROTECTED;

	return ukplat_page_set_attr(ukplat_pt_get_active(), base, pages, prot,
				    0);
}

int uk_rsi_set_memory_protected(__u64 addr, unsigned long numpages)
{
	unsigned long prot, phys_addr;
	__u64 ret;
	int rc;

	prot = PAGE_ATTR_PROT_RW;
	rc = ukplat_page_set_attr(ukplat_pt_get_active(), addr, numpages, prot,
				  0);
	if (unlikely(rc))
		return rc;

	phys_addr = ukplat_virt_to_phys((void *)addr);
	ret = uk_rsi_setup_memory(phys_addr, phys_addr + (numpages * PAGE_SIZE),
				  RSI_RIPAS_RAM);
	if (ret != RSI_SUCCESS)
		return -ENOTSUP;

	return 0;
}

int uk_rsi_set_memory_shared(__u64 addr, unsigned long numpages)
{
	unsigned long prot, phys_addr;
	__u64 ret;
	int rc;

	phys_addr = ukplat_virt_to_phys((void *)addr);
	rc = uk_rsi_setup_memory(phys_addr, phys_addr + (numpages * PAGE_SIZE),
				 RSI_RIPAS_EMPTY);
	if (unlikely(rc))
		return rc;

	prot = PAGE_ATTR_PROT_RW | PAGE_ATTR_RME_UNPROTECTED;
	ret = ukplat_page_set_attr(ukplat_pt_get_active(), addr, numpages, prot,
				   0);
	if (ret != RSI_SUCCESS)
		return -ENOTSUP;

	return 0;
}
#endif /* CONFIG_PAGING */
