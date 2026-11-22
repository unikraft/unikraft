/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/arch/util.h>
#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/plat/spinlock.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>
#include <uk/vmem.h>
#include <uk/vmem/vma_types.h>

#define COW_TEST_FRAMES 128
#define COW_TEST_PAGE_FLAGS UK_PAGING_PAGE_FLAG_SIZE(UK_PAGING_PAGE_LEVEL)
#define COW_TEST_TIMEOUT (5 * UKARCH_NSEC_PER_SEC)

struct cow_test_frame {
	__paddr_t paddr;
	unsigned long pages;
	bool live;
};

struct cow_test_frames {
	struct uk_falloc fa;
	struct uk_falloc *real;
	struct __spinlock lock;
	struct cow_test_frame records[COW_TEST_FRAMES];
	unsigned long allocated;
	unsigned long freed;
	unsigned long live;
	unsigned int attempts;
	unsigned int errors;
	bool fail;
};

struct cow_test_metadata {
	struct uk_alloc *real;
	unsigned long allocated;
	unsigned long freed;
	unsigned int remaining;
	bool fail;
	bool poison;
	unsigned long poisoned;
};

struct cow_test_family {
	struct cow_test_frames frames;
	struct uk_alloc *resident;
	unsigned long metadata_allocated;
	unsigned long metadata_freed;
	unsigned int released;
	unsigned int cleanup_errors;
};

struct cow_test_space {
	struct uk_vas vas;
	struct uk_pagetable pt;
	struct cow_test_family *family;
};

static int cow_test_falloc(struct uk_falloc *fa, __paddr_t *paddr,
			   unsigned long pages, unsigned long flags)
{
	struct cow_test_frames *p;
	unsigned long irqf;
	unsigned int i;
	int rc;

	p = __containerof(fa, struct cow_test_frames, fa);
	ukplat_spin_lock_irqsave(&p->lock, irqf);
	p->attempts++;
	if (p->fail) {
		*paddr = UK_PAGING_PADDR_INV;
		rc = -ENOMEM;
		goto out;
	}
	for (i = 0; i < COW_TEST_FRAMES; i++)
		if (!p->records[i].live)
			break;
	if (i == COW_TEST_FRAMES) {
		*paddr = UK_PAGING_PADDR_INV;
		rc = -ENOMEM;
		goto out;
	}
	rc = p->real->falloc(p->real, paddr, pages, flags);
	if (!rc) {
		p->records[i].paddr = *paddr;
		p->records[i].pages = pages;
		p->records[i].live = true;
		p->allocated += pages;
		p->live += pages;
	}
out:
	ukplat_spin_unlock_irqrestore(&p->lock, irqf);
	return rc;
}

static int cow_test_ffree(struct uk_falloc *fa, __paddr_t paddr,
			  unsigned long pages)
{
	struct cow_test_frames *p;
	unsigned long irqf;
	unsigned int i;
	int rc;

	p = __containerof(fa, struct cow_test_frames, fa);
	ukplat_spin_lock_irqsave(&p->lock, irqf);
	rc = p->real->ffree(p->real, paddr, pages);
	if (rc) {
		p->errors++;
		goto out;
	}
	/* Initial kernel-branch page tables predate installation of this
	 * wrapper. Track subsequent allocations, including application PTs.
	 */
	for (i = 0; i < COW_TEST_FRAMES; i++) {
		if (p->records[i].live && p->records[i].paddr == paddr &&
		    p->records[i].pages == pages) {
			p->records[i].live = false;
			p->freed += pages;
			p->live -= pages;
			break;
		}
	}
out:
	ukplat_spin_unlock_irqrestore(&p->lock, irqf);
	return rc;
}

static void *cow_test_palloc(struct uk_alloc *a, unsigned long pages)
{
	struct cow_test_metadata *p = (void *)a->priv;
	void *ptr;

	if (p->fail) {
		if (!p->remaining)
			return NULL;
		p->remaining--;
	}
	ptr = uk_palloc(p->real, pages);
	if (ptr)
		__atomic_add_fetch(&p->allocated, pages, __ATOMIC_RELAXED);
	return ptr;
}

static void cow_test_pfree(struct uk_alloc *a, void *ptr, unsigned long pages)
{
	struct cow_test_metadata *p = (void *)a->priv;

	if (p->poison) {
		memset_isr(ptr, 0, pages * UK_PAGING_PAGE_SIZE);
		p->poisoned += pages;
	}
	uk_pfree(p->real, ptr, pages);
	__atomic_add_fetch(&p->freed, pages, __ATOMIC_RELAXED);
}

