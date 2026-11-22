/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_VMEM_H__
#define __UK_VMEM_H__

/* This library provides support for virtual address space (VAS) management. A
 * virtual address space is a list of virtual memory areas (VMA) describing
 * mappings and address reservations in the address space. The functions in
 * this library provide the means to create, modify, and remove VMAs.
 *
 * Newly created VMAs are placed         ┌───────────────────────┐ 0x000000...0
 * in the address space at a             ├─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─┤
 * sufficiently large hole starting      │/  /  /  /  /  /  /  / │
 * at the VMA base if not specified      │  /  / UNIKRAFT  /  /  │
 * otherwise. Adjacent VMAs              │ /  /  /  /  /  /  /  /│
 * which have compatible properties      ├─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─┤
 * (e.g., same protections) are          │                       │
 * merged. Existing VMAs may be split    ├───────────────────────┤◄─── VMA Base
 * into separate VMAs if properties      │/  /  /  VMA 1 /  /  / │
 * are changed, for example, by          ├───────────────────────┤
 * changing protections of an address    │/  /  /  /  /  /  /  / │
 * range.                                │  /  /   VMA 2   /  /  │
 *                                       │ /  /  /  /  /  /  /  /│
 * VMAs can define custom handlers       ├───────────────────────┤
 * (e.g., for page fault handling). Thus │                       │
 * allowing to implement different VMA   ├───────────────────────┤◄─ User
 * types like anonymous memory, stacks,  │/  /  /  VMA 3 /  /  / │   selected
 * or file mappings. Depending on memory ├───────────────────────┤
 * type demand-paging or pre-allocation  │                       │
 * are supported.                        └───────────────────────┘ 0xffffff...f
 */

#include <uk/config.h>
#include <uk/arch/spinlock.h>
#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/alloc.h>
#if CONFIG_LIBUKVMEM_VAS_REFCOUNT
#include <uk/refcount.h>
#endif
#ifdef CONFIG_LIBUKPAGING
#include <uk/paging.h>
#endif /* CONFIG_LIBUKPAGING */

