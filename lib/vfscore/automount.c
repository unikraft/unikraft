/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * VFS Automatic Mounts
 *
 * Authors: Simon Kuenzer <simon@unikraft.io>
 *          Robert Hrusecky <roberth@cs.utexas.edu>
 *          Omar Jamil <omarj2898@gmail.com>
 *          Sachin Beldona <sachinbeldona@utexas.edu>
 *          Sergiu Moga <sergiu@unikraft.io>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2023-2024, Unikraft GmbH. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/mount.h>
#include <uk/assert.h>
#ifdef CONFIG_LIBUKCPIO
#include <uk/cpio.h>
#endif /* CONFIG_LIBUKCPIO */
#include <uk/init.h>
#include <uk/libparam.h>
#include <uk/plat/memory.h>
#include <sys/stat.h>
#include <vfscore/mount.h>
#include <errno.h>
#include <uk/config.h>
#include <uk/arch/types.h>

#define LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP			':'
#define LIBVFSCORE_FSTAB_UKOPTS_ARGS_SEP			','

#define LIBVFSCORE_EXTRACT_DRV					"extract"
#define LIBVFSCORE_EXTRACT_DEV_INITRD0				"initrd0"
#define LIBVFSCORE_EXTRACT_DEV_EMBEDDED				"embedded"

#define LIBVFSCORE_UKOPT_MKMP					(0x1 << 0)

struct vfscore_volume {
	/* Volume source device */
	const char *sdev;
	/* Mount point absolute path */
	char *path;
	/* Corresponding filesystem driver name */
	const char *drv;
	/* Mount flags */
	unsigned long flags;
	/* Mount options */
	const char *opts;
	/* Unikraft mount options, see vfscore_mount_volume() */
	unsigned int ukopts;
};

#if CONFIG_LIBVFSCORE_AUTOMOUNT_UP
static char *next_arg(char **argptr, int separator)
{
	char *nsep;
	char *arg;

	UK_ASSERT(argptr);

	if (!*argptr || (*argptr)[0] == '\0') {
		/* We likely got called again after we already
		 * returned the last argument
		 */
		*argptr = NULL;
		return NULL;
	}

	arg = *argptr;
	nsep = strchr(*argptr, separator);
	if (!nsep) {
		/* No next separator, we hit the last argument */
		*argptr = NULL;
		goto out;
	}

	/* Split C string by overwriting the separator */
	nsep[0] = '\0';
	/* Move argptr to next argument */
	*argptr = nsep + 1;

out:
	/* Return NULL for empty arguments */
	if (*arg == '\0')
		return NULL;
	return arg;
}
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_UP */

/*
 * Compiled-in file system table: `fstab_ci`
 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI0
const struct vfscore_volume __fstab_ci0 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0 */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI1
const struct vfscore_volume __fstab_ci1 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1 */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI2
const struct vfscore_volume __fstab_ci2 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2 */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI3
const struct vfscore_volume __fstab_ci3 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3 */

