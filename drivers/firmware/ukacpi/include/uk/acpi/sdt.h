/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, University POLITEHNICA of Bucharest. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ACPI_SDT_H__
#define __UK_ACPI_SDT_H__

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#include <uk/arch/types.h>
#include <uk/essentials.h>

#define UK_ACPI_OEM_ID_LEN				6
#define UK_ACPI_OEM_TAB_ID_LEN				8
#define UK_ACPI_SDT_SIG_LEN				4
#define UK_ACPI_SDT_CREATOR_ID_LEN			4

struct uk_acpi_sdt_hdr {
	char sig[UK_ACPI_SDT_SIG_LEN];
	__u32 tab_len;
	__u8 revision;
	__u8 checksum;
	char oem_id[UK_ACPI_OEM_ID_LEN];
	char oem_table_id[UK_ACPI_OEM_TAB_ID_LEN];
	__u32 oem_revision;
	char creator_id[UK_ACPI_SDT_CREATOR_ID_LEN];
	__u32 creator_revision;
} __packed;

struct uk_acpi_subsdt_hdr {
	__u8 type;
	__u8 len;
} __packed;

#define UK_ACPI_RSDT_SIG				"RSDT"
struct uk_acpi_rsdt {
	struct uk_acpi_sdt_hdr hdr;
	__u32 entry[];
} __packed;

#define UK_ACPI_XSDT_SIG				"XSDT"
struct uk_acpi_xsdt {
	struct uk_acpi_sdt_hdr hdr;
	__u64 entry[];
} __packed;

#define UK_ACPI_MADT_SIG				"APIC"
#define UK_ACPI_MADT_FLAGS_PCAT_COMPAT			0x0001
struct uk_acpi_madt {
	struct uk_acpi_sdt_hdr hdr;
	__u32 lapic_paddr;
	__u32 flags;
	__u8 entries[];
} __packed;

#define UK_ACPI_GAS_ASID_SYS_MEM			0x00
#define UK_ACPI_GAS_ASID_SYS_IO				0x01
#define UK_ACPI_GAS_ASID_PCI_CFG			0x02
#define UK_ACPI_GAS_ASID_EMBED_CTLR			0x03
#define UK_ACPI_GAS_ASID_SMBUS				0x04
#define UK_ACPI_GAS_ASID_SYS_CMOS			0x05
#define UK_ACPI_GAS_ASID_PCI_BAR			0x06
#define UK_ACPI_GAS_ASID_IPMI				0x07
#define UK_ACPI_GAS_ASID_GPIO				0x08
#define UK_ACPI_GAS_ASID_GENERIC_SBUS			0x09
#define UK_ACPI_GAS_ASID_PCC				0x0A
#define UK_ACPI_GAS_ASID_FFIXED_HW			0x7F
struct uk_acpi_gas {
	__u8 asid;
	__u8 bit_sz;
	__u8 bit_off;
	__u8 access_sz;
	__u64 addr;
} __packed;

#define UK_ACPI_FADT_SIG				"FACP"
#define UK_ACPI_FADT_FLAGS_WBINVD			(1 <<  0)
#define UK_ACPI_FADT_FLAGS_WBINVD_FLUSH			(1 <<  1)
#define UK_ACPI_FADT_FLAGS_PROC_C1			(1 <<  2)
#define UK_ACPI_FADT_FLAGS_P_C2_UP			(1 <<  3)
#define UK_ACPI_FADT_FLAGS_PWR_BUTTON			(1 <<  4)
#define UK_ACPI_FADT_FLAGS_SLP_BUTTON			(1 <<  5)
#define UK_ACPI_FADT_FLAGS_FIX_RTC			(1 <<  6)
#define UK_ACPI_FADT_FLAGS_RTC_S4			(1 <<  7)
#define UK_ACPI_FADT_FLAGS_TMR_VAL_EXT			(1 <<  8)
#define UK_ACPI_FADT_FLAGS_DCK_CAP			(1 <<  9)
#define UK_ACPI_FADT_FLAGS_RST_REG_SUP			(1 << 10)
#define UK_ACPI_FADT_FLAGS_SEALED_CASE			(1 << 11)
#define UK_ACPI_FADT_FLAGS_HEADLESS			(1 << 12)
#define UK_ACPI_FADT_FLAGS_CPU_SW_SLP			(1 << 13)
#define UK_ACPI_FADT_FLAGS_PCIE_WALK			(1 << 14)
#define UK_ACPI_FADT_FLAGS_USE_PLAT_CLK			(1 << 15)
#define UK_ACPI_FADT_FLAGS_S4_RTC_STS_VALID		(1 << 16)
#define UK_ACPI_FADT_FLAGS_REMOTE_PWR_ON_CAP		(1 << 17)
#define UK_ACPI_FADT_FLAGS_FORCE_APIC_CLUSTER_MODEL	(1 << 18)
#define UK_ACPI_FADT_FLAGS_FORCE_APIC_PHYS_DEST_MODE	(1 << 19)
#define UK_ACPI_FADT_FLAGS_HW_REDUCED			(1 << 20)
#define UK_ACPI_FADT_FLAGS_LOW_PWR_S0_IDLE_CAP		(1 << 21)
#define UK_ACPI_FADT_X86_BFLAGS_LEGACY_DEVS		(1 <<  1)
#define UK_ACPI_FADT_X86_BFLAGS_8042			(1 <<  2)
#define UK_ACPI_FADT_X86_BFLAGS_NO_VGA			(1 <<  3)
#define UK_ACPI_FADT_X86_BFLAGS_NO_MSI			(1 <<  4)
#define UK_ACPI_FADT_X86_BFLAGS_NO_PCIE_ASPM		(1 <<  5)
#define UK_ACPI_FADT_X86_BFLAGS_NO_CMOS_RTC		(1 <<  6)
#define UK_ACPI_FADT_ARM_BFLAGS_PSCI			(1 <<  0)
#define UK_ACPI_FADT_ARM_BFLAGS_PSCI_HVC		(1 <<  1)
struct uk_acpi_fadt {
	struct uk_acpi_sdt_hdr hdr;
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
	struct uk_acpi_gas rst_reg;
	__u8 rst_val;
	__u16 arm_bflags;
	__u8 minor_version;
	__u64 xfacs_paddr;
	__u64 xdsdt_paddr;
	struct uk_acpi_gas xpm1a_evt_blk;
	struct uk_acpi_gas xpm1b_evt_blk;
	struct uk_acpi_gas xpm1a_ctlr_blk;
	struct uk_acpi_gas xpm1b_ctlr_blk;
	struct uk_acpi_gas xpm2_ctlr_blk;
	struct uk_acpi_gas xpm_tmr_blk;
	struct uk_acpi_gas xgpe0_blk;
	struct uk_acpi_gas xgpe1_blk;
	struct uk_acpi_gas slp_ctlr_blk;
	struct uk_acpi_gas slp_sts_blk;
	__u64 hyp_id;
} __packed;

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_ACPI_SDT_H__ */
