/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#define _GNU_SOURCE
#include <uk/libparam.h>
#include <uk/essentials.h>
#include <uk/init.h>
#include <string.h>
#include "environ.h"

#define LIBPOSIX_ENVIRON_ENV_VAR(x)		e##x

#define DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(x)			\
	static char LIBPOSIX_ENVIRON_ENV_VAR(x)[] =		\
				CONFIG_LIBPOSIX_ENVIRON_ENVP##x


#if CONFIG_LIBPOSIX_ENVIRON_ENVP0_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(0);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP0_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP1_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(1);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP1_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP2_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(2);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP2_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP3_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(3);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP3_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP4_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(4);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP4_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP5_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(5);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP5_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP6_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(6);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP6_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP7_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(7);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP7_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP8_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(8);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP8_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP9_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(9);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP9_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP10_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(10);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP10_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP11_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(11);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP11_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP12_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(12);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP12_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP13_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(13);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP13_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP14_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(14);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP14_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP15_NOTEMPTY
DECLARE_LIBPOSIX_ENVIRON_ENV_VAR(15);
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP15_NOTEMPTY */

/*
 * Size the array
 */
static char *__init_env[1 /* null termination */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP0_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP0_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP1_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP1_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP2_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP2_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP3_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP3_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP4_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP4_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP5_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP5_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP6_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP6_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP7_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP7_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP8_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP8_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP9_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP9_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP10_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP10_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP11_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP11_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP12_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP12_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP13_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP13_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP14_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP14_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP15_NOTEMPTY
	+ 1
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP15_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_LIBPARAM
	+ CONFIG_LIBPOSIX_ENVIRON_LIBPARAM_MAXCOUNT
#endif /* CONFIG_LIBPOSIX_ENVIRON_LIBPARAM */
	] = {
#if CONFIG_LIBPOSIX_ENVIRON_ENVP0_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(0),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP0_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP1_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(1),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP1_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP2_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(2),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP2_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP3_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(3),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP3_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP4_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(4),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP4_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP5_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(5),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP5_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP6_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(6),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP6_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP7_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(7),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP7_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP8_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(8),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP8_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP9_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(9),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP9_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP10_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(10),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP10_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP11_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(11),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP11_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP12_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(12),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP12_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP13_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(13),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP13_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP14_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(14),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP14_NOTEMPTY */
#if CONFIG_LIBPOSIX_ENVIRON_ENVP15_NOTEMPTY
	LIBPOSIX_ENVIRON_ENV_VAR(15),
#endif /* CONFIG_LIBPOSIX_ENVIRON_ENVP15_NOTEMPTY */
	NULL /* termination */
};

#if CONFIG_LIBPOSIX_ENVIRON_LIBPARAM
UK_LIBPARAM_PARAM_ARR_ALIAS(vars,
			    &(__init_env[ARRAY_SIZE(__init_env) -
				CONFIG_LIBPOSIX_ENVIRON_LIBPARAM_MAXCOUNT - 1]),
			    charp,
			    CONFIG_LIBPOSIX_ENVIRON_LIBPARAM_MAXCOUNT,
			    "Environment variables");
#endif

char **__environ = __init_env;
__weak_alias(__environ, ___environ);
__weak_alias(__environ, _environ);
__weak_alias(__environ, environ);

/* Find the first occurrence of a variable within __init_env */
static char **find_vnpos(const char *varname, size_t varname_len, char **end)
{
	char **i;

	for (i = __init_env; *i && i != end; i++) {
		if (!strncmp(varname, *i, varname_len) &&
		    (varname_len[*i] == '=' || varname_len[*i] == '\0'))
			return i;
	}
	return NULL;
}

/* Eliminate duplicates from __init_env */
static int uniquify_env(struct uk_init_ctx *ictx __unused)
{
	char **rpos; /*< read position */
	char **wpos; /*< write position */

	for (rpos = __init_env, wpos = __init_env; *rpos; rpos++) {
		size_t namelen;
		char **opos; /*< overwrite position */

		namelen = strchrnul(*rpos, '=') - *rpos;
		opos = find_vnpos(*rpos, namelen, wpos);

		if (opos) {
			*opos = *rpos; /* replace an entry */
		} else {
			if (*wpos != *rpos)
				*wpos = *rpos; /* append an entry */
			wpos++;
		}
	}
	*(wpos++) = NULL; /* NULL termination */

	return 0;
}

uk_sys_initcall_prio(uniquify_env, 0x0, UK_PRIO_EARLIEST);
