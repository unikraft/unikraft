/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <uk/init.h>
#include <uk/argparse.h>
#include <uk/essentials.h>
#include <uk/fs/pathutil.h>
#include <uk/posix-vfs.h>
#include <uk/plat/memory.h>

#include "fstab.h"

#define FSTAB_VOL_ARGS_SEP ':'
#define FSTAB_UKOPTS_ARGS_SEP FSTAB_VOL_ARGS_SEP

#define FSTAB_FSTYPE_EXTRACT "extract"

#define FSTAB_DEV_INITRD0 "initrd0"
#define FSTAB_DEV_EINITRD "embedded"

/**
 * INTERNAL. Get the memregion of initrd0.
 *
 * @return
 *  != NULL on success, NULL on failure.
 */
static inline
struct ukplat_memregion_desc *fstab_get_initrd(void)
{
	struct ukplat_memregion_desc *ret;
	const int r = ukplat_memregion_find_initrd0(&ret);

	return (r < 0 || ret->len == 0) ? NULL : ret;
}

#if CONFIG_LIBUKCPIO

#include <uk/cpio.h>

static
int fstab_extract(const struct vfs_fstab_entry *e,
		  const struct ukplat_memregion_desc *initrd0)
{
	const void *base;
	size_t len;
	int r;

	if (!strcmp(e->dev, FSTAB_DEV_INITRD0)) {
		if (unlikely(!initrd0)) {
			uk_pr_crit("Could not find an initrd!\n");
			return -ENOENT;
		}
		base = (const void *)initrd0->vbase;
		len = initrd0->len;
#if CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD
	} else if (!strcmp(e->dev, FSTAB_DEV_EINITRD)) {
		base = (const void *)vfs_einitrd_start;
		len = (uintptr_t)&vfs_einitrd_end - (uintptr_t)base;
#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD */
	} else {
		uk_pr_crit("Invalid initrd source: %s\n", e->dev);
		return -EINVAL;
	}

	if (unlikely(!len))
		uk_pr_warn("Initrd is empty: %s\n", e->dev);

	uk_pr_info("Extracting initrd %s @ %p (%zu bytes) to %s...\n",
		   e->dev, base, len, e->path);
	r = ukcpio_extract(e->path, base, len);
	if (unlikely(r)) {
		uk_pr_crit("Failed to extract CPIO to %s: %d\n", e->path, r);
		return -EIO;
	}
	return 0;
}

#else /* !CONFIG_LIBUKCPIO */

static
int fstab_extract(const struct vfs_fstab_entry *e __unused,
		  const struct ukplat_memregion_desc *initrd0 __unused)
{
	uk_pr_err("Extraction attempted via fstab without CPIO support\n");
	return -ENOSYS;
}

#endif /* !CONFIG_LIBUKCPIO */

/**
 * Internal convenience mkdir function that ignores EEXIST.
 */
static inline
int fstab_mkdir(const char *path)
{
	int r = uk_sys_mkdir(path, 0777);

	return r == -EEXIST ? 0 : r;
}

static
int fstab_mkparents(const char *path, size_t len, size_t firstsep)
{
	char tmp[len + 1];
	size_t end = firstsep;
	int r;

	UK_ASSERT(firstsep < len);
	memcpy(tmp, path, len);
	do {
		char sep = tmp[end];

		/* mkdir up to end */
		tmp[end] = '\0';
		r = fstab_mkdir(tmp);
		if (unlikely(r))
			return r;
		tmp[end] = sep;
		/* Skip separator */
		end++;
		/* Find next parent component */
		end += uk_fs_path_sep(&tmp[end], len - end);
	} while (end < len);
	return 0;
}

static
int fstab_mkmp(const char *path)
{
	struct uk_fs_poslen p = uk_fs_path_len_lead(path, 1);

	if (unlikely(!p.len))
		return 0;

	if (p.pos != p.len) {
		int r = fstab_mkparents(path, p.len, p.pos);

		if (unlikely(r))
			return r;
	}
	return fstab_mkdir(path);
}

