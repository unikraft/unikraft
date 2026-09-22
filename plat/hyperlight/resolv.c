/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * The guest's /etc/resolv.conf, from the host.
 *
 * A rootfs image ships a resolver configuration naming public resolvers,
 * which is right on a laptop and wrong in a pod, where names resolve
 * through the cluster's resolver and a search list that includes the
 * pod's namespace.  That file belongs to the place the guest runs, so
 * the host may hand one over: GetResolvConf returns its content, which
 * is written over /etc/resolv.conf once the rootfs is mounted, before
 * the loaded program's main(), and again on every restore from a
 * snapshot, since the snapshot carries the file of the machine that
 * took it.  An empty answer, or no such host function, leaves the
 * image's file alone.
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/init.h>
#include <uk/print.h>

#include <hyperlight-x86/hcall.h>
#include <hyperlight-x86/resolv.h>

#if CONFIG_LIBVFSCORE
/* vfscore's syscall implementations, callable from the kernel without a
 * libc, as lib/ukcpio does.
 */
int uk_syscall_do_open(const char *, int, mode_t);
int uk_syscall_do_close(int);
ssize_t uk_syscall_do_write(int, const void *, size_t);
int uk_syscall_do_mkdir(const char *, mode_t);
int uk_syscall_do_rename(const char *, const char *);

#define RESOLV_CONF_PATH	"/etc/resolv.conf"
#define RESOLV_CONF_TMP		"/etc/.resolv.conf.host"

/* Write @data to @path in full.  A write of nothing is an error too:
 * it would otherwise loop forever.
 */
static int write_file(const char *path, const char *data, __sz len)
{
	__sz off = 0;
	int fd, rc = 0;

	fd = uk_syscall_do_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return fd;
	while (off < len) {
		ssize_t n = uk_syscall_do_write(fd, data + off, len - off);

		if (n <= 0) {
			rc = n < 0 ? (int)n : -EIO;
			break;
		}
		off += (__sz)n;
	}
	uk_syscall_do_close(fd);
	return rc;
}

/* Replace @path with @data: written to a sibling first and renamed over
 * it, so a failure leaves the file that was there, and a symlink the
 * image may keep at @path is replaced rather than followed.
 */
static int replace_file(const char *path, const char *tmp,
			const char *data, __sz len)
{
	int rc = write_file(tmp, data, len);

	if (rc < 0)
		return rc;
	return uk_syscall_do_rename(tmp, path);
}

void hyperlight_resolv_apply(void)
{
	char *buf;
	__sz cap, len = 0;
	int rc;

	/* The content arrives on the input stack, so hl_hcall_max_payload()
	 * bytes always hold it.  This runs twice in a guest's life, so the
	 * buffer is not kept.
	 */
	cap = hl_hcall_max_payload();
	buf = cap ? uk_malloc(uk_alloc_get_default(), cap) : __NULL;
	if (unlikely(!buf)) {
		uk_pr_err("resolv: no memory for the host's resolv.conf\n");
		return;
	}
	/* No such host function, or nothing set: the image's file stands. */
	if (hl_hcall_string("GetResolvConf", __NULL, 0, buf, cap, &len) < 0 ||
	    len == 0)
		goto out;

	rc = uk_syscall_do_mkdir("/etc", 0755);
	if (rc < 0 && rc != -EEXIST) {
		uk_pr_err("resolv: cannot create /etc: %d\n", rc);
		goto out;
	}
	rc = replace_file(RESOLV_CONF_PATH, RESOLV_CONF_TMP, buf, len);
	if (rc < 0)
		uk_pr_err("resolv: cannot write " RESOLV_CONF_PATH ": %d\n", rc);
out:
	uk_free(uk_alloc_get_default(), buf);
}
#else /* !CONFIG_LIBVFSCORE */
/* No filesystem to write to: a native kernel resolves nothing. */
void hyperlight_resolv_apply(void)
{
}
#endif /* CONFIG_LIBVFSCORE */

/* After the rootfs is mounted and posix-environ, before the ELF loader's
 * main(), like the host environment (dispatch.c).
 */
static int hyperlight_resolv_init(struct uk_init_ctx *ictx __unused)
{
	hyperlight_resolv_apply();
	return 0;
}
uk_late_initcall(hyperlight_resolv_init, 0x0);
