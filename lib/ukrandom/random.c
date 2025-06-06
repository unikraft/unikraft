/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#include <errno.h>
#include <string.h>
#include <uk/assert.h>
#include <uk/config.h>
#include <uk/init.h>
#include <uk/print.h>
#include <uk/random.h>
#include <uk/random/driver.h>

#if CONFIG_LIBUKRANDOM_DTB_SEED || CONFIG_LIBUKRANDOM_CMDLINE_SEED
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/prio.h>
#endif /* CONFIG_LIBUKRANDOM_DTB_SEED || CONFIG_LIBUKRANDOM_CMDLINE_SEED */

#include "swrand.h"

struct uk_random_driver *driver;

int __check_result uk_random_fill_buffer(void *buf, size_t buflen)
{
	__sz step, remain, i;
	__u32 rd;
	int rc;

	if (!driver)
		return -ENODEV;

	step = sizeof(__u32);
	remain = buflen % step;

	for (i = 0; i < buflen - remain; i += step) {
		rc = uk_swrand_randr(&rd);
		if (unlikely(rc))
			return rc;
		memcpy((char *)buf + i, &rd, step);
	}

	/* fill the remaining bytes of the buffer */
	if (remain > 0) {
		rc = uk_swrand_randr(&rd);
		if (unlikely(rc))
			return rc;
		memcpy((char *)buf + i, &rd, remain);
	}

	return 0;
}

int uk_random_init(struct uk_random_driver *drv)
{
	int rc;

	UK_ASSERT(drv);

	if (driver) { /* initialized */
		uk_pr_debug("CSPRNG already initialized, ignoring driver %s\n",
			    drv->name);
		return 0;
	}

	driver = drv;

	rc = uk_swrand_init(&driver);
	if (!rc)
		uk_pr_info("CSPRNG seed source: %s\n", driver->name);

	return rc;
}

#if CONFIG_LIBUKRANDOM_DTB_SEED || CONFIG_LIBUKRANDOM_CMDLINE_SEED
/* Do not print a warning if the user did not pass a seed.
 * It may be the case that the application does not require
 * randomness, or that a hardware RNG will be initialized later
 * via init.
 *
 * For the same reasons, do not return an arror as that would cause
 * init to trigger a fatal error. At the worst case the application
 * (or the kernel) will fail later, upon request for entropy.
 */
int uk_random_early_init(struct ukplat_bootinfo __maybe_unused *bi)
{
	int rc;
#if CONFIG_LIBUKRANDOM_CMDLINE_SEED
	rc = uk_swrand_cmdline_init(&driver);
	if (!rc) {
		uk_pr_info("CSPRNG seed source: cmdline\n");
		return 0;
	}
#endif /* CONFIG_LIBUKRANDOM_CMDLINE_SEED */

#if CONFIG_LIBUKRANDOM_DTB_SEED
	rc = uk_swrand_fdt_init((void *)bi->dtb, &driver);
	if (!rc) {
		uk_pr_info("CSPRNG seed source: dtb\n");
		return 0;
	}
#endif /* CONFIG_LIBUKRANDOM_DTB_SEED */

	return 0;
}

UK_BOOT_EARLYTAB_ENTRY(uk_random_early_init, UK_PRIO_AFTER(2));
#endif /* CONFIG_LIBUKRANDOM_DTB_SEED */
