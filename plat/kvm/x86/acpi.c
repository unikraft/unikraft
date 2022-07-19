/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2021, University POLITEHNICA of Bucharest. All rights reserved.
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
#include <x86/acpi/acpi.h>
#include <x86/acpi/madt.h>

#include <string.h>
#include <errno.h>

#define RSDT_ENTRIES(rsdt) (((rsdt)->h.Length - sizeof((rsdt)->h)) / 4)

static __u8 acpi_version;
static struct RSDPDescriptor *acpi_rsdp;
static struct RSDT *acpi_rsdt;
static struct MADT *acpi_madt;

/*
 * Compute checksum for ACPI RSDP table.
 */

static inline int verify_rsdp_checksum(struct RSDPDescriptor *rsdp)
{
	__u8 checksum = 0;
	__u8 *ptr = (__u8 *)rsdp;

	while (ptr < (__u8 *)(rsdp + 1))
		checksum += *ptr++;

	return checksum == 0 ? 0 : -1;
}

/*
 * Compute checksum for any ACPI table, except RSDP.
 */

static inline int verify_acpi_checksum(struct ACPISDTHeader *h)
{
	__u8 checksum = 0;
	__u32 i;

	for (i = 0; i < h->Length; i++)
		checksum += ((__u8 *)h)[i];

	return checksum == 0 ? 0 : -1;
}

/**
 * Find the Root System Descriptor Pointer (RSDP) in the physical memory area
 * 0xe0000 -> 0xfffff and determine ACPI version.
 *
 * @return 0 on success, -ENOENT if the table is not found, or invalid,
 * -ENOTSUP if the ACPI version is not supported.
 */

static int detect_acpi_version(void)
{
	__u8 *start_addr = (__u8 *)0xe0000; /* BIOS read-only memory space */
	__u8 *end_addr = (__u8 *)0xfffff;
	__u8 *ptr;

	UK_ASSERT(!acpi_rsdp);
	UK_ASSERT(!acpi_version);

	for (ptr = start_addr; ptr < end_addr; ptr += 16) {
		if (!memcmp(ptr, "RSD PTR ", 8)) {
			acpi_rsdp = (struct RSDPDescriptor *)ptr;
			uk_pr_debug("ACPI RSDP present at %p\n", ptr);
			break;
		}
	}

	if (!acpi_rsdp) {
		uk_pr_debug("ACPI RSDP not found\n");
		return -ENOENT;
	}

	if (verify_rsdp_checksum(acpi_rsdp) != 0) {
		uk_pr_err("ACPI RSDP corrupted\n");

		acpi_rsdp = NULL;
		return -ENOENT;
	}

	uk_pr_info("ACPI version detected: ");
	if (acpi_rsdp->Revision == 0) {
		uk_pr_info("1.0\n");

		acpi_version = 1;
	} else {
		uk_pr_info(">= 2\n");

		/*
		 * TODO: add support for ACPI version 2 and greater
		 */
		uk_pr_err("ACPI version not supported\n");

		return -ENOTSUP;
	}

	return 0;
}

/*
 * Find the ACPI Root System Descriptor Table (RSDT) and check if it's valid.
 */

static int acpi10_find_rsdt(void)
{
	UK_ASSERT(acpi_version == 1);
	UK_ASSERT(acpi_rsdp);

	acpi_rsdt = (struct RSDT *)((__uptr)acpi_rsdp->RsdtAddress);
	uk_pr_debug("ACPI RSDT present at %p\n", acpi_rsdt);

	if (unlikely(verify_acpi_checksum(&acpi_rsdt->h) != 0)) {
		uk_pr_err("ACPI RSDT corrupted\n");

		acpi_rsdt = NULL;
		return -ENOENT;
	}

	return 0;
}

/*
 * Find the Multiple APIC Descriptor Table (MADT) in the RSDT and check if
 * it's valid. MADT can be found by searching for the string "APIC" in the
 * first 4 bytes of each table entry.
 */

static int acpi10_find_madt(void)
{
	int entries, i;
	struct ACPISDTHeader *h;

	UK_ASSERT(acpi_version == 1);
	UK_ASSERT(acpi_rsdt);
	UK_ASSERT(!acpi_madt);

	entries = RSDT_ENTRIES(acpi_rsdt);

	for (i = 0; i < entries; i++) {
		h = (struct ACPISDTHeader *)((__uptr)acpi_rsdt->Entry[i]);

		if (memcmp(h->Signature, "APIC", 4) != 0)
			continue; /* Not an APIC entry */

		uk_pr_debug("ACPI MADT present at %p\n", h);

		if (verify_acpi_checksum(h) != 0) {
			uk_pr_err("ACPI MADT corrupted\n");
			return -ENOENT;
		}

		acpi_madt = (struct MADT *)h;
		return 0;
	}

	/* no MADT was found */
	return -ENOENT;
}

/*
 * Print the detected ACPI tables to the debug output.
 */

#ifdef UK_DEBUG
static void acpi10_list_tables(void)
{
	int entries, i;
	struct ACPISDTHeader *h;

	UK_ASSERT(acpi_version == 1);
	UK_ASSERT(acpi_rsdt);

	entries = RSDT_ENTRIES(acpi_rsdt);

	uk_pr_debug("%d ACPI tables found\n", entries);

	for (i = 0; i < entries; i++) {
		h = (struct ACPISDTHeader *)((__uptr)acpi_rsdt->Entry[i]);
		uk_pr_debug("%p: %.4s\n", h, h->Signature);
	}
}
#endif /* UK_DEBUG */

/*
 * Initialize ACPI 1.0 data structures.
 */

static int acpi10_init(void)
{
	int ret;

	UK_ASSERT(acpi_version == 1);

	if ((ret = acpi10_find_rsdt()) != 0)
		return ret;

#ifdef UK_DEBUG
	acpi10_list_tables();
#endif

	if ((ret = acpi10_find_madt()) != 0)
		return ret;

	return 0;
}

/*
 * Detect ACPI version and discover ACPI tables.
 */

int acpi_init(void)
{
	int ret;

	UK_ASSERT(!acpi_version);

	if ((ret = detect_acpi_version()) != 0)
		return ret;

	/* Try to initialize the respective ACPI support. If it fails, we reset
	 * acpi_version to indicate that ACPI support is not provided.
	 */
	if (acpi_version == 1) {
		if ((ret = acpi10_init()) != 0) {
			acpi_version = 0;
			return ret;
		}
	} else {
		UK_ASSERT(!acpi_version);
		return -ENOTSUP;
	}

	return 0;
}

/*
 * Return detected ACPI version.
 */

int acpi_get_version(void)
{
	return acpi_version;
}

/*
 * Return the Multiple APIC Descriptor Table (MADT).
 */

struct MADT *acpi_get_madt(void)
{
	UK_ASSERT(acpi_version);
	UK_ASSERT(acpi_madt);

	return acpi_madt;
}
