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

#ifndef __PLAT_CMN_X86_SDT_H__
#define __PLAT_CMN_X86_SDT_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

#define ACPI_OEM_ID_LEN					6
#define ACPI_OEM_TAB_ID_LEN				8
#define ACPI_SDT_SIG_LEN				4
#define ACPI_SDT_CREATOR_ID_LEN				4

struct acpi_sdt_hdr {
	char sig[ACPI_SDT_SIG_LEN];
	__u32 tab_len;
	__u8 revision;
	__u8 checksum;
	char oem_id[ACPI_OEM_ID_LEN];
	char oem_table_id[ACPI_OEM_TAB_ID_LEN];
	__u32 oem_revision;
	char creator_id[ACPI_SDT_CREATOR_ID_LEN];
	__u32 creator_revision;
} __packed;

struct acpi_subsdt_hdr {
	__u8 type;
	__u8 len;
} __packed;

#define ACPI_RSDT_SIG					"RSDT"
struct acpi_rsdt {
	struct acpi_sdt_hdr hdr;
	__u32 entry[];
} __packed;

#define ACPI_XSDT_SIG					"XSDT"
struct acpi_xsdt {
	struct acpi_sdt_hdr hdr;
	__u64 entry[];
} __packed;

#endif /* __PLAT_CMN_X86_SDT_H__ */