static int cow_test_family_init(struct cow_test_family *family)
{
	struct uk_alloc *real = uk_vas_get_kernel()->a;
	struct cow_test_metadata *p;
	struct uk_alloc *a;

	memset_isr(family, 0, sizeof(*family));
	a = uk_malloc(real, sizeof(*a) + sizeof(*p));
	if (!a)
		return -ENOMEM;
	memset_isr(a, 0, sizeof(*a) + sizeof(*p));
	p = (void *)a->priv;
	p->real = real;
	a->malloc = uk_malloc_ifpages;
	a->free = uk_free_ifpages;
	a->palloc = cow_test_palloc;
	a->pfree = cow_test_pfree;
	family->resident = a;
	family->frames.real = uk_vas_get_kernel()->pt->fa;
	family->frames.fa.falloc = cow_test_falloc;
	family->frames.fa.ffree = cow_test_ffree;
	ukarch_spin_init(&family->frames.lock);
	return 0;
}

static void cow_test_family_close(struct cow_test_family *family)
{
	struct cow_test_metadata *p = (void *)family->resident->priv;

	family->metadata_allocated = p->allocated;
	family->metadata_freed = p->freed;
	uk_free(p->real, family->resident);
	family->resident = NULL;
}

static void cow_test_space_release(struct uk_vas *vas)
{
	struct cow_test_space *s;
	struct cow_test_family *family;
	int rc;

	s = __containerof(vas, struct cow_test_space, vas);
	family = s->family;
	uk_vas_destroy(vas);
	/* VMAs released their frames; remaining leaves are kernel aliases. */
	rc = uk_paging_pt_free(&s->pt, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	if (rc)
		family->cleanup_errors++;
	family->released++;
	uk_free(uk_vas_get_kernel()->a, s);
}

static struct cow_test_space *cow_test_space_new(struct cow_test_family *family)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct cow_test_space *s = uk_malloc(kernel->a, sizeof(*s));
	int rc;

	if (!s)
		return NULL;
	memset_isr(s, 0, sizeof(*s));
	s->family = family;
	rc = uk_paging_pt_clone_shared(&s->pt, kernel->pt, 0, span);
	if (rc)
		goto fail;
	s->pt.fa = &family->frames.fa;
	rc = uk_vas_init_owned(&s->vas, &s->pt, family->resident,
			       cow_test_space_release);
	if (rc) {
		uk_paging_pt_free(&s->pt, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
		goto fail;
	}
	s->vas.vma_base = span;
	return s;
fail:
	uk_free(kernel->a, s);
	return NULL;
}

static void cow_test_space_put(struct cow_test_space **s)
{
	if (*s) {
		uk_vas_put(&(*s)->vas);
		*s = NULL;
	}
}

static int cow_test_mapping(struct cow_test_space *s, __vaddr_t vaddr,
			    __paddr_t *paddr, unsigned long *attr)
{
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	__pte_t pte;
	int rc;

	rc = uk_paging_pt_walk(&s->pt, vaddr, &level, NULL, &pte);
	if (rc)
		return rc;
	if (!UK_PAGING_PT_Lx_PTE_PRESENT(pte, level))
		return -ENOENT;
	if (level != UK_PAGING_PAGE_LEVEL)
		return -E2BIG;
	if (paddr)
		*paddr = UK_PAGING_PT_Lx_PTE_PADDR(pte, level);
	if (attr)
		*attr = uk_paging_attr_from_pte(pte, level);
	return 0;
}

static int cow_test_read(struct cow_test_space *s, __vaddr_t vaddr,
			 __u32 *value)
{
	__paddr_t paddr;
	__vaddr_t mapped;
	int rc;

	rc = uk_vas_fault(&s->vas, vaddr, UK_VMA_FAULT_READ);
	if (rc)
		return rc;
	rc = cow_test_mapping(s, vaddr, &paddr, NULL);
	if (rc)
		return rc;
	mapped = uk_paging_page_kmap(&s->pt, paddr, 1, 0);
	if (mapped == UK_PAGING_VADDR_INV)
		return -ENOMEM;
	*value = *(__u32 *)(mapped + vaddr % UK_PAGING_PAGE_SIZE);
	uk_paging_page_kunmap(&s->pt, mapped, 1, 0);
	return 0;
}

static unsigned long cow_test_kernel_alias_frames(void)
{
	struct uk_pagetable *pt = uk_vas_get_kernel()->pt;
	unsigned long frames = 0;
	unsigned int level;

	/* Count mapped base-frame equivalents, including the direct map. */
	for (level = 0; level < UK_PAL_PT_LEVELS; level++)
		frames += pt->nr_lx_pages[level] *
			(UK_PAGING_PAGE_Lx_SIZE(level) / UK_PAGING_PAGE_SIZE);
	return frames;
}

static int cow_test_clone_cycle(struct cow_test_family *family,
				unsigned int cycle)
{
	struct cow_test_space *parent = NULL, *child = NULL;
	__vaddr_t address = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	__u32 value = 0;
	int rc = -ENOMEM;

	parent = cow_test_space_new(family);
	child = cow_test_space_new(family);
	if (!parent || !child)
		goto out;
	rc = uk_vas_enable_cow(&parent->vas, family->resident);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(&parent->vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	if (rc)
		goto out;
	rc = uk_vas_store_u32(&parent->vas, address, cycle);
	if (rc)
		goto out;
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	if (rc)
		goto out;
	rc = uk_vas_store_u32(&child->vas, address, cycle + 64);
	if (rc)
		goto out;
	rc = cow_test_read(parent, address, &value);
	if (!rc && value != cycle)
		rc = -EIO;
	if (rc)
		goto out;
	cow_test_space_put(&parent);
	rc = cow_test_read(child, address, &value);
	if (!rc && value != cycle + 64)
		rc = -EIO;
out:
	cow_test_space_put(&parent);
	cow_test_space_put(&child);
	return rc;
}

UK_TESTCASE(cow_vmas, bounded_repeated_clone_resource_balance)
{
	struct cow_test_family family;
	struct uk_falloc *fa = uk_vas_get_kernel()->pt->fa;
	unsigned long first_free = 0, first_aliases = 0, first_allocated = 0;
	unsigned long free_bytes, aliases, allocated;
	unsigned int cycle;
	int rc;

	for (cycle = 1; cycle <= 64; cycle++) {
		rc = cow_test_family_init(&family);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			return;
		rc = cow_test_clone_cycle(&family, cycle);
		cow_test_family_close(&family);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(family.released, 2);
		UK_TEST_EXPECT_ZERO(family.cleanup_errors);
		UK_TEST_EXPECT_ZERO(family.frames.errors);
		UK_TEST_EXPECT_ZERO(family.frames.live);
		UK_TEST_EXPECT_SNUM_EQ(family.frames.allocated,
				       family.frames.freed);
		UK_TEST_EXPECT_SNUM_EQ(family.metadata_allocated,
				       family.metadata_freed);
		free_bytes = fa->free_memory;
		aliases = cow_test_kernel_alias_frames();
		allocated = (fa->total_memory - free_bytes) /
			UK_PAGING_PAGE_SIZE;
		if (cycle == 1) {
			first_free = free_bytes;
			first_aliases = aliases;
			first_allocated = allocated;
		}
		UK_TEST_EXPECT_SNUM_EQ(free_bytes, first_free);
		UK_TEST_EXPECT_SNUM_EQ(aliases, first_aliases);
		UK_TEST_EXPECT_SNUM_EQ(allocated, first_allocated);
		uk_pr_info("COW_VMA_CYCLE cycle=%u free_bytes=%lu alias_frames=%lu kernel_allocated_frames=%lu app_live=%lu metadata_live=%lu\n",
			   cycle,
			   free_bytes, aliases, allocated, family.frames.live,
			   family.metadata_allocated - family.metadata_freed);
		if (rc)
			return;
	}
}

struct cow_test_remote_group {
	bool go;
	bool cancel;
};

struct cow_test_remote {
	struct cow_test_remote_group *group;
	struct uk_semaphore reaped;
	struct uk_vas *vas;
	__vaddr_t address;
	__u32 before;
	__u32 after;
	__u32 observed;
	unsigned int cpu;
	bool active;
	bool ready;
	bool completed;
};

static void cow_test_remote_worker(void *arg) __noreturn;

static void cow_test_remote_worker(void *arg)
{
	struct cow_test_remote *p = arg;

	p->active = uk_vas_get_active() == p->vas &&
		uk_paging_pt_get_active() == p->vas->pt &&
		uk_sched_current()->lcpu_idx == p->cpu &&
		!uk_lcpu_irqs_disabled();
	/* Populate a writable remote TLB entry before the parent is cloned. */
	if (p->active)
		__atomic_store_n((__u32 *)p->address, p->before,
				 __ATOMIC_RELAXED);
	__atomic_store_n(&p->ready, true, __ATOMIC_RELEASE);
	while (!__atomic_load_n(&p->group->go, __ATOMIC_ACQUIRE))
		uk_arch_spinwait();
	if (!p->group->cancel && p->active) {
		__atomic_store_n((__u32 *)p->address, p->after,
				 __ATOMIC_RELAXED);
		p->observed =
			__atomic_load_n((__u32 *)p->address, __ATOMIC_RELAXED);
		p->completed = true;
	}
	uk_thread_exit();
}

static void cow_test_remote_reaped(struct uk_thread *thread)
{
	struct cow_test_remote *p = thread->priv;

	uk_semaphore_up(&p->reaped);
}

static int cow_test_remote_start(struct uk_sched *sched,
				 struct cow_test_remote *p)
{
	struct uk_thread *thread;
	int rc;

	uk_semaphore_init(&p->reaped, 0);
	thread = uk_thread_create_fn1(sched->a, cow_test_remote_worker, p,
				      sched->a_stack, 0, sched->a_auxstack, 0,
				      sched->a_uktls, false, "cow-remote", p,
				      cow_test_remote_reaped);
	if (!thread)
		return -ENOMEM;
	rc = uk_thread_set_vas(thread, p->vas);
	if (!rc)
		rc = uk_sched_thread_add(sched, thread);
	if (rc)
		uk_thread_release(thread);
	return rc;
}

static int cow_test_remote_ready(struct cow_test_remote *p)
{
	__nsec end = ukplat_monotonic_clock() + COW_TEST_TIMEOUT;

	while (!__atomic_load_n(&p->ready, __ATOMIC_ACQUIRE)) {
		if (ukplat_monotonic_clock() >= end)
			return -ETIMEDOUT;
		uk_arch_spinwait();
	}
	return 0;
}

static void cow_test_remote_join(struct cow_test_remote *p,
				 unsigned int started, unsigned int *reaped,
				 struct uk_vas *vas)
{
	__nsec end;
	__nsec elapsed;

	for (; *reaped < started; (*reaped)++) {
		elapsed = uk_semaphore_down_to(&p[*reaped].reaped,
					       COW_TEST_TIMEOUT);
		if (elapsed == __NSEC_MAX)
			UK_CRASH("COW remote writer was not reaped\n");
	}
	/* The destructor precedes freeing thread storage and its VAS pin. */
	end = ukplat_monotonic_clock() + COW_TEST_TIMEOUT;
	while (uk_refcount_read(&vas->refs) != 1) {
		if (ukplat_monotonic_clock() >= end)
			UK_CRASH("COW remote writer retained its VAS\n");
		uk_arch_spinwait();
	}
}

UK_TESTCASE(cow_vmas, cached_remote_writes_after_clone)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_sched *local = uk_sched_current();
	unsigned int cpus = uk_sched_started_count();
	unsigned int words = MAX(cpus - 1, 1U);
	struct cow_test_remote_group group = { .cancel = true };
	struct cow_test_remote *probes = NULL;
	struct cow_test_family family;
	struct cow_test_space *parent = NULL, *child = NULL;
	__vaddr_t address = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	__paddr_t original = 0, after = 0;
	unsigned long attr = 0, allocated = 0, copies = 0;
	unsigned int started = 0, reaped = 0, i;
	__u32 value = 0;
	bool completed = false;
	int rc;

	UK_TEST_EXPECT(words <= UK_PAGING_PAGE_SIZE / sizeof(value));
	if (words > UK_PAGING_PAGE_SIZE / sizeof(value))
		return;
	rc = cow_test_family_init(&family);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	probes = uk_malloc(kernel->a, words * sizeof(*probes));
	if (probes)
		memset_isr(probes, 0, words * sizeof(*probes));
	parent = cow_test_space_new(&family);
	child = cow_test_space_new(&family);
	UK_TEST_EXPECT_NOT_NULL(probes);
	UK_TEST_EXPECT_NOT_NULL(parent);
	UK_TEST_EXPECT_NOT_NULL(child);
	if (!probes || !parent || !child)
		goto out;
	rc = uk_vas_enable_cow(&parent->vas, family.resident);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(&parent->vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	for (i = 0; i < words; i++) {
		rc = uk_vas_store_u32(&parent->vas,
				      address + i * sizeof(value),
				      0x11000000U + i);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			goto out;
	}
	for (i = 0; i < cpus; i++) {
		struct uk_sched *sched = uk_sched_select_round_robin(local);
		struct cow_test_remote *p;

		if (sched->lcpu_idx == local->lcpu_idx)
			continue;
		UK_TEST_EXPECT(started < words);
		if (started >= words)
			goto out;
		p = &probes[started];
		p->group = &group;
		p->vas = &parent->vas;
		p->address = address + started * sizeof(value);
		p->before = 0x11000000U + started;
		p->after = 0x22000000U + started;
		p->cpu = sched->lcpu_idx;
		rc = cow_test_remote_start(sched, p);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			goto out;
		started++;
	}
	UK_TEST_EXPECT_SNUM_EQ(started, cpus - 1);
	for (i = 0; i < started; i++) {
		rc = cow_test_remote_ready(&probes[i]);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			goto out;
		UK_TEST_EXPECT(probes[i].active);
	}
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, &original, NULL));
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, NULL, &attr));
	UK_TEST_EXPECT_ZERO(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(child, address, &after, &attr));
	UK_TEST_EXPECT_SNUM_EQ(after, original);
	UK_TEST_EXPECT_ZERO(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	allocated = family.frames.allocated;
	group.cancel = false;
	__atomic_store_n(&group.go, true, __ATOMIC_RELEASE);
	if (!started) {
		rc = uk_vas_store_u32(&parent->vas, address, 0x22000000U);
		UK_TEST_EXPECT_ZERO(rc);
	}
	cow_test_remote_join(probes, started, &reaped, &parent->vas);
	copies = family.frames.allocated - allocated;
	UK_TEST_EXPECT_SNUM_EQ(copies, 1);
	for (i = 0; i < started; i++) {
		UK_TEST_EXPECT(probes[i].completed);
		UK_TEST_EXPECT_SNUM_EQ(probes[i].observed, probes[i].after);
	}
	for (i = 0; i < words; i++) {
		rc = cow_test_read(parent, address + i * sizeof(value), &value);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(value, 0x22000000U + i);
		rc = cow_test_read(child, address + i * sizeof(value), &value);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(value, 0x11000000U + i);
	}
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, &after, NULL));
	UK_TEST_EXPECT(after != original);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(child, address, &after, NULL));
	UK_TEST_EXPECT_SNUM_EQ(after, original);
	completed = true;
