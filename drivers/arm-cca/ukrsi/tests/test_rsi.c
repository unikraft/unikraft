/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <arm/smccc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/paging.h>
#include <uk/plat/io.h>
#include <uk/rsi.h>
#include <uk/test.h>

#define FID_INVALID	0xc5000041

/**
 * Tests the failure condition of uk_rsi_attestation_token_init
 */
UK_TESTCASE(ukrsi, uktest_test_attestation_token_continue_fail)
{
	char buffer[PAGE_SIZE] __align(PAGE_SIZE) = {0};
	unsigned long challenge[8];
	unsigned long ret;
	unsigned long size;

	/* Test with not ATTEST_IN_PROGRESS */
	ret = uk_rsi_attestation_token_continue(ukplat_virt_to_phys(buffer),
						&size);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_STATE);

	/* Test with a different attest address */
	ret = uk_rsi_attestation_token_init(ukplat_virt_to_phys(buffer),
					    challenge);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	ret = uk_rsi_attestation_token_continue(
	    ukplat_virt_to_phys(buffer) + 10, &size);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

/**
 * Tests the failure condition of uk_rsi_attestation_token_init
 */
UK_TESTCASE(ukrsi, uktest_test_attestation_token_init_fail)
{
	char buffer[PAGE_SIZE] __align(PAGE_SIZE) = {0};
	unsigned long challenge[8];
	unsigned long ret;

	memset(challenge, 0xAB, sizeof(challenge));

	/* Test with a non-page aligned address */
	ret = uk_rsi_attestation_token_init(ukplat_virt_to_phys(buffer) + 10,
					    challenge);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an out-of-bound address */
	ret = uk_rsi_attestation_token_init(ARM64_INVALID_ADDR, challenge);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

/**
 * Tests the RSI attestion
 */
UK_TESTCASE(ukrsi, uktest_test_attestation)
{
	char buffer[PAGE_SIZE] __align(PAGE_SIZE) = {0};
	unsigned long challenge[8];
	unsigned long i;
	unsigned long ret;
	unsigned long size;

	memset(challenge, 0xAB, sizeof(challenge));

	/* Get attestation in normal case */
	ret = uk_rsi_generate_attestation_token(
		ukplat_virt_to_phys(buffer),
		challenge, &size);

	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	UK_TEST_EXPECT_NOT_ZERO(size);

	for (i = 0; i < size; i++) {
		if (buffer[i])
			printf("0x%02x ", buffer[i]);
		else
			printf("0x%#02x ", buffer[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (size % 8 != 7)
		printf("\n");
}

/**
 * Tests the host call
 */
UK_TESTCASE(ukrsi, uktest_test_hostcall_fail)
{
	struct rsi_host_call_data __align(256) host_call_data = {0};
	unsigned long ret;

	/* Test with a not-aligned address */
	ret = uk_rsi_host_call(ukplat_virt_to_phys(&host_call_data) + 10);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an out-of-bound address */
	ret = uk_rsi_host_call(ARM64_INVALID_ADDR);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

/**
 * Tests the host call
 */
UK_TESTCASE(ukrsi, uktest_test_hostcall)
{
	struct rsi_host_call_data __align(256) host_call_data = {0};
	unsigned long ret;

	/* Test with normal host call */
	host_call_data.gprs[0] = SMCCC_FID_SMCCC_VERSION;

	ret = uk_rsi_host_call(ukplat_virt_to_phys(&host_call_data));

	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	UK_TEST_EXPECT_SNUM_EQ(host_call_data.gprs[0], SMCCC_VERSION_1_1);

	/* Test with unsupported host call */
	host_call_data.gprs[0] = FID_INVALID;

	ret = uk_rsi_host_call(ukplat_virt_to_phys(&host_call_data));

	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	UK_TEST_EXPECT_SNUM_EQ(host_call_data.gprs[0], SMCCC_NOT_SUPPORTED);
}

UK_TESTCASE(ukrsi, uktest_test_ripas_get_fail)
{
	char buffer[PAGE_SIZE] __align(PAGE_SIZE) = {0};
	unsigned long ret;
	unsigned char ripas;

	/* Test with a not-aligned address */
	ret = uk_rsi_ipa_state_get(ukplat_virt_to_phys(&buffer) + 10, &ripas);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an out-of-bound address */
	ret = uk_rsi_ipa_state_get(ARM64_INVALID_ADDR, &ripas);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

UK_TESTCASE(ukrsi, uktest_test_ripas_set_fail)
{
	char buffer[PAGE_SIZE] __align(PAGE_SIZE) = {0};
	unsigned long ret;

	/* Test with a not-aligned base address */
	ret = uk_rsi_setup_memory(ukplat_virt_to_phys(buffer) + 10,
				  ukplat_virt_to_phys(buffer) + PAGE_SIZE,
				  RSI_RIPAS_RAM);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with a not-aligned top address */
	ret = uk_rsi_setup_memory(ukplat_virt_to_phys(buffer),
				  ukplat_virt_to_phys(buffer) + PAGE_SIZE + 10,
				  RSI_RIPAS_RAM);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an invalid size */
	ret = uk_rsi_setup_memory(ukplat_virt_to_phys(buffer),
				  ukplat_virt_to_phys(buffer) - PAGE_SIZE,
				  RSI_RIPAS_RAM);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an invalid address */
	ret = uk_rsi_setup_memory(
	    ARM64_INVALID_ADDR, ARM64_INVALID_ADDR + PAGE_SIZE, RSI_RIPAS_RAM);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an invalid RIPAS */
	ret = uk_rsi_setup_memory(ukplat_virt_to_phys(buffer),
				  ukplat_virt_to_phys(buffer) + PAGE_SIZE,
				  RSI_RIPAS_DESTROYED + 1);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

UK_TESTCASE(ukrsi, uktest_test_ripas_get_set)
{
	char buffer[PAGE_SIZE] __align(PAGE_SIZE) = {0};
	unsigned long ret;
	unsigned char ripas;

	/* test ripas of the buffer */
	ret = uk_rsi_ipa_state_get(ukplat_virt_to_phys(buffer), &ripas);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	UK_TEST_EXPECT_SNUM_EQ(ripas, RSI_RIPAS_RAM);

	/* set ripas and then get */
	ret = uk_rsi_setup_memory(ukplat_virt_to_phys(buffer),
				  ukplat_virt_to_phys(buffer) + PAGE_SIZE,
				  RSI_RIPAS_EMPTY);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);

	ret = uk_rsi_ipa_state_get(ukplat_virt_to_phys(buffer), &ripas);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	UK_TEST_EXPECT_SNUM_EQ(ripas, RSI_RIPAS_EMPTY);

	ret = uk_rsi_setup_memory(ukplat_virt_to_phys(buffer),
				  ukplat_virt_to_phys(buffer) + PAGE_SIZE,
				  RSI_RIPAS_RAM);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);

	ret = uk_rsi_ipa_state_get(ukplat_virt_to_phys(buffer), &ripas);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	UK_TEST_EXPECT_SNUM_EQ(ripas, RSI_RIPAS_RAM);
}

#if CONFIG_PAGING
UK_TESTCASE(ukrsi, uktest_test_unprotected_memory)
{
	char buffer[PAGE_SIZE * 2] __align(PAGE_SIZE) = {0};
	int i;

	/* set memory shared and write memory */
	uk_rsi_set_memory_shared((unsigned long)buffer, 2);
	for (i = 0; i < 2; i++)
		buffer[i * PAGE_SIZE] = i + 1;

	/* read memory */
	for (i = 0; i < 2; i++)
		UK_TEST_EXPECT_SNUM_EQ(buffer[i * PAGE_SIZE], i + 1);

	/* set memory protected and read memory */
	uk_rsi_set_memory_protected((unsigned long)buffer, 2);
	for (i = 0; i < 2; i++)
		UK_TEST_EXPECT_SNUM_EQ(buffer[i * PAGE_SIZE], 0);
}
#endif /* CONFIG_PAGING */

/**
 * Tests the uk_rsi_measurement_extend failure function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_measurement_extend_fail)
{
	unsigned long value[8];
	unsigned long ret;

	memset(value, 0xAB, sizeof(value));

	/* Test with out-of-bound index */
	ret = uk_rsi_measurement_extend(0, sizeof(value), value);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	ret = uk_rsi_measurement_extend(5, sizeof(value), value);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with too large size */
	ret = uk_rsi_measurement_extend(1, 65, value);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

/**
 * Tests the uk_rsi_measurement_extend function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_measurement_extend)
{
	unsigned long value[8];
	unsigned long ret;
	int i;

	memset(value, 0xAB, sizeof(value));

	for (i = 1; i < 5; i++) {
		ret = uk_rsi_measurement_extend(i, sizeof(value), value);
		UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
	}
}

/**
 * Tests the uk_rsi_measurement_extend failure function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_measurement_read_fail)
{
	unsigned long value[8];
	unsigned long ret;

	memset(value, 0xAB, sizeof(value));

	/* Test with out-of-bound index */
	ret = uk_rsi_measurement_read(5, value);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

/**
 * Tests the uk_rsi_measurement_extend function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_measurement_read)
{
	unsigned long value[8];
	unsigned long ret;
	int i, j;

	memset(value, 0xAB, sizeof(value));

	for (i = 0; i < 5; i++) {
		ret = uk_rsi_measurement_read(i, value);
		UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);
		printf("Measurement %d:\n", i);
		for (j = 0; j < 8; j++)
			printf("0x%016lx ", value[j]);
		printf("\n");
	}
}

/**
 * Tests the uk_rsi_realm_config failure function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_realm_config_fail)
{
	struct rsi_realm_config config __align(PAGE_SIZE) = {0};
	unsigned long ret;

	/* Test with a non-page aligned address */
	ret = uk_rsi_realm_config(ukplat_virt_to_phys(&config) + 10);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);

	/* Test with an out-of-bound address */
	ret = uk_rsi_realm_config(ARM64_INVALID_ADDR);
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_ERROR_INPUT);
}

/**
 * Tests the uk_rsi_realm_config function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_realm_config)
{
	struct rsi_realm_config config __align(PAGE_SIZE) = {0};
	unsigned long ret;

	ret = uk_rsi_realm_config(ukplat_virt_to_phys(&config));
	UK_TEST_EXPECT_SNUM_EQ(ret, RSI_SUCCESS);

	UK_TEST_EXPECT_SNUM_EQ(config.ipa_width, 33);
}

/**
 * Tests the uk_rsi_version function
 */
UK_TESTCASE(ukrsi, uktest_test_rsi_version)
{
	unsigned long version = uk_rsi_version();

	/* Check if the version is valid, 0xC0000 is a lagacy version */
	UK_TEST_EXPECT_SNUM_EQ(version, 0xC0000);
	UK_TEST_EXPECT_NOT_ZERO(version);
}

uk_testsuite_register(ukrsi, NULL);
