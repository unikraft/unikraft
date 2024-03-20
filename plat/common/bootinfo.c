/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/plat/common/bootinfo.h>

/**
 * Space reservation for boot information. Will be populated by build script.
 */
__section(".uk_bootinfo") __align(8) __used static char
bi_bootinfo_sec[UKPLAT_BOOTINFO_SIZE(CONFIG_UKPLAT_MEMREGION_MAX_COUNT)];

/**
 * Pointer to boot information. Can be changed at runtime if the boot
 * information should be overridden by using ukplat_bootinfo_set()
 */
static struct ukplat_bootinfo *bi_bootinfo;

struct ukplat_bootinfo *ukplat_bootinfo_get(void)
{
	struct ukplat_bootinfo *bi = (struct ukplat_bootinfo *)bi_bootinfo_sec;

	if (unlikely(!bi_bootinfo)) {
		if (unlikely(bi->magic != UKPLAT_BOOTINFO_MAGIC ||
			     bi->version != UKPLAT_BOOTINFO_VERSION ||
			     bi->mrds.count == 0))
			return NULL;

		bi_bootinfo = bi;
	}

	return bi_bootinfo;
}

void ukplat_bootinfo_set(struct ukplat_bootinfo *bi)
{
	UK_ASSERT(bi->magic == UKPLAT_BOOTINFO_MAGIC);
	UK_ASSERT(bi->version == UKPLAT_BOOTINFO_VERSION);
	UK_ASSERT(bi->mrds.count > 0);

	bi_bootinfo = bi;
}

void ukplat_bootinfo_print(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	const struct ukplat_memregion_desc *mrd;
	const char *type;
	__u32 i;

	UK_ASSERT(bi);

	uk_pr_info("Unikraft " STRINGIFY(UK_CODENAME)
		   " (" STRINGIFY(UK_FULLVERSION) ")\n");

	uk_pr_info("Architecture: " CONFIG_UK_ARCH "\n");

	if (*bi->bootloader || *bi->bootprotocol)
		uk_pr_info("Boot loader : %s-%s\n",
			   (*bi->bootloader) ? bi->bootloader : "unknown",
			   (*bi->bootprotocol) ? bi->bootprotocol : "unknown");

	if (bi->cmdline)
		uk_pr_info("Command line: %s\n", (const char *)bi->cmdline);

	uk_pr_info("Boot memory map:\n");
	for (i = 0; i < bi->mrds.count; i++) {
		mrd = &bi->mrds.mrds[i];

		/* Don't print untyped regions */
		if (!mrd->type)
			continue;

		switch (mrd->type) {
		case UKPLAT_MEMRT_RESERVED:
			type = "rsvd";
			break;
		case UKPLAT_MEMRT_KERNEL:
			type = "krnl";
			break;
		case UKPLAT_MEMRT_INITRD:
			type = "ramd";
			break;
		case UKPLAT_MEMRT_CMDLINE:
			type = "cmdl";
			break;
		case UKPLAT_MEMRT_DEVICETREE:
			type = "dtb ";
			break;
		case UKPLAT_MEMRT_STACK:
			type = "stck";
			break;
		default:
			type = "";
			break;
		}

		uk_pr_info(" %012lx-%012lx %012lx-%012lx %c%c%c %016lx %s %s\n",
			   mrd->pbase, mrd->pbase + mrd->pg_count * PAGE_SIZE,
			   mrd->pbase + mrd->pg_off,
			   mrd->pbase + mrd->pg_off + mrd->len,
			   (mrd->flags & UKPLAT_MEMRF_READ) ? 'r' : '-',
			   (mrd->flags & UKPLAT_MEMRF_WRITE) ? 'w' : '-',
			   (mrd->flags & UKPLAT_MEMRF_EXECUTE) ? 'x' : '-',
			   mrd->vbase,
			   type,
#ifdef CONFIG_UKPLAT_MEMRNAME
			   mrd->name
#else /* CONFIG_UKPLAT_MEMRNAME */
			   ""
#endif /* !CONFIG_UKPLAT_MEMRNAME */
			   );
	}
}
