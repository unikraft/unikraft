# ukvmem

This library provides support for virtual address space (VAS) management. A
virtual address space is a list of virtual memory areas (VMA) describing
mappings and address reservations in the address space. The functions in
this library provide the means to create, modify, and remove VMAs.

Newly created VMAs are placed in the address space at a sufficiently large hole
starting at the VMA base if not specified otherwise. Adjacent VMAs which have
compatible properties (e.g., same protections) are merged. Existing VMAs may be
split into separate VMAs if properties are changed, for example, by changing
protections of an address range.

VMAs can define custom handlers (e.g., for page fault handling). Thus allowing
to implement different VMA types. The library comes with implementations for:
- anonymous memory
- thread stacks
- direct-mapped physical memory
- file mappings

Depending on memory type demand paging or pre-allocation are supported.

## Example 1
Create an anonymous writable memory mapping with demand paging. Physical
memory is dynamically allocated on access.
```C
__vaddr_t vaddr = __VADDR_ANY;
uk_vma_map_anon(uk_vas_get_active(), &vaddr, <SIZE>, PAGE_ATTR_PROT_RW,
                0, NULL);
```
## Example 2
Create a writable memory mapping of an existing physical memory range
(e.g., to map device memory). UK_VMA_MAP_POPULATE is optional and just
ensures that the mappings in the page table are already established so they
are present in interrupt context.
```C
__vaddr_t vaddr = __VADDR_ANY;
uk_vma_map_dma(uk_vas_get_active(), &vaddr, <SIZE>, PAGE_ATTR_PROT_RW,
               UK_VMA_MAP_POPULATE, NULL, <PADDR>);
```

## Example 3
Create a writable memory mapping of a newly allocated physical memory
area that uses 2MB pages. The size must be a multiple of 2MB.
```C
__vaddr_t vaddr = __VADDR_ANY;
uk_vma_map_anon(uk_vas_get_active(), &vaddr, <SIZE>, PAGE_ATTR_PROT_RW,
                UK_VMA_MAP_SIZE_2MB, NULL);
```

## Example 4
Create a fixed VMA for a mapping already existing in the pagetable (e.g.,
for an initrd). Note that the attributes specified must match the configuration
in the page table. Otherwise, behavior is undefined.
```C
uk_vma_reserve_ex(uk_vas_get_active(), <VADDR>, <SIZE>,
                  PAGE_ATTR_PROT_READ, 0, ".initrd");
```

## Example 5
Reserve a 1GB address range, create an initial 1MB mapping, extend the
mapping by 2MB, and shrink the mapping by 1MB again. The reserved range
acts as a placeholder which can be only be used for other mappings by
explicitly providing a virtual address of the range and specifying the
UK_VMA_MAP_REPLACE flag. The flag is also used to replace part of the
heap mapping with a reservation again. The figures to the right depict
the VMAs after each operation.
```C
__vaddr_t vaddr = HEAP_BASE;                          ┌────────┐0MB
uk_vma_reserve(uk_vas_get_active(),                   │        │
               &vaddr, 0x40000000);                   │  RSVD  │
                                                      │        │
                                                      │        │
                                                      └ ─ ─ ─ ─┘
// Initial 1MB mapping                                ┌────────┐0MB
vaddr = HEAP_BASE;                                    │  HEAP  │
uk_vma_map_anon(uk_vas_get_active(),                  ├────────┤1MB
           &vaddr, 0x100000, PAGE_ATTR_PROT_RW,       │  RSVD  │
           UK_VMA_MAP_REPLACE, "HEAP");               │        │
                                                      └ ─ ─ ─ ─┘
// Grow mapping by 2MB                                ┌────────┐0MB
vaddr += 0x100000;                                    │/  /  / │
uk_vma_map_anon(uk_vas_get_active(),                  │  HEAP  │
           &vaddr, 0x200000, PAGE_ATTR_PROT_RW,       │ /  /  /│
           UK_VMA_MAP_REPLACE, "HEAP");               ├────────┤3MB
                                                      └ ─ ─ ─ ─┘
// Shrink mapping by 1MB                              ┌────────┐0MB
vaddr += 0x100000;                                    │/  /  / │
uk_vma_reserve_ex(uk_vas_get_active(),                │  HEAP  │
             &vaddr, 0x100000, 0,                     ├────────┤2MB
             UK_VMA_MAP_REPLACE, NULL);               │  RSVD  │
                                                      └ ─ ─ ─ ─┘
```

## Example 6
It is also possible to just create a large virtual memory area that uses
demand paging and only release the physical memory when not needed anymore.
Thus keeping the VMA intact and accessible.
```C
vaddr = HEAP_BASE;
uk_vma_map_anon(uk_vas_get_active(), &vaddr, 0x300000, PAGE_ATTR_PROT_RW, 0,
                "HEAP");

/* ... heap manager decides to release some physical memory in an unused area */
vaddr = HEAP_BASE + 0x200000;
uk_vma_advise(uk_vas_get_active(), vaddr, 0x100000, UK_VMA_ADV_DONTNEED, 0);
```

## Example 7
Create a linear ring buffer that mirrors the buffer at the end to avoid
copying (see https://en.wikipedia.org/wiki/Circular_buffer#Optimization).
Note: Precede an address reservation for the whole 2 * PAGE_SIZE * <PAGES>
range to ensure race free allocation of the virtual addresses.
Note: Physical memory mapped with uk_vma_map_dma() will not be freed
automatically when the VMA is unmapped.
```C
struct uk_vas *vas = uk_vas_get_active();
__paddr_t paddr = uk_falloc(vas->pt->fa, <PAGES>);
__vaddr_t vaddr = __VADDR_ANY;
uk_vma_map_dma(vas, &vaddr, PAGE_SIZE * <PAGES>, PAGE_ATTR_PROT_RW,
               UK_VMA_MAP_POPULATE, NULL, paddr);
vaddr += PAGE_SIZE * <PAGES>;
uk_vma_map_dma(vas, &vaddr, PAGE_SIZE * <PAGES>, PAGE_ATTR_PROT_RW,
               UK_VMA_MAP_POPULATE, NULL, paddr);
```
