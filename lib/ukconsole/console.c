/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/console.h>
#include <uk/print.h>
#include <uk/assert.h>

struct uk_console_ops *uk_console_ops;

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
