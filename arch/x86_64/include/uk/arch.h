/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_ARCH_H__
#define __UK_ARCH_H__

/** Cache line size */
#define UK_ARCH_CACHE_LINE_SIZE			64

/* CPUID feature bits in ECX and EDX when EAX=1 */
/** x2APIC feature */
#define UK_ARCH_CPUID1_ECX_X2APIC		(1 << 21)
/** XSAVE feature */
#define UK_ARCH_CPUID1_ECX_XSAVE		(1 << 26)
/** OS XSAVE enabled */
#define UK_ARCH_CPUID1_ECX_OSXSAVE		(1 << 27)
/** AVX feature */
#define UK_ARCH_CPUID1_ECX_AVX			(1 << 28)
/** RDRAND instruction */
#define UK_ARCH_CPUID1_ECX_RDRAND		(1 << 30)
/** FPU feature */
#define UK_ARCH_CPUID1_EDX_FPU			(1 << 0)
/** Page attribute table */
#define UK_ARCH_CPUID1_EDX_PAT			(1 << 16)
/** FXSAVE/FXRSTOR instructions */
#define UK_ARCH_CPUID1_EDX_FXSR			(1 << 24)
/** SSE feature */
#define UK_ARCH_CPUID1_EDX_SSE			(1 << 25)

/* CPUID feature bits in EBX and ECX when EAX=7, ECX=0 */
/** FSGSBASE instructions */
#define UK_ARCH_CPUID7_EBX_FSGSBASE		(1 << 0)
/** Protection keys for user pages */
#define UK_ARCH_CPUID7_ECX_PKU			(1 << 3)
/** OS protection keys enabled */
#define UK_ARCH_CPUID7_ECX_OSPKE		(1 << 4)
/** 5-level paging */
#define UK_ARCH_CPUID7_ECX_LA57			(1 << 16)
/** RDSEED instruction */
#define UK_ARCH_CPUID7_EBX_RDSEED		(1 << 18)

/* CPUID feature bits when EAX=0xd, ECX=1 */
/** XSAVEOPT instruction */
#define UK_ARCH_CPUIDD1_EAX_XSAVEOPT		(1 << 0)

/* CPUID 80000001H:EDX feature list */
/** No-execute page protection */
#define UK_ARCH_CPUID81_NX			(1 << 20)
/** 1GB pages */
#define UK_ARCH_CPUID81_PAGE1GB			(1 << 26)
/** Long mode (64-bit) */
#define UK_ARCH_CPUID81_LM			(1 << 29)
/** SYSCALL/SYSRET instructions */
#define UK_ARCH_CPUID3_SYSCALL			(1 << 11)

/* EFLAGS register */
/** Carry flag */
#define UK_ARCH_EFLAGS_CF			(1 << 0)
/** Parity flag */
#define UK_ARCH_EFLAGS_PF			(1 << 2)
/** Auxiliary flag */
#define UK_ARCH_EFLAGS_AF			(1 << 4)
/** Zero flag */
#define UK_ARCH_EFLAGS_ZF			(1 << 6)
/** Sign flag */
#define UK_ARCH_EFLAGS_SF			(1 << 7)
/** Trap flag */
#define UK_ARCH_EFLAGS_TF			(1 << 8)
/** Interrupt flag */
#define UK_ARCH_EFLAGS_IF			(1 << 9)
/** Direction flag */
#define UK_ARCH_EFLAGS_DF			(1 << 10)
/** Overflow flag */
#define UK_ARCH_EFLAGS_OF			(1 << 11)
/** Nested task flag */
#define UK_ARCH_EFLAGS_NT			(1 << 14)
/** Resume flag */
#define UK_ARCH_EFLAGS_RF			(1 << 16)
/** Virtual 8086 mode flag */
#define UK_ARCH_EFLAGS_VM			(1 << 17)
/** Alignment check flag */
#define UK_ARCH_EFLAGS_AC			(1 << 18)
/** Virtual interrupt flag */
#define UK_ARCH_EFLAGS_VIF			(1 << 19)
/** Virtual interrupt pending */
#define UK_ARCH_EFLAGS_VIP			(1 << 20)
/** ID flag */
#define UK_ARCH_EFLAGS_ID			(1 << 21)

