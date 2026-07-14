/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, University POLITEHNICA of Bucharest. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/print.h>
#include <uk/assert.h>
#include <uk/acpi.h>
#include <uk/acpi/prio.h>
#include <string.h>
#include <errno.h>
#if CONFIG_LIBUKEFI
#include <uk/efi.h>
#endif /* CONFIG_LIBUKEFI */
#include <uk/plat/common/bootinfo.h>

#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKBOOT */

#define RSDP10_LEN		20
#define BIOS_ROM_START		0xE0000UL
#define BIOS_ROM_END		0xFFFFFUL
#define BIOS_ROM_STEP		16

static struct uk_acpi_madt *acpi_madt;
static struct uk_acpi_fadt *acpi_fadt;
static __u8 acpi_rsdt_entries;
static void *acpi_rsdt;
static __u8 acpi10;

static struct {
	struct uk_acpi_sdt_hdr **sdt;
	const char *sig;
} acpi_sdts[] = {
	{
		.sdt = (struct uk_acpi_sdt_hdr **)&acpi_fadt,
		.sig = UK_ACPI_FADT_SIG,
	},
	{
		.sdt = (struct uk_acpi_sdt_hdr **)&acpi_madt,
		.sig = UK_ACPI_MADT_SIG,
	},
};

static inline __paddr_t get_rsdt_entry(int idx)
{
	__u8 *entryp = (__u8 *)acpi_rsdt + sizeof(struct uk_acpi_sdt_hdr);

	if (acpi10)
		return ((__u32 *)entryp)[idx];

	return ((__u64 *)entryp)[idx];
}

static __u8 get_acpi_checksum(void __maybe_unused *buf, __sz __maybe_unused len)
{
#if CONFIG_LIBUKACPI_CHECKSUM
	const __u8 *const ptr_end = (__u8 *)buf + len;
	const __u8 *ptr = (__u8 *)buf;
	__u8 checksum = 0;

	while (ptr < ptr_end)
		checksum += *ptr++;

	return checksum;
#else /* !CONFIG_LIBUKACPI_CHECKSUM */
	return 0;
#endif /* CONFIG_LIBUKACPI_CHECKSUM */
}

static void acpi_init_tables(void)
{
	struct uk_acpi_sdt_hdr *h;
	const char *sig;
	__sz i, j;

	UK_ASSERT(acpi_rsdt);

	for (i = 0; i < acpi_rsdt_entries; i++)
		for (j = 0; j < ARRAY_SIZE(acpi_sdts); j++) {
			if (*acpi_sdts[j].sdt)
				continue;

			h = (struct uk_acpi_sdt_hdr *)get_rsdt_entry(i);
			sig = acpi_sdts[j].sig;

			if (!memcmp(h->sig, sig, UK_ACPI_SDT_SIG_LEN)) {
				if (unlikely(get_acpi_checksum(h,
							       h->tab_len))) {
					uk_pr_warn("ACPI %s corrupted\n", sig);

					continue;
				}

				*acpi_sdts[j].sdt = h;

				continue;
			}
		}
}

/*
 * Print the detected ACPI tables to the debug output.
 */
#if UK_DEBUG
static void acpi_list_tables(void)
{
	int i;

	UK_ASSERT(acpi_rsdt);

	uk_pr_debug("%d ACPI tables found from %.4s\n", acpi_rsdt_entries,
		    acpi10 ? UK_ACPI_RSDT_SIG : UK_ACPI_XSDT_SIG);
	for (i = 0; i < ARRAY_SIZE(acpi_sdts); i++) {
		if (!acpi_sdts[i].sdt)
			continue;

		uk_pr_debug("%p: %.4s\n", acpi_sdts[i].sdt, acpi_sdts[i].sig);
	}
}
#endif /* UK_DEBUG */

