/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __PLAT_CMN_MPS_H__
#define __PLAT_CMN_MPS_H__

#include <stddef.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

#define UK_MPS_SIG_LEN				4
#define UK_MPS_PARAGRAPH_LEN			16
#define UK_MPS_OEM_ID_LEN			8
#define UK_MPS_PRODUCT_ID_LEN			12

#define UK_MPS_MPFP_SIG				"_MP_"
#define UK_MPS_MPFP_FEAT_B1_MPC_PRESENT_MASK	0xFF
#define UK_MPS_MPFP_FEAT_B2_IMCRP_PRESENT	(1 << 7)
struct uk_mps_mpfp {
	__u8 sig[UK_MPS_SIG_LEN];
	__u32 mpc_paddr;
	__u8 tbl_len;
	__u8 revision;
	__u8 checksum;
	__u8 feat_b1;		/* MP feature info byte 1 */
	__u8 feat_b2;		/* MP feature info byte 2 */
	__u8 feat_b35[3];	/* MP feature info byts 3-5, RESERVED */
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_mpfp) == UK_MPS_PARAGRAPH_LEN);

#define UK_MPS_MPC_SIG				"PCMP"
#define UK_MPS_MPC_TYPE_CPU			0x0
#define UK_MPS_MPC_TYPE_BUS			0x1
#define UK_MPS_MPC_TYPE_IOAPIC			0x2
#define UK_MPS_MPC_TYPE_INTSRC			0x3
#define UK_MPS_MPC_TYPE_LINTSRC			0x4
#define UK_MPS_XMPC_TYPE_SYSMEM			0x80
#define UK_MPS_XMPC_TYPE_BUSLINK		0x81
#define UK_MPS_XMPC_TYPE_BUSOVR			0x82
struct uk_mps_mpc {
	__u8 sig[UK_MPS_SIG_LEN];
	__u16 tbl_len;
	__u8 revision;
	__u8 checksum;
	__u8 oem_id[UK_MPS_OEM_ID_LEN];
	__u8 product_id[UK_MPS_PRODUCT_ID_LEN];
	__u32 oem_tbl_paddr;
	__u16 oem_tbl_len;
	__u16 oem_tbl_count;
	__u32 lapic_paddr;
	__u16 oem_xtbl_len;
	__u8 oem_xtbl_checksum;
	__u8 reserved;
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_mpc) == 44);

#define UK_MPS_CPU_FLAGS_EN			(1 << 0)
#define UK_MPS_CPU_FLAGS_BP			(1 << 1)
#define UK_MPS_CPU_SIG_LEN			4
#define UK_MPS_CPU_SIG_STEPPING_MASK		0x000F
#define UK_MPS_CPU_SIG_MODEL_MASK		0x00F0
#define UK_MPS_CPU_SIG_FAMILY_MASK		0x0F00
#define UK_MPS_CPU_FEAT_FPU			(1 << 0)
#define UK_MPS_CPU_FEAT_MCE			(1 << 7)
#define UK_MPS_CPU_FEAT_CX8			(1 << 8)
#define UK_MPS_CPU_FEAT_APIC			(1 << 9)
struct uk_mps_cpu {
	__u8 type;
	__u8 lapic_id;
	__u8 lapic_version;
	__u8 cpu_flags;
	__u32 cpu_sig;
	__u32 cpu_feat;
	__u32 reserved[2];
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_cpu) == 20);

#define UK_MPS_BUS_STR_LEN			6
#define UK_MPS_BUS_STR_CBUS			"CBUS"
#define UK_MPS_BUS_STR_CBUSII			"CBUSII"
#define UK_MPS_BUS_STR_EISA			"EISA"
#define UK_MPS_BUS_STR_FUTURE			"FUTURE"
#define UK_MPS_BUS_STR_INTERN			"INTERN"
#define UK_MPS_BUS_STR_ISA			"ISA"
#define UK_MPS_BUS_STR_MBI			"MBI"
#define UK_MPS_BUS_STR_MBII			"MBII"
#define UK_MPS_BUS_STR_MCA			"MCA"
#define UK_MPS_BUS_STR_MPI			"MPI"
#define UK_MPS_BUS_STR_MPSA			"MPSA"
#define UK_MPS_BUS_STR_NUBUS			"NUBUS"
#define UK_MPS_BUS_STR_PCI			"PCI"
#define UK_MPS_BUS_STR_PCMCIA			"PCMCIA"
#define UK_MPS_BUS_STR_TC			"TC"
#define UK_MPS_BUS_STR_VL			"VL"
#define UK_MPS_BUS_STR_VME			"VME"
#define UK_MPS_BUS_STR_XPRESS			"XPRESS"
struct uk_mps_bus {
	__u8 type;
	__u8 bus_id;
	__u8 bus_str[UK_MPS_BUS_STR_LEN];
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_bus) == 8);