out:
	if (started) {
		if (!__atomic_load_n(&group.go, __ATOMIC_ACQUIRE)) {
			group.cancel = true;
			__atomic_store_n(&group.go, true, __ATOMIC_RELEASE);
		}
		cow_test_remote_join(probes, started, &reaped, &parent->vas);
	}
	uk_free(kernel->a, probes);
	cow_test_space_put(&parent);
	cow_test_space_put(&child);
	cow_test_family_close(&family);
	UK_TEST_EXPECT_ZERO(family.cleanup_errors);
	UK_TEST_EXPECT_ZERO(family.frames.errors);
	UK_TEST_EXPECT_ZERO(family.frames.live);
	UK_TEST_EXPECT_SNUM_EQ(family.frames.allocated, family.frames.freed);
	UK_TEST_EXPECT_SNUM_EQ(family.metadata_allocated,
			       family.metadata_freed);
	if (completed)
		uk_pr_info("COW_VMA_REMOTE_PASS cpus=%u remote=%u copies=%lu\n",
			   cpus, started, copies);
}

struct cow_test_pending_fault {
	struct uk_vma *vma;
	struct uk_vm_fault_op *operation;
	unsigned int prepared;
	unsigned int results;
	int status;
};

static bool cow_test_completion_active;
static unsigned int cow_test_completion_faults;

