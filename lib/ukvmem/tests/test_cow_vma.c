/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/essentials.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/plat/spinlock.h>
#include <uk/test.h>
#include <uk/vmem.h>
#include <uk/vmem/vma_types.h>

#define COW_TEST_FRAMES 128

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
