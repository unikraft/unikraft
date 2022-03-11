/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Unikraft bootstrapping
 *
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

#include <uk/config.h>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#if CONFIG_LIBUKBOOT_INITBBUDDY
#include <uk/allocbbuddy.h>
#elif CONFIG_LIBUKBOOT_INITREGION
#include <uk/allocregion.h>
#elif CONFIG_LIBUKBOOT_INITMIMALLOC
#include <uk/mimalloc.h>
#elif CONFIG_LIBUKBOOT_INITTLSF
#include <uk/tlsf.h>
#elif CONFIG_LIBUKBOOT_INITTINYALLOC
#include <uk/tinyalloc.h>
#endif
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#endif
#include <uk/arch/lcpu.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/memory.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/irq.h>
#include <uk/plat/time.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/init.h>
#include <uk/argparse.h>
#ifdef CONFIG_LIBUKLIBPARAM
#include <uk/libparam.h>
#endif /* CONFIG_LIBUKLIBPARAM */
#ifdef CONFIG_LIBUKSP
#include <uk/sp.h>
#endif
#include "banner.h"

int main(int argc, char *argv[]) __weak;

/* defined in <uk/plat.h> */
void ukplat_entry_argp(char *arg0, char *argb, __sz argb_len)
{
	static char *argv[CONFIG_LIBUKBOOT_MAXNBARGS];
	int argc = 0;

	if (arg0) {
		argv[0] = arg0;
		argc += 1;
	}
	if (argb && argb_len) {
		argc += uk_argnparse(argb, argb_len, arg0 ? &argv[1] : &argv[0],
				     arg0 ? (CONFIG_LIBUKBOOT_MAXNBARGS - 1)
					  : CONFIG_LIBUKBOOT_MAXNBARGS);
	}
	ukplat_entry(argc, argv);
}