#if CONFIG_ARCH_X86_64 && CONFIG_LIBUKPLAT_NATIVE_EXCEPT
#include <uk/arch/x86_64.h>
#include <uk/event.h>
#include <uk/pcpuvar.h>

static unsigned int cow_test_completion_cpu;

static int cow_test_completion_observe(void *arg)
{
	__u64 orig_gsbase = uk_arch_x86_64_enter_kernel_gsbase();
	struct uk_lcpu_except_err_ctx *ctx = arg;
	struct uk_lcpu_regs *regs;

	if (__atomic_load_n(&cow_test_completion_active, __ATOMIC_ACQUIRE) &&
	    uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx) ==
	    cow_test_completion_cpu) {
		regs = (struct uk_lcpu_regs *)
			uk_lcpu_except_err_ctx_get_regs(ctx);
		__atomic_add_fetch(&cow_test_completion_faults, 1,
				   __ATOMIC_RELAXED);
		uk_pr_info_isr("COW_VMA_AFTER_UNMAP_FAULT ip=%#lx\n",
			       (unsigned long)uk_lcpu_regs_get(regs, RIP));
	}
	uk_arch_x86_64_exit_kernel_gsbase(orig_gsbase);
	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT,
		      cow_test_completion_observe,
		      UK_PRIO_BEFORE(CONFIG_LIBUKVMEM_PAGEFAULT_HANDLER_PRIO));