/* Basic CPU control in CR0 */
/** Protection Enable */
#define UK_ARCH_CR0_PE				(1 << 0)
/** Monitor Coprocessor */
#define UK_ARCH_CR0_MP				(1 << 1)
/** Emulation */
#define UK_ARCH_CR0_EM				(1 << 2)
/** Task Switched */
#define UK_ARCH_CR0_TS				(1 << 3)
/** Numeric Exception */
#define UK_ARCH_CR0_NE				(1 << 5)
/** Write Protect */
#define UK_ARCH_CR0_WP				(1 << 16)
/** Paging */
#define UK_ARCH_CR0_PG				(1 << 31)

/* Intel CPU features in CR4 */
/** Enable PAE */
#define UK_ARCH_CR4_PAE				(1 << 5)
/** OS support for FXSAVE/FXRSTOR */
#define UK_ARCH_CR4_OSFXSR			(1 << 9)
/** OS support for FP exceptions */
#define UK_ARCH_CR4_OSXMMEXCPT			(1 << 10)
/** Enable FSGSBASE */
#define UK_ARCH_CR4_FSGSBASE			(1 << 16)
/** Enable XSAVE, extended states */
#define UK_ARCH_CR4_OSXSAVE			(1 << 18)
/** Enable protection keys */
#define UK_ARCH_CR4_PKE				(1 << 22)

/* Extended Control Register 0 (XCR0) */
/** x87 FPU state */
#define UK_ARCH_XCR0_X87			(1 << 0)
/** SSE state */
#define UK_ARCH_XCR0_SSE			(1 << 1)
/** AVX state */
#define UK_ARCH_XCR0_AVX			(1 << 2)
/** Protection key state */
#define UK_ARCH_XCR0_PKRU			(1 << 9)

/* Model-specific register addresses */
/** FS base register in 64-bit mode */
#define UK_ARCH_MSR_FS_BASE			0xc0000100
/** GS base register in 64-bit mode */
#define UK_ARCH_MSR_GS_BASE			0xc0000101
/** Used in conjunction with swapgs instruction */
#define UK_ARCH_MSR_KERNEL_GS_BASE		0xc0000102
/** Extended feature register */
#define UK_ARCH_MSR_EFER			0xc0000080
/** Legacy mode SYSCALL target */
#define UK_ARCH_MSR_STAR			0xc0000081
/** Long mode SYSCALL target */
#define UK_ARCH_MSR_LSTAR			0xc0000082
/** Compat mode SYSCALL target */
#define UK_ARCH_MSR_CSTAR			0xc0000083
/** EFLAGS mask for syscall */
#define UK_ARCH_MSR_SYSCALL_MASK		0xc0000084
/** Page attribute table configuration */
#define UK_ARCH_MSR_PAT				0x277

/* MSR EFER bits */
/** System call extensions */
#define UK_ARCH_EFER_SCE			(1 << 0)
/** Long mode enable */
#define UK_ARCH_EFER_LME			(1 << 8)
/** Long mode active */
#define UK_ARCH_EFER_LMA			(1 << 10)
/** No-execute enable */
#define UK_ARCH_EFER_NXE			(1 << 11)
/** Secure virtual machine enable */
#define UK_ARCH_EFER_SVME			(1 << 12)
/** Long mode segment limit enable */
#define UK_ARCH_EFER_LMSLE			(1 << 13)
/** Fast FXSAVE/FXRSTOR */
#define UK_ARCH_EFER_FFXSR			(1 << 14)
/** Translation cache extension */
#define UK_ARCH_EFER_TCE			(1 << 15)

