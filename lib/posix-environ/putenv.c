/* SPDX-License-Identifier: MIT */
/* Copyright © 2005-2020 Rich Felker, et al.
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the MIT License (the "License", see COPYING.md).
 * You may not use this file except in compliance with the License.
 */
/*
 * The code in this file was derived from musl 1.2.3:
 * Source: https://www.musl-libc.org/
 * File: src/env/putenv.c
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "environ.h"

int __putenv(char *s, size_t l, char *r)
{
	size_t i=0;
	if (__environ) {
		for (char **e = __environ; *e; e++, i++)
			if (!strncmp(s, *e, l+1)) {
				char *tmp = *e;
				*e = s;
				__env_rm_add(tmp, r);
				return 0;
			}
	}
	static char **oldenv;
	char **newenv;
	if (__environ == oldenv) {
		newenv = realloc(oldenv, sizeof *newenv * (i+2));
		if (!newenv) goto oom;
	} else {
		newenv = malloc(sizeof *newenv * (i+2));
		if (!newenv) goto oom;
		if (i) memcpy(newenv, __environ, sizeof *newenv * i);
		free(oldenv);
	}
	newenv[i] = s;
	newenv[i+1] = 0;
	__environ = oldenv = newenv;
	if (r) __env_rm_add(0, r);
	return 0;
oom:
	free(r);
	return -1;
}

int putenv(char *s)
{
	size_t l = strchrnul(s, '=') - s;
	if (!l || !s[l]) return unsetenv(s);
	return __putenv(s, l, 0);
}