#endif

static int cow_test_pending_map(struct uk_pagetable *pt __unused,
				__vaddr_t vaddr, __vaddr_t pt_vaddr __unused,
				unsigned int level, __pte_t *pte, void *arg)
{
	struct cow_test_pending_fault *p = arg;
	struct uk_vm_fault fault = {
		.vaddr = vaddr,
		.vbase = vaddr,
		.len = UK_PAGING_PAGE_Lx_SIZE(level),
		.paddr = UK_PAGING_PT_Lx_PTE_PADDR(*pte, level),
		.type = UK_VMA_FAULT_WRITE | UK_VMA_FAULT_NONPRESENT |
			UK_VMA_FAULT_SOFT | UK_VMA_FAULT_IRQS_ENABLED,
		.pte = *pte,
		.level = level,
		.operation = &p->operation
	};
	int rc;

	p->prepared++;
	rc = p->vma->ops->fault(p->vma, &fault);
	if (!rc)
		*pte = UK_PAGING_PT_Lx_PTE_SET_PADDR(fault.pte, level,
						     fault.paddr);
	return rc;
}

static void cow_test_pending_result(struct uk_pagetable *pt,
				    __vaddr_t vaddr __unused,
				    __vaddr_t pt_vaddr __unused,
				    unsigned int level __unused,
				    int status, void *arg)
{
	struct cow_test_pending_fault *p = arg;
	struct uk_vm_fault_op *op = p->operation;

	p->operation = NULL;
	p->results++;
	p->status = status;
	if (op)
		op->result(op, status, pt->defer_batch);
}

