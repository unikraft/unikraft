/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#ifdef CONFIG_LIBPOSIX_SYSINFO

#ifdef __cplusplus
extern "C" {
#endif

/* #include <features.h> */

struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
#ifdef _GNU_SOURCE
	char domainname[65];
#else
	char __domainname[65];
#endif
};

int uname(struct utsname *name);

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_LIBPOSIX_SYSINFO */
#endif /* _SYS_UTSNAME_H */
