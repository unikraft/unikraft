/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * The host as the CSPRNG's entropy source, for a CPU without a generator
 * the kernel can use: arm64 cores without FEAT_RNG (Apple's, among
 * others), where LIBUKRANDOM_LCPU cannot work.  libukrandom seeds from it
 * at boot and reseeds from it on hl_resume(), so each guest restored from
 * one snapshot draws a stream of its own.
 */

#include <errno.h>
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/print.h>
#include <uk/random/driver.h>

#include <hyperlight/hcall.h>

static int hyperlight_random_bytes(__u8 *buf, __sz size)
{
	/* hluk serves at most 4 KiB per call; 256-byte calls stay well inside
	 * the I/O stacks, and a seed is only 32 bytes.
	 */
	while (size) {
		__sz n = size < 256 ? size : 256;

		if (hl_call_get_random_bytes(buf, n) < 0)
			return -EIO;
		buf += n;
		size -= n;
	}
	return 0;
}

static struct uk_random_driver_ops hyperlight_random_ops = {
	.random_bytes = hyperlight_random_bytes,
	.seed_bytes = hyperlight_random_bytes,
	.seed_bytes_fb = hyperlight_random_bytes,
};

static struct uk_random_driver hyperlight_random_driver = {
	.name = "Hyperlight host",
	.ops = &hyperlight_random_ops,
};

static int hyperlight_random_init(struct ukplat_bootinfo *bi __unused)
{
	int rc = uk_random_init(&hyperlight_random_driver);

	/* Critical, as for any driver: the CSPRNG must not run unseeded. */
	if (unlikely(rc))
		uk_pr_err("Could not seed the CSPRNG from the host (%d)\n", rc);
	return rc;
}

UK_BOOT_EARLYTAB_ENTRY(hyperlight_random_init, UK_RANDOM_EARLY_DRIVER_PRIO);