#if CONFIG_LIBUKEFI
static struct uk_acpi_rsdp *acpi_get_efi_st_rsdp(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	uk_efi_uintn_t ct_count, i;
	struct uk_efi_cfg_tbl *ct;
	struct uk_acpi_rsdp *rsdp;

	UK_ASSERT(bi);

	if (!bi->efi_st)
		return __NULL;

	ct = ((struct uk_efi_sys_tbl *)bi->efi_st)->configuration_table;
	ct_count = ((struct uk_efi_sys_tbl *)bi->efi_st)->number_of_table_entries;

	UK_ASSERT(ct);
	UK_ASSERT(ct_count);

	rsdp = __NULL;
	for (i = 0; i < ct_count; i++)
		if (!memcmp(&ct[i].vendor_guid, UK_EFI_ACPI20_TABLE_GUID,
			    sizeof(ct[i].vendor_guid))) {
			rsdp = ct[i].vendor_table;

			break;
		} else if (!memcmp(&ct[i].vendor_guid, UK_EFI_ACPI10_TABLE_GUID,
				 sizeof(ct[i].vendor_guid))) {
			rsdp = ct[i].vendor_table;
		}

	uk_pr_debug("ACPI RSDP present at %p\n", rsdp);

	return rsdp;
}
#endif /* CONFIG_LIBUKEFI */

#if CONFIG_ARCH_X86_64
static struct uk_acpi_rsdp *acpi_get_bios_rom_rsdp(void)
{
	__paddr_t ptr;

	for (ptr = BIOS_ROM_START; ptr < BIOS_ROM_END; ptr += BIOS_ROM_STEP)
		if (!memcmp((void *)ptr,
			    UK_ACPI_RSDP_SIG, sizeof(UK_ACPI_RSDP_SIG) - 1)) {
			uk_pr_debug("ACPI RSDP present at %lx\n", ptr);

			return (struct uk_acpi_rsdp *)ptr;
		}

	return __NULL;
}
#endif /* CONFIG_ARCH_X86_64 */

static struct uk_acpi_rsdp *acpi_get_rsdp(void)
{
	struct uk_acpi_rsdp *rsdp __maybe_unused;

#if CONFIG_LIBUKEFI
	rsdp = acpi_get_efi_st_rsdp();
	if (rsdp)
		return rsdp;
#endif /* CONFIG_LIBUKEFI */

#if CONFIG_ARCH_X86_64
	return acpi_get_bios_rom_rsdp();
#else /* !CONFIG_ARCH_X86_64 */
	return __NULL;
#endif /* !CONFIG_ARCH_X86_64 */
}

/*
 * Detect ACPI version and discover ACPI tables.
 */
int uk_acpi_init(void)
{
	struct uk_acpi_rsdp *rsdp;
	struct uk_acpi_sdt_hdr *h;

	rsdp = acpi_get_rsdp();
	if (unlikely(!rsdp))
		return -ENOENT;

	if (unlikely(get_acpi_checksum(rsdp, RSDP10_LEN))) {
		uk_pr_err("ACPI 1.0 RSDP corrupted\n");

		return -ENOENT;
	}

	if (rsdp->revision == 0) {
		h = (struct uk_acpi_sdt_hdr *)((__uptr)rsdp->rsdt_paddr);
		acpi_rsdt_entries = (h->tab_len - sizeof(*h)) / 4;
		acpi10 = 1;
	} else {
		if (unlikely(get_acpi_checksum(rsdp, sizeof(*rsdp)))) {
			uk_pr_err("ACPI 1.0 RSDP corrupted\n");

			return -ENOENT;
		}

		h = (struct uk_acpi_sdt_hdr *)rsdp->xsdt_paddr;
		acpi_rsdt_entries = (h->tab_len - sizeof(*h)) / 8;
	}

	UK_ASSERT(h);

	if (unlikely(get_acpi_checksum(h, h->tab_len))) {
		uk_pr_err("ACPI RSDT corrupted\n");

		return -ENOENT;
	}

	acpi_rsdt = h;

	acpi_init_tables();

#if UK_DEBUG
	acpi_list_tables();
#endif /* UK_DEBUG */

	return 0;
}

#if CONFIG_LIBUKBOOT
static int boot_acpi_init(struct ukplat_bootinfo *bi __unused)
{
	return uk_acpi_init();
}

UK_BOOT_EARLYTAB_ENTRY(boot_acpi_init, UK_ACPI_INIT_PRIO);
#endif /* CONFIG_LIBUKBOOT */

/*
 * Return the Multiple APIC Descriptor Table (MADT).
 */
struct uk_acpi_madt *uk_acpi_get_madt(void)
{
	return acpi_madt;
}

/*
 * Return the Fixed ACPI Description Table (FADT).
 */
struct uk_acpi_fadt *uk_acpi_get_fadt(void)
{
	return acpi_fadt;
}