#define UK_MPS_IOAPIC_FLAGS_EN			(1 << 0)
struct uk_mps_ioapic {
	__u8 type;
	__u8 ioapic_id;
	__u8 ioapic_version;
	__u8 ioapic_flags;
	__u32 ioapic_paddr;
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_ioapic) == 8);

#define UK_MPS_INTSRC_IRQ_TYPE_INT		0x0
#define UK_MPS_INTSRC_IRQ_TYPE_NMI		0x1
#define UK_MPS_INTSRC_IRQ_TYPE_SMI		0x2
#define UK_MPS_INTSRC_IRQ_TYPE_EXTINT		0x3
#define UK_MPS_INTSRC_IRQ_FLAG_PO_DEFAULT	0x0
#define UK_MPS_INTSRC_IRQ_FLAG_PO_HI		0x1
#define UK_MPS_INTSRC_IRQ_FLAG_PO_LO		0x3
#define UK_MPS_INTSRC_IRQ_FLAG_EL_DEFAULT	0x0
#define UK_MPS_INTSRC_IRQ_FLAG_EL_EDGE		0x1
#define UK_MPS_INTSRC_IRQ_FLAG_EL_LEVEL		0x3
struct uk_mps_intsrc {
	__u8 type;
	__u8 irq_type;
	__u16 irq_flag;
	__u8 src_bus_id;
	__u8 src_bus_irq;
	__u8 dest_ioapic_id;
	__u8 dest_ioapic_irq;
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_intsrc) == 8);

#define UK_MPS_LINTSRC_IRQ_TYPE_INT		0x0
#define UK_MPS_LINTSRC_IRQ_TYPE_NMI		0x1
#define UK_MPS_LINTSRC_IRQ_TYPE_SMI		0x2
#define UK_MPS_LINTSRC_IRQ_TYPE_EXTINT		0x3
#define UK_MPS_LINTSRC_IRQ_FLAG_PO_DEFAULT	0x0
#define UK_MPS_LINTSRC_IRQ_FLAG_PO_HI		0x1
#define UK_MPS_LINTSRC_IRQ_FLAG_PO_LO		0x3
#define UK_MPS_LINTSRC_IRQ_FLAG_EL_DEFAULT	0x0
#define UK_MPS_LINTSRC_IRQ_FLAG_EL_EDGE		0x1
#define UK_MPS_LINTSRC_IRQ_FLAG_EL_LEVEL	0x3
struct uk_mps_lintsrc {
	__u8 type;
	__u8 irq_type;
	__u16 irq_flag;
	__u8 src_bus_id;
	__u8 src_bus_irq;
	__u8 dest_lapic_id;
	__u8 dest_lapic_irq;
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_lintsrc) == 8);

#define UK_MPS_SYSMEM_MEM_TYPE_IO			0x0
#define UK_MPS_SYSMEM_MEM_TYPE_MEM			0x1
#define UK_MPS_SYSMEM_MEM_TYPE_PREFETCH			0x2
struct uk_mps_sysmem {
	__u8 type;
	__u8 len;
	__u8 bus_id;
	__u8 mem_type;
	__u64 mem_base;
	__u64 mem_len;
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_sysmem) == 20);

#define UK_MPS_BUSLINK_BUS_INFO_SD			(1 << 0)
struct uk_mps_buslink {
	__u8 type;
	__u8 len;
	__u8 bus_id;
	__u8 bus_info;
	__u8 parent_bus;
	__u8 reserved[3];
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_buslink) == 8);

#define UK_MPS_BUSOVR_ADDR_MOD_PR			(1 << 0)
#define UK_MPS_BUSOVR_RANGES_ISA_IO			0x0
#define UK_MPS_BUSOVR_RANGES_VGA_IO			0x1
struct uk_mps_busovr {
	__u8 type;
	__u8 len;
	__u8 bus_id;
	__u8 addr_mod;
	__u32 ranges;
} __packed;

UK_CTASSERT(sizeof(struct uk_mps_busovr) == 8);

int uk_mps_init(void);

struct uk_mps_mpfp *uk_mps_get_mpfp(void);

struct uk_mps_mpc *uk_mps_get_mpc(void);

#endif /* __PLAT_CMN_MPS_H__ */