static
int fstab_mount(const struct vfs_fstab_entry *e,
		const struct ukplat_memregion_desc *initrd0)
{
	int r;

	UK_ASSERT(e);
	UK_ASSERT(e->dev);
	UK_ASSERT(e->path);
	UK_ASSERT(e->fstype);

	/* Handle ukopts: skip if/no initrd; mkmp */
	if (((e->ukopts & FSTAB_UKOPT_IFINITRD0) && !initrd0) ||
	    ((e->ukopts & FSTAB_UKOPT_IFNOINITRD0) && initrd0))
		return 0;
	if (e->ukopts & FSTAB_UKOPT_MKMP) {
		r = fstab_mkmp(e->path);
		if (unlikely(r))
			uk_pr_crit("Could not create mount path '%s': %d\n",
				   e->path, r);
		/* Path may exist even on mkdir failure, attempt mount anyway */
	}

	if (!strcmp(e->fstype, FSTAB_FSTYPE_EXTRACT)) {
		r = fstab_extract(e, initrd0);
	} else if (!strcmp(e->dev, FSTAB_DEV_INITRD0)) {
		if (unlikely(!initrd0)) {
			uk_pr_crit("Could not find an initrd!\n");
			return -ENOENT;
		}
		r = uk_sys_mount((const void *)initrd0->vbase, e->path,
				 e->fstype, e->flags, e->opts);
#if CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD
	} else if (!strcmp(e->dev, FSTAB_DEV_EINITRD)) {
		r = uk_sys_mount((const void *)vfs_einitrd_start, e->path,
				 e->fstype, e->flags, e->opts);
#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_EINITRD */
	} else {
		r = uk_sys_mount(e->dev, e->path, e->fstype, e->flags, e->opts);
	}
	return r < 0 ? r : 1;
}

#if CONFIG_LIBPOSIX_VFS_FSTAB_USER

static
unsigned int fstab_parse_ukopts(char *ukopts_str)
{
	int ret = 0;
	char *opt;

	for (;;) {
		opt = uk_nextarg(&ukopts_str, FSTAB_UKOPTS_ARGS_SEP);
		if (!opt) {
			if (!ukopts_str)
				break; /* End of option string */
			continue; /* Empty option */
		}
		if (!strcmp(opt, FSTAB_OPTSTR_MKMP)) {
			ret |= FSTAB_UKOPT_MKMP;
			continue;
		}
		if (!strcmp(opt, FSTAB_OPTSTR_IFINITRD0)) {
			ret |= FSTAB_UKOPT_IFINITRD0;
			continue;
		}
		if (!strcmp(opt, FSTAB_OPTSTR_IFNOINITRD0)) {
			ret |= FSTAB_UKOPT_IFNOINITRD0;
			continue;
		}
		uk_pr_warn("Ignoring unknown ukopt: %s\n", opt);
	}
	return ret;
}

static
int fstab_parse(char *entry_str, struct vfs_fstab_entry *e)
{
	char *arg;
	char *endptr;

	UK_ASSERT(entry_str);
	UK_ASSERT(e);

	/* Parse dev/path/fstype w/ fallbacks */
	arg = uk_nextarg(&entry_str, FSTAB_VOL_ARGS_SEP);
	e->dev = arg ? arg : "";
	arg = uk_nextarg(&entry_str, FSTAB_VOL_ARGS_SEP);
	e->path = arg ? arg : "";
	arg = uk_nextarg(&entry_str, FSTAB_VOL_ARGS_SEP);
	e->fstype = arg ? arg : "";
	/* Convert flags to numeric */
	arg = uk_nextarg(&entry_str, FSTAB_VOL_ARGS_SEP);
	if (arg && arg[0]) {
		errno = 0;
		e->flags = strtol(arg, &endptr, 0);
		if (unlikely(*endptr))
			uk_pr_warn("Invalid characters in mount flags: %s\n",
				   endptr);
		if (unlikely(errno == ERANGE))
			uk_pr_warn("Mount flags value out of range: %s\n", arg);
	} else {
		e->flags = 0;
	}
	/* Opts string passed verbatim */
	e->opts = uk_nextarg(&entry_str, FSTAB_VOL_ARGS_SEP);
	/* Anything left over is interpreted as ukopts */
	e->ukopts = fstab_parse_ukopts(entry_str);

	uk_pr_debug("Parsed %s:%s:%s:0x%lx:%s:0x%x\n",
		    e->dev, e->path, e->fstype, e->flags, e->opts, e->ukopts);
	return 0;
}