/* defined in <uk/plat.h> */
void ukplat_entry(int argc, char *argv[])
{
	int kern_args = 0;
	int rc = 0;
#if CONFIG_LIBUKALLOC
	struct uk_alloc *a = NULL;
#endif
#if !CONFIG_LIBUKBOOT_NOALLOC
	struct ukplat_memregion_desc md;
#endif
#if CONFIG_LIBUKSCHED
	struct uk_sched *s = NULL;
#endif
#if CONFIG_LIBUKALLOC
	void *tls;
#endif
	uk_ctor_func_t *ctorfn;
	uk_init_func_t *initfn;
	int i;

	uk_pr_info("Unikraft constructor table at %p - %p\n",
		   &uk_ctortab_start[0], &uk_ctortab_end);
	uk_ctortab_foreach(ctorfn, uk_ctortab_start, uk_ctortab_end) {
		UK_ASSERT(*ctorfn);
		uk_pr_debug("Call constructor: %p())...\n", *ctorfn);
		(*ctorfn)();
	}

#ifdef CONFIG_LIBUKLIBPARAM
	rc = (argc > 1) ? uk_libparam_parse(argv[0], argc - 1, &argv[1]) : 0;
	if (unlikely(rc < 0))
		uk_pr_crit("Failed to parse the kernel argument\n");
	else {
		kern_args = rc;
		uk_pr_info("Found %d library args\n", kern_args);
	}
#endif /* CONFIG_LIBUKLIBPARAM */

#if !CONFIG_LIBUKBOOT_NOALLOC
	/* initialize memory allocator
	 * FIXME: allocators are hard-coded for now
	 */
	uk_pr_info("Initialize memory allocator...\n");
	ukplat_memregion_foreach(&md, UKPLAT_MEMRF_ALLOCATABLE) {
#if CONFIG_UKPLAT_MEMRNAME
		uk_pr_debug("Try memory region: %p - %p (flags: 0x%02x, name: %s)...\n",
			    md.base, (void *)((size_t)md.base + md.len),
			    md.flags, md.name);
#else
		uk_pr_debug("Try memory region: %p - %p (flags: 0x%02x)...\n",
			    md.base, (void *)((size_t)md.base + md.len),
			    md.flags);
#endif

		/* try to use memory region to initialize allocator
		 * if it fails, we will try  again with the next region.
		 * As soon we have an allocator, we simply add every
		 * subsequent region to it
		 */
		if (!a) {
#if CONFIG_LIBUKBOOT_INITBBUDDY
			a = uk_allocbbuddy_init(md.base, md.len);
#elif CONFIG_LIBUKBOOT_INITREGION
			a = uk_allocregion_init(md.base, md.len);
#elif CONFIG_LIBUKBOOT_INITMIMALLOC
			a = uk_mimalloc_init(md.base, md.len);
#elif CONFIG_LIBUKBOOT_INITTLSF
			a = uk_tlsf_init(md.base, md.len);
#elif CONFIG_LIBUKBOOT_INITTINYALLOC
			a = uk_tinyalloc_init(md.base, md.len);
#endif
		} else {
			uk_alloc_addmem(a, md.base, md.len);
		}
	}
	if (unlikely(!a))
		uk_pr_warn("No suitable memory region for memory allocator. Continue without heap\n");
	else {
		rc = ukplat_memallocator_set(a);
		if (unlikely(rc != 0))
			UK_CRASH("Could not set the platform memory allocator\n");
	}
#endif

#if CONFIG_LIBUKALLOC
	/* Allocate a TLS for this execution context */
	tls = uk_memalign(a,
			  ukarch_tls_area_align(),
			  ukarch_tls_area_size()
			  + ukarch_ectx_size()
			  + ukarch_ectx_align());
	if (!tls) {
		UK_CRASH("Failed to allocate and initialize TLS\n");
	}
	/* Copy from TLS master template */
	ukarch_tls_area_copy(tls);
	/* Activate TLS */
	ukplat_tlsp_set(ukarch_tls_pointer(tls));
#endif /* CONFIG_LIBUKALLOC */

#if CONFIG_LIBUKALLOC
	uk_pr_info("Initialize IRQ subsystem...\n");
	rc = ukplat_irq_init(a);
	if (unlikely(rc != 0))
		UK_CRASH("Could not initialize the platform IRQ subsystem\n");
#endif

	/* On most platforms the timer depend on an initialized IRQ subsystem */
	uk_pr_info("Initialize platform time...\n");
	ukplat_time_init();

#if CONFIG_LIBUKSCHED
	/* Init scheduler. */
	s = uk_sched_default_init(a);
	if (unlikely(!s))
		UK_CRASH("Could not initialize the scheduler\n");
	uk_sched_start(s);
#endif

	argc -= kern_args;
	argv = &argv[kern_args];

	/* Enable interrupts before starting the application */
	ukplat_lcpu_enable_irq();

	/**
	 * Run init table
	 */
	uk_pr_info("Init Table @ %p - %p\n",
		   &uk_inittab_start[0], &uk_inittab_end);
	uk_inittab_foreach(initfn, uk_inittab_start, uk_inittab_end) {
		UK_ASSERT(*initfn);
		uk_pr_debug("Call init function: %p()...\n", *initfn);
		rc = (*initfn)();
		if (rc < 0) {
			uk_pr_err("Init function at %p returned error %d\n",
				  *initfn, rc);
			rc = UKPLAT_CRASH;
			goto exit;
		}
	}

#ifdef CONFIG_LIBUKSP
	uk_stack_chk_guard_setup();
#endif

	print_banner(stdout);
	fflush(stdout);

	/*
	 * Application
	 *
	 * We are calling the application constructors right before calling
	 * the application's main(). All of our Unikraft systems, VFS,
	 * networking stack is initialized at this point. This way we closely
	 * mimic what a regular user application (e.g., BSD, Linux) would expect
	 * from its OS being initialized.
	 */
	uk_pr_info("Pre-init table at %p - %p\n",
		   &__preinit_array_start[0], &__preinit_array_end);
	uk_ctortab_foreach(ctorfn,
			   __preinit_array_start, __preinit_array_end) {
		if (!*ctorfn)
			continue;

		uk_pr_debug("Call pre-init constructor: %p()...\n", *ctorfn);
		(*ctorfn)();
	}

	uk_pr_info("Constructor table at %p - %p\n",
		   &__init_array_start[0], &__init_array_end);
	uk_ctortab_foreach(ctorfn, __init_array_start, __init_array_end) {
		if (!*ctorfn)
			continue;

		uk_pr_debug("Call constructor: %p()...\n", *ctorfn);
		(*ctorfn)();
	}

	uk_pr_info("Calling main(%d, [", argc);
	for (i = 0; i < argc; ++i) {
		uk_pr_info("'%s'", argv[i]);
		if ((i + 1) < argc)
			uk_pr_info(", ");
	}
	uk_pr_info("])\n");

	rc = main(argc, argv);
	uk_pr_info("main returned %d, halting system\n", rc);
	rc = (rc != 0) ? UKPLAT_CRASH : UKPLAT_HALT;

exit:
	ukplat_terminate(rc); /* does not return */
}
