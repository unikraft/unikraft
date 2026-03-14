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
 */

#include <stddef.h>
#include <uk/arch/limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <uk/essentials.h>
#include <uk/config.h>
#include <sys/sysinfo.h>
#include <uk/syscall.h>

#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#endif /* CONFIG_HAVE_PAGING */

/**
 * The Unikraft `struct utsname` structure.
 *
 * * `sysname` is set to "Unikraft".
 * * `nodename` is set to "unikraft".
 * * `release` is set to * `5-{Unikraft Release Codename}` for glibc
 *   compatibility (see the comments below).
 * * `version` is set to the Unikraft full version.
 * * `machine` is set to the current architecture.
 */
static struct utsname utsname = {
	.sysname	= "Unikraft",
	.nodename	= "unikraft",
	/* glibc and some other applications look into the release field to
	 * check the kernel version: We prepend a proper kernel version in order
	 * to be "new enough" for it.
	 */
	.release	= "5.15.148-" STRINGIFY(UK_CODENAME),
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

UK_SYSCALL_R_DEFINE(int, sysinfo, struct sysinfo *, info)
{
#ifdef CONFIG_HAVE_PAGING
	struct uk_pagetable *pt;
	__sz total_memory;
	unsigned int mem_unit = 1;
#endif /* CONFIG_HAVE_PAGING */

	if (!info)
		return -EFAULT;

	memset(info, 0, sizeof(*info));

	info->procs = 1; /* number of processes */

#ifdef CONFIG_HAVE_PAGING
	pt = ukplat_pt_get_active();

	total_memory = pt->fa->total_memory;
	while (total_memory > __UL_MAX) {
		total_memory >>= 1;
		mem_unit <<= 1;
	}

	info->totalram = (unsigned long) (pt->fa->total_memory / mem_unit);
	info->freeram = (unsigned long) (pt->fa->free_memory / mem_unit);
	info->mem_unit = mem_unit;
#endif /* CONFIG_HAVE_PAGING */

	return 0;
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

#ifdef CONFIG_LIBPOSIX_USER
	if (name == _SC_GETPW_R_SIZE_MAX)
		return -1;
#endif /* CONFIG_LIBPOSIX_USER */

#ifdef CONFIG_HAVE_PAGING
	if (name == _SC_PHYS_PAGES) {
		struct uk_pagetable *pt;

		pt = ukplat_pt_get_active();
		return pt->fa->total_memory / PAGE_SIZE;
	}

	if (name == _SC_AVPHYS_PAGES) {
		struct uk_pagetable *pt;

		pt = ukplat_pt_get_active();
		return pt->fa->free_memory / PAGE_SIZE;
	}
#endif /* CONFIG_HAVE_PAGING */

#if CONFIG_LIBPOSIX_FDTAB
	if (name == _SC_OPEN_MAX)
		return CONFIG_LIBPOSIX_FDTAB_MAXFDS;
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

UK_SYSCALL_R_DEFINE(int, sethostname, const char*, name, size_t, len)
{
	if (name == NULL) {
		return -EFAULT;
	}

	if (len + 1 > sizeof(utsname.nodename)) {
		return -EINVAL;
	}

	memcpy(utsname.nodename, name, len);
	utsname.nodename[len] = '\0';
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

UK_SYSCALL_R_DEFINE(int, getcpu, unsigned int *, cpu, unsigned int *, node,
		    void *, tcache)
{
	/* tcache is ignored since Linux 2.6.24 */

	if (cpu)
		*cpu = 0;

	if (node)
		*node = 0;

	return 0;
}
