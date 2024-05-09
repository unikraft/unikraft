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

#include "uk/plat/io.h"
#include <uk/config.h>

#ifdef CONFIG_LIBUKSEV
#include <uk/sev.h>
#include <uk/asm/sev-ghcb.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include <uk/boot.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#ifdef CONFIG_LIBUKVMEM
#include <uk/vmem.h>
#endif /* CONFIG_LIBUKVMEM */
#endif /* CONFIG_HAVE_PAGING */
/* FIXME: allocators are hard-coded for now */
#if CONFIG_LIBUKBOOT_INITBBUDDY
#include <uk/allocbbuddy.h>
#define uk_alloc_init uk_allocbbuddy_init
#elif CONFIG_LIBUKBOOT_INITREGION
#include <uk/allocregion.h>
#define uk_alloc_init uk_allocregion_init
#elif CONFIG_LIBUKBOOT_INITMIMALLOC
#include <uk/mimalloc.h>
#define uk_alloc_init uk_mimalloc_init
#elif CONFIG_LIBUKBOOT_INITTLSF
#include <uk/tlsf.h>
#define uk_alloc_init uk_tlsf_init
#elif CONFIG_LIBUKBOOT_INITTINYALLOC
#include <uk/tinyalloc.h>
#define uk_alloc_init uk_tinyalloc_init
#endif
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#endif /* CONFIG_LIBUKSCHED */
#if CONFIG_LIBUKBOOT_INITSCHEDCOOP
#include <uk/schedcoop.h>
#endif /* CONFIG_LIBUKBOOT_INITSCHEDCOOP */
#include <uk/arch/lcpu.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/memory.h>
#include <uk/plat/lcpu.h>
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
#include <uk/arch/paging.h>
#include <uk/arch/tls.h>
#include <uk/plat/tls.h>
#if CONFIG_LIBUKBOOT_MAINTHREAD
#include "shutdown_req.h"
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
#include <uk/errptr.h>
#include "banner.h"

#if CONFIG_LIBUKBOOT_NOSCHED
#include <uk/plat/common/lcpu.h>
#endif /* CONFIG_LIBUKBOOT_NOSCHED */

#if CONFIG_LIBUKINTCTLR
#include <uk/intctlr.h>
#endif /* CONFIG_LIBUKINTCTLR */

int main(int argc, char *argv[]) __weak;
static inline int do_main(int argc, char *argv[]);

#if CONFIG_LIBUKBOOT_MAINTHREAD
static __noreturn void main_thread(void *, void *);
static void main_thread_dtor(struct uk_thread *m);
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

#if defined(CONFIG_LIBUKBOOT_HEAP_BASE) && defined(CONFIG_LIBUKVMEM)
static struct uk_vas kernel_vas;
#endif /* CONFIG_LIBUKBOOT_HEAP_BASE && CONFIG_LIBUKVMEM */

static struct uk_alloc *heap_init()
{
	struct uk_alloc *a = NULL;
#ifdef CONFIG_LIBUKBOOT_HEAP_BASE
	struct uk_pagetable *pt = ukplat_pt_get_active();
	__sz free_pages, alloc_pages;
	__vaddr_t heap_base;
#ifdef CONFIG_LIBUKVMEM
	__vaddr_t vaddr;
#endif /* CONFIG_LIBUKVMEM */
	int rc;
	int pg_attr = PAGE_ATTR_PROT_RW;
#ifdef CONFIG_HAVE_MEM_ENCRYPT
	pg_attr |= PAGE_ATTR_ENCRYPT;
#endif
#else /* CONFIG_LIBUKBOOT_HEAP_BASE */
	struct ukplat_memregion_desc *md;
#endif /* !CONFIG_LIBUKBOOT_HEAP_BASE */

#ifdef CONFIG_LIBUKBOOT_HEAP_BASE
	UK_ASSERT(pt);

	/* Paging is enabled. The available physical memory has already been
	 * added to the frame allocator. No heap area has been mapped. We will
	 * do so at the configured heap base.
	 */
	heap_base = CONFIG_LIBUKBOOT_HEAP_BASE;

#ifdef CONFIG_LIBUKVMEM
#define HEAP_INITIAL_PAGES		16
#define HEAP_INITIAL_LEN		(HEAP_INITIAL_PAGES << PAGE_SHIFT)
	/* In addition to paging, we have virtual address space management. We
	 * will thus also represent the heap as a dedicated VMA to enable
	 * on-demand paging for the heap. However, we have a chicken-egg
	 * problem here. This is because we need an allocator for allocating
	 * VMA objects but to do so, we need to create the heap VMA first.
	 * Theoretically, we could use a simple region allocator to bootstrap.
	 * But then, we would have to cope with VMA objects from different
	 * allocators. So instead, we just initialize a small heap using fixed
	 * mappings and then create the VMA on top. Afterwards, we add the
	 * remainder of the VMA to the allocator.
	 */

