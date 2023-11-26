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
 */

#include <errno.h>
#include <uk/arch/types.h>
#include <uk/config.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <linuxu/console.h>
#include <linuxu/irq.h>
#include <linuxu/syscall.h>
#include <uk/plat/console.h>
#include <uk/plat/bootstrap.h>
#include <uk/assert.h>
#include <uk/errptr.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/libparam.h>

#define MB2B		(1024 * 1024)

static __u32 heap_size = CONFIG_LINUXU_DEFAULT_HEAPMB;
UK_LIB_PARAM(heap_size, __u32);

static const char *initrd;
UK_LIB_PARAM_STR(initrd);

static void __linuxu_plat_heap_init(void)
{
	void *pret;
	__u32 len;
	int rc;

	len = heap_size * MB2B;
	uk_pr_info("Allocate memory for heap (%u MiB)\n", heap_size);

	/**
	 * Allocate heap memory
	 */
	if (len > 0) {
		pret = sys_mapmem(NULL, len);
		if (PTRISERR(pret)) {
			rc = PTR2ERR(pret);
			uk_pr_err("Failed to allocate memory for heap: %d\n",
				  rc);
		} else {
			rc = ukplat_memregion_list_insert(
				&ukplat_bootinfo_get()->mrds,
				&(struct ukplat_memregion_desc){
					.vbase = (__vaddr_t)pret,
					.pbase = (__paddr_t)pret,
					.len   = len,
					.type  = UKPLAT_MEMRT_FREE,
					.flags = UKPLAT_MEMRF_READ |
						 UKPLAT_MEMRF_WRITE |
						 UKPLAT_MEMRF_MAP,
				});
			if (unlikely(rc < 0))
				uk_pr_err("Failed to add heap memory region descriptor.");
		}
	}
}

static void __linuxu_plat_initrd_init(void)
{
	void *pret;
	__u32 len;
	int rc;
	struct k_stat file_info;

	if (!initrd) {
		uk_pr_debug("No initrd present.\n");
	} else {
		uk_pr_debug("Mapping in initrd file: %s\n", initrd);
		int initrd_fd = sys_open(initrd, K_O_RDONLY, 0);

		if (initrd_fd < 0)
			uk_pr_crit("Failed to open %s for initrd\n", initrd);

		/**
		 * Find initrd file size
		 */
		if (sys_fstat(initrd_fd, &file_info) < 0) {
			uk_pr_crit("sys_fstat failed for initrd file\n");
			sys_close(initrd_fd);
		}
		len = file_info.st_size;
		/**
		 * Allocate initrd memory
		 */
		if (len > 0) {
			pret = sys_mmap(NULL, len,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_PRIVATE, initrd_fd, 0);
			if (PTRISERR(pret)) {
				rc = PTR2ERR(pret);
				uk_pr_crit("Failed to memory-map initrd: %d\n",
					   rc);
				sys_close(initrd_fd);
			}

			rc = ukplat_memregion_list_insert(
				&ukplat_bootinfo_get()->mrds,
				&(struct ukplat_memregion_desc){
					.vbase = (__vaddr_t)pret,
					.pbase = (__paddr_t)pret,
					.len   = len,
					.type  = UKPLAT_MEMRT_INITRD,
					.flags = UKPLAT_MEMRF_READ |
						 UKPLAT_MEMRF_WRITE |
						 UKPLAT_MEMRF_MAP,
				});
			if (unlikely(rc < 0))
				uk_pr_err("Failed to add initrd memory region descriptor.");
		} else {
			uk_pr_info("Ignoring empty initrd file.\n");
			sys_close(initrd_fd);
		}
	}
}

void _liblinuxuplat_entry(int argc, char *argv[]) __noreturn;

void _liblinuxuplat_entry(int argc, char *argv[])
{
	/*
	 * Initialize platform console
	 */
	_liblinuxuplat_init_console();

	__linuxu_plat_heap_init();

	__linuxu_plat_initrd_init();

	ukplat_irq_init();

	/*
	 * Enter Unikraft
	 */
	ukplat_entry(argc, argv);
}