static const struct vfscore_volume *fstab_ci[] = {
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI0
	&__fstab_ci0,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI0 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI1
	&__fstab_ci1,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI1 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI2
	&__fstab_ci2,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI2 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI3
	&__fstab_ci3,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI3 */
	NULL
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_UP
/*
 * Compiled-in fall-back file system table: `fstab_fb`
 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB0
const struct vfscore_volume __fstab_fb0 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0 */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB1
const struct vfscore_volume __fstab_fb1 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1 */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB2
const struct vfscore_volume __fstab_fb2 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2 */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB3
const struct vfscore_volume __fstab_fb3 = {
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_DEV_ARG
	.sdev = CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_DEV_ARG,
#else
	.sdev = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_DEV_ARG */
	.path = CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_MP_ARG,
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_DRIVER_ARG
	.drv = CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_DRIVER_ARG,
#else
	.drv = "",
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_DRIVER_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_FLAGS_ARG
	.flags = CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_FLAGS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_FLAGS_ARG */
#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_OPTS_ARG
	.opts = CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_OPTS_ARG,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_OPTS_ARG */
	.ukopts = 0x0
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_UKOPTS_MKMP_ARG
		  | LIBVFSCORE_UKOPT_MKMP
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3_UKOPTS_MKMP_ARG */
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3 */

static const struct vfscore_volume *fstab_fb[] = {
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB0
	&__fstab_fb0,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB0 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB1
	&__fstab_fb1,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB1 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB2
	&__fstab_fb2,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB2 */
#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB3
	&__fstab_fb3,
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB3 */
	NULL
};
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB */

static char *fstab_up[CONFIG_LIBVFSCORE_AUTOMOUNT_UP_SIZE] = { NULL };

UK_LIBPARAM_PARAM_ARR_ALIAS(fstab, &fstab_up, charp,
			    CONFIG_LIBVFSCORE_AUTOMOUNT_UP_SIZE,
			"Automount table: dev:path:fs[:flags[:opts[:ukopts]]]");

#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_UP */

#if CONFIG_LIBUKCPIO
#if CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD
extern const char vfscore_einitrd_start[];
extern const char vfscore_einitrd_end;
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD */

static int vfscore_extract_volume(const struct vfscore_volume *vv)
{
	const void *vbase = NULL;
	size_t vlen = 0;
	int rc;

	UK_ASSERT(vv);
	UK_ASSERT(vv->path);

	/* Detect which initrd to use */
	/* TODO: Support multiple Initial RAM Disks */
	if (!strcmp(vv->sdev, LIBVFSCORE_EXTRACT_DEV_INITRD0)) {
		struct ukplat_memregion_desc *initrd;

		rc = ukplat_memregion_find_initrd0(&initrd);
		if (unlikely(rc < 0 || initrd->len == 0)) {
			uk_pr_crit("Could not find an initrd!\n");
			return -1;
		}

		vbase = (void *)initrd->vbase;
		vlen = initrd->len;
	}
#if CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD
	else if (!strcmp(vv->sdev, LIBVFSCORE_EXTRACT_DEV_EMBEDDED)) {
		vbase = (const void *)vfscore_einitrd_start;
		vlen  = (size_t)((uintptr_t)&vfscore_einitrd_end
				 - (uintptr_t)vfscore_einitrd_start);
	}
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD */
	else {
		uk_pr_crit("\"%s\" is an invalid or unsupported initrd source!\n",
			   vv->sdev);
		return -EINVAL;
	}

	if (unlikely(vlen == 0))
		uk_pr_warn("Initrd \"%s\" seems to be empty.\n", vv->sdev);

	uk_pr_info("Extracting initrd @ %p (%"__PRIsz" bytes, source: \"%s\") to %s...\n",
		   vbase, vlen, vv->sdev, vv->path);
	rc = ukcpio_extract(vv->path, vbase, vlen);
	if (unlikely(rc)) {
		uk_pr_crit("Failed to extract cpio archive to %s: %d\n",
			   vv->path, rc);
		return -EIO;
	}
	return 0;
}
#endif /* CONFIG_LIBUKCPIO */

/* Handle `mkmp` Unikraft Mount Option */
static int vfscore_ukopt_mkmp(char *path)
{
	char *pos, *prev_pos;
	int rc;

	UK_ASSERT(path);
	UK_ASSERT(path[0] == '/');

	uk_pr_debug(" mkmp: Ensure mount path \"%s\" exists\n", path);
	pos = path;
	do {
		prev_pos = pos;
		pos = strchr(pos + 1, '/');

		if (pos) {
			if (pos[0] == '\0')
				break;

			/* Zero out the next '/' */
			*pos = '\0';
		}

		/* Do not allow `/./` or `/../` in the path. Also do not allow
		 * overwriting .. or . files
		 */
		if (unlikely(prev_pos[1] == '.' &&
			     /* /../ and /.. */
			     ((prev_pos[2] == '.' &&
			       (prev_pos[3] == '/' || prev_pos[3] == '\0')) ||
				/* OR /./ and /. */
			      (prev_pos[2] == '/' || prev_pos[2] == '\0')
			     ))) {
			uk_pr_err("'.' or '..' are not supported in mount paths.\n");
			return -EINVAL;
		}

		/* mkdir() with S_IRWXU */
		rc = mkdir(path, 0700);
		if (rc && errno != EEXIST)
			return -errno;

		/* Restore current '/' */
		if (pos)
			*pos = '/';

		/* Handle paths with multiple `/` */
		while (pos && pos[1] == '/')
			pos++;
	} while (pos);

	return 0;
}

static inline int vfscore_mount_volume(const struct vfscore_volume *vv)
{
	int rc;

	UK_ASSERT(vv);

	/*
	 * Sanity checks
	 */
	/* Only absolute paths as mountpoint */
	if (unlikely(!vv->path)) {
		uk_pr_err("fstab: Entry without mountpoint\n");
		return -EINVAL;
	}
	if (unlikely(vv->path[0] != '/')) {
		uk_pr_err("fstab: Mountpoint \"%s\" is not absolute\n",
			  vv->path);
		return -EINVAL;
	}
	/* Drv are mandatory */
	if (unlikely(!vv->drv || vv->drv[0] == '\0')) {
		uk_pr_err("fstab: Entry without filesystem driver\n");
		return -EINVAL;
	}

	uk_pr_debug("vfs.fstab: Mounting: %s:%s:%s:0x%lx:%s:0x%x...\n",
		    vv->sdev == NULL ? "none" : vv->sdev,
		    vv->path, vv->drv, vv->flags,
		    vv->opts == NULL ? "" : vv->opts,
		    vv->ukopts);

	if (vv->ukopts & LIBVFSCORE_UKOPT_MKMP) {
		rc = vfscore_ukopt_mkmp(vv->path);
		if (unlikely(rc < 0))
			return rc;
	}

#if CONFIG_LIBUKCPIO
	if (!strcmp(vv->drv, LIBVFSCORE_EXTRACT_DRV)) {
		return vfscore_extract_volume(vv);
	}
#endif /* CONFIG_LIBUKCPIO */
	return mount(vv->sdev == NULL ? "" : vv->sdev,
		     vv->path, vv->drv, vv->flags, vv->opts);
}

#ifdef CONFIG_LIBVFSCORE_AUTOMOUNT_UP
/**
 * ukopts must follow the pattern below, each option separated by
 * the character defined through LIBVFSCORE_FSTAB_UKOPTS_ARGS_SEP (e.g. with
 * LIBVFSCORE_FSTAB_UKOPTS_ARGS_SEP = ','):
 *	[<ukopt1>,<ukopt2>,<ukopt3>,...,<ukoptN>]
 *
 * Currently implemented, Unikraft Mount options:
 * - mkmp	Make mount point. Ensures that the specified mount point
 *		exists. If it does not exist in the current vfs, the directory
 *		structure is created.
 */
static unsigned int vfscore_volume_parse_ukopts(char *ukopts)
{
	char *pos;
	char *opt;
	int ret = 0x0;

	pos = ukopts;
	for (;;) {
		opt = next_arg(&pos, LIBVFSCORE_FSTAB_UKOPTS_ARGS_SEP);

		if (!opt) {
			if (!pos)
				break; /* end of options */
			continue; /* empty option */
		}

		if (!strcmp(opt, "mkmp")) {
			ret |= LIBVFSCORE_UKOPT_MKMP;
			continue;
		}

		uk_pr_warn("Ignoring unknown ukopt \"%s\"\n", opt);
	}

	return ret;
}

/**
 * Expected command-line argument format:
 *	vfs.fstab=[
 *		"<src_dev>:<mntpoint>:<fsdriver>[:<flags>:<opts>:<ukopts>]"
 *		"<src_dev>:<mntpoint>:<fsdriver>[:<flags>:<opts>:<ukopts>]"
 *		...
 *	]
 * These list elements are expected to be separated by whitespaces.
 * Mount options, flags and Unikraft mount options are optional.
 */

static int vfscore_parse_volume(char *v, struct vfscore_volume *vv)
{
	const char *strflags;
	char *pos;
	char *ukopts;

	UK_ASSERT(v);
	UK_ASSERT(vv);

	pos = v;
	vv->sdev   = next_arg(&pos, LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP);
	vv->path   = next_arg(&pos, LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP);
	vv->drv    = next_arg(&pos, LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP);
	strflags   = next_arg(&pos, LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP);
	vv->opts   = next_arg(&pos, LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP);
	ukopts     = next_arg(&pos, LIBVFSCORE_FSTAB_VOLUME_ARGS_SEP);

	/* Fill source device with empty string if missing */
	if (!vv->sdev)
		vv->sdev = "";

	/* Check that given path is absolute */
	if (unlikely(vv->path[0] != '/')) {
		uk_pr_err("vfs.fstab: Mountpoint \"%s\" is not absolute\n",
			  vv->path);
		return -EINVAL;
	}

	/* Parse flags */
	if (strflags && strflags[0] != '\0')
		vv->flags = strtol(strflags, NULL, 0);
	else
		vv->flags = 0;

	/* Parse ukopts */
	vv->ukopts = vfscore_volume_parse_ukopts(ukopts); /* handles NULL */

	uk_pr_debug("vfs.fstab: Parsed: %s:%s:%s:0x%lx:%s:%0x\n",
		    vv->sdev == NULL ? "none" : vv->sdev,
		    vv->path == NULL ? "" : vv->path,
		    vv->drv  == NULL ? "" : vv->drv,
		    vv->flags,
		    vv->opts == NULL ? "" : vv->opts,
		    vv->ukopts);
	return 0;
}

static int vfscore_automount_volumes_up(void)
{
	int mounted = 0;
	int rc;

	for (unsigned long i = 0; i < ARRAY_SIZE(fstab_up); ++i) {
		struct vfscore_volume vv;

		if (!fstab_up[i])
			continue;

		rc = vfscore_parse_volume(fstab_up[i], &vv);
		if (unlikely(rc < 0))
			return rc;

		rc = vfscore_mount_volume(&vv);
		if (unlikely(rc < 0))
			return rc;

		mounted++;
	}

	return mounted;
}
#endif /* !CONFIG_LIBVFSCORE_AUTOMOUNT_UP */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI || CONFIG_LIBVFSCORE_AUTOMOUNT_FB
static int vfscore_automount_volumes(const struct vfscore_volume *vvs[],
				     int count)
{
	int mounted = 0;
	int rc;

	UK_ASSERT(vvs);

	for (int i = 0; i < count; ++i) {
		if (!vvs[i])
			continue;

		rc = vfscore_mount_volume(vvs[i]);
		if (unlikely(rc < 0))
			return rc;

		mounted++;
	}

	return mounted;
}
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI || CONFIG_LIBVFSCORE_AUTOMOUNT_FB */


extern struct uk_list_head mount_list;

static void vfscore_autoumount(const struct uk_term_ctx *tctx __unused)
{
	struct mount *mp;
	int rc;

	uk_pr_debug("Unmounting filesystems...\n");
	uk_list_for_each_entry_reverse(mp, &mount_list, mnt_list) {
		/* For now, flags = 0 is enough. */
		rc = VFS_UNMOUNT(mp, 0);
		if (unlikely(rc))
			uk_pr_err("Failed to unmount %s: error %d.\n",
				  mp->m_path, rc);
	}
}

static int vfscore_automount(struct uk_init_ctx *ictx __unused)
{
	int rc;
	int count __maybe_unused = 0;

#if CONFIG_LIBVFSCORE_AUTOMOUNT_CI
	uk_pr_debug("Mounting volumes from compiled-in table (%lu entries)...\n",
		    ARRAY_SIZE(fstab_ci) - 1);
	rc = vfscore_automount_volumes(fstab_ci, ARRAY_SIZE(fstab_ci));
	if (unlikely(rc < 0))
		goto err_umount;
	count = rc;
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_CI */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_UP
	uk_pr_debug("Mounting volumes from user-provided table...\n");
	rc = vfscore_automount_volumes_up();
	if (unlikely(rc < 0))
		goto err_umount;
	count += rc;
	if (rc == 0)
		uk_pr_debug(" Note: Table is empty.\n");

#if CONFIG_LIBVFSCORE_AUTOMOUNT_FB
	if (rc == 0) {
		uk_pr_debug("Mounting volumes from fallback table (%lu entries)...\n",
			    ARRAY_SIZE(fstab_fb) - 1);
		rc = vfscore_automount_volumes(fstab_fb, ARRAY_SIZE(fstab_fb));
		if (unlikely(rc < 0))
			goto err_umount;
		count += rc;
	}
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_FB */

#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_UP */

	uk_pr_debug("%d volumes mounted in total.\n", count);
	return 0;

err_umount: __maybe_unused
	vfscore_autoumount(NULL);
	return rc;
}

uk_rootfs_initcall_prio(vfscore_automount, vfscore_autoumount, 4);