	rc = ukplat_page_map(pt, heap_base, __PADDR_ANY,
			     HEAP_INITIAL_PAGES, pg_attr, 0);

	if (unlikely(rc))
		return NULL;

	a = uk_alloc_init((void *)heap_base, HEAP_INITIAL_PAGES << PAGE_SHIFT);
	if (unlikely(!a))
		return NULL;

	rc = uk_vas_init(&kernel_vas, pt, a);
	if (unlikely(rc))
		return NULL;

	rc = uk_vas_set_active(&kernel_vas);
	if (unlikely(rc))
		return NULL;

	free_pages  = pt->fa->free_memory >> PAGE_SHIFT;
	alloc_pages = free_pages - PT_PAGES(free_pages);

	vaddr = heap_base;
	rc = uk_vma_map_anon(&kernel_vas, &vaddr,
			     (alloc_pages + HEAP_INITIAL_PAGES) << PAGE_SHIFT,
			     pg_attr, UK_VMA_MAP_UNINITIALIZED,
			     "heap");
	if (unlikely(rc))
		return NULL;

	rc = uk_alloc_addmem(a, (void *)(heap_base + HEAP_INITIAL_LEN),
			     (alloc_pages - HEAP_INITIAL_PAGES) << PAGE_SHIFT);
	if (unlikely(rc))
		return NULL;
#else /* CONFIG_LIBUKVMEM */
	free_pages  = pt->fa->free_memory >> PAGE_SHIFT;
	alloc_pages = free_pages - PT_PAGES(free_pages);

	rc = ukplat_page_map(pt, heap_base, __PADDR_ANY, alloc_pages,
			     pg_attr, 0);
	if (unlikely(rc))
		return NULL;

	a = uk_alloc_init((void *)heap_base, alloc_pages << PAGE_SHIFT);
#endif /* !CONFIG_LIBUKVMEM */
#else /* CONFIG_LIBUKBOOT_HEAP_BASE */
	/* Paging is disabled so we still have the static boot page table set
	 * that maps (some of) the physical memory to virtual memory. The
	 * UKPLAT_MEMRT_FREE memory regions reflect this. Try to use these
	 * memory regions to initialize the allocator. If it fails, we will try
	 * again with the next region. As soon we have an allocator, we simply
	 * add every subsequent region to it.
	 */
	ukplat_memregion_foreach(&md, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT(md->vbase == md->pbase);
		UK_ASSERT(!(md->pbase & ~PAGE_MASK));
		UK_ASSERT(md->len);
		UK_ASSERT(!(md->len & ~PAGE_MASK));

		uk_pr_debug("Trying %p-%p 0x%02x %s\n",
			    (void *)md->vbase, (void *)(md->vbase + md->len),
			    md->flags,
#if CONFIG_UKPLAT_MEMRNAME
			    md->name
#else /* CONFIG_UKPLAT_MEMRNAME */
			    ""
#endif /* !CONFIG_UKPLAT_MEMRNAME */
			    );

		if (!a)
			a = uk_alloc_init((void *)md->vbase, md->len);
		else
			uk_alloc_addmem(a, (void *)md->vbase, md->len);
	}
#endif /* !CONFIG_LIBUKBOOT_HEAP_BASE */

	return a;
}

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

#if CONFIG_LIBPOSIX_ENVIRON
extern char **environ;
#endif /* CONFIG_LIBPOSIX_ENVIRON */