/* Page Tables */
/** Page present */
#define UK_ARCH_PTE_PRESENT			0x001UL
/** Page read/write */
#define UK_ARCH_PTE_RW				0x002UL
/** Page user/supervisor */
#define UK_ARCH_PTE_US				0x004UL
/** Page write-through */
#define UK_ARCH_PTE_PWT				0x008UL
/** Page cache disabled */
#define UK_ARCH_PTE_PCD				0x010UL
/** Page accessed */
#define UK_ARCH_PTE_ACCESSED			0x020UL
/** Page dirty */
#define UK_ARCH_PTE_DIRTY			0x040UL
/** Page size extension */
#define UK_ARCH_PTE_PSE				0x080UL
/** Global page */
#define UK_ARCH_PTE_GLOBAL			0x100UL
/** User-defined bits 1 */
#define UK_ARCH_PTE_USER1_MASK			0xE00UL
/** User-defined bits 2 */
#define UK_ARCH_PTE_USER2_MASK			(0x7FUL << 52)
/** Memory protection key */
#define UK_ARCH_PTE_MPK_MASK			(0xFUL << 59)
/** No-execute */
#define UK_ARCH_PTE_NX				(1UL << 63)

/** Page allows write-combining */
#define UK_ARCH_PAGE_ATTR_WRITECOMBINE		0x08

/* Page fault error code bits */
/** 0=non-present, 1=prot */
#define UK_ARCH_PF_EC_P				0x0001UL
/** 0=read, 1=write */
#define UK_ARCH_PF_EC_WR			0x0002UL
/** 0=kernel, 1=user */
#define UK_ARCH_PF_EC_US			0x0004UL
/** Reserved bit in PTE */
#define UK_ARCH_PF_EC_RSVD			0x0008UL
/** Instruction fetch */
#define UK_ARCH_PF_EC_ID			0x0010UL
/** Protection key violation */
#define UK_ARCH_PF_EC_PK			0x0020UL
/** Shadow stack access */
#define UK_ARCH_PF_EC_SS			0x0040UL
/** SGX access control viol. */
#define UK_ARCH_PF_EC_SGX			0x8000UL
/** No translation using HLAT */
#define UK_ARCH_PF_EC_HLAT			0x0080UL

/* Page attribute table (PAT) */
/** Uncacheable (UC) */
#define UK_ARCH_PAT_UC				0x00
/** Write combining (WC) */
#define UK_ARCH_PAT_WC				0x01
/** Write through (WT) */
#define UK_ARCH_PAT_WT				0x04
/** Write protected (WP) */
#define UK_ARCH_PAT_WP				0x05
/** Write back (WB) */
#define UK_ARCH_PAT_WB				0x06
/** Uncached (UC-) */
#define UK_ARCH_PAT_UCM				0x07

/* Global descriptor table (GDT) */
/** Null descriptor */
#define UK_ARCH_GDT_DESC_NULL			0
/** Code segment descriptor */
#define UK_ARCH_GDT_DESC_CODE			1
/** Data segment descriptor */
#define UK_ARCH_GDT_DESC_DATA			2
/** TSS descriptor low part */
#define UK_ARCH_GDT_DESC_TSS_LO			3
/** TSS descriptor high part */
#define UK_ARCH_GDT_DESC_TSS_HI			4
/** TSS descriptor */
#define UK_ARCH_GDT_DESC_TSS			UK_ARCH_GDT_DESC_TSS_LO

/** LDT descriptor type */
#define UK_ARCH_GDT_DESC_TYPE_LDT		0x2
/** TSS available type */
#define UK_ARCH_GDT_DESC_TYPE_TSS_AVAIL		0x9
/** TSS busy type */
#define UK_ARCH_GDT_DESC_TYPE_TSS_BUSY		0xb
/** Call gate type */
#define UK_ARCH_GDT_DESC_TYPE_CALL		0xc
/** Interrupt gate type */
#define UK_ARCH_GDT_DESC_TYPE_INTR		0xe
/** Trap gate type */
#define UK_ARCH_GDT_DESC_TYPE_TRAP		0xf

/** Kernel privilege level */
#define UK_ARCH_GDT_DESC_DPL_KERNEL		0
/** User privilege level */
#define UK_ARCH_GDT_DESC_DPL_USER		3

