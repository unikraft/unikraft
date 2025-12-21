/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef _DLFCN_H
#define _DLFCN_H

#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LIBPOSIX_LIBDL

#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_NOLOAD 4
#define RTLD_NODELETE 4096
#define RTLD_GLOBAL 256
#define RTLD_LOCAL  0

#define RTLD_NEXT    ((void *)-1)
#define RTLD_DEFAULT ((void *)0)

#define RTLD_DI_LINKMAP 2

int dlclose(void *handle);
char *dlerror(void);
void *dlopen(const char *filename, int flags);
void *dlsym(void *restrict handle, const char *restrict symbol);

typedef struct {
	const char *dli_fname;
	void *dli_fbase;
	const char *dli_sname;
	void *dli_saddr;
} Dl_info;

int dladdr(const void *addr, Dl_info *info);
int dlinfo(void *handle, int request, void *info);
void *dlvsym(void *handle, const char *symbol, const char *version);

#endif

#ifdef __cplusplus
}
#endif
#endif /* _DLFCN_H */
