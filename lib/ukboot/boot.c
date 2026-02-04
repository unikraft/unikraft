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
#include <uk/plat/config.h>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include <uk/boot.h>
#ifdef CONFIG_LIBUKPAGING
#include <uk/falloc.h>
#include <uk/paging.h>
#ifdef CONFIG_LIBUKVMEM
#include <uk/vmem.h>
#endif /* CONFIG_LIBUKVMEM */
#endif /* CONFIG_LIBUKPAGING */
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
#if CONFIG_LIBUKBOOT_ALLOCSTACK
#include <uk/allocstack.h>
#if CONFIG_LIBUKBOOT_ALLOCSTACK_PREMAP_ORDER
#define ALLOCSTACK_INITIAL_SIZE				\
	(UK_PAGING_PAGE_SIZE * (1 << CONFIG_LIBUKBOOT_ALLOCSTACK_PREMAP_ORDER))
#endif /* CONFIG_LIBUKBOOT_ALLOCSTACK_PREMAP_ORDER */
#endif /* CONFIG_LIBUKBOOT_ALLOCSTACK */
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#endif /* CONFIG_LIBUKSCHED */
#if CONFIG_LIBUKBOOT_INITSCHEDCOOP
#include <uk/schedcoop.h>
#endif /* CONFIG_LIBUKBOOT_INITSCHEDCOOP */
#include <uk/lcpu.h>
#include <uk/pm.h>
#include <uk/plat/memory.h>
#include <uk/plat/time.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/init.h>
#ifdef CONFIG_LIBUKSP
#include <uk/sp.h>
#endif
#include <uk/arch/tls.h>
#if CONFIG_LIBUKBOOT_MAINTHREAD
#include "shutdown_req.h"
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
#include <uk/errptr.h>
#include "banner.h"

#if CONFIG_LIBUKINTCTLR
#include <uk/intctlr.h>
#endif /* CONFIG_LIBUKINTCTLR */

#include "init.h"

extern char **boot_argv;
extern int boot_argc;

int main(int argc, char *argv[]) __weak;

#if CONFIG_LIBUKBOOT_MAINTHREAD
static __noreturn void main_thread(void *, void *);
static void main_thread_dtor(struct uk_thread *m);
struct uk_semaphore main_sema;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

#if defined(CONFIG_LIBUKBOOT_HEAP_BASE) && defined(CONFIG_LIBUKVMEM)
static struct uk_vas kernel_vas;
#endif /* CONFIG_LIBUKBOOT_HEAP_BASE && CONFIG_LIBUKVMEM */

static struct uk_alloc *heap_init()
{
	struct uk_alloc *a = NULL;
#ifdef CONFIG_LIBUKBOOT_HEAP_BASE
	struct uk_pagetable *pt = uk_paging_pt_get_active();
	__sz free_pages, alloc_pages;
	__vaddr_t heap_base;
#ifdef CONFIG_LIBUKVMEM
	__vaddr_t vaddr;
#endif /* CONFIG_LIBUKVMEM */
	int rc;
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
#define HEAP_INITIAL_PAGES	16
#define HEAP_INITIAL_LEN	(HEAP_INITIAL_PAGES << UK_PAGING_PAGE_SHIFT)
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
	rc = uk_paging_page_map(pt, heap_base, UK_PAGING_PADDR_ANY,
				HEAP_INITIAL_PAGES, UK_PAGING_PAGE_ATTR_PROT_RW,
				0);
	if (unlikely(rc))
		return NULL;

	a = uk_alloc_init((void *)heap_base,
			  HEAP_INITIAL_PAGES << UK_PAGING_PAGE_SHIFT);
	if (unlikely(!a))
		return NULL;

	rc = uk_vas_init(&kernel_vas, pt, a);
	if (unlikely(rc))
		return NULL;

	rc = uk_vas_set_active(&kernel_vas);
	if (unlikely(rc))
		return NULL;

	free_pages  = pt->fa->free_memory >> UK_PAGING_PAGE_SHIFT;
	alloc_pages = free_pages - UK_PAGING_PT_PAGES(free_pages);

	vaddr = heap_base;
	rc = uk_vma_map_anon(&kernel_vas, &vaddr,
			     (alloc_pages + HEAP_INITIAL_PAGES) << UK_PAGING_PAGE_SHIFT,
			     UK_PAGING_PAGE_ATTR_PROT_RW,
			     UK_VMA_MAP_UNINITIALIZED,
			     "heap");
	if (unlikely(rc))
		return NULL;