/** Calculate GDT descriptor offset */
#define UK_ARCH_GDT_DESC_OFFSET(n)		((n) * 0x8)
/** Number of GDT entries */
#define UK_ARCH_GDT_NUM_ENTRIES			5

/**
 * 32-bit code segment descriptor value
 * Seg. Limit                       : 0xfffff
 * Base                             : 0x00000000
 * Type                             : 0xa (execute/read/accessed)
 * Code or Data Segment (S)         : 0x1 (true)
 * Descriptor Privilege Level (DPL) : 0x0 (most privileged)
 * Segment Present (P)              : 0x1 (true)
 * Default Operation Size (D)       : 0x1 (32-bit)
 * Granularity (G)                  : 0x1 (4KiB)
 */
#define UK_ARCH_GDT_DESC_CODE32_VAL		0x00cf9b000000ffff

/**
 * 32-bit data segment descriptor value
 * Seg. Limit                       : 0xfffff
 * Base                             : 0x00000000
 * Type                             : 0x3 (read/write/accessed)
 * Code or Data Segment (S)         : 0x1 (true)
 * Descriptor Privilege Level (DPL) : 0x0 (most privileged)
 * Segment Present (P)              : 0x1 (true)
 * Granularity (G)                  : 0x1 (4KiB)
 */
#define UK_ARCH_GDT_DESC_DATA32_VAL		0x00cf93000000ffff

/**
 * 64-bit code segment descriptor value
 * Seg. Limit                       : 0xfffff
 * Base                             : 0x00000000
 * Type                             : 0xb (execute/read/accessed)
 * Code or Data Segment (S)         : 0x1 (true)
 * Descriptor Privilege Level (DPL) : 0x0 (most privileged)
 * Segment Present (P)              : 0x1 (true)
 * 64-bit Code Segment (L)          : 0x1 (true)
 * Granularity (G)                  : 0x1 (4KiB)
 */
#define UK_ARCH_GDT_DESC_CODE64_VAL		0x00af9b000000ffff
/** 64-bit data segment descriptor value */
#define UK_ARCH_GDT_DESC_DATA64_VAL		UK_ARCH_GDT_DESC_DATA32_VAL

/* Interrupt descriptor table (IDT) */
/** Code segment for IDT */
#define UK_ARCH_IDT_DESC_CODE			UK_ARCH_GDT_DESC_CODE

/** Interrupt gate type */
#define UK_ARCH_IDT_DESC_TYPE_INTR		UK_ARCH_GDT_DESC_TYPE_INTR

/** Kernel privilege level for IDT */
#define UK_ARCH_IDT_DESC_DPL_KERNEL		UK_ARCH_GDT_DESC_DPL_KERNEL
/** User privilege level for IDT */
#define UK_ARCH_IDT_DESC_DPL_USER		UK_ARCH_GDT_DESC_DPL_USER

/** Calculate IDT descriptor offset */
#define UK_ARCH_IDT_DESC_OFFSET(n)		UK_ARCH_GDT_DESC_OFFSET(n)
/** First user IRQ vector */
#define UK_ARCH_IDT_USER_IRQ_START		32
/** Number of IDT entries */
#define UK_ARCH_IDT_NUM_ENTRIES			256