/* defined in <uk/plat.h> */
void ukplat_entry(int argc, char *argv[])
{
	struct uk_init_ctx ictx = { 0 };
	/* NOTE: Default target is crash for failed initialization (inittab) */
	struct uk_term_ctx tctx = { .target = UKPLAT_CRASH };
	int rc = 0;
#if CONFIG_LIBUKALLOC
	struct uk_alloc *a = NULL;
#endif
#if !CONFIG_LIBUKBOOT_NOALLOC
	void *tls = NULL;
#endif
#if CONFIG_LIBUKSCHED
	struct uk_sched *s = NULL;
#endif
#if CONFIG_LIBUKBOOT_MAINTHREAD
	struct uk_thread *m;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
	uk_ctor_func_t *ctorfn;
	struct uk_inittab_entry *init_entry;
	void *auxstack;

#if CONFIG_LIBUKBOOT_MAINTHREAD
	/* Initialize shutdown control structure */
	uk_boot_shutdown_ctl_init();
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

	uk_pr_info("Unikraft constructor table at %p - %p\n",
		   &uk_ctortab_start[0], &uk_ctortab_end);
	uk_ctortab_foreach(ctorfn, uk_ctortab_start, uk_ctortab_end) {
		UK_ASSERT(*ctorfn);
		uk_pr_debug("Call constructor: %p())...\n", *ctorfn);
		(*ctorfn)();
	}

#ifdef CONFIG_LIBUKLIBPARAM
	/*
	 * First, we scan if we can find the stop sequence in the kernel
	 * cmdline. If not, we assume that there are no uklibparam arguments in
	 * the command line.
	 * NOTE: argv[0] contains the kernel/program name that we need to
	 *       hide from the parser.
	 */
	rc = uk_libparam_parse(argc - 1, &argv[1], UK_LIBPARAM_F_SCAN);
	if (rc > 0 && rc < (argc - 1)) {
		/* In this case, we did successfully scan for uklibparam
		 * arguments and stop sequence is at rc < (argc - 1).
		 */
		/* Run a second pass for parsing */
		rc = uk_libparam_parse(argc - 1, &argv[1], UK_LIBPARAM_F_USAGE);
		if (rc < 0) /* go down on errors (including USAGE) */
			ukplat_halt();

		/* Drop uklibparam parameters from argv but keep argv[0].
		 * We are going to replace the stop sequence with argv[0].
		 */
		rc += 1; /* include argv[0]; we use rc as idx to stop seq */
		argc -= rc;
		argv[rc] = argv[0];
		argv = &argv[rc];
	}
#endif /* CONFIG_LIBUKLIBPARAM */

#if !CONFIG_LIBUKBOOT_NOALLOC
	uk_pr_info("Initialize memory allocator...\n");

	a = heap_init();
	if (unlikely(!a))
		UK_CRASH("Failed to initialize memory allocator\n");
	else {
		rc = ukplat_memallocator_set(a);
		if (unlikely(rc != 0))
			UK_CRASH("Could not set the platform memory allocator\n");
	}

	/* Allocate a TLS for this execution context */
	tls = uk_memalign(a,
			  ukarch_tls_area_align(),
			  ukarch_tls_area_size());
	if (!tls)
		UK_CRASH("Failed to allocate and initialize TLS\n");

	/* Copy from TLS master template */
	ukarch_tls_area_init(tls);
	/* Activate TLS */
	ukplat_tlsp_set(ukarch_tls_tlsp(tls));

	/* Allocate auxiliary stack for this execution context */
	auxstack = uk_memalign(uk_alloc_get_default(),
			       UKPLAT_AUXSP_ALIGN, UKPLAT_AUXSP_LEN);
	if (unlikely(!auxstack))
		UK_CRASH("Failed to allocate the auxiliary stack\n");
	/* Activate auxiliary stack */
	ukplat_lcpu_set_auxsp(ukarch_gen_sp(auxstack, UKPLAT_AUXSP_LEN));
#if CONFIG_LIBUKVMEM
	rc = uk_vma_advise(uk_vas_get_active(),
			   PAGE_ALIGN_DOWN((__vaddr_t)auxstack),
			   PAGE_ALIGN_UP((__vaddr_t)auxstack +
				  UKPLAT_AUXSP_LEN -
				  PAGE_ALIGN_DOWN((__vaddr_t)auxstack)),
			   UK_VMA_ADV_WILLNEED,
			   UK_VMA_FLAG_UNINITIALIZED);
	if (unlikely(rc))
		UK_CRASH("Could not setup physical memory\n");
#endif /* CONFIG_LIBUKVMEM */
#endif /* !CONFIG_LIBUKBOOT_NOALLOC */

#if CONFIG_LIBUKINTCTLR
	uk_pr_info("Initialize the IRQ subsystem...\n");
	rc = uk_intctlr_init(a);
	if (unlikely(rc))
		UK_CRASH("Could not initialize the IRQ subsystem\n");
#endif /* CONFIG_LIBUKINTCTLR */

	/* On most platforms the timer depend on an initialized IRQ subsystem */
	uk_pr_info("Initialize platform time...\n");
	ukplat_time_init();

#if !CONFIG_LIBUKBOOT_NOSCHED
	uk_pr_info("Initialize scheduling...\n");
#if CONFIG_LIBUKBOOT_INITSCHEDCOOP
	s = uk_schedcoop_create(a);
#endif
	if (unlikely(!s))
		UK_CRASH("Failed to initialize scheduling\n");
	uk_sched_start(s);
#endif /* !CONFIG_LIBUKBOOT_NOSCHED */

	ictx.cmdline.argc = argc;
	ictx.cmdline.argv = argv;

	/* Enable interrupts before starting the application */
	ukplat_lcpu_enable_irq();

	/**
	 * Run init table
	 */
	uk_pr_info("Init Table @ %p - %p\n",
		   &uk_inittab_start[0], &uk_inittab_end);
	uk_inittab_foreach(init_entry, uk_inittab_start, uk_inittab_end) {
		UK_ASSERT(init_entry);

		if (!init_entry->init)
			continue;

		uk_pr_debug("Call init function: %p(%p)...\n",
			    init_entry->init, &ictx);
		rc = (*init_entry->init)(&ictx);
		if (rc < 0) {
			uk_pr_err("Init function at %p returned error %d\n",
				  init_entry->init, rc);
			goto exit;
		}
	}

#ifdef CONFIG_LIBUKSP
	uk_stack_chk_guard_setup();
#endif

	print_banner(stdout);
	fflush(stdout);

#if !CONFIG_LIBUKBOOT_MAINTHREAD
	do_main(ictx.cmdline.argc, ictx.cmdline.argv);
	tctx.target = UKPLAT_HALT;

#else /* CONFIG_LIBUKBOOT_MAINTHREAD */
	m = uk_sched_thread_create_fn2(s, main_thread,
				       (void *)((long)argc), (void *)argv,
				       0x0 /* default stack size */,
				       0x0 /* default auxiliary stack size */,
				       false, false,
				       "main", NULL,
				       main_thread_dtor);
	if (unlikely(!m || PTRISERR(m))) {
		uk_pr_err("Failed to launch application's main()\n");
		goto exit;
	}

	/* Block execution of "init" until we receive the first request */
	tctx.target = uk_boot_shutdown_barrier();
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

exit:
	uk_pr_info("Halting system (%d)\n", tctx.target);

	/**
	 * Call termination functions from init table in reverse order
	 */
	/* NOTE: The init loop left `init_entry` at the position that is one
	 *       step further after the last successfully initialized  entry
	 */
	init_entry--;
	uk_inittab_foreach_reverse2(init_entry, uk_inittab_start) {
		UK_ASSERT(init_entry);

		if (!init_entry->term)
			continue;

		uk_pr_debug("Call term function: %p(%p)...\n",
			    init_entry->term, &tctx);
		(*init_entry->term)(&tctx);
	}

	ukplat_terminate(tctx.target); /* does not return */
}

