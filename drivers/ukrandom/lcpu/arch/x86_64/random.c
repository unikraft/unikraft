/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/types.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/random/driver.h>

/* Intel Digital Random Number Generator (DRNG)
 * Sect. 5.2.1 Retry Recommendations
 */
#define RDRAND_RETRY_LIMIT	10

static __bool have_rdseed;

static inline int rdrand(__u64 *val)
{
	__u8 res;

	for (int i = 0; i < RDRAND_RETRY_LIMIT; i++) {
		__asm__ __volatile__(
			"	rdrand	%0\n" /* get rand */
			"	setc	%1\n" /* get result */
			: "=r" (*val), "=qm" (res)
		);
		if (unlikely(!res))
			return -EIO;
	}

	return 0;
}

static inline int rdseed(__u64 *val)
{
	__u8 res;

	if (!have_rdseed)
		return -ENOTSUP;

	__asm__ __volatile__(
		"	rdseed	%0\n" /* get seed */
		"	setc	%1\n" /* get result */
		: "=r" (*val), "=qm" (res)
	);

	if (unlikely(!res))
		return -EIO;

	return 0;
}

static int random_bytes(__u8 *buf, __sz size)
{
	__sz remain;
	__u64 val;
	int rc;

	for (__sz i = 0; i < (size / sizeof(__u64)); i++) {
		rc = rdrand((__u64 *)&buf[i * sizeof(__u64)]);
		if (unlikely(rc))
			return rc;
	}

	remain = size % sizeof(__u64);

	if (remain) {
		rc = rdrand(&val);
		if (unlikely(rc))
			return rc;

		for (__sz i = 0; i < remain; i++)
			buf[size - remain + i] = (__u8)(val >> (8 * i));
	}

	return 0;
}

static int seed_bytes(__u8 *buf, __sz size)
{
	__sz remain;
	__u64 val;
	int rc;

	for (__sz i = 0; i < (size / sizeof(__u64)); i++) {
		rc = rdseed((__u64 *)&buf[i * sizeof(__u64)]);
		if (unlikely(rc))
			return rc;
	}

	remain = size % sizeof(__u64);

	if (remain) {
		rc = rdseed(&val);
		if (unlikely(rc))
			return rc;

		for (__sz i = 0; i < remain; i++)
			buf[size - remain + i] = (__u8)(val >> (8 * i));
	}

	return 0;
}

/* Intel Digital Random Number Generator (DRNG)
 * Sect. 5.3.1 Retry Recommendations
 */
static int seed_bytes_fb(__u8 *buf, __sz size)
{
	int rc;

	if (!have_rdseed)
		return random_bytes(buf, size);

	rc = seed_bytes(buf, size);
	if (unlikely(rc))
		return random_bytes(buf, size);

	return 0;
}

static struct uk_random_driver_ops ops = {
	.random_bytes = random_bytes,
	.seed_bytes = seed_bytes,
	.seed_bytes_fb = seed_bytes_fb,
};

struct uk_random_driver_ops *device_init(void)
{
	__u32 eax, ebx, ecx, edx;

	ukarch_x86_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	if (unlikely(!(ecx & X86_CPUID1_ECX_RDRAND))) {
		uk_pr_err("The CPU does not implement RDRAND\n");
		return ERR2PTR(-ENOTSUP);
	}

	ukarch_x86_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
	if (ebx & X86_CPUID7_EBX_RDSEED)
		have_rdseed = __true;
	else
		uk_pr_warn("The CPU does not implement RDSEED");

	return &ops;
}