/** Divide error exception */
#define UK_ARCH_TRAPNUM_DIVIDE_ERROR		0
/** Debug exception */
#define UK_ARCH_TRAPNUM_DEBUG			1
/** NMI interrupt */
#define UK_ARCH_TRAPNUM_NMI			2
/** Breakpoint exception */
#define UK_ARCH_TRAPNUM_INT3			3
/** Overflow exception */
#define UK_ARCH_TRAPNUM_OVERFLOW		4
/** BOUND range exceeded exception */
#define UK_ARCH_TRAPNUM_BOUNDS			5
/** Invalid opcode exception */
#define UK_ARCH_TRAPNUM_INVALID_OP		6
/** Device not available exception */
#define UK_ARCH_TRAPNUM_NO_DEVICE		7
/** Double fault exception */
#define UK_ARCH_TRAPNUM_DOUBLE_FAULT		8
/** Coprocessor segment overrun */
#define UK_ARCH_TRAPNUM_COPROC_SEG_OVERRUN	9
/** Invalid TSS exception */
#define UK_ARCH_TRAPNUM_INVALID_TSS		10
/** Segment not present */
#define UK_ARCH_TRAPNUM_NO_SEGMENT		11
/** Stack fault exception */
#define UK_ARCH_TRAPNUM_STACK_ERROR		12
/** General protection exception */
#define UK_ARCH_TRAPNUM_GP_FAULT		13
/** Page fault exception */
#define UK_ARCH_TRAPNUM_PAGE_FAULT		14
/** x87 FPU error exception */
#define UK_ARCH_TRAPNUM_COPROC_ERROR		16
/** Alignment check exception */
#define UK_ARCH_TRAPNUM_ALIGNMENT_CHECK		17
/** Machine check exception */
#define UK_ARCH_TRAPNUM_MACHINE_CHECK		18
/** SIMD floating-point exception */
#define UK_ARCH_TRAPNUM_SIMD_ERROR		19
/** Virtualization exception */
#define UK_ARCH_TRAPNUM_VIRT_ERROR		20
/** Security exception */
#define UK_ARCH_TRAPNUM_SECURITY_ERROR		21

#if !__ASSEMBLY__
#include <uk/essentials.h>

/**
 * Protected mode descriptor table pointer
 */
struct uk_arch_desc_table_ptr32 {
	/** Table size in bytes minus 1 */
	__u16 limit;
	/** Linear base address */
	__u32 base;
} __packed;

/**
 * Long mode descriptor table pointer
 */
struct uk_arch_desc_table_ptr64 {
	/** Table size in bytes minus 1 */
	__u16 limit;
	/** Linear base address */
	__u64 base;
} __packed;

/**
 * 32-bit segment descriptor (GDT/LDT user segment)
 */
struct uk_arch_seg_desc32 {
	union {
		/* Raw backing integers. */
		struct {
			__u32 lo, hi;
		};
		/* Common named fields. */
		struct {
			/** Segment limit low 16 bits */
			__u64 limit_lo:16;
			/** Base address low 24 bits */
			__u64 base_lo:24;
			/** Segment type */
			__u64 type:4;
			/** Descriptor type (0=system, 1=code/data) */
			__u64 s:1;
			/** Descriptor privilege level */
			__u64 dpl:2;
			/** Segment present */
			__u64 p:1;
			/** Segment limit high 4 bits */
			__u64 limit_hi:4;
			/** Available for system use */
			__u64 avl:1;
			/** 64-bit code segment */
			__u64 l:1;
			/** Default operation size (0=16-bit, 1=32-bit) */
			__u64 d:1;
			/** Granularity (0=byte, 1=4KiB) */
			__u64 gran:1;
			/** Base address high 8 bits */
			__u64 base_hi:8;
		};
		/* Code segment specific field names. */
		struct {
			/** Segment limit low 16 bits */
			__u64 limit_lo:16;
			/** Base address low 24 bits */
			__u64 base_lo:24;
			/** Accessed */
			__u64 a:1;
			/** Readable */
			__u64 r:1;
			/** Conforming */
			__u64 c:1;
			/** Executable */
			__u64 x:1;
			/** Descriptor type (must be 1) */
			__u64 s:1;
			/** Descriptor privilege level */
			__u64 dpl:2;
			/** Segment present */
			__u64 p:1;
			/** Segment limit high 4 bits */
			__u64 limit_hi:4;
			/** Available for system use */
			__u64 avl:1;
			/** 64-bit code segment */
			__u64 l:1;
			/** Default operation size */
			__u64 d:1;
			/** Granularity */
			__u64 gran:1;
			/** Base address high 8 bits */
			__u64 base_hi:8;
		} code;
		/* Data segment specific field names. */
		struct {
			/** Segment limit low 16 bits */
			__u64 limit_lo:16;
			/** Base address low 24 bits */
			__u64 base_lo:24;
			/** Accessed */
			__u64 a:1;
			/** Writable */
			__u64 w:1;
			/** Expand-down */
			__u64 e:1;
			/** Executable (must be 0 for data) */
			__u64 x:1;
			/** Descriptor type (must be 1) */
			__u64 s:1;
			/** Descriptor privilege level */
			__u64 dpl:2;
			/** Segment present */
			__u64 p:1;
			/** Segment limit high 4 bits */
			__u64 limit_hi:4;
			/** Available for system use */
			__u64 avl:1;
			/** Reserved */
			__u64 reserved:1;
			/** Big (0=16-bit stack, 1=32-bit stack) */
			__u64 b:1;
			/** Granularity */
			__u64 gran:1;
			/** Base address high 8 bits */
			__u64 base_hi:8;
		} data;

