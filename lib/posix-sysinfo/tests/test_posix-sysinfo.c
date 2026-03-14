/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <uk/test.h>
#include <stddef.h>
#include <uk/arch/limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/config.h>
#include <uk/print.h>

#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#endif /* CONFIG_HAVE_PAGING */

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sysinfo_no_info)
{
	struct sysinfo *info = NULL;
	UK_TEST_EXPECT(sysinfo(info) == -1);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sysinfo_no_paging)
{
	struct sysinfo info;

	UK_TEST_EXPECT(sysinfo(&info) == 0);
	UK_TEST_EXPECT(info.uptime == 0);
	UK_TEST_EXPECT(info.loads[0] == 0);
	UK_TEST_EXPECT(info.loads[1] == 0);
	UK_TEST_EXPECT(info.loads[2] == 0);
#ifdef CONFIG_HAVE_PAGING
	struct uk_pagetable *pt = ukplat_pt_get_active();
	UK_TEST_EXPECT(info.totalram == (unsigned long)(pt->fa->total_memory / info.mem_unit));
	UK_TEST_EXPECT(info.freeram <= info.totalram);
#else
	UK_TEST_EXPECT(info.totalram == 0);
	UK_TEST_EXPECT(info.freeram == 0);
#endif
	UK_TEST_EXPECT(info.sharedram == 0);
	UK_TEST_EXPECT(info.bufferram == 0);
	UK_TEST_EXPECT(info.totalswap == 0);
	UK_TEST_EXPECT(info.freeswap == 0);
	UK_TEST_EXPECT(info.procs == 1);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sysinfo_with_paging)
{
#ifdef CONFIG_HAVE_PAGING
	struct uk_pagetable *pt;
	struct sysinfo info;

	__sz original_total, original_free;

	pt = ukplat_pt_get_active();

	original_total = pt->fa->total_memory;
	original_free = pt->fa->free_memory;

	pt->fa->total_memory = (__sz)__UL_MAX;
	pt->fa->free_memory = (__sz)__UL_MAX / 2;

	UK_TEST_EXPECT(sysinfo(&info) == 0);

	UK_TEST_EXPECT(info.mem_unit == 1);
	UK_TEST_EXPECT(info.totalram == (unsigned long)(__UL_MAX));
	UK_TEST_EXPECT(info.freeram == (unsigned long)(__UL_MAX / 2));

	pt->fa->total_memory = original_total;
	pt->fa->free_memory = original_free;

#endif /* CONFIG_HAVE_PAGING */
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_fpathconf)
{
	int fd __unused = 0;
	int name __unused = 0;
	long expected_result = 0;
	UK_TEST_EXPECT(fpathconf(fd, name) == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_pathconf)
{
	const char *path = NULL;
	int name = 0;
	long expected_result = 0;
	UK_TEST_EXPECT(pathconf(path, name) == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_nprocessors_onln)
{
	int name = _SC_NPROCESSORS_ONLN;
	long expected_result = 1;
	UK_TEST_EXPECT(sysconf(name) == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_pagesize)
{
	int name = _SC_PAGESIZE;
	long expected_result = __PAGE_SIZE;
	UK_TEST_EXPECT(sysconf(name) == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_phys_pages)
{
	int name = _SC_PHYS_PAGES;
#ifdef CONFIG_HAVE_PAGING
	struct uk_pagetable *pt;
	pt = ukplat_pt_get_active();

	long expected_result = pt->fa->total_memory / PAGE_SIZE;

	UK_TEST_EXPECT(sysconf(name) == expected_result);
#else
	UK_TEST_EXPECT(sysconf(name) == 0);
#endif /* CONFIG_HAVE_PAGING */
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_avphys_pages)
{
	int name = _SC_AVPHYS_PAGES;
#ifdef CONFIG_HAVE_PAGING
	struct uk_pagetable *pt;
	pt = ukplat_pt_get_active();

	long expected_result = pt->fa->free_memory / PAGE_SIZE;
	UK_TEST_EXPECT(sysconf(name) == expected_result);
#else
	UK_TEST_EXPECT(sysconf(name) == 0);
#endif /* CONFIG_HAVE_PAGING */
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_open_max)
{
	int name = _SC_OPEN_MAX;
#if CONFIG_LIBPOSIX_FDTAB
	long expected_result = CONFIG_LIBPOSIX_FDTAB_MAXFDS;
	UK_TEST_EXPECT(sysconf(name) == expected_result);
#else
	UK_TEST_EXPECT(sysconf(name) == 0);
#endif
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_getpw_r_size_max)
{
	int name = _SC_GETPW_R_SIZE_MAX;
#ifdef CONFIG_LIBPOSIX_USER
	long expected_result = -1;
	UK_TEST_EXPECT(sysconf(name) == expected_result);
#else
	UK_TEST_EXPECT(sysconf(name) == 0);
#endif /* CONFIG_LIBPOSIX_USER */
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sc_unkown_name)
{
	int name = -1;
	long expected_result = 0;
	UK_TEST_EXPECT(sysconf(name) == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_confstr)
{
	int name = 0;
	char *buf = NULL;
	size_t len = 0;
	size_t expected_result = 0;
	UK_TEST_EXPECT(confstr(name, buf, len) == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_getpagesize)
{
	int expected_result = __PAGE_SIZE;
	UK_TEST_EXPECT(getpagesize() == expected_result);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_uname_null_buf)
{
	struct utsname *buf = NULL;
	UK_TEST_EXPECT(uname(buf) == -1);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_uname_valid_buf)
{
	struct utsname buf;
	UK_TEST_EXPECT(uname(&buf) == 0);
	UK_TEST_EXPECT(strcmp(buf.sysname, "Unikraft") == 0);
	UK_TEST_EXPECT(strcmp(buf.nodename, "unikraft") == 0);
	UK_TEST_EXPECT(strcmp(buf.release, "5.15.148-" STRINGIFY(UK_CODENAME)) == 0);
	UK_TEST_EXPECT(strcmp(buf.version, STRINGIFY(UK_FULLVERSION)) == 0);
#ifdef CONFIG_ARCH_X86_64
	UK_TEST_EXPECT(strcmp(buf.machine, "x86_64") == 0);
#elif CONFIG_ARCH_ARM_64
	UK_TEST_EXPECT(strcmp(buf.machine, "arm64") == 0);
#elif CONFIG_ARCH_ARM_32
	UK_TEST_EXPECT(strcmp(buf.machine, "arm32") == 0);
#endif
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sethostname_null_name)
{
	const char *name = NULL;
	size_t len = 0;
	UK_TEST_EXPECT(sethostname(name, len) == -1);
	sethostname("unikraft", strlen("unikraft"));
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sethostname_too_long_name)
{
	const char *name = "This name is too long for the nodename field in the utsname struct";
	size_t len = strlen(name);
	UK_TEST_EXPECT(sethostname(name, len) == -1);
	sethostname("unikraft", strlen("unikraft"));
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_sethostname_valid_name)
{
	const char *name = "valid-name";
	size_t len = strlen(name);
	struct utsname buf;

	UK_TEST_EXPECT(sethostname(name, len) == 0);

	uname(&buf);
	UK_TEST_EXPECT(strcmp(buf.nodename, name) == 0);

	sethostname("unikraft", strlen("unikraft"));
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_gethostname_valid_case)
{
	char name[sizeof(((struct utsname *)0)->nodename)];
	size_t len = sizeof(name);
	struct utsname buf;

	uname(&buf);

	UK_TEST_EXPECT(gethostname(name, len) == 0);
	UK_TEST_EXPECT(strcmp(buf.nodename, name) == 0);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_gethostname_buf_too_long)
{
	/* "unikraft" + '\0' = 9 bytes; a buffer of 4 is too small */
	char name[4];
	size_t len = sizeof(name);
	UK_TEST_EXPECT(gethostname(name, len) == -1);
	UK_TEST_EXPECT(errno == ENAMETOOLONG);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_getcpu_valid)
{
	unsigned int cpu = 99;
	unsigned int node = 99;
	UK_TEST_EXPECT(getcpu(&cpu, &node, NULL) == 0);
	UK_TEST_EXPECT(cpu == 0);
	UK_TEST_EXPECT(node == 0);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_getcpu_null_args)
{
	UK_TEST_EXPECT(getcpu(NULL, NULL, NULL) == 0);
}

UK_TESTCASE(posix_sysinfo_testsuite, posix_sysinfo_getcpu_partial)
{
	unsigned int cpu = 99;
	UK_TEST_EXPECT(getcpu(&cpu, NULL, NULL) == 0);
	UK_TEST_EXPECT(cpu == 0);
}

uk_testsuite_register(posix_sysinfo_testsuite, NULL);
