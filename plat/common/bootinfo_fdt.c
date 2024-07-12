/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/common/sections.h>
#include <libfdt.h>

#define ukplat_bootinfo_crash(s)		ukplat_crash()

static void fdt_bootinfo_mem_mrd(struct ukplat_bootinfo *bi, void *fdtp)
{
	struct ukplat_memregion_desc mrd = {0};
	int prop_len, prop_min_len;
	__u64 mem_base, mem_sz;
	int naddr, nsz, nmem;
	const __u64 *regs;
	int rc;

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
	 * The property must contain at least the start address
	 * and size, each of which is 8-bytes.
	 * For now, we only support one memory bank.
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

	/* Check that we are not placed at the top of the memory region */
	mrd.len = __BASE_ADDR - mem_base;
	if (!mrd.len)
		goto end_mrd;

	mrd.pbase = (__paddr_t)PAGE_ALIGN_DOWN(mem_base);
	mrd.vbase = (__vaddr_t)PAGE_ALIGN_DOWN(mem_base);
	mrd.pg_off = mem_base - mrd.pbase;
	mrd.type  = UKPLAT_MEMRT_FREE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
	mrd.pg_count = PAGE_COUNT(mrd.pg_off + mrd.len);

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		ukplat_bootinfo_crash("Could not add free memory descriptor");

end_mrd:
	/* Check that we are not placed at the end of the memory region */
	mrd.len   = mem_base + mem_sz - __END;
	if (!mrd.len)
		return;

	mrd.pbase = (__paddr_t)PAGE_ALIGN_DOWN(__END);
	mrd.vbase = (__vaddr_t)PAGE_ALIGN_DOWN(__END);
	mrd.pg_off = __END - mrd.pbase;
	mrd.type  = UKPLAT_MEMRT_FREE;
	mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
	mrd.pg_count = PAGE_COUNT(mrd.pg_off + mrd.len);

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		ukplat_bootinfo_crash("Could not add free memory descriptor");
}

static void fdt_bootinfo_cmdl_init(struct ukplat_bootinfo *bi, void *fdtp)
{
	const void *fdt_cmdl;
	int fdt_cmdl_len;
	int nchosen;

	nchosen = fdt_path_offset((void *)fdtp, "/chosen");
	if (unlikely(nchosen < 0))
		return;

	fdt_cmdl = fdt_getprop((void *)fdtp, nchosen, "bootargs",
			       &fdt_cmdl_len);
	if (unlikely(!fdt_cmdl || fdt_cmdl_len <= 0))
		return;

	bi->cmdline = (__u64)fdt_cmdl;
	bi->cmdline_len = (__u64)fdt_cmdl_len;
}

/* Ideally the initrd nodes would use #address-cells, yet these nodes are not
 * defined by the device-tree spec, and as such there is no formal requirement
 * that they do so. In fact, QEMU virt uses a 32-bit address here, despite
 * defining 2 address cells. To handle such cases, use the property length to
 * determine the correct address.
 */
#define initrd_addr(val, len)                          \
	(len == 4 ? fdt32_to_cpu(val) : fdt64_to_cpu(val))

static void fdt_bootinfo_initrd_mrd(struct ukplat_bootinfo *bi, void *fdtp)
{
	struct ukplat_memregion_desc mrd = {0};
	const __u64 *fdt_initrd_start;
	const __u64 *fdt_initrd_end;
	int start_len, end_len;
	__u64 initrd_base;
	int nchosen;
	int rc;

	nchosen = fdt_path_offset(fdtp, "/chosen");
	if (unlikely(nchosen < 0))
		return;

	fdt_initrd_start = fdt_getprop(fdtp, nchosen, "linux,initrd-start",
				       &start_len);
	if (unlikely(!fdt_initrd_start || start_len <= 0))
		return;

	fdt_initrd_end = fdt_getprop(fdtp, nchosen, "linux,initrd-end",
				     &end_len);
	if (unlikely(!fdt_initrd_end || end_len <= 0))
		return;

	initrd_base = initrd_addr(fdt_initrd_start[0], start_len);
	mrd.pbase = PAGE_ALIGN_DOWN(initrd_base);
	mrd.vbase = mrd.pbase;
	mrd.pg_off = initrd_base - mrd.pbase;
	mrd.len = initrd_addr(fdt_initrd_end[0], end_len) - initrd_base;
	mrd.pg_count = PAGE_COUNT(mrd.pg_off + mrd.len);
	mrd.type = UKPLAT_MEMRT_INITRD;
	mrd.flags = UKPLAT_MEMRF_READ;

	rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
	if (unlikely(rc < 0))
		ukplat_bootinfo_crash("Could not add initrd memory descriptor");
}

static void fdt_bootinfo_fdt_mrd(struct ukplat_bootinfo *bi, void *fdtp)
{
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	mrd.pbase = (__paddr_t)PAGE_ALIGN_DOWN((__uptr)fdtp);
	mrd.vbase = (__vaddr_t)PAGE_ALIGN_DOWN((__uptr)fdtp);
	mrd.pg_off = (__u64)fdtp - mrd.pbase;
	mrd.len = fdt_totalsize(fdtp);
	mrd.pg_count = PAGE_COUNT(mrd.pg_off + mrd.len);
	mrd.type = UKPLAT_MEMRT_DEVICETREE;
	mrd.flags = UKPLAT_MEMRF_READ;

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

	if (unlikely(fdt_check_header(fdtp)))
		ukplat_bootinfo_crash("Invalid DTB");

	fdt_bootinfo_fdt_mrd(bi, fdtp);
	fdt_bootinfo_mem_mrd(bi, fdtp);
	fdt_bootinfo_initrd_mrd(bi, fdtp);

	fdt_bootinfo_cmdl_init(bi, fdtp);

	bi->dtb = (__u64)fdtp;
}
