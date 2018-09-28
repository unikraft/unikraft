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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <uk/config.h>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#if CONFIG_LIBUKALLOC && CONFIG_LIBUKALLOCBBUDDY && CONFIG_LIBUKBOOT_INITALLOC
#include <uk/allocbbuddy.h>
#endif
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#endif
#include <uk/arch/lcpu.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/ctors.h>
#include <uk/plat/memory.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/irq.h>
#include <uk/plat/time.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/argparse.h>
#if CONFIG_LIBUKBUS
#include <uk/bus.h>
#endif /* CONFIG_LIBUKBUS */

int main(int argc, char *argv[]) __weak;
#ifdef CONFIG_LIBLWIP
extern int liblwip_init(void);
#endif /* CONFIG_LIBLWIP */

static void main_thread_func(void *arg) __noreturn;

struct thread_main_arg {
	int argc;
	char **argv;
};

static void main_thread_func(void *arg)
{
	int i;
	int ret;
	struct thread_main_arg *tma = arg;

#ifdef CONFIG_LIBUKBUS
	uk_pr_info("Initialize bus handlers...\n");
	uk_bus_init_all(uk_alloc_get_default());
	uk_pr_info("Probe buses...\n");
	uk_bus_probe_all();
#endif /* CONFIG_LIBUKBUS */

#ifdef CONFIG_LIBLWIP
	/*
	 * TODO: This is an initial implementation where we call the
	 * initialization of lwip directly. We will remove this call
	 * as soon as we introduced a more generic scheme for
	 * (external) library initializations.
	 */
	liblwip_init();
#endif /* CONFIG_LIBLWIP */

#if CONFIG_LIBUKBOOT_BANNER
	printf("Welcome to  _ __             _____\n");
	printf(" __ _____  (_) /__ _______ _/ _/ /_\n");
	printf("/ // / _ \\/ /  '_// __/ _ `/ _/ __/\n");
	printf("\\_,_/_//_/_/_/\\_\\/_/  \\_,_/_/ \\__/\n");
	printf("%35s\n",
	       STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));
#endif

	uk_pr_info("Calling main(%d, [", tma->argc);
	for (i = 0; i < tma->argc; ++i) {
		uk_pr_info("'%s'", tma->argv[i]);
		if ((i + 1) < tma->argc)
			uk_pr_info(", ");
	}
	uk_pr_info("])\n");

	ret = main(tma->argc, tma->argv);
	uk_pr_info("main returned %d, halting system\n", ret);
	ret = (ret != 0) ? UKPLAT_CRASH : UKPLAT_HALT;
	ukplat_terminate(ret); /* does not return */
}

/* defined in <uk/plat.h> */
void ukplat_entry_argp(char *arg0, char *argb, __sz argb_len)
{
	char *argv[CONFIG_LIBUKBOOT_MAXNBARGS];
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
	int i;
	struct thread_main_arg tma;
#if CONFIG_LIBUKALLOC
	struct uk_alloc *a = NULL;
	int rc;
#endif
#if CONFIG_LIBUKALLOC && CONFIG_LIBUKALLOCBBUDDY && CONFIG_LIBUKBOOT_INITALLOC
	struct ukplat_memregion_desc md;
#endif
#if CONFIG_LIBUKSCHED
	struct uk_sched *s = NULL;
	struct uk_thread *main_thread = NULL;
#endif

	uk_pr_info("Pre-init table at %p - %p\n",
		   __preinit_array_start, &__preinit_array_end);
	ukplat_ctor_foreach(__preinit_array_start, __preinit_array_end, i) {
		if (__preinit_array_start[i]) {
			uk_pr_debug("Call pre-init constructor (entry %d (%p): %p())...\n",
				    i, &__preinit_array_start[i],
				    __preinit_array_start[i]);
			__preinit_array_start[i]();
		}
	}

	uk_pr_info("Constructor table at %p - %p\n",
		   __init_array_start, &__init_array_end);
	ukplat_ctor_foreach(__init_array_start, __init_array_end, i) {
		if (__init_array_start[i]) {
			uk_pr_debug("Call constructor (entry %d (%p): %p())...\n",
				    i, &__init_array_start[i],
				    __init_array_start[i]);
			__init_array_start[i]();
		}
	}

#if CONFIG_LIBUKALLOC && CONFIG_LIBUKALLOCBBUDDY && CONFIG_LIBUKBOOT_INITALLOC
	/* initialize memory allocator
	 * FIXME: ukallocbbuddy is hard-coded for now
	 */
	if (ukplat_memregion_count() > 0) {
		uk_pr_info("Initialize memory allocator...\n");
		for (i = 0; i < ukplat_memregion_count(); ++i) {
			/* Check if memory region is usable for allocators */
			if (ukplat_memregion_get(i, &md) < 0)
				continue;

			if ((md.flags & UKPLAT_MEMRF_ALLOCATABLE)
			    != UKPLAT_MEMRF_ALLOCATABLE) {
#if CONFIG_UKPLAT_MEMRNAME
				uk_pr_debug("Skip memory region %d: %p - %p (flags: 0x%02x, name: %s)\n",
					    i, md.base, (void *)((size_t)md.base
								 + md.len),
					    md.flags, md.name);
#else
				uk_pr_debug("Skip memory region %d: %p - %p (flags: 0x%02x)\n",
					    i, md.base, (void *)((size_t)md.base
								 + md.len),
					    md.flags);
#endif
				continue;
			}

#if CONFIG_UKPLAT_MEMRNAME
			uk_pr_debug("Try  memory region %d: %p - %p (flags: 0x%02x, name: %s)...\n",
				    i, md.base, (void *)((size_t)md.base
							 + md.len),
				    md.flags, md.name);
#else
			uk_pr_debug("Try  memory region %d: %p - %p (flags: 0x%02x)...\n",
				    i, md.base, (void *)((size_t)md.base
							 + md.len),
				    md.flags);
#endif
			/* try to use memory region to initialize allocator
			 * if it fails, we will try  again with the next region.
			 * As soon we have an allocator, we simply add every
			 * subsequent region to it
			 */
			if (unlikely(!a))
				a = uk_allocbbuddy_init(md.base, md.len);
			else
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
#endif

	tma.argc = argc;
	tma.argv = argv;

#if CONFIG_LIBUKSCHED
	main_thread = uk_thread_create("main", main_thread_func, &tma);
	if (unlikely(!main_thread))
		UK_CRASH("Could not create main thread\n");
	uk_sched_start(s);
#else
	/* Enable interrupts before starting the application */
	ukplat_lcpu_enable_irq();
	main_thread_func(&tma);
#endif
}

/* Internal main */
int main(int argc __unused, char *argv[] __unused)
{
	printf("weak main() called. Symbol was not replaced!\n");
	return -EINVAL;
}
