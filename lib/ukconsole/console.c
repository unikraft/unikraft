/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/console.h>
#include <uk/print.h>
#include <uk/assert.h>

extern struct uk_console_ops *uk_console_ops;

int uk_console_coutk(const char *buf, unsigned int len)
{
	return uk_console_ops->coutk(buf, len);
}

int uk_console_coutd(const char *buf, unsigned int len)
{
	return uk_console_ops->coutd(buf, len);
}

int uk_console_cink(char *buf, unsigned int maxlen)
{
	return uk_console_ops->cink(buf, maxlen);
}

static int uk_console_null_coutk(const char *buf, unsigned int len)
{
	return len;
}

static int uk_console_null_coutd(const char *buf, unsigned int len)
{
	return len;
}

static int uk_console_null_cink(char *buf, unsigned int maxlen)
{
	return 0;
}

static void uk_console_null_init(struct ukplat_bootinfo *bi)
{
	return;
}

struct uk_console_ops uk_console_null_ops = {
	.coutk = uk_console_null_coutk,
	.coutd = uk_console_null_coutd,
	.cink = uk_console_null_cink,
	.init = uk_console_null_init,
};