UK_TESTCASE(cow_vmas, unmap_before_new_page_completion)
{
	struct cow_test_family family;
	struct cow_test_space *space = NULL;
	struct cow_test_pending_fault pending = { 0 };
	struct uk_paging_page_mapx mapx = {
		.map = cow_test_pending_map, .ctx = &pending
	};
	struct uk_paging_free_batch batch;
	struct cow_test_metadata *metadata;
	__vaddr_t address = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	unsigned long irqf;
	int rc;

	rc = cow_test_family_init(&family);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	metadata = (void *)family.resident->priv;
	space = cow_test_space_new(&family);
	UK_TEST_EXPECT_NOT_NULL(space);
	if (!space)
		goto out;
	rc = uk_vas_enable_cow(&space->vas, family.resident);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(&space->vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW,
			     UK_VMA_MAP_SIZE_4KB, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	pending.vma = uk_vma_find(&space->vas, address);
	UK_TEST_EXPECT_NOT_NULL(pending.vma);
	if (!pending.vma)
		goto out;
	/* Hold only the completion, after the real mapper committed its PTE. */
	ukplat_spin_lock_irqsave(&space->vas.lock, irqf);
	uk_paging_batch_begin(&batch, &space->pt);
	rc = uk_paging_page_mapx_with_result(&space->pt, address, 0, 1,
					     UK_PAGING_PAGE_ATTR_PROT_RW,
					     COW_TEST_PAGE_FLAGS, &mapx,
					     cow_test_pending_result);
					     uk_paging_batch_end(&batch);
	ukplat_spin_unlock_irqrestore(&space->vas.lock, irqf);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(pending.prepared, 1);
	UK_TEST_EXPECT_SNUM_EQ(pending.results, 1);
	UK_TEST_EXPECT_ZERO(pending.status);
	UK_TEST_EXPECT_NOT_NULL(batch.calls);
	/* Another CPU may unmap this VMA before the fault completion runs. */
	metadata->poison = true;
	rc = uk_vma_unmap(&space->vas, address, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_NULL(uk_vma_find(&space->vas, address));
	UK_TEST_EXPECT(metadata->poisoned > 0);
	__atomic_store_n(&cow_test_completion_faults, 0, __ATOMIC_RELAXED);
#if CONFIG_ARCH_X86_64 && CONFIG_LIBUKPLAT_NATIVE_EXCEPT
	cow_test_completion_cpu = uk_sched_current()->lcpu_idx;
#endif
	uk_pr_info("COW_VMA_AFTER_UNMAP_BEGIN\n");
	__atomic_store_n(&cow_test_completion_active, true, __ATOMIC_RELEASE);
	uk_paging_free_batch_finish(&batch);
	__atomic_store_n(&cow_test_completion_active, false, __ATOMIC_RELEASE);
	UK_TEST_EXPECT_ZERO(cow_test_completion_faults);
	uk_pr_info("COW_VMA_AFTER_UNMAP_PASS\n");
out:
	cow_test_space_put(&space);
	cow_test_family_close(&family);
	UK_TEST_EXPECT_ZERO(family.cleanup_errors);
	UK_TEST_EXPECT_ZERO(family.frames.errors);
	UK_TEST_EXPECT_ZERO(family.frames.live);
	UK_TEST_EXPECT_SNUM_EQ(family.frames.allocated, family.frames.freed);
	UK_TEST_EXPECT_SNUM_EQ(family.metadata_allocated,
			       family.metadata_freed);
}

UK_TESTCASE(cow_vmas, private_clone_and_parent_exit)
{
	struct cow_test_family family;
	struct cow_test_space *parent = NULL, *child = NULL;
	__vaddr_t address = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	__paddr_t original = 0, copied = 0, after = 0;
	unsigned long attr = 0;
	__u32 value = 0;
	int rc;

	rc = cow_test_family_init(&family);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	parent = cow_test_space_new(&family);
	child = cow_test_space_new(&family);
	UK_TEST_EXPECT_NOT_NULL(parent);
	UK_TEST_EXPECT_NOT_NULL(child);
	if (!parent || !child)
		goto out;
	rc = uk_vas_enable_cow(&parent->vas, family.resident);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(&parent->vas, &address, 3 * UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_store_u32(&parent->vas, address, 0x12345678U);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = cow_test_mapping(parent, address, &original, &attr);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(child, address, &copied, &attr));
	UK_TEST_EXPECT_SNUM_EQ(original, copied);
	UK_TEST_EXPECT_ZERO(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	rc = uk_vas_store_u32(&child->vas, address, 0xabcdef01U);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(cow_test_mapping(child, address, &copied, &attr));
	UK_TEST_EXPECT(copied != original);
	UK_TEST_EXPECT(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	UK_TEST_EXPECT_ZERO(cow_test_read(parent, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0x12345678U);
	UK_TEST_EXPECT_ZERO(cow_test_read(child, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0xabcdef01U);
	rc = uk_vas_store_u32(&parent->vas, address, 0x11223344U);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, &after, NULL));
	UK_TEST_EXPECT_SNUM_EQ(after, original);
	rc = uk_vas_store_u32(&parent->vas,
			      address + UK_PAGING_PAGE_SIZE, 0x11111111U);
	UK_TEST_EXPECT_ZERO(rc);
	rc = uk_vas_store_u32(&child->vas,
			      address + UK_PAGING_PAGE_SIZE, 0x22222222U);
	UK_TEST_EXPECT_ZERO(rc);
	rc = cow_test_read(parent, address + UK_PAGING_PAGE_SIZE, &value);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x11111111U);
	rc = cow_test_read(child, address + UK_PAGING_PAGE_SIZE, &value);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(value, 0x22222222U);
	cow_test_space_put(&parent);
	UK_TEST_EXPECT_SNUM_EQ(family.released, 1);
	UK_TEST_EXPECT_ZERO(cow_test_read(child, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0xabcdef01U);
	rc = uk_vas_store_u32(&child->vas,
			      address + 2 * UK_PAGING_PAGE_SIZE, 0x55667788U);
	UK_TEST_EXPECT_ZERO(rc);
out:
	cow_test_space_put(&parent);
	cow_test_space_put(&child);
	cow_test_family_close(&family);
	UK_TEST_EXPECT_ZERO(family.cleanup_errors);
	UK_TEST_EXPECT_ZERO(family.frames.errors);
	UK_TEST_EXPECT_ZERO(family.frames.live);
	UK_TEST_EXPECT_SNUM_EQ(family.frames.allocated, family.frames.freed);
	UK_TEST_EXPECT_SNUM_EQ(family.metadata_allocated,
			       family.metadata_freed);
}

UK_TESTCASE(cow_vmas, protection_discard_and_partial_unmap)
{
	struct cow_test_family family;
	struct cow_test_space *parent = NULL, *child = NULL;
	__vaddr_t address = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	__vaddr_t middle;
	unsigned long attr = 0;
	__u32 value = 0;
	unsigned int i;
	int rc;

	rc = cow_test_family_init(&family);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	parent = cow_test_space_new(&family);
	child = cow_test_space_new(&family);
	UK_TEST_EXPECT_NOT_NULL(parent);
	UK_TEST_EXPECT_NOT_NULL(child);
	if (!parent || !child)
		goto out;
	rc = uk_vas_enable_cow(&parent->vas, family.resident);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(&parent->vas, &address, 3 * UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	for (i = 0; i < 3; i++) {
		rc = uk_vas_store_u32(&parent->vas,
				      address + i * UK_PAGING_PAGE_SIZE, i + 1);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			goto out;
	}
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	middle = address + UK_PAGING_PAGE_SIZE;
	rc = uk_vma_set_attr(&child->vas, middle, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_READ, 0);
	UK_TEST_EXPECT_ZERO(rc);
	rc = uk_vas_fault(&child->vas, middle, UK_VMA_FAULT_WRITE);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EFAULT);
	rc = uk_vma_set_attr(&child->vas, middle, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(child, middle, NULL, &attr));
	UK_TEST_EXPECT_ZERO(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(&child->vas, middle, 99));
	UK_TEST_EXPECT_ZERO(cow_test_read(parent, middle, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 2);
	rc = uk_vma_advise(&child->vas, middle, UK_PAGING_PAGE_SIZE,
			   UK_VMA_ADV_DONTNEED, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(cow_test_read(child, middle, &value));
	UK_TEST_EXPECT_ZERO(value);
	UK_TEST_EXPECT_ZERO(cow_test_read(parent, middle, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 2);
	rc = uk_vma_unmap(&child->vas, middle, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_NULL(uk_vma_find(&child->vas, middle));
	UK_TEST_EXPECT_ZERO(cow_test_read(child, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 1);
	rc = cow_test_read(child, address + 2 * UK_PAGING_PAGE_SIZE, &value);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(value, 3);
out:
	cow_test_space_put(&parent);
	cow_test_space_put(&child);
	cow_test_family_close(&family);
	UK_TEST_EXPECT_ZERO(family.cleanup_errors);
	UK_TEST_EXPECT_ZERO(family.frames.errors);
	UK_TEST_EXPECT_ZERO(family.frames.live);
	UK_TEST_EXPECT_SNUM_EQ(family.frames.allocated, family.frames.freed);
	UK_TEST_EXPECT_SNUM_EQ(family.metadata_allocated,
			       family.metadata_freed);
}

UK_TESTCASE(cow_vmas, allocation_failure_preserves_source)
{
	struct cow_test_family family;
	struct cow_test_space *parent = NULL, *child = NULL;
	struct cow_test_metadata *metadata;
	__vaddr_t address = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	__vaddr_t reserved;
	__paddr_t original = 0, after = 0;
	unsigned long attr = 0;
	unsigned int attempts;
	__u32 value = 0;
	int rc;

	rc = cow_test_family_init(&family);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	metadata = (void *)family.resident->priv;
	parent = cow_test_space_new(&family);
	child = cow_test_space_new(&family);
	UK_TEST_EXPECT_NOT_NULL(parent);
	UK_TEST_EXPECT_NOT_NULL(child);
	if (!parent || !child)
		goto out;
	rc = uk_vas_enable_cow(&parent->vas, family.resident);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_map_anon(&parent->vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	metadata->fail = true;
	metadata->remaining = 0;
	rc = uk_vas_store_u32(&parent->vas, address, 17);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_SNUM_EQ(cow_test_mapping(parent, address, NULL, NULL),
			       -ENOENT);
	metadata->fail = false;
	rc = uk_vas_store_u32(&parent->vas, address, 17);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, &original, NULL));
	reserved = address + UK_PAGING_PAGE_SIZE;
	rc = uk_vma_reserve(&parent->vas, &reserved, UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOTSUP);
	UK_TEST_EXPECT_NULL(child->vas.cow);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, &after, &attr));
	UK_TEST_EXPECT_SNUM_EQ(original, after);
	UK_TEST_EXPECT(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	rc = uk_vma_unmap(&parent->vas, reserved, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	/* The destination context succeeds; its first VMA allocation fails. */
	metadata->fail = true;
	metadata->remaining = 1;
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_NULL(parent->vas.transition);
	UK_TEST_EXPECT_NULL(child->vas.cow);
	UK_TEST_EXPECT(uk_list_empty(&child->vas.vma_list));
	UK_TEST_EXPECT_ZERO(cow_test_mapping(parent, address, &after, &attr));
	UK_TEST_EXPECT_SNUM_EQ(original, after);
	UK_TEST_EXPECT(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	metadata->fail = false;
	rc = uk_vas_clone_cow(&child->vas, &parent->vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	attempts = family.frames.attempts;
	family.frames.fail = true;
	rc = uk_vas_store_u32(&child->vas, address, 29);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOMEM);
	UK_TEST_EXPECT_SNUM_EQ(family.frames.attempts, attempts + 1);
	UK_TEST_EXPECT_NULL(child->vas.transition);
	UK_TEST_EXPECT_ZERO(cow_test_mapping(child, address, &after, &attr));
	UK_TEST_EXPECT_SNUM_EQ(after, original);
	UK_TEST_EXPECT_ZERO(attr & UK_PAGING_PAGE_ATTR_PROT_WRITE);
	UK_TEST_EXPECT_ZERO(cow_test_read(parent, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 17);
	UK_TEST_EXPECT_ZERO(cow_test_read(child, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 17);
	family.frames.fail = false;
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(&child->vas, address, 29));
	UK_TEST_EXPECT_ZERO(cow_test_read(child, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 29);
	UK_TEST_EXPECT_ZERO(cow_test_read(parent, address, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 17);
out:
	metadata->fail = false;
	family.frames.fail = false;
	cow_test_space_put(&parent);
	cow_test_space_put(&child);
	cow_test_family_close(&family);
	UK_TEST_EXPECT_ZERO(family.cleanup_errors);
	UK_TEST_EXPECT_ZERO(family.frames.errors);
	UK_TEST_EXPECT_ZERO(family.frames.live);
	UK_TEST_EXPECT_SNUM_EQ(family.frames.allocated, family.frames.freed);
	UK_TEST_EXPECT_SNUM_EQ(family.metadata_allocated,
			       family.metadata_freed);
}

_UK_TESTSUITE(cow_vmas, NULL);

static void __constructor run_cow_vmas(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(cow_vmas));
}
