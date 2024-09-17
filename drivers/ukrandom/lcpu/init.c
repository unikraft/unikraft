/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/errptr.h>
#include <uk/random/driver.h>

#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKBOOT */

struct uk_random_driver_ops *device_init(void);

struct uk_random_driver driver = {
	.name = "CPU"
};

static int uk_random_lcpu_init(struct ukplat_bootinfo __unused *bi)
{
	int rc;

	/* initialize the device */
	driver.ops = device_init();
	if (unlikely(PTRISERR(driver.ops))) {
		uk_pr_err("Could not initialize the HWRNG (%d)\n",
			  PTR2ERR(driver.ops));
		return PTR2ERR(driver.ops);
	}

	/* register with libukrandom */
	rc = uk_random_init(&driver);
	if (unlikely(rc)) {
		uk_pr_err("Could not register with libukrandom (%d)\n", rc);
		return rc;
	}

	return 0;
}

#if CONFIG_LIBUKBOOT
UK_BOOT_EARLYTAB_ENTRY(uk_random_lcpu_init, UK_RANDOM_EARLY_DRIVER_PRIO);
#endif /* CONFIG_LIBUKBOOT */
