/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, University POLITEHNICA of Bucharest. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ACPI_H__
#define __UK_ACPI_H__

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#include <uk/acpi/sdt.h>
#include <uk/acpi/madt.h>

#define UK_ACPI_RSDP_SIG			"RSD PTR "
#define UK_ACPI_RSDP_SIG_LEN			8

struct uk_acpi_rsdp {
	char sig[UK_ACPI_RSDP_SIG_LEN];
	__u8 checksum;
	char oem_id[UK_ACPI_OEM_ID_LEN];
	__u8 revision;
	__u32 rsdt_paddr;
	__u32 tab_len;
	__u64 xsdt_paddr;
	__u8 xchecksum;
	__u8 reserved[3];
} __packed;

/**
 * Get the Fixed ACPI Description Table (FADT).
 *
 * @return ACPI table pointer on success, NULL otherwise.
 */
struct uk_acpi_fadt *uk_acpi_get_fadt(void);

/**
 * Detect ACPI version and fetch ACPI tables.
 *
 * @return 0 on success, -errno otherwise.
 */
int uk_acpi_init(void);

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_ACPI_H__ */
