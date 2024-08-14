/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/arch/paging.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/lcpu.h>
#include <uk/plat/common/memory.h>
#include <uk/plat/common/sections.h>
#include <uk/reloc.h>
#include <kvm-x86/multiboot2.h>

#include <errno.h>
#include <string.h>

#define multiboot2_crash(msg, rc)	ukplat_crash()

#define MULTIBOOT_TAG_ALIGN_UP(x)	ALIGN_UP((x), MULTIBOOT_TAG_ALIGN)

void _ukplat_entry(struct lcpu *lcpu, struct ukplat_bootinfo *bi);

static inline void mrd_insert(struct ukplat_bootinfo *bi,
			      const struct ukplat_memregion_desc *mrd)
{
	int rc;

	if (unlikely(mrd->len == 0))
		return;

	rc = ukplat_memregion_list_insert(&bi->mrds, mrd);
	if (unlikely(rc < 0))
		multiboot2_crash("Cannot insert bootinfo memory region", rc);
}

void handle_mmap_tag(struct ukplat_bootinfo *bi, struct multiboot_tag *tag)
{
	struct ukplat_memregion_desc mrd = {0};
	__sz i, mmap_total_sz, mmap_entry_cnt;
	struct multiboot_tag_mmap *mmap_tag;
	struct multiboot_mmap_entry *mmap;
	multiboot_memory_map_t *m;
	__paddr_t start, end;

	mmap_tag = (struct multiboot_tag_mmap *)tag;
	mmap = (struct multiboot_mmap_entry *)mmap_tag->entries;

	mmap_total_sz = mmap_tag->size - sizeof(*mmap_tag);
	mmap_entry_cnt = mmap_total_sz / mmap_tag->entry_size;

	for (i = 0; i < mmap_entry_cnt; i++) {
		m = mmap + i;

		start = MAX(m->addr, __PAGE_SIZE);
		end   = m->addr + m->len;
		if (unlikely(end <= start ||
			     end - start < PAGE_SIZE))
			continue;

		mrd.pbase = PAGE_ALIGN_DOWN(start);
		mrd.vbase = mrd.pbase; /* 1:1 mapping */
		mrd.pg_off = start - mrd.pbase;
		mrd.len = end - start;
		mrd.pg_count = PAGE_COUNT(mrd.pg_off + mrd.len);

		if (m->type == MULTIBOOT_MEMORY_AVAILABLE) {
			mrd.type  = UKPLAT_MEMRT_FREE;
			mrd.flags = UKPLAT_MEMRF_READ |
				    UKPLAT_MEMRF_WRITE;

			/* Free memory regions have
			 * mrd.len == mrd.pg_count * PAGE_SIZE
			 */
			mrd.len = PAGE_ALIGN_UP(mrd.len +
						mrd.pg_off);
		} else {
			mrd.type  = UKPLAT_MEMRT_RESERVED;
			mrd.flags = UKPLAT_MEMRF_READ;

			/* We assume that reserved regions can't
			 * overlap with loaded modules.
			 */
		}

		mrd_insert(bi, &mrd);
	}
}

/**
 * Multiboot entry point called after lcpu initialization. We enter with the
 * 1:1 boot page table set. Physical and virtual addresses thus match for all
 * regions in the mapped range.
 */
void multiboot2_entry(struct lcpu *lcpu, unsigned long addr)
{
	struct ukplat_memregion_desc mrd = {0};
	struct multiboot_tag_string *tag_str;
	struct multiboot_tag_module *mod_tag;
	struct ukplat_bootinfo *bi;
	struct multiboot_tag *tag;
	int rc;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		multiboot2_crash("Incompatible or corrupted bootinfo", -EINVAL);

	/* We have to call this here as the very early do_uk_reloc32 relocator
	 * does not also relocate the UKPLAT_MEMRT_KERNEL mrd's like its C
	 * equivalent, do_uk_reloc, does.
	 */
	do_uk_reloc_kmrds(0, 0);

	/* Ensure that the memory map contains the legacy high mem area */
	rc = ukplat_memregion_list_insert_legacy_hi_mem(&bi->mrds);
	if (unlikely(rc))
		multiboot2_crash("Could not insert legacy memory region", rc);

	/**
	 * Initialize the first tag address, skipping total_size(4) and
	 * reserved (4) fields of the boot information structure
	 */
	tag = (struct multiboot_tag *)(addr + 8);

	/* Parse Multiboot2 tags */
	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		switch (tag->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			tag_str = (struct multiboot_tag_string *)tag;
			bi->cmdline = (__u64)tag_str->string;
			bi->cmdline_len = tag_str->size;
			break;
		case MULTIBOOT_TAG_TYPE_MODULE:
			mod_tag = (struct multiboot_tag_module *)tag;
			mrd.pbase = PAGE_ALIGN_DOWN(mod_tag->mod_start);
			mrd.vbase = mrd.pbase;
			mrd.pg_off = mod_tag->mod_start - mrd.pbase;
			mrd.len = mod_tag->mod_end - mod_tag->mod_start;
			mrd.pg_count = PAGE_COUNT(mrd.pg_off + mrd.len);
			mrd.type = UKPLAT_MEMRT_INITRD;
			mrd.flags = UKPLAT_MEMRF_READ;

#if CONFIG_UKPLAT_MEMRNAME
			strncpy(mrd.name,
				(char *)(__uptr)mod_tag->cmdline,
				sizeof(mrd.name) - 1);
#endif /* CONFIG_UKPLAT_MEMRNAME */

			mrd_insert(bi, &mrd);
			break;
		case MULTIBOOT_TAG_TYPE_MMAP:
			handle_mmap_tag(bi, tag);
			break;
		}

		tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
		       MULTIBOOT_TAG_ALIGN_UP(tag->size));
	}

	memcpy(bi->bootprotocol, "multiboot2", sizeof("multiboot2"));

	_ukplat_entry(lcpu, bi);
}
