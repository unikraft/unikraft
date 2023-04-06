/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/sections.h>
#ifdef CONFIG_LIBFDT
#include <libfdt.h>
#endif /* CONFIG_LIBFDT */

#define ukplat_bootinfo_crash(s)		ukplat_crash()

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

		uk_pr_info(" %012lx-%012lx %012lx %c%c%c %016lx %s %s\n",
			   mrd->pbase, mrd->pbase + mrd->len, mrd->len,
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

#ifdef CONFIG_LIBFDT
static void fdt_bootinfo_mem_mrd(struct ukplat_bootinfo *bi, void *fdtp)
{
	struct ukplat_memregion_desc mrd = {0};
	int prop_len, prop_min_len;
	__u64 mem_base, mem_sz;
	int naddr, nsz, nmem;
	const __u64 *regs;
	int rc;

	/* search for assigned VM memory in DTB */
	rc = fdt_check_header(fdtp);
	if (unlikely(rc))
		ukplat_bootinfo_crash("Reserved memory is not supported");

	nmem = fdt_node_offset_by_prop_value(fdtp, -1, "device_type",
					     "memory", sizeof("memory"));
	if (unlikely(nmem < 0))
		ukplat_bootinfo_crash("No memory found in DTB");

	naddr = fdt_address_cells(fdtp, nmem);
	if (unlikely(naddr < 0 || naddr >= FDT_MAX_NCELLS))
		ukplat_bootinfo_crash("Could not find proper address cells!");

	nsz = fdt_size_cells(fdtp, nmem);
	if (unlikely(nsz < 0 || nsz >= FDT_MAX_NCELLS))
		ukplat_bootinfo_crash("Could not find proper size cells!");

	/*
	 * QEMU will always provide us at least one bank of memory.
	 * unikraft will use the first bank for the time-being.
	 * The property must contain at least the start address
	 * and size, each of which is 8-bytes.
	 * TODO: Support more than one memory@ node/regs/ranges properties.
	 */
	prop_len = 0;
	prop_min_len = (int)sizeof(fdt32_t) * (naddr + nsz);
	regs = fdt_getprop(fdtp, nmem, "reg", &prop_len);
	if (unlikely(!regs || prop_len < prop_min_len))
		ukplat_bootinfo_crash("Bad 'reg' property or more than one memory bank.");

	mem_sz = fdt64_to_cpu(regs[1]);
	mem_base = fdt64_to_cpu(regs[0]);
	if (unlikely(!RANGE_CONTAIN(mem_base, mem_sz,
				    __BASE_ADDR, __END - __BASE_ADDR)))
		ukplat_bootinfo_crash("Image outside of RAM");

	mrd.vbase = (__vaddr_t)mem_base;
	mrd.pbase = (__paddr_t)mem_base;
	mrd.len   = __BASE_ADDR - mem_base;
	mrd.type  = UKPLAT_MEMRT_FREE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		ukplat_bootinfo_crash("Could not add free memory descriptor");

	mrd.vbase = (__vaddr_t)__END;
	mrd.pbase = (__paddr_t)__END;
	mrd.len   = mem_base + mem_sz - __END;
	mrd.type  = UKPLAT_MEMRT_FREE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		ukplat_bootinfo_crash("Could not add free memory descriptor");
}

static void fdt_bootinfo_cmdl_mrd(struct ukplat_bootinfo *bi, void *fdtp)
{
	const void *fdt_cmdl;
	int fdt_cmdl_len;
	__sz cmdl_len;
	int nchosen;
	char *cmdl;

	nchosen = fdt_path_offset(fdtp, "/chosen");
	if (unlikely(!nchosen))
		return;

	fdt_cmdl = fdt_getprop(fdtp, nchosen, "bootargs", &fdt_cmdl_len);
	if (unlikely(!fdt_cmdl || fdt_cmdl_len <= 0))
		return;

	cmdl = ukplat_memregion_alloc(fdt_cmdl_len + sizeof(CONFIG_UK_NAME) + 1,
				      UKPLAT_MEMRT_CMDLINE,
				      UKPLAT_MEMRF_READ |
				      UKPLAT_MEMRF_MAP);
	if (unlikely(!cmdl))
		ukplat_bootinfo_crash("Command-line alloc failed\n");

	cmdl_len = sizeof(CONFIG_UK_NAME);
	strncpy(cmdl, CONFIG_UK_NAME, cmdl_len);
	cmdl[cmdl_len - 1] = ' ';
	strncpy(cmdl + cmdl_len, fdt_cmdl, fdt_cmdl_len);
	cmdl_len += fdt_cmdl_len;
	cmdl[cmdl_len] = '\0';

	bi->cmdline = (__u64)cmdl;
	bi->cmdline_len = (__u64)cmdl_len;
}

static void fdt_bootinfo_fdt_mrd(struct ukplat_bootinfo *bi, void *fdtp)
{
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	rc = fdt_check_header(fdtp);
	if (unlikely(rc))
		ukplat_bootinfo_crash("Invalid DTB");

	mrd.vbase = (__vaddr_t)fdtp;
	mrd.pbase = (__paddr_t)fdtp;
	mrd.len   = fdt_totalsize(fdtp);
	mrd.type  = UKPLAT_MEMRT_DEVICETREE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_MAP;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		ukplat_bootinfo_crash("Could not insert DT memory descriptor");
}

void ukplat_bootinfo_fdt_setup(void *fdtp)
{
	struct ukplat_bootinfo *bi;

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		ukplat_bootinfo_crash("Invalid bootinfo");

	fdt_bootinfo_fdt_mrd(bi, fdtp);
	fdt_bootinfo_mem_mrd(bi, fdtp);
	ukplat_memregion_list_coalesce(&bi->mrds);
	fdt_bootinfo_cmdl_mrd(bi, fdtp);

	bi->dtb = (__u64)fdtp;
}
#endif /* CONFIG_LIBFDT */
