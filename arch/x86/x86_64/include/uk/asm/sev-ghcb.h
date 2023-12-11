/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_SEV_GHCB_H__
#define __UKARCH_SEV_GHCB_H__

#include <uk/essentials.h>
#include <uk/bitops.h>

struct ghcb_save_area {
	__u8 reserved1[0xcb];
	__u8 cpl;
	__u8 reserved2[0x74];
	__u64 xss;
	__u8 reserved3[0x18];
	__u64 dr7;
	__u8 reserved4[0x90];
	__u64 rax;
	__u8 reserved5[0x108];
	__u64 rcx;
	__u64 rdx;
	__u64 rbx;
	__u8 reserved6[0x70];
	__u64 sw_exitcode;
	__u64 sw_exitinfo1;
	__u64 sw_exitinfo2;
	/* Physical address shared buffer */
	__u64 sw_scratch;
	__u8 reserved7[0x38];
	__u64 xcr0;
	/* Valid state of the fields in  this struct. */
	__u8 valid_bitmap[16];
	__u64 x87_state_gpa;
} __packed;

#define GHCB_VALID_BIT(field)                                                  \
	(__offsetof(struct ghcb_save_area, field) / sizeof(__u64))

/* Set a field to a value, and also update the valid bitmap
 * @param ghcb virtual address of the GHCB struct
 * @field name of the field
 * @value the new updated value
 * */
#define GHCB_SAVE_AREA_SET_FIELD(ghcb, field, value)                           \
	ghcb->save_area.field = (__u64)value;                                  \
	__uk_set_bit(GHCB_VALID_BIT(field),                                    \
		     (unsigned long *)ghcb->save_area.valid_bitmap)

#define GHCB_SAVE_AREA_GET_VALID(ghcb, field)                                  \
	uk_test_bit(GHCB_VALID_BIT(field),                                   \
		      (unsigned long *)ghcb->save_area.valid_bitmap)

struct ghcb {
	struct ghcb_save_area save_area;
	__u8 reserved[0x3f8];
	__u8 shared_buffer[0x7f0];
	__u8 reserved1[0x0a];
	__u16 protocol_version; /* negotiated SEV-ES/GHCB protocol version */
	__u32 ghcb_usage;
} __packed;

#define SEV_ES_MSR_GHCB					0xc0010130
/* GHCB MSR[11:0]*/
#define SEV_GHCB_MSR_GHCB_INFO_MASK			(UK_BIT(12) - 1)
/* GHCB MSR[63:12]*/
#define SEV_GHCB_MSR_GHCB_DATA_MASK                                            \
	((UK_BIT_ULL(64) - 1) & ~SEV_GHCB_MSR_GHCB_INFO_MASK)

#define SEV_GHCB_MSR_RESP_CODE(msr)		((msr) & SEV_GHCB_MSR_GHCB_INFO_MASK)


/* Value in GHCBInfo (GHCB[11:0]) for GHCB MSR Protocol*/
#define SEV_GHCB_MSR_GPA				0x000
#define SEV_GHCB_MSR_INFO				0x001
#define SEV_GHCB_MSR_INFO_REQ				0x002
#define SEV_GHCB_MSR_CPUID_REQ				0x004
#define SEV_GHCB_MSR_CPUID_REQ				0x004
#define SEV_GHCB_MSR_CPUID_RESP				0x005
#define SEV_GHCB_MSR_AP_RESET_HOLD_REQ			0x006
#define SEV_GHCB_MSR_AP_RESET_HOLD_RESP			0x007
#define SEV_GHCB_MSR_PREF_GPA_REQ			0x010
#define SEV_GHCB_MSR_PREF_GPA_RESP			0x011
#define SEV_GHCB_MSR_REG_GPA_REQ			0x012
#define SEV_GHCB_MSR_REG_GPA_RESP			0x013
#define SEV_GHCB_MSR_SNP_PSC_REQ			0x014
#define SEV_GHCB_MSR_SNP_PSC_RESP			0x015
#define SEV_GHCB_MSR_HV_FEAT_REQ			0x080
#define SEV_GHCB_MSR_HV_FEAT_RESP			0x081
#define SEV_GHCB_MSR_TERM_REQ				0x100

/* Values in GHCBInfo (GHCB[63:12]) for GHCB MSR Protocol*/
/* CPUID */
#define SEV_GHCB_MSR_CPUID_REQ_RAX			0
#define SEV_GHCB_MSR_CPUID_REQ_RBX			1
#define SEV_GHCB_MSR_CPUID_REQ_RCX			2
#define SEV_GHCB_MSR_CPUID_REQ_RDX			3
#define SEV_GHCB_MSR_CPUID_REQ_SET_REG(reg)		(((__u64)(reg) & 0x3) << 30)
#define SEV_GHCB_MSR_CPUID_REQ_SET_FN(fn)		(((__u64)fn) << 32)
#define SEV_GHCB_MSR_CPUID_REQ_VAL(reg, fn)		\
	(SEV_GHCB_MSR_CPUID_REQ				\
	| (((__u64)(reg) & 0x3) << 30)			\
	| (((__u64)fn) << 32))

/* Terminate */
#define SEV_GHCB_MSR_TERM_REQ_VAL(term_set, term_code)	\
	(SEV_GHCB_MSR_TERM_REQ				\
	| (((__u64)(term_set) & 0xf) << 12)		\
	| (((__u64)(term_code) & 0xff) << 16))

/* Register GHCB GPA */
#define SEV_GHCB_MSR_REG_GPA_REQ_VAL(pfn)               \
	(SEV_GHCB_MSR_REG_GPA_REQ			\
	| ((__u64)(pfn) << 12))
#define SEV_GHCB_MSR_REG_GPA_RESP_PFN(val)		\
	(((__u64)(val) >> 12)

/* Page State Change */
#define SEV_GHCB_MSR_SNP_PSC_PG_PRIVATE			0x001
#define SEV_GHCB_MSR_SNP_PSC_PG_SHARED			0x002
#define SEV_GHCB_MSR_SNP_PSC_REQ_VAL(pg_op, pfn)	\
	((SEV_GHCB_MSR_SNP_PSC_REQ			\
	| (((__u64)(pg_op) & 0xF) << 52)		\
	| ((__u64)(pfn)  << 12))			\
	& (UK_BIT(56) - 1))
#define SEV_GHCB_MSR_SNP_PSC_RESP_VAL(val)		((__u64)(val) >> 32)




#define SEV_GHCB_USAGE_DEFAULT 0ULL
#define SEV_GHCB_USAGE_HYPERCALL 1ULL
#endif /* __UKARCH_SEV_GHCB_H__ */
