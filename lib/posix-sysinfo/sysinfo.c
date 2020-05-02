/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Badoiu Vlad-Andrei <vlad_andrei.badoiu@stud.acs.upb.ro>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stddef.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <uk/essentials.h>
#include <uk/config.h>
#include <sys/sysinfo.h>
#include <uk/syscall.h>

#if CONFIG_LIBVFSCORE
/* For FDTABLE_MAX_FILES. */
#include <vfscore/file.h>
#endif

static struct utsname utsname = {
	.sysname	= "Unikraft",
	.nodename	= "unikraft",
	/* glibc looks into the release field to check the kernel version:
	 * We prepend '5-' in order to be "new enough" for it.
	 */
	.release	= "5-" STRINGIFY(UK_CODENAME),
	.version	= STRINGIFY(UK_FULLVERSION),
#ifdef CONFIG_ARCH_X86_64
	.machine	= "x86_64"
#elif CONFIG_ARCH_ARM_64
	.machine	= "arm64"
#elif CONFIG_ARCH_ARM_32
	.machine	= "arm32"
#else
#error "Set your machine architecture!"
#endif
};

int sysinfo(struct sysinfo *info __unused)
{
	return -1;
}

long fpathconf(int fd __unused, int name __unused)
{
	return 0;
}

long pathconf(const char *path __unused, int name __unused)
{
	return 0;
}

long sysconf(int name)
{
	if (name == _SC_NPROCESSORS_ONLN)
		return 1;

	if (name == _SC_PAGESIZE)
		return __PAGE_SIZE;

#if CONFIG_LIBVFSCORE
	if (name == _SC_OPEN_MAX)
		return FDTABLE_MAX_FILES;
#endif

	return 0;
}

size_t confstr(int name __unused, char *buf __unused, size_t len __unused)
{
	return 0;
}

int getpagesize(void)
{
	return __PAGE_SIZE;
}

UK_SYSCALL_R_DEFINE(int, uname, struct utsname *, buf)
{
	if (buf == NULL)
		return -EFAULT;

	memcpy(buf, &utsname, sizeof(struct utsname));
	return 0;
}

int sethostname(const char *name, size_t len)
{
	if (name == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (len > sizeof(utsname.nodename)) {
		errno = EINVAL;
		return -1;
	}

	strncpy(utsname.nodename, name, len);
	if (len < sizeof(utsname.nodename))
		utsname.nodename[len] = 0;
	return 0;
}

int gethostname(char *name, size_t len)
{
	struct utsname buf;
	size_t node_len;
	int rc;

	rc = uname(&buf);
	if (rc)
		return -1;

	node_len = strlen(buf.nodename) + 1;
	if (node_len > len) {
		errno = ENAMETOOLONG;
		return -1;
	}

	strncpy(name, buf.nodename, len);

	return 0;
}