static
int fstab_process_user(const struct ukplat_memregion_desc *initrd0)
{
	int mounted = 0;
	struct vfs_fstab_entry ent;
	int r;

	for (size_t i = 0; i < ARRAY_SIZE(fstab_user); i++) {
		if (!fstab_user[i])
			break;

		r = fstab_parse(fstab_user[i], &ent);
		if (unlikely(r))
			return r;
		r = fstab_mount(&ent, initrd0);
		if (unlikely(r < 0))
			return r;
		if (r > 0)
			mounted++;
	}
	return mounted;
}

#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_USER */

#if CONFIG_LIBPOSIX_VFS_FSTAB_BUILTIN || CONFIG_LIBPOSIX_VFS_FSTAB_FALLBACK
static
int fstab_process_internal(const struct vfs_fstab_entry fstab[],
			   size_t len,
			   const struct ukplat_memregion_desc *initrd0)
{
	int mounted = 0;
	int r;

	for (size_t i = 0; i < len; i++) {
		r = fstab_mount(&fstab[i], initrd0);
		if (unlikely(r < 0))
			return r;
		if (r > 0)
			mounted++;
	}
	return mounted;
}
#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_BUILTIN || CONFIG_LIBPOSIX_VFS_FSTAB_FALLBACK */

static
int init_vfs_fstab(struct uk_init_ctx *ictx __unused)
{
	const struct ukplat_memregion_desc *initrd0 = fstab_get_initrd();
	int r;
	int count __maybe_unused = 0;

#if CONFIG_LIBPOSIX_VFS_FSTAB_BUILTIN
	uk_pr_debug("Mounting from built-in fstab (%zu entries)...\n",
		    ARRAY_SIZE(fstab_builtin) - 1);
	r = fstab_process_internal(fstab_builtin, ARRAY_SIZE(fstab_builtin),
				   initrd0);
	if (unlikely(r < 0))
		return r;
	uk_pr_debug("Successfully mounted %d volumes\n", r);
	count += r;
#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_BUILTIN */

#if CONFIG_LIBPOSIX_VFS_FSTAB_USER
	uk_pr_debug("Mounting from user-provided table...\n");
	r = fstab_process_user(initrd0);
	if (unlikely(r < 0))
		return r;
	uk_pr_debug("Successfully mounted %d volumes\n", r);
	count += r;

#if CONFIG_LIBPOSIX_VFS_FSTAB_FALLBACK
	if (unlikely(r == 0)) {
		uk_pr_debug("Mounting from fallback table (%zu entries)...\n",
			    ARRAY_SIZE(fstab_fallback) - 1);
		r = fstab_process_internal(fstab_fallback,
					   ARRAY_SIZE(fstab_fallback), initrd0);
		if (unlikely(r < 0))
			return r;
		uk_pr_debug("Successfully mounted %d volumes\n", r);
		count += r;
	}
#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_FALLBACK */
#endif /* CONFIG_LIBPOSIX_VFS_FSTAB_USER */

	uk_pr_debug("Mounted %d entries total\n", count);
	return 0;
}

/* The fstab runs latest in the rootfs init cycle */
uk_rootfs_initcall(init_vfs_fstab, 0);