	rc = uk_alloc_addmem(a, (void *)(heap_base + HEAP_INITIAL_LEN),
			     (alloc_pages - HEAP_INITIAL_PAGES) << UK_PAGING_PAGE_SHIFT);
	if (unlikely(rc))
		return NULL;
#else /* CONFIG_LIBUKVMEM */
	free_pages  = pt->fa->free_memory >> UK_PAGING_PAGE_SHIFT;
	alloc_pages = free_pages - UK_PAGING_PT_PAGES(free_pages);

	rc = uk_paging_page_map(pt, heap_base, UK_PAGING_PADDR_ANY,
				alloc_pages, UK_PAGING_PAGE_ATTR_PROT_RW, 0);
	if (unlikely(rc))
		return NULL;

	a = uk_alloc_init((void *)heap_base,
			  alloc_pages << UK_PAGING_PAGE_SHIFT);
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
		UK_ASSERT_VALID_FREE_MRD(md);

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

#if CONFIG_LIBPOSIX_ENVIRON
extern char **environ;
#endif /* CONFIG_LIBPOSIX_ENVIRON */

void uk_boot_entry(void)
{
	struct uk_init_ctx ictx = { 0 };
	struct uk_term_ctx tctx = {
		.exit_code = 0,
		.target = UK_PM_SHUTDOWN_OP_SYSCRASH,
	};
	int rc = 0;
#if CONFIG_LIBUKALLOC
	struct uk_alloc *a = NULL, *sa = NULL, *auxsa = NULL;
#endif
#if CONFIG_LIBUKBOOT_INITALLOC
	struct ukarch_auxspcb *auxspcb;
	__uptr auxsp, uktlsp;
	void *tls = NULL;
	void *auxstack;
#endif
#if CONFIG_LIBUKSCHED
	struct uk_sched *s = NULL;
#endif
#if CONFIG_LIBUKBOOT_MAINTHREAD
	struct uk_thread *m;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
	uk_ctor_func_t *ctorfn;
	struct uk_inittab_entry *init_entry;

	/* Make sure uk_early_init has been called to process
	 * the cmdline. We should expect at minimum one arg,
	 * CONFIG_UK_NAME.
	 */
	UK_ASSERT(boot_argc);

#if CONFIG_LIBUKBOOT_MAINTHREAD
	/* Initialize main thread semaphore */
	uk_semaphore_init(&main_sema, 0);
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

#if CONFIG_LIBUKBOOT_INITALLOC
	uk_pr_info("Initialize memory allocator...\n");

	a = heap_init();
	if (unlikely(!a))
		UK_CRASH("Failed to initialize memory allocator\n");
	else {
		rc = ukplat_memallocator_set(a);
		if (unlikely(rc != 0))
			UK_CRASH("Could not set the platform memory allocator\n");
	}

	sa = uk_allocstack_init(a
#if CONFIG_LIBUKVMEM
				, &kernel_vas, ALLOCSTACK_INITIAL_SIZE
#endif /* CONFIG_LIBUKVMEM */
			       );
	if (unlikely(!sa))
		UK_CRASH("Could not initialize stack allocator\n");

	auxsa = uk_allocstack_init(a
#if CONFIG_LIBUKVMEM
				    /* The auxiliary stack must be always
				     * fully mapped.
				     */
				    , &kernel_vas, AUXSTACK_SIZE
#endif /* CONFIG_LIBUKVMEM */
				   );
	if (unlikely(!auxsa))
		UK_CRASH("Could not initialize auxiliary stack allocator\n");

	/* Allocate a TLS for this execution context */
	tls = uk_memalign(a,
			  ukarch_tls_area_align(),
			  ukarch_tls_area_size());
	if (!tls)
		UK_CRASH("Failed to allocate and initialize TLS\n");

	/* Copy from TLS master template */
	ukarch_tls_area_init(tls);
	/* Activate TLS */
	uktlsp = ukarch_tls_tlsp(tls);
	uk_lcpu_tlsp_set(uktlsp);

	/* Allocate auxiliary stack for this execution context */
	auxstack = uk_memalign(auxsa,
			       UKARCH_AUXSP_ALIGN, AUXSTACK_SIZE);
	if (unlikely(!auxstack))
		UK_CRASH("Failed to allocate the auxiliary stack\n");

	/* Activate auxiliary stack */
	auxsp = ukarch_gen_sp(auxstack, AUXSTACK_SIZE);
	ukarch_auxsp_init(auxsp);
	auxspcb = ukarch_auxsp_get_cb(auxsp);
	UK_ASSERT(auxspcb);
	ukarch_auxspcb_set_uktlsp(auxspcb, uktlsp);
	uk_lcpu_set_auxsp(auxsp);
#endif /* CONFIG_LIBUKBOOT_INITALLOC */

#if CONFIG_LIBUKINTCTLR
	uk_pr_info("Initialize the IRQ subsystem...\n");
	rc = uk_intctlr_init(a);
	if (unlikely(rc))
		UK_CRASH("Could not initialize the IRQ subsystem\n");
#endif /* CONFIG_LIBUKINTCTLR */

	/* On most platforms the timer depend on an initialized IRQ subsystem */
	uk_pr_info("Initialize platform time...\n");
	ukplat_time_init();

#if CONFIG_LIBUKBOOT_INITSCHED
	uk_pr_info("Initialize scheduling...\n");
#if CONFIG_LIBUKBOOT_INITSCHEDCOOP
	s = uk_schedcoop_create(a, sa, auxsa, a);
#endif
	if (unlikely(!s))
		UK_CRASH("Failed to initialize scheduling\n");
	uk_sched_start(s);
#endif /* CONFIG_LIBUKBOOT_INITSCHED */

	ictx.cmdline.argc = boot_argc;
	ictx.cmdline.argv = boot_argv;

#if CONFIG_LIBUKBOOT_MAINTHREAD
	/* Start main thread (will block on semaphore) */
	m = uk_sched_thread_create_fn2(s, main_thread,
				       &ictx, &tctx,
				       0x0 /* default stack size */,
				       0x0 /* default auxiliary stack size */,
				       false, false,
				       "main", NULL,
				       main_thread_dtor);
	if (unlikely(!m || PTRISERR(m)))
		UK_CRASH("Failed to launch application's main()\n");

	ictx.tmain = m;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

	/* Enable interrupts before starting the application */
	uk_lcpu_enable_irq();

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
	tctx.exit_code = do_main(ictx.cmdline.argc, ictx.cmdline.argv);
	tctx.target = UK_PM_SHUTDOWN_OP_SYSHALT;
#else /* CONFIG_LIBUKBOOT_MAINTHREAD */
	/* Unblock main thread (will execute main()) */
	uk_semaphore_up(&main_sema);

	/* Block execution of "init" until we receive the first request.
	 * The exit code will be set by the main thread.
	 */
	tctx.target = uk_boot_shutdown_barrier();
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

exit:
	uk_pr_info("Shutting down system (%d)\n", tctx.target);

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

	uk_pr_debug("Unikraft terminates with exit status %d (target: %d)\n",
		    tctx.exit_code, tctx.target);
	uk_pm_shutdown(tctx.target); /* does not return */
}

int do_main(int argc, char *argv[])
{
	char **envp __maybe_unused;
	uk_ctor_func_t *ctorfn;
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
/* Pass ictx so that inittab handlers can update args */
static __noreturn void main_thread(void *a0, void *a1)
{
	struct uk_init_ctx *ictx = (struct uk_init_ctx *)a0;
	struct uk_term_ctx *tctx = (struct uk_term_ctx *)a1;

	UK_ASSERT(ictx);
	UK_ASSERT(tctx);

	/* block until we are allowed to execute main() */
	uk_semaphore_down(&main_sema);
#if CONFIG_LIBUKBOOT_INIT
	tctx->exit_code = do_init(ictx->cmdline.argc, ictx->cmdline.argv);
#else /* !CONFIG_LIBUKBOOT_INIT */
	tctx->exit_code = do_main(ictx->cmdline.argc, ictx->cmdline.argv);
#endif /* !CONFIG_LIBUKBOOT_INIT */

#if !CONFIG_LIBUKBOOT_MAINTHREAD_NOHALT
	/* NOTE: The scheduler's garbage collector would also initiate a
	 *       shutdown request via `main_thread_dtor()`.
	 *       But because of an unknown delay before the GC is called
	 *       we already request a shutdown here in order to go down
	 *       as earlier as possible.
	 */
	uk_boot_shutdown_req(UK_PM_SHUTDOWN_OP_SYSHALT);
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
	uk_boot_shutdown_req(UK_PM_SHUTDOWN_OP_SYSHALT);
#endif /* !LIBUKBOOT_MAINTHREAD_NOHALT */
}
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
