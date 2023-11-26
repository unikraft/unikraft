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

#ifndef __PLAT_CMN_SDT_H__
#define __PLAT_CMN_SDT_H__

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

#define ACPI_MADT_SIG					"APIC"
#define ACPI_MADT_FLAGS_PCAT_COMPAT			0x0001
struct acpi_madt {
	struct acpi_sdt_hdr hdr;
	__u32 lapic_paddr;
	__u32 flags;
	__u8 entries[];
} __packed;

#define ACPI_GAS_ASID_SYS_MEM				0x00
#define ACPI_GAS_ASID_SYS_IO				0x01
#define ACPI_GAS_ASID_PCI_CFG				0x02
#define ACPI_GAS_ASID_EMBED_CTLR			0x03
#define ACPI_GAS_ASID_SMBUS				0x04
#define ACPI_GAS_ASID_SYS_CMOS				0x05
#define ACPI_GAS_ASID_PCI_BAR				0x06
#define ACPI_GAS_ASID_IPMI				0x07
#define ACPI_GAS_ASID_GPIO				0x08
#define ACPI_GAS_ASID_GENERIC_SBUS			0x09
#define ACPI_GAS_ASID_PCC				0x0A
#define ACPI_GAS_ASID_FFIXED_HW				0x7F
struct acpi_gas {
	__u8 asid;
	__u8 bit_sz;
	__u8 bit_off;
	__u8 access_sz;
	__u64 addr;
} __packed;

#define ACPI_FADT_SIG					"FACP"
#define ACPI_FADT_FLAGS_WBINVD				(1 <<  0)
#define ACPI_FADT_FLAGS_WBINVD_FLUSH			(1 <<  1)
#define ACPI_FADT_FLAGS_PROC_C1				(1 <<  2)
#define ACPI_FADT_FLAGS_P_C2_UP				(1 <<  3)
#define ACPI_FADT_FLAGS_PWR_BUTTON			(1 <<  4)
#define ACPI_FADT_FLAGS_SLP_BUTTON			(1 <<  5)
#define ACPI_FADT_FLAGS_FIX_RTC				(1 <<  6)
#define ACPI_FADT_FLAGS_RTC_S4				(1 <<  7)
#define ACPI_FADT_FLAGS_TMR_VAL_EXT			(1 <<  8)
#define ACPI_FADT_FLAGS_DCK_CAP				(1 <<  9)
#define ACPI_FADT_FLAGS_RST_REG_SUP			(1 << 10)
#define ACPI_FADT_FLAGS_SEALED_CASE			(1 << 11)
#define ACPI_FADT_FLAGS_HEADLESS			(1 << 12)
#define ACPI_FADT_FLAGS_CPU_SW_SLP			(1 << 13)
#define ACPI_FADT_FLAGS_PCIE_WALK			(1 << 14)
#define ACPI_FADT_FLAGS_USE_PLAT_CLK			(1 << 15)
#define ACPI_FADT_FLAGS_S4_RTC_STS_VALID		(1 << 16)
#define ACPI_FADT_FLAGS_REMOTE_PWR_ON_CAP		(1 << 17)
#define ACPI_FADT_FLAGS_FORCE_APIC_CLUSTER_MODEL	(1 << 18)
#define ACPI_FADT_FLAGS_FORCE_APIC_PHYS_DEST_MODE	(1 << 19)
#define ACPI_FADT_FLAGS_HW_REDUCED			(1 << 20)
#define ACPI_FADT_FLAGS_LOW_PWR_S0_IDLE_CAP		(1 << 21)
#define ACPI_FADT_X86_BFLAGS_LEGACY_DEVS		(1 <<  1)
#define ACPI_FADT_X86_BFLAGS_8042			(1 <<  2)
#define ACPI_FADT_X86_BFLAGS_NO_VGA			(1 <<  3)
#define ACPI_FADT_X86_BFLAGS_NO_MSI			(1 <<  4)
#define ACPI_FADT_X86_BFLAGS_NO_PCIE_ASPM		(1 <<  5)
#define ACPI_FADT_X86_BFLAGS_NO_CMOS_RTC		(1 <<  6)
#define ACPI_FADT_ARM_BFLAGS_PSCI			(1 <<  0)
#define ACPI_FADT_ARM_BFLAGS_PSCI_HVC			(1 <<  1)
struct acpi_fadt {
	struct acpi_sdt_hdr hdr;
	__u32 facs_paddr;
	__u32 dsdt_paddr;
	__u8 reserved0;
	__u8 pref_pm_prof;
	__u16 sci_irq;
	__u32 smi_cmd;
	__u8 acpi_enable;
	__u8 acpi_disable;
	__u8 s4bios_req;
	__u8 pstate_ctlr;
	__u32 pm1a_evt_blk;
	__u32 pm1b_evt_blk;
	__u32 pm1a_ctlr_blk;
	__u32 pm1b_ctlr_blk;
	__u32 pm2_ctlr_blk;
	__u32 pm_tmr_blk;
	__u32 gpe0_blk;
	__u32 gpe1_blk;
	__u8 pm1_evt_sz;
	__u8 pm1_ctlr_sz;
	__u8 pm2_ctlr_sz;
	__u8 pm_tmr_sz;
	__u8 gpe0_blk_sz;
	__u8 gpe1_blk_sz;
	__u8 gpe1_base;
	__u8 cst_ctlr;
	__u16 c2_lat;
	__u16 c3_lat;
	__u16 flush_sz;
	__u16 flush_stride;
	__u8 duty_offset;
	__u8 duty_width;
	__u8 day_alarm;
	__u8 month_alarm;
	__u8 century;
	__u16 x86_bflags;
	__u8 reserved1;
	__u32 flags;
	struct acpi_gas rst_reg;
	__u8 rst_val;
	__u16 arm_bflags;
	__u8 minor_version;
	__u64 xfacs_paddr;
	__u64 xdsdt_paddr;
	struct acpi_gas xpm1a_evt_blk;
	struct acpi_gas xpm1b_evt_blk;
	struct acpi_gas xpm1a_ctlr_blk;
	struct acpi_gas xpm1b_ctlr_blk;
	struct acpi_gas xpm2_ctlr_blk;
	struct acpi_gas xpm_tmr_blk;
	struct acpi_gas xgpe0_blk;
	struct acpi_gas xgpe1_blk;
	struct acpi_gas slp_ctlr_blk;
	struct acpi_gas slp_sts_blk;
	__u64 hyp_id;
} __packed;

#endif /* __PLAT_CMN_SDT_H__ */
