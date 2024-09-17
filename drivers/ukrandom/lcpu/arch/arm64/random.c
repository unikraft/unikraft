/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <uk/arch/lcpu.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/random/driver.h>

static inline int rndr(__u64 *val)
{
	__u64 res;

	__asm__ __volatile__("	mrs	%x0, RNDR\n" /* Get rand */
			     "	mrs	%x1, NZCV\n" /* Get result */
			     : "=r"(*val), "=r"(res));

	if (unlikely(res != 0))
		return -EIO;

	return res;
}

static inline int rndrrs(__u64 *val)
{
	__u64 res;

	__asm__ __volatile__("	mrs	%x0, RNDRRS\n" /* Get rand */
			     "	mrs	%x1, NZCV\n"   /* Get result */
			     : "=r"(*val), "=r"(res));

	if (unlikely(res != 0))
		return -EIO;

	return res;
}

static int random_bytes(__u8 *buf, __sz size)
{
	__sz remain;
	__u64 val;
	int rc;

	for (__sz i = 0; i < (size / sizeof(__u64)); i++) {
		rc = rndr((__u64 *)&buf[i * sizeof(__u64)]);
		if (unlikely(rc))
			return rc;
	}

	remain = size % sizeof(__u64);

	if (remain) {
		rc = rndr(&val);
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
		rc = rndrrs((__u64 *)&buf[i * sizeof(__u64)]);
		if (unlikely(rc))
			return rc;
	}

	remain = size % sizeof(__u64);

	if (remain) {
		rc = rndrrs(&val);
		if (unlikely(rc))
			return rc;

		for (__sz i = 0; i < remain; i++)
			buf[size - remain + i] = (__u8)(val >> (8 * i));
	}

	return 0;
}

static int seed_bytes_fb(__u8 *buf, __sz size)
{
	int rc;

	/* It is unclear whether this provides any benefit as Arm
	 * specifies different possibilities for the implementation
	 * of RNDRRS (DDI 0487K.a Sect. DD23.2.148)
	 */
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
	__u64 isar;
	__u64 rndr;

	__asm__ __volatile__("mrs %x0, ID_AA64ISAR0_EL1\n" : "=r"(isar));

	rndr = (isar >> ID_AA64ISAR0_EL1_RNDR_SHIFT) & ID_AA64ISAR0_EL1_RNDR_MASK;
	if (unlikely(!rndr)) {
		uk_pr_err("CPU does not implement FEAT_RNG\n");
		return ERR2PTR(-ENOTSUP);
	}

	return &ops;
}
