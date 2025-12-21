/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef _GRP_H
#define _GRP_H

#include <uk/config.h>

#if CONFIG_LIBPOSIX_USER
#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_size_t
#define __NEED_gid_t

#ifdef _GNU_SOURCE
#define __NEED_FILE
#endif

#include <nolibc-internal/shareddefs.h>

struct group {
	char *gr_name;
	char *gr_passwd;
	gid_t gr_gid;
	char **gr_mem;
};

struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);
int getgrgid_r(gid_t gid, struct group *grp,
	       char *buffer, size_t bufsize, struct group **result);
int getgrnam_r(const char *name, struct group *grp,
	       char *buf, size_t size, struct group **result);

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
struct group *getgrent(void);
void endgrent(void);
void setgrent(void);
#endif

#ifdef _GNU_SOURCE
struct group *fgetgrent(FILE *stream);
int putgrent(const struct group *grp, FILE *stream);
#endif

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroup);
int setgroups(size_t size, const gid_t *list);
int initgroups(const char *user, gid_t group);
#endif

#ifdef __cplusplus
}
#endif
#endif /*CONFIG_LIBPOSIX_USER*/
#endif /*_GRP_H*/
