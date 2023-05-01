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
#include <uk/arch/types.h>
#include <uk/arch/paging.h>
#include <uk/list.h>
#include <uk/alloc.h>
#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */

#ifdef __cplusplus
extern "C" {
#endif

struct uk_pagetable;

struct uk_vma;
struct uk_vma_ops;

/** Virtual address space (VAS) */
struct uk_vas {
	/** Allocator to use for VMAs */
	struct uk_alloc *a;

#ifdef CONFIG_HAVE_PAGING
	/** Page table that represents the virtual address space */
	struct uk_pagetable *pt;

	/** Base address where to start putting new VMAs */
	__vaddr_t vma_base;
#endif /* CONFIG_HAVE_PAGING */

	/** List of VMAs, sorted by address */
	struct uk_list_head vma_list;

	/** VAS flags */
#define UK_VAS_FLAG_NO_PAGING		0x1 /* On-demand paging disabled */
	unsigned long flags;
};

/**
 * Returns the current paging flag and temporarily disables demand-paging for
 * the address space.
 *
 * @param vas
 *   The address space to disable paging for
 *
 * @return
 *   The current paging flag
 */
static inline unsigned long uk_vas_paging_savef(struct uk_vas *vas)
{
	unsigned long saved_flags = vas->flags;

	vas->flags |= UK_VAS_FLAG_NO_PAGING;
	return saved_flags & UK_VAS_FLAG_NO_PAGING;
}

/**
 * Restores the paging flag
 *
 * @param vas
 *   The address space to restore the paging flag for
 * @param flag
 *   The paging flag to restore
 */
static inline void uk_vas_paging_restoref(struct uk_vas *vas,
					  unsigned long flag)
{
	vas->flags = (vas->flags & ~UK_VAS_FLAG_NO_PAGING) | flag;
}

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

	/** Optional name of the VMA */
	const char *name;
};

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

#ifdef CONFIG_HAVE_PAGING
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
	struct __regs *regs;
#endif /* CONFIG_HAVE_PAGING */
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
	 * The handler may use a temporary mapping (see ukplat_page_kmap()) to
	 * initialize the physical memory.
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

/**
 * Initializes a new virtual address space using the provided page table.
 *
 * @param vas
 *   Pointer to an uninitialized virtual address space object
 * @param pt
 *   The page table to use for the address space
 * @param a
 *   Pointer to an allocator that should be used to allocate VMA metadata
 *
 * @return
 *   0 on success, a negative errno error otherwise
 */
int uk_vas_init(struct uk_vas *vas, struct uk_pagetable *pt,
		struct uk_alloc *a);

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
 *   aligned to the specified page size. Can be __VADDR_ANY to automatically
 *   select a virtual address. The variable will receive the address on
 *   success.
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

#ifdef __cplusplus
}
#endif

#include <uk/vma_types.h>

#endif /* __UK_VMEM_H__ */
