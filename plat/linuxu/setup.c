/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/config.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <linuxu/setup.h>
#include <linuxu/console.h>
#include <linuxu/syscall.h>
#include <uk/plat/console.h>
#include <uk/plat/bootstrap.h>
#include <uk/assert.h>
#include <uk/errptr.h>

struct liblinuxuplat_opts _liblinuxuplat_opts = { 0 };

#define _coutk_chr(c)				\
	ukplat_coutk((char *) &(c), 1)
#define _coutk_str(str)				\
	ukplat_coutk((str), strlen(str))

static const char *sopts = "h?Vm:";
static struct option lopts[] = {
	{"help",	no_argument,		NULL,	'h'},
	{"version",	no_argument,		NULL,	'V'},
	{"heapmem",	required_argument,	NULL,	'm'},
	{NULL, 0, NULL, 0}
};

static void version(void)
{
	_coutk_str("Unikraft "
		   STRINGIFY(UK_CODENAME) " "
		   STRINGIFY(UK_FULLVERSION) "\n");
}

static void usage(const char *progname)
{
	_coutk_str("Usage: ");
	_coutk_str(progname);
	_coutk_str(" [[LINUXU PLATFORM ARGUMENT]].. -- [[ARGUMENT]]..\n\n");
	_coutk_str("Unikraft LinuxU platform arguments:\n");
	_coutk_str("Mandatory arguments to long options are mandatory for short options too.\n");
	_coutk_str("  -h, --help                 display this help and exit\n");
	_coutk_str("  -V, --version              display Unikraft version and exit\n");
	_coutk_str("  -m, --heapmem [MBYTES]     allocate MBYTES as heap memory\n");
}

static int parseopts(int argc, char *argv[], struct liblinuxuplat_opts *opts)
{
	const char *progname = argv[0];
	char *old_optarg;
	int old_optind;
	int old_optopt;
	char **argvopt;
	int opt, optidx;
	int ret;

	/*
	 * Clear & set default options
	 */
	memset(opts, 0, sizeof(*opts));
	_liblinuxuplat_opts.heap.len = (size_t)(CONFIG_LINUXU_DEFAULT_HEAPMB)
					* 1024 * 1024;

	/*
	 * Parse arguments
	 */
	old_optind = optind;
	old_optopt = optopt;
	old_optarg = optarg;
	argvopt = argv;
	optind = 1;
	while ((opt = getopt_long(argc, argvopt, sopts, lopts, &optidx)) >= 0) {
		switch (opt) {
		case 'h':
		case '?': /* usage */
			usage(progname);
			ukplat_halt();
		case 'V': /* version */
			version();
			ukplat_halt();
		case 'm':
			_liblinuxuplat_opts.heap.len = (((size_t)
							 strtoul(optarg,
								 NULL, 10))
							* 1024 * 1024);
			break;
		default:
			_coutk_str(progname);
			_coutk_str(": invalid option: -");
			_coutk_chr(opt);
			_coutk_str("\n");
			usage(progname);
			ret = -EINVAL;
			goto out;
		}
	}
	ret = optind;

out:
	/*
	 * Restore getopt state for later calls
	 */
	optind = old_optind;
	optopt = old_optopt;
	optarg = old_optarg;
	return ret;
}

void _liblinuxuplat_entry(int argc, char *argv[]) __noreturn;

void _liblinuxuplat_entry(int argc, char *argv[])
{
	char *progname = argv[0];
	int ret;
	void *pret;

	/*
	 * Initialize platform console
	 */
	_liblinuxuplat_init_console();

	/*
	 * Parse LinuxU platform arguments
	 */
	if ((ret = parseopts(argc, argv, &_liblinuxuplat_opts)) < 0)
		ukplat_crash();

	/*
	 * Remove arguments related to LinuxU platform
	 * and set progname again as argument 0
	 */
	argc -= (ret - 1);
	argv += (ret - 1);
	argv[0] = progname;

	/*
	 * Allocate heap memory
	 */
	if (_liblinuxuplat_opts.heap.len > 0) {
		pret = sys_mapmem(NULL, _liblinuxuplat_opts.heap.len);
		if (PTRISERR(pret))
			uk_printd(DLVL_ERR, "Failed to allocate memory for heap: %d\n", PTR2ERR(pret));
		else
			_liblinuxuplat_opts.heap.base = pret;
	}

	/*
	 * Enter Unikraft
	 */
	ukplat_entry(argc, argv);
}
