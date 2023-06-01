/* SPDX-License-Identifier: MIT */
/* Copyright © 2005-2020 Rich Felker, et al.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the MIT License (the "License", see COPYING.md).
 * You may not use this file except in compliance with the License.
 */
/*
 * The code in this file was derived from musl 1.2.3:
 * Source: https://www.musl-libc.org/
 * File: src/env/clearenv.c
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include "environ.h"

int clearenv()
{
	char **e = __environ;
	__environ = 0;
	if (e) while (*e) __env_rm_add(*e++, 0);
	return 0;
}