#ifdef __cplusplus
extern "C" {
#endif

struct uk_pagetable;

struct uk_vma;
struct uk_vma_ops;
struct vmem_cow_vas;
#if CONFIG_LIBUKVMEM_COW
struct vmem_vas_transition;
#endif

/** Virtual address space (VAS) */
struct uk_vas {
	/** Allocator to use for VMAs */
	struct uk_alloc *a;
#if CONFIG_LIBUKVMEM_VAS_REFCOUNT
	/** Present only for address spaces with an explicit final release. */
	__atomic refs;
	void (*release)(struct uk_vas *vas);
#endif

#ifdef CONFIG_LIBUKPAGING
	/** Page table that represents the virtual address space */
	struct uk_pagetable *pt;

	/** Base address where to start putting new VMAs */
	__vaddr_t vma_base;
#if CONFIG_LIBUKVMEM_VAS_BOUNDS
	/** Optional allocation interval [start, end); zero end is unbounded. */
	__vaddr_t range_start;
	__vaddr_t range_end;
#endif
#endif /* CONFIG_LIBUKPAGING */

	/** List of VMAs, sorted by address */
	struct uk_list_head vma_list;

	/** VAS flags */
#define UK_VAS_FLAG_NO_PAGING		0x1 /* On-demand paging disabled */
	unsigned long flags;

	/** Serializes VMA list and page-table mutations across CPUs/IRQs */
	struct __spinlock lock;
#if CONFIG_LIBUKVMEM_COW
	/** Mapping transition awaiting completion outside the VAS lock. */
	struct vmem_vas_transition *transition;
#endif
#if CONFIG_LIBUKVMEM_COW_VMA
	/** Private mapping references; shared metadata outlives all clones. */
	struct vmem_cow_vas *cow;
#endif
};

/**
 * Suspend demand paging for this VAS on the executing CPU. Do not
 * yield while a saved state is outstanding. Restore in reverse order.
 *
 * @return An opaque state for uk_vas_paging_restoref().
 */
unsigned long uk_vas_paging_savef(struct uk_vas *vas);
void uk_vas_paging_restoref(struct uk_vas *vas, unsigned long state);
int uk_vas_paging_enabled(struct uk_vas *vas);

/** Virtual memory area (VMA) */
struct uk_vma {
	__vaddr_t start;
	__vaddr_t end;

	struct uk_vas *vas;
	const struct uk_vma_ops *ops;

	struct uk_list_head vma_list;

	/** Page attributes for pages in the VMA (see PAGE_ATTR_*) */
	unsigned long attr;

	/** VMA flags - high word bits are from mapping flags */
#define UK_VMA_FLAG_UNINITIALIZED	0x1 /* Do not initialize memory */
	unsigned long flags;

	/** Desired page level (-1 = no preference) */
	int page_lvl;

	/** Active unlocked sync invocations pinning this VMA against destroy */
	__u32 sync_inflight;

	/** Optional name of the VMA */
	const char *name;
};

/**
 * Returns the length of a VMA in bytes.
 */
static inline __sz uk_vma_len(struct uk_vma *vma)
{
	UK_ASSERT(vma->end > vma->start);

	return vma->end - vma->start;
}

#if CONFIG_LIBUKVMEM_COW
/**
 * Staged fault ownership. The result callback runs exactly once after the
 * mapper outcome, under VAS and paging locks with IRQs masked. A zero result
 * commits ownership; other values discard it. Retirement of replaced frames
 * still belongs to the supplied TLB batch. Do not block, yield or reenter
 * VAS/paging operations here; enqueue later work in the batch instead. The
 * descriptor must outlive the fault callback and may be freed by result().
 */
struct uk_vm_fault_op {
	void (*result)(struct uk_vm_fault_op *op, int status,
		       struct uk_paging_free_batch *batch);
};
#endif

/** Page fault context */
struct uk_vm_fault {
	/** Faulting virtual address */
	const __vaddr_t vaddr;

	/**
	 * Number of bytes starting at vbase affected by the fault. This
	 * is not the size of the faulting access, but the amount of contiguous
	 * physical memory which has to be supplied by the fault handler.
	 */
	const __sz len;

	/**
	 * Base address of the memory region for which the fault should be
	 * resolved (not the base address of the VMA). For an access in the
	 * middle of a page this is likely the address of the affected page.
	 * But it might also be different if preceding memory should also be
	 * paged-in.
	 */
	const __vaddr_t vbase;

#ifdef CONFIG_LIBUKPAGING
	/**
	 * Mapped physical address, if any. Modify to change mapping. The
	 * physical memory must be aligned to its size.
	 */
	__paddr_t paddr;

	/** Type of the fault */
#define UK_VMA_FAULT_ACCESSTYPE		0x03
#define UK_VMA_FAULT_READ		0x00 /* Attempted read access */
#define UK_VMA_FAULT_WRITE		0x01 /* Attempted write access */
#define UK_VMA_FAULT_EXEC		0x02 /* Attempted instruction fetch */

#define UK_VMA_FAULT_NONPRESENT		0x04 /* Page not present */
#define UK_VMA_FAULT_MISCONFIG		0x08 /* Misconfiguration in PT */

#define UK_VMA_FAULT_SOFT		0x10 /* Software-generated fault */
/* The interrupted context permits maskable interrupts during completion. */
#define UK_VMA_FAULT_IRQS_ENABLED	0x20
	const unsigned int type;

	/**
	 * PTE that will be used to resolve the fault. Can be modified. The
	 * physical address bits are overwritten with the value of the paddr
	 * field when the PTE is applied to the page table.
	 */
	__pte_t pte;

	/** Level of the PTE that will be used to resolve the fault. */
	const unsigned int level;

	/** Trap frame */
	struct uk_lcpu_regs *regs;
#if CONFIG_LIBUKVMEM_COW
	/** Optional staged ownership result, set by the backend. */
	struct uk_vm_fault_op **operation;
#endif
#endif /* CONFIG_LIBUKPAGING */
};

/** VMA operations */
struct uk_vma_ops {
	/**
	 * Returns the base virtual address from which to begin the search for
	 * a large enough hole to allocate.
	 *
	 * Can be __NULL, in which case the common base address is used.
	 *
	 * @param vas
	 *   The virtual address space where the VMA will be created
	 * @param data
	 *   Pointer to VMA-type specific data supplied to the map function
	 * @param flags
	 *   The flags supplied to the map function
	 *
	 * @return
	 *   The base virtual address to use
	 */
	__vaddr_t (*get_base)(struct uk_vas *vas, void *data,
			      unsigned long flags);

	/**
	 * Allocates a VMA object and initializes its VMA-type specific
	 * attributes. The name of the VMA object must be initialized but can
	 * be __NULL. All other fields of the base VMA object can be left
	 * uninitialized.
	 *
	 * Note, a user-supplied name always overrides the name set by this
	 * handler. To avoid a memory leak a copy of the string pointer should
	 * be kept to free the string in destroy()
	 *
	 * Can be __NULL, if no extended VMA object is required.
	 *
	 * @param vas
	 *   The virtual address space to create the VMA for
	 * @param vaddr
	 *   The virtual address where the VMA will be created
	 * @param len
	 *   The length of the virtual memory area in bytes
	 * @param data
	 *   Pointer to VMA-type specific data supplied to the map function
	 * @param attr
	 *   Page attributes for the pages in the memory area
	 * @param[in,out] flags
	 *   The flags supplied to the map function. The handler may
	 *   set/unset the following flags (besides VMA-type specific ones):
	 *     - UK_VMA_MAP_POPULATE
	 *     - UK_VMA_MAP_UNINITIALIZED
	 * @param[out] vma
	 *   Pointer to the allocated VMA object
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise
	 */
	int (*new)(struct uk_vas *vas, __vaddr_t vaddr, __sz len, void *data,
		   unsigned long attr, unsigned long *flags,
		   struct uk_vma **vma);

	/**
	 * Frees any references that this VMA might hold. The memory for the
	 * VMA itself is freed by the caller.
	 *
	 * Can be __NULL.
	 *
	 * @param vma
	 *   The virtual memory area to be destroyed
	 */
	void (*destroy)(struct uk_vma *vma);

	/**
	 * The fault handler is called whenever a page fault is raised for an
	 * address in the VMA. This may happen, for instance, due to a failed
	 * memory access, a misconfiguration in the page table, or a due to a
	 * software-generated page fault to prefault memory. The handler is
	 * responsible for resolving the fault by providing a physical
	 * address that should be mapped at the faulting virtual address.
	 *
	 * The handler may use a temporary mapping (see uk_paging_page_kmap())
	 * to initialize the physical memory.
	 *
	 * Cannot be __NULL, if the VMA should map physical memory.
	 *
	 * @param vma
	 *   The virtual memory area that experienced the fault
	 * @param[in,out] fault
	 *   Pointer to the fault description. Changes to the paddr and pte
	 *   fields are applied to the mapping by the caller when the handler
	 *   returns.
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise
	 */
	int (*fault)(struct uk_vma *vma, struct uk_vm_fault *fault);

	/**
	 * Unmaps a range of the VMA. It is the handler's responsibily to
	 * perform the actual unmap in the page table and potentially release
	 * the physical memory.
	 *
	 * Cannot be __NULL, if the VMA should map physical memory.
	 *
	 * @param vma
	 *   The VMA in which a range should be unmapped
	 * @param vaddr
	 *   The base address of the range which should be unmapped
	 * @param len
	 *   The length of the range in bytes
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise
	 */
	int (*unmap)(struct uk_vma *vma, __vaddr_t vaddr, __sz len);

	/** Synchronize backing storage for a mapped range. May block in normal
	 * thread context. Return 0 or a negative errno; no callback means that
	 * the mapping has no backing storage requiring synchronization.
	 */
	int (*sync)(struct uk_vma *vma, __vaddr_t vaddr, __sz len);

	/**
	 * Allocates a new VMA and configures its VMA-type specific properties
	 * when a VMA is split into multiple separate VMAs. This may happen
	 * when a range of a VMA is removed or protections are changed. The
	 * handler also has to adjust any VMA-type specific properties for the
	 * existing VMA, if necessary.
	 *
	 * Cannot be __NULL if new() is supplied. Otherwise, can be __NULL.
	 *
	 * @param vma
	 *   The VMA to split
	 * @param vaddr
	 *   The address at which to split the VMA. The new VMA will cover the
	 *   area [vaddr, vma->end).
	 * @param[out] new_vma
	 *   A variable receiving a pointer to the newly allocated VMA
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise. Return -EPERM to
	 *   deny the split.
	 */
	int (*split)(struct uk_vma *vma, __vaddr_t vaddr,
		     struct uk_vma **new_vma);

	/**
	 * Merges two adjacent VMAs when they become compatible in their type
	 * and attributes. This callback gives the VMA implementation the
	 * opportunity to implement checks if the merge is valid and adjust
	 * VMA-specific properties to reflect the merge.
	 *
	 * Can be __NULL.
	 *
	 * @param vma
	 *   The left-hand side (lower addresses) VMA of the merge. This VMA
	 *   will represent the new larger VMA
	 * @param next
	 *   The right-hand side (larger addresses) VMA of the merge. This VMA
	 *   will be destroyed by the caller after the merge
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise. Return -EPERM to
	 *   deny the merge.
	 */
	int (*merge)(struct uk_vma *vma, struct uk_vma *next);

	/**
	 * Sets the given attributes for the VMA. The caller takes care of
	 * updating the generic VMA properties. However, it is the handler's
	 * responsibility to apply the changed attributes to the page table.
	 *
	 * Can be __NULL, in which case the attribute change is ignored.
	 *
	 * @param vma
	 *   The VMA for which to change the attributes
	 * @param attr
	 *   The new attributes (see PAGE_ATTR_*)
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise
	 */
	int (*set_attr)(struct uk_vma *vma, unsigned long attr);

	/**
	 * Applies the given advice to the specified address range.
	 *
	 * Can be __NULL, in which case the advice is ignored.
	 *
	 * @param vma
	 *   The VMA to operate on
	 * @param vaddr
	 *   The base virtual address of the range that the advice should be
	 *   applied to
	 * @param len
	 *   The number of bytes of the address range
	 * @param advice
	 *   The advice to take (see UK_VMA_ADV_*)
	 *
	 * @return
	 *   0 on success, a negative errno error otherwise
	 */
	int (*advise)(struct uk_vma *vma, __vaddr_t vaddr, __sz len,
		      unsigned long advice);
};

/**
 * Returns the currently active virtual address space.
 */
struct uk_vas *uk_vas_get_active(void);

#if CONFIG_LIBUKVMEM_THREAD_VAS
/** Return the boot address space used by kernel threads. */
struct uk_vas *uk_vas_get_kernel(void);

/** Set the kernel address space once, before starting other CPUs. */
void uk_vas_set_kernel(struct uk_vas *vas);
#endif

/**
 * Switches the currently active virtual address space.
 *
 * @param vas
 *   The fully-initialized address space to switch to
 *
 * @return
 *   0 on success, a negative errno error otherwise
 */
int uk_vas_set_active(struct uk_vas *vas);

#if CONFIG_LIBUKVMEM_COW_VMA
/**
 * Enable private anonymous COW mappings in an empty, unpublished owned VAS.
 * The allocator supplies resident malloc/free and palloc/pfree operations
 * that cannot fault, block or yield, and outlives this VAS and all its clones.
 * VMA allocators of cloned address spaces have the same context constraints.
 */
int uk_vas_enable_cow(struct uk_vas *vas, struct uk_alloc *resident);

/**
 * Clone supported mappings into an empty, unpublished owned destination VAS.
 * Both page tables already map the shared kernel. Their application branches
 * are private, and the destination has no application PTEs. The caller holds
 * both objects live, permits interrupts, and does not access private source
 * mappings while the operation is running. Failure leaves the destination
 * empty; harmless source write protection may remain until the next fault.
 */
int uk_vas_clone_cow(struct uk_vas *dst, struct uk_vas *src);

/** A pinned shared mapping identity. Compare object and byte offset only. */
struct uk_vma_shared_ref {
	const void *object;
	__sz offset;
	void (*get)(const void *object);
	void (*put)(const void *object);
};

/**
 * Pin the shared backing at a byte range in a live VAS. Never faults, waits
 * for a VAS transition or allocates. Returns -EAGAIN during a transition,
 * -EFAULT for an unmapped range, or -ENOTSUP for a non-shared backend.
 * The output is unchanged on failure. The caller retains the VAS while
 * looking up the range; the returned pin can outlive every mapping of it.
 */
int uk_vas_get_shared_ref(struct uk_vas *vas, __vaddr_t addr, __sz len,
			  struct uk_vma_shared_ref *out);

/** Acquire another pin before copying a live reference. */
void uk_vma_shared_ref_get(const struct uk_vma_shared_ref *ref);

/** Release and clear a pin outside locks, with IRQs enabled. */
void uk_vma_shared_ref_put(struct uk_vma_shared_ref *ref);
#endif

#if CONFIG_LIBUKVMEM_VAS_ACCESS
/**
 * Resolve a read, write or execute access in an explicit VAS. The caller holds
 * the VAS live and calls with IRQs enabled outside locks. Software faults may
 * allocate and finish remote TLB invalidations. Returns 0 or a negative errno.
 */
int uk_vas_fault(struct uk_vas *vas, __vaddr_t vaddr, unsigned int access);

/**
 * Read an aligned word without faulting, allocating or waiting for a VAS
 * transition. Retains the owning VAS lock through the direct-map load.
 * Returns -EAGAIN for an absent PTE or an ongoing transition; other access
 * failures return a negative errno. The output is unchanged on failure.
 * The caller holds the VAS live; the function is safe with IRQs disabled.
 */
int uk_vas_load_u32_try(struct uk_vas *vas, __vaddr_t addr, __u32 *out);

#if CONFIG_LIBUKVMEM_COW_VMA
/**
 * Read a word and pin its shared identity in the same VAS transaction.
 * A private mapping returns an empty reference. The reference output must
 * be empty on entry to a successful call. Both outputs stay unchanged on
 * failure; other constraints match uk_vas_load_u32_try().
 */
int uk_vas_load_u32_shared_try(struct uk_vas *vas, __vaddr_t addr,
			       __u32 *out, struct uk_vma_shared_ref *ref);
#endif

/**
 * Store an aligned 32-bit value in an explicit VAS, without activating it.
 * Resolve a writable mapping with IRQs enabled, then recheck and store through
 * the direct map under the owning VAS lock. The caller holds the VAS live and
 * calls outside locks. Returns 0 or a negative errno; misalignment is -EINVAL.
 */
int uk_vas_store_u32(struct uk_vas *vas, __vaddr_t vaddr, __u32 value);
#endif

/**
 * Initializes a new virtual address space using the provided page table.
 *
 * @param vas
 *   Pointer to an uninitialized virtual address space object
 * @param pt
 *   The page table to use for the address space
 * @param a
 *   Allocator for VMA metadata. Allocation and free must not fault into
 *   this address space while its lock is held.
 *
 * @return
 *   0 on success, a negative errno error otherwise
 */
int uk_vas_init(struct uk_vas *vas, struct uk_pagetable *pt,
		struct uk_alloc *a);

#if CONFIG_LIBUKVMEM_VAS_BOUNDS
/**
 * Limit VMA placement to the page-aligned interval [start, end).
 * The initialized VAS must be inactive, empty and unpublished; the caller
 * retains exclusive ownership while configuring it. Existing page-table
 * mappings are not changed. Fixed and automatic VMA placements both obey
 * this interval, including replacements.
 */
int uk_vas_set_bounds(struct uk_vas *vas, __vaddr_t start, __vaddr_t end);
#endif

#if CONFIG_LIBUKVMEM_VAS_REFCOUNT
/**
 * Initialize an owned VAS with one caller reference. The final release owns
 * cleanup of its VMAs, page table and containing object. It runs with IRQs
 * enabled and after the VAS is no longer active on any CPU. Each activation
 * and thread binding must retain a reference until leaving that VAS.
 */
int uk_vas_init_owned(struct uk_vas *vas, struct uk_pagetable *pt,
		      struct uk_alloc *a, void (*release)(struct uk_vas *));

/** Acquire a caller-held live reference; unowned VAS and NULL are unchanged. */
struct uk_vas *uk_vas_get(struct uk_vas *vas);

/** Drop a reference outside locks; a final release requires IRQs enabled. */
void uk_vas_put(struct uk_vas *vas);

static inline int uk_vas_is_owned(const struct uk_vas *vas)
{
	return vas && vas->release;
}
#endif

/**
 * Unmaps and frees all VMAs in the virtual address space and finalizes the
 * address space object. The memory for the VAS object itself is not freed. The
 * referenced page table is not released either.
 *
 * @param vas
 *   The virtual address space to destroy
 */
void uk_vas_destroy(struct uk_vas *vas);

/**
 * Returns the virtual memory area at the given virtual address.
 *
 * @param vas
 *   The virtual address space to operate on
 * @param vaddr
 *   The virtual address to lookup
 *
 * @return
 *   The VMA at the given address, or __NULL if there is no VMA. The VMA may not
 *   be modified. Use the uk_vma_* functions to adjust the address range.
 *   Performing any operations on the virtual address space may free the VMA.
 */
const struct uk_vma *uk_vma_find(struct uk_vas *vas, __vaddr_t vaddr);

/**
 * Returns the first VMA in the VAS sorted by ascending virtual address or
 * __NULL if there are no VMAs in the virtual address space.
 */
static inline const struct uk_vma *uk_vma_first(struct uk_vas *vas)
{
	return uk_list_first_entry_or_null(&vas->vma_list, struct uk_vma,
					   vma_list);
}

/**
 * Returns the last VMA in the VAS sorted by ascending virtual address or
 * __NULL if there are no VMAs in the virtual address space.
 */
static inline const struct uk_vma *uk_vma_last(struct uk_vas *vas)
{
	return uk_list_last_entry_or_null(&vas->vma_list, struct uk_vma,
					  vma_list);
}

/**
 * Returns the previous VMA in the VAS sorted by ascending virtual address or
 * __NULL if this was the first VMA.
 */
static inline const struct uk_vma *uk_vma_prev(const struct uk_vma *vma)
{
	return (vma->vma_list.prev == &vma->vas->vma_list) ?
		__NULL : uk_list_prev_entry(vma, vma_list);
}

/**
 * Returns the next VMA in the VAS sorted by ascending virtual address or
 * __NULL if this was the last VMA.
 */
static inline const struct uk_vma *uk_vma_next(const struct uk_vma *vma)
{
	return (vma->vma_list.next == &vma->vas->vma_list) ?
		__NULL : uk_list_next_entry(vma, vma_list);
}

/* Generic flags - these can be set for multiple functions */
#define UK_VMA_FLAG_STRICT_VMA_CHECK	0x1000 /* Address ranges in VMAs only */

/* VMA map operation flags */
#define UK_VMA_MAP_POPULATE		0x01 /* Prefault memory */
#define UK_VMA_MAP_UNINITIALIZED	0x02 /* Do not zero anonymous memory */
#define UK_VMA_MAP_REPLACE		0x04 /* Replace existing VMAs */

#define UK_VMA_MAP_SIZE_SHIFT		5
#define UK_VMA_MAP_SIZE_BITS		6
#define UK_VMA_MAP_SIZE_MASK						\
	((1UL << UK_VMA_MAP_SIZE_BITS) - 1)

#define UK_VMA_MAP_SIZE(order)						\
	(((order) & UK_VMA_MAP_SIZE_MASK) << UK_VMA_MAP_SIZE_SHIFT)

#define UK_VMA_MAP_SIZE_TO_ORDER(flag)					\
	(((flag) >> UK_VMA_MAP_SIZE_SHIFT) & UK_VMA_MAP_SIZE_MASK)

/* Predefined page sizes available on popular architectures */
#define UK_VMA_MAP_SIZE_4KB		(12UL << UK_VMA_MAP_SIZE_SHIFT)
#define UK_VMA_MAP_SIZE_2MB		(21UL << UK_VMA_MAP_SIZE_SHIFT)
#define UK_VMA_MAP_SIZE_1GB		(30UL << UK_VMA_MAP_SIZE_SHIFT)

/* The high word bits of the flags are usable for VMA-type specific flags */
#define UK_VMA_MAP_EXTF_SHIFT		(sizeof(unsigned long) * 4)
#define UK_VMA_MAP_EXTF_BITS		UK_VMA_MAP_EXTF_SHIFT
#define UK_VMA_MAP_EXTF_MASK						\
	((1UL << UK_VMA_MAP_EXTF_BITS) - 1)

/**
 * Creates a new virtual memory area in the virtual address space. The
 * type of the memory area is defined by the given operations.
 *
 * @param vas
 *   The virtual address space to operate on
 * @param[in,out] vaddr
 *   The virtual address where to create the virtual memory area. Must be
 *   aligned to the specified page size. Can be UK_PAGING_VADDR_ANY to
 *   automatically select a virtual address. The variable will receive the
 *   address on success.
 * @param len
 *   The length of the virtual memory area in bytes. len must always be aligned
 *   to the desired page size. If UK_VMA_MAP_REPLACE is specified and vaddr+len
 *   falls into an existing VMA, vaddr+len must be aligned to the page size of
 *   the VMA that contains this address
 * @param attr
 *   Page attributes to set for the pages in the memory area (see PAGE_ATTR_*)
 * @param flags
 *   Mapping flags (see UK_VMA_MAP_*) and VMA-type specific flags.
 *   UK_VMA_MAP_POPULATE can be used to enfore pre-paging of the whole area.
 *
 *   When supported by the respective VMA type, UK_VMA_MAP_UNINITIALIZED
 *   instructs the VMA implementation to not initialize newly mapped memory
 *   (e.g., to not zero anonymous memory or load file contents). This can be
 *   used to avoid overhead if the memory is overwritten anyways.
 *
 *   Use UK_VMA_MAP_SIZE() or one of the pre-defined UK_VMA_MAP_SIZE_* macros
 *   to enforce a certain page size. If no page size is given vaddr and
 *   len must be aligned to the default page size of the architecture (i.e.,
 *   4KiB on x86).
 *
 *   Use UK_VMA_MAP_REPLACE to replace any colliding address ranges from other
 *   VMAs with this one. Note that this only works if the conflicting VMAs
 *   implement and allow the split and unmap operations.
 * @param name
 *   Optional pointer to a null-terminated string used as name for the VMA in
 *   listings. Can be __NULL.
 * @param ops
 *   Pointer to the operations defining the behavior of the VMA
 * @param args
 *   Pointer to additional VMA-type specific arguments
 *
 * @return
 *   0 on success, a negative errno error otherwise
 *   - EINVAL if vaddr or len are not properly aligned
 *   - EEXIST if UK_VMA_MAP_REPLACE was not specified and the specified
 *            range clashes with an existing mapping
 *   - ENOMEM if no suitable free address range could be found or there is not
 *            enough memory to allocate management data structures
 */
int uk_vma_map(struct uk_vas *vas, __vaddr_t *vaddr, __sz len,
	       unsigned long attr, unsigned long flags, const char *name,
	       const struct uk_vma_ops *ops, void *args);

/**
 * Removes any mapping in the specified virtual address range.
 *
 * @param vas
 *   The virtual address space to operate on
 * @param vaddr
 *   The base address of the virtual address range to unmap. vaddr must be
 *   aligned to the page size. If the address falls into a VMA that enforces a
 *   larger page size, vaddr must be aligned to this page size.
 * @param len
 *   The number of bytes to unmap. len must be aligned to the page size. If
 *   vaddr+len falls into a VMA that enforces a larger page size, vaddr+len
 *   must be aligned to this page size.
 * @param flags
 *   One of the generic flags (UK_VMA_FLAG_*)
 *
 *   With UK_VMA_FLAG_STRICT_VMA_CHECK attempting to unmap an address range
 *   that has no mappings is considered an error. This means there must also be
 *   no holes in the range.
 *
 * @return
 *   0 on success, a negative errno error otherwise
 *   - EINVAL if vaddr or len are not properly aligned
 *   - ENOMEM if there is not enough memory to allocate management data
 *            structures
 *   - ENOENT if strict checking is enabled and at least some part of the
 *            address range is not mapped
 */
int uk_vma_unmap(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		 unsigned long flags);

int uk_vma_unmap_at(struct uk_vas *vas, __vaddr_t vaddr);

/**
 * Changes the paging attributes in the specified virtual address range.
 *
 * @param vas
 *   The virtual address space to operate on
 * @param vaddr
 *   The base address of the virtual address range to change. vaddr must be
 *   aligned to page size. If the address falls into a VMA that enforces a
 *   larger page size, vaddr must be aligned to this page size.
 * @param len
 *   The length of the address range in bytes. len must be aligned to the page
 *   size. If vaddr+len falls into a VMA that enforces a larger page size,
 *   vaddr+len must be aligned to this page size.
 * @param attr
 *   Page attributes to set for the pages in the memory area (see PAGE_ATTR_*)
 * @param flags
 *   One of the generic flags (UK_VMA_FLAG_*)
 *
 * @return
 *   0 on success, a negative errno error otherwise
 *   - EINVAL if vaddr or len are not properly aligned.
 *   - ENOMEM if there is not enough memory to allocate management data
 *            structures.
 */
int uk_vma_set_attr(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		    unsigned long attr, unsigned long flags);

/* VMA advices */
#define UK_VMA_ADV_DONTNEED		0x01 /* Physical memory can be freed */
#define UK_VMA_ADV_WILLNEED		0x02 /* Area should be prefaulted */

/* The high word bits of the advice are usable for VMA-type specific advices */
#define UK_VMA_ADV_EXTF_SHIFT		(sizeof(unsigned long) * 4)
#define UK_VMA_ADV_EXTF_BITS		UK_VMA_ADV_EXTF_SHIFT
#define UK_VMA_ADV_EXTF_MASK						\
	((1UL << UK_VMA_ADV_EXTF_BITS) - 1)

/**
 * Gives the virtual memory system advice about the specified address range.
 *
 * @param vas
 *   The virtual address space to operate on
 * @param vaddr
 *   The base address of the virtual address range that the advice should be
 *   applied to. vaddr must be aligned to page size. If the address falls into
 *   a VMA that enforces a larger page size, vaddr must be aligned to this page
 *   size.
 * @param len
 *   The length of the address range in bytes. len must be aligned to the page
 *   size. If vaddr+len falls into a VMA that enforces a larger page size,
 *   vaddr+len must be aligned to this page size.
 * @param advice
 *   One or more advices (see UK_VMA_ADV_*)
 *
 *   UK_VMA_ADV_DONTNEED informs the virtual memory system that the contents in
 *   the address range is not needed anymore and the physical memory may be
 *   released. On a subsequent access the address range behaves like on the
 *   first access (e.g., demand paging).
 *
 *   UK_VMA_ADV_WILLNEED informs the virtual memory system that the pages will
 *   be needed soon and should be paged in. This can be used to reduce the
 *   number of page faults.
 * @param flags
 *   One of the generic flags (UK_VMA_FLAG_*)
 *
 * @return
 *   0 on success, a negative errno error otherwise
 */
int uk_vma_advise(struct uk_vas *vas, __vaddr_t vaddr, __sz len,
		  unsigned long advice, unsigned long flags);

/** Synchronize a fully mapped, page-aligned range without changing VMAs.
 * Returns -ENOENT for holes and propagates backing-store errors. No disk or
 * file knowledge belongs in the VMA walker; each mapping owns its callback.
 */
int uk_vma_sync(struct uk_vas *vas, __vaddr_t vaddr, __sz len);

#ifdef __cplusplus
}
#endif

#include <uk/vmem/vma_types.h>

#endif /* __UK_VMEM_H__ */
