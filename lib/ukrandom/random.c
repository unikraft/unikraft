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

#include "swrand.h"

struct uk_random_driver *driver;

int __check_result uk_random_fill_buffer(void *buf, size_t buflen)
{
	__sz step, chunk_size, i;
	__u32 rd;

	if (!driver)
		return -ENODEV;

	step = sizeof(__u32);
	chunk_size = buflen % step;

	for (i = 0; i < buflen - chunk_size; i += step)
		*(__u32 *)((char *)buf + i) = uk_swrand_randr();

	/* fill the remaining bytes of the buffer */
	if (chunk_size > 0) {
		rd = uk_swrand_randr();
		memcpy(buf + i, &rd, chunk_size);
	}

	return 0;
}

int uk_random_init(struct uk_random_driver *drv)
{
	UK_ASSERT(drv);

	if (driver) /* initialized */
		return 0;

	driver = drv;

	return uk_swrand_init(&driver);
}