static inline int do_main(int argc, char *argv[])
{
	uk_ctor_func_t *ctorfn;
#if CONFIG_LIBPOSIX_ENVIRON
	char **envp;
#endif /* CONFIG_LIBPOSIX_ENVIRON */
	int ret;

	/*
	 * Application
	 *
	 * We are calling the application constructors right before calling
	 * the application's main(). All of our Unikraft systems, VFS,
	 * networking stack are initialized at this point. This way we closely
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

		uk_pr_debug("Call constructor: %p(%d, %p)...\n", *ctorfn,
			    argc, argv);
		(*ctorfn)(argc, argv);
	}

#if CONFIG_LIBUKDEBUG_PRINTK_INFO
#if CONFIG_LIBPOSIX_ENVIRON
	envp = environ;
	if (envp) {
		uk_pr_info("Environment variables:\n");
		while (*envp) {
			uk_pr_info("\t%s\n", *envp);
			envp++;
		}
	}
#endif /* CONFIG_LIBPOSIX_ENVIRON */

	uk_pr_info("Calling main(%d, [", argc);
	for (int i = 0; i < argc; ++i) {
		uk_pr_info("'%s'", argv[i]);
		if ((i + 1) < argc)
			uk_pr_info(", ");
	}
	uk_pr_info("])\n");
#endif /* CONFIG_LIBUKDEBUG_PRINTK_INFO */

	ret = main(argc, argv);
	uk_pr_info("main returned %d\n", ret);
	return ret;
}

#if CONFIG_LIBUKBOOT_MAINTHREAD
static __noreturn void main_thread(void *a_argc, void *a_argv)
{
	int argc = (int)((__uptr)a_argc);
	char **argv = (char **)a_argv;

	do_main(argc, argv);
#if !CONFIG_LIBUKBOOT_MAINTHREAD_NOHALT
	/* NOTE: The scheduler's garbage collector would also initiate a
	 *       shutdown request via `main_thread_dtor()`.
	 *       But because of an unknown delay before the GC is called
	 *       we already request a shutdown here in order to go down
	 *       as earlier as possible.
	 */
	uk_boot_shutdown_req(UKPLAT_HALT);
#endif /* !LIBUKBOOT_MAINTHREAD_NOHALT */
	/* Terminate "main" thread */
	uk_thread_exit();
}

static void main_thread_dtor(struct uk_thread *m __unused)
{
#if !CONFIG_LIBUKBOOT_MAINTHREAD_NOHALT
	/* NOTE: Here, we request a shutdown as soon as "main" exits.
	 *       It covers also the cases where the "main" thread just exits
	 *       without returning to the caller of `main()` (for example
	 *       via `uk_thread_exit()`).
	 */
	uk_boot_shutdown_req(UKPLAT_HALT);
#endif /* !LIBUKBOOT_MAINTHREAD_NOHALT */
}
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
