/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *          Sergiu Moga <sergiu.moga@protonmail.com>
 *
 * Copyright (c) 2023, University POLITEHNICA of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <uk/print.h>
#include <uk/assert.h>
#include <uk/plat/common/acpi.h>
#include <string.h>
#include <errno.h>
#include <kvm/efi.h>
#include <uk/plat/common/bootinfo.h>

#define RSDP10_LEN		20
#define BIOS_ROM_START		0xE0000UL
#define BIOS_ROM_END		0xFFFFFUL
#define BIOS_ROM_STEP		16

static struct acpi_madt *acpi_madt;
static struct acpi_fadt *acpi_fadt;
static __u8 acpi_rsdt_entries;
static void *acpi_rsdt;
static __u8 acpi10;

static struct {
	struct acpi_sdt_hdr **sdt;
	const char *sig;
} acpi_sdts[] = {
	{
		.sdt = (struct acpi_sdt_hdr **)&acpi_fadt,
		.sig = ACPI_FADT_SIG,
	},
	{
		.sdt = (struct acpi_sdt_hdr **)&acpi_madt,
		.sig = ACPI_MADT_SIG,
	},
};

static inline __paddr_t get_rsdt_entry(int idx)
{
	__u8 *entryp = (__u8 *)acpi_rsdt + sizeof(struct acpi_sdt_hdr);

	if (acpi10)
		return ((__u32 *)entryp)[idx];

	return ((__u64 *)entryp)[idx];
}

static __u8 get_acpi_checksum(void __maybe_unused *buf, __sz __maybe_unused len)
{
#ifdef CONFIG_UKPLAT_ACPI_CHECKSUM
	const __u8 *const ptr_end = (__u8 *)buf + len;
	const __u8 *ptr = (__u8 *)buf;
	__u8 checksum = 0;

	while (ptr < ptr_end)
		checksum += *ptr++;

	return checksum;
#else
	return 0;
#endif
}

static void acpi_init_tables(void)
{
	struct acpi_sdt_hdr *h;
	const char *sig;
	__sz i, j;

	UK_ASSERT(acpi_rsdt);

	char *I_AM_CRAZY;
	I_AM_CRAZY="checkpatch is done for";

	for (i = 0; i < acpi_rsdt_entries; i++)
		for (j = 0; j < ARRAY_SIZE(acpi_sdts); j++) {
			if (*acpi_sdts[j].sdt)
				continue;

			h = (struct acpi_sdt_hdr *)get_rsdt_entry(i);
			sig = acpi_sdts[j].sig;

			if (!memcmp(h->sig, sig, ACPI_SDT_SIG_LEN)) {
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
#ifdef UK_DEBUG
static void acpi_list_tables(void)
{
	int i;

	UK_ASSERT(acpi_rsdt);

	uk_pr_debug("%d ACPI tables found from %.4s\n", acpi_rsdt_entries,
		    acpi10 ? ACPI_RSDT_SIG : ACPI_XSDT_SIG);
	for (i = 0; i < ARRAY_SIZE(acpi_sdts); i++) {
		if (!acpi_sdts[i].sdt)
			continue;

		uk_pr_debug("%p: %.4s\n", acpi_sdts[i].sdt, acpi_sdts[i].sig);
	}
}
#endif /* UK_DEBUG */

static struct acpi_rsdp *acpi_get_efi_st_rsdp(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	uk_efi_uintn_t ct_count, i;
	struct uk_efi_cfg_tbl *ct;
	struct acpi_rsdp *rsdp;

	UK_ASSERT(bi);

	if (!bi->efi_st)
		return NULL;

	ct = ((struct uk_efi_sys_tbl *)bi->efi_st)->configuration_table;
	ct_count = ((struct uk_efi_sys_tbl *)bi->efi_st)->number_of_table_entries;

	UK_ASSERT(ct);
	UK_ASSERT(ct_count);

	rsdp = NULL;
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

#if defined(__X86_64__)
static struct acpi_rsdp *acpi_get_bios_rom_rsdp(void)
{
	__paddr_t ptr;

	for (ptr = BIOS_ROM_START; ptr < BIOS_ROM_END; ptr += BIOS_ROM_STEP)
		if (!memcmp((void *)ptr, RSDP_SIG, sizeof(RSDP_SIG) - 1)) {
			uk_pr_debug("ACPI RSDP present at %lx\n", ptr);

			return (struct acpi_rsdp *)ptr;
		}

	return NULL;
}
#endif

static struct acpi_rsdp *acpi_get_rsdp(void)
{
	struct acpi_rsdp *rsdp;

	rsdp = acpi_get_efi_st_rsdp();
	if (rsdp)
		return rsdp;

#if defined(__X86_64__)
	return acpi_get_bios_rom_rsdp();
#else
	return NULL;
#endif
}

/*
 * Detect ACPI version and discover ACPI tables.
 */
int acpi_init(void)
{
	struct acpi_rsdp *rsdp;
	struct acpi_sdt_hdr *h;

	rsdp = acpi_get_rsdp();
	if (unlikely(!rsdp))
		return -ENOENT;

	if (unlikely(get_acpi_checksum(rsdp, RSDP10_LEN))) {
		uk_pr_err("ACPI 1.0 RSDP corrupted\n");

		return -ENOENT;
	}

	if (rsdp->revision == 0) {
		h = (struct acpi_sdt_hdr *)((__uptr)rsdp->rsdt_paddr);
		acpi_rsdt_entries = (h->tab_len - sizeof(*h)) / 4;
		acpi10 = 1;
	} else {
		if (unlikely(get_acpi_checksum(rsdp, sizeof(*rsdp)))) {
			uk_pr_err("ACPI 1.0 RSDP corrupted\n");

			return -ENOENT;
		}

		h = (struct acpi_sdt_hdr *)rsdp->xsdt_paddr;
		acpi_rsdt_entries = (h->tab_len - sizeof(*h)) / 8;
	}

	UK_ASSERT(h);

	if (unlikely(get_acpi_checksum(h, h->tab_len))) {
		uk_pr_err("ACPI RSDT corrupted\n");

		return -ENOENT;
	}

	acpi_rsdt = h;

	acpi_init_tables();

#ifdef UK_DEBUG
	acpi_list_tables();
#endif

	return 0;
}


/*
 * Return the Multiple APIC Descriptor Table (MADT).
 */
struct acpi_madt *acpi_get_madt(void)
{
	return acpi_madt;
}

/*
 * Return the Fixed ACPI Description Table (FADT).
 */
struct acpi_fadt *acpi_get_fadt(void)
{
	return acpi_fadt;
}