		__u64 raw;
	};
} __packed;

/**
 * 64-bit system segment descriptor
 */
struct uk_arch_seg_desc64 {
	union {
		struct {
			__u64 lo, hi;
		};
		struct {
			/** Segment limit low 16 bits */
			__u64 limit_lo:16;
			/** Base address low 24 bits */
			__u64 base_lo:24;
			/** Segment type */
			__u64 type:4;
			/** Must be 0 for system descriptors */
			__u64 zero:1;
			/** Descriptor privilege level */
			__u64 dpl:2;
			/** Segment present */
			__u64 p:1;
			/** Segment limit high 4 bits */
			__u64 limit_hi:4;
			/** Available for system use */
			__u64 avl:1;
			/** Unused */
			__u64 unused:2;
			/** Granularity */
			__u64 gran:1;
			/** Base address high 40 bits */
			__u64 base_hi:40;
			/** Reserved */
			__u64 reserved:8;
			/** Must be 0 */
			__u64 zero1:5;
			/** Reserved */
			__u64 reserved1:19;
		} __packed;
	};
} __packed;

/**
 * 32-bit gate descriptor (IDT entry, GDT task/call gate)
 */
struct uk_arch_seg_gate_desc32 {
	union {
		struct {
			__u32 lo, hi;
		};
		struct {
			/** Offset low 16 bits */
			__u32 offset_lo:16;
			/** Segment selector */
			__u32 selector:16;
			/** Reserved */
			__u32 reserved:8;
			/** Gate type */
			__u32 type:4;
			/** Must be 0 for gates */
			__u32 s:1;
			/** Descriptor privilege level */
			__u32 dpl:2;
			/** Segment present */
			__u32 p:1;
			/** Offset high 16 bits */
			__u32 offset_hi:16;
		};
	};
} __packed;

/**
 * 64-bit gate descriptor (long mode IDT entry)
 */
struct uk_arch_seg_gate_desc64 {
	union {
		struct {
			__u64 lo, hi;
		};
		struct {
			/** Offset low 16 bits */
			__u64 offset_lo:16;
			/** Segment selector */
			__u64 selector:16;
			/** Interrupt stack table offset */
			__u64 ist:3;
			/** Reserved */
			__u64 reserved:5;
			/** Gate type */
			__u64 type:4;
			/** Must be 0 for gates */
			__u64 s: 1;
			/** Descriptor privilege level */
			__u64 dpl: 2;
			/** Segment present */
			__u64 p: 1;
			/** Offset high 48 bits */
			__u64 offset_hi:48;
			/** Reserved */
			__u64 reserved1:32;
		} __packed;
	};
} __packed;

/**
 * 64-bit Task State Segment
 */
struct uk_arch_tss64 {
	/** Reserved */
	__u32 reserved;
	/** Privilege level 0-2 stack pointers */
	__u64 rsp[3];
	/** Reserved */
	__u64 reserved2;
	/** Interrupt stack table (1-based) */
	__u64 ist[7];
	/** Reserved */
	__u64 reserved3;
	/** Reserved */
	__u16 reserved4;
	/** I/O map base address */
	__u16 iomap_base;
} __packed;
#endif /* !__ASSEMBLY__ */
#endif /* __UK_ARCH_H__ */
