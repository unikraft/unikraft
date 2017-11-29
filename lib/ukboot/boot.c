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
#include <errno.h>

#if LIBUKALLOC && LIBUKALLOCBBUDDY && LIBUKBOOT_INITALLOC
#include <uk/allocbbuddy.h>
#endif
#if LIBUKSCHED && LIBUKSCHEDCOOP
#include <uk/schedcoop.h>
#endif
#include <uk/arch/lcpu.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/memory.h>
#include <uk/plat/time.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/argparse.h>

int main(int argc, char *argv[]) __weak;

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

	uk_printd(DLVL_INFO, "Calling main(%d, [", tma->argc);
	for (i = 0; i < tma->argc; ++i) {
		uk_printd(DLVL_INFO, "'%s'", tma->argv[i]);
		if ((i + 1) < tma->argc)
			uk_printd(DLVL_INFO, ", ");
	}
	uk_printd(DLVL_INFO, "])\n");

	/* call main */
	ret = main(tma->argc, tma->argv);
	uk_printd(DLVL_INFO, "main returned %d, halting system\n", ret);
	ret = (ret != 0) ? UKPLAT_CRASH : UKPLAT_HALT;
	ukplat_terminate(ret); /* does not return */
}

/* defined in <uk/plat.h> */
void ukplat_entry_argp(char *arg0, char *argb, __sz argb_len)
{
	char *argv[LIBUKBOOT_MAXNBARGS];
	int argc = 0;

	if (arg0) {
		argv[0] = arg0;
		argc += 1;
	}
	if (argb && argb_len) {
		argc += uk_argnparse(argb, argb_len, arg0 ? &argv[1] : &argv[0],
				     arg0 ? (LIBUKBOOT_MAXNBARGS - 1)
					  : LIBUKBOOT_MAXNBARGS);
	}
	ukplat_entry(argc, argv);
}

/* defined in <uk/plat.h> */
void ukplat_entry(int argc, char *argv[])
{
	int i;
	struct thread_main_arg tma;
#if LIBUKALLOC || LIBUKSCHED
	struct uk_alloc *a = NULL;
#endif
#if LIBUKALLOC && LIBUKALLOCBBUDDY && LIBUKBOOT_INITALLOC
	struct ukplat_memregion_desc md;
#endif
#if HAVE_SCHED
	struct uk_sched *s = NULL;
	struct uk_thread *main_thread = NULL;
#endif

#if LIBUKBOOT_BANNER
	uk_printk("Welcome to  _ __             _____\n");
	uk_printk(" __ _____  (_) /__ _______ _/ _/ /_\n");
	uk_printk("/ // / _ \\/ /  '_// __/ _ `/ _/ __/\n");
	uk_printk("\\_,_/_//_/_/_/\\_\\/_/  \\_,_/_/ \\__/\n");
	uk_printk("%35s\n",
		  STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));
#endif

	ukplat_time_init();

#if LIBUKALLOC && LIBUKALLOCBBUDDY && LIBUKBOOT_INITALLOC
	/* initialize memory allocator
	 * FIXME: ukallocbbuddy is hard-coded for now
	 */
	if (ukplat_memregion_count() > 0) {
		uk_printd(DLVL_INFO, "Initialize memory allocator...\n");
		for (i = 0; i < ukplat_memregion_count(); ++i) {
			/* Check if memory region is usable for allocators */
			if (ukplat_memregion_get(i, &md) < 0)
				continue;

			if ((md.flags & UKPLAT_MEMRF_ALLOCATABLE)
			    != UKPLAT_MEMRF_ALLOCATABLE) {
				uk_printd(DLVL_EXTRA, "Skip memory region %d: %p - %p (flags: 0x%02x)\n",
					  i, md.base, (void *)((size_t)md.base
							       + md.len),
					  md.flags);
				continue;
			}

			uk_printd(DLVL_EXTRA, "Try  memory region %d: %p - %p (flags: 0x%02x)...\n",
				  i, md.base, (void *)((size_t)md.base
						       + md.len),
				  md.flags);
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
		uk_printd(DLVL_WARN, "No suitable memory region for memory allocator. Continue without heap\n");
#endif

#if HAVE_SCHED
	/* Init scheduler. */
	s = uk_schedcoop_init(a);
	if (unlikely(!s))
		UK_CRASH("Could not initialize the scheduler.");
#endif

	tma.argc = argc;
	tma.argv = argv;

#if HAVE_SCHED
	main_thread = uk_thread_create("main", main_thread_func, &tma);
	if (unlikely(!main_thread))
		UK_CRASH("Could not create main thread.");
	uk_thread_start(main_thread);
	uk_sched_run(s);
#else
	main_thread_func(&tma);
#endif
}

/* Internal main */
int main(int argc __unused, char *argv[] __unused)
{
	uk_printkd(DLVL_ERR, "weak main() called. Symbol was not replaced!\n");
	return -EINVAL;
}
