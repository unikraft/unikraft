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

#ifndef __PLAT_CMN_ACPI_H__
#define __PLAT_CMN_ACPI_H__

#include "sdt.h"
#include "madt.h"

#define RSDP_SIG		"RSD PTR "
#define RSDP_SIG_LEN		8

struct acpi_rsdp {
	char sig[RSDP_SIG_LEN];
	__u8 checksum;
	char oem_id[ACPI_OEM_ID_LEN];
	__u8 revision;
	__u32 rsdt_paddr;
	__u32 tab_len;
	__u64 xsdt_paddr;
	__u8 xchecksum;
	__u8 reserved[3];
} __packed;

/**
 * Get the Multiple APIC Descriptor Table (MADT).
 *
 * @return ACPI table pointer on success, NULL otherwise.
 */
struct acpi_madt *acpi_get_madt(void);

/**
 * Get the Fixed ACPI Description Table (FADT).
 *
 * @return ACPI table pointer on success, NULL otherwise.
 */
struct acpi_fadt *acpi_get_fadt(void);

/**
 * Detect ACPI version and fetch ACPI tables.
 *
 * @return 0 on success, -errno otherwise.
 */
int acpi_init(void);

#endif /* __PLAT_CMN_ACPI_H__ */
