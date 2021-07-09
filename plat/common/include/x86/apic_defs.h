#ifndef __PLAT_CMN_X86_XAPIC_DEFS_H__
#define __PLAT_CMN_X86_XAPIC_DEFS_H__

/* MSR addresses */
#define IA32_APIC_BASE		0x1b
#define x2APIC_BASE			0x800
#define x2APIC_SPUR			0x80F
#define x2APIC_ESR			0x828
#define x2APIC_ICR			0x830

#define APIC_BASE_EXTD		10
#define APIC_BASE_EN			11
#define APIC_SPUR_EN			8
#define x2APIC_CPUID_BIT		21

/* ICR delivery modes */
#define APIC_ICR_DMODE_FIXED		0x0
#define APIC_ICR_DMODE_SMI		0x200
#define APIC_ICR_DMODE_NMI		0x400
#define APIC_ICR_DMODE_INIT		0x500
#define APIC_ICR_DMODE_SUP		0x600

/* Other bits of ICR */
#define APIC_ICR_DESTMODE_PHYSICAL	0x0
#define APIC_ICR_DESTMODE_LOGICAL	0x800
#define APIC_ICR_LEVEL_DEASSERT	0x0
#define APIC_ICR_LEVEL_ASSERT		0x4000
#define APIC_ICR_TRIGGER_EDGE		0x0
#define APIC_ICR_TRIGGER_LEVEL	0x8000

/* Compute logical destination ID from the physical one */
#define x2apic_logical_dest(x) ((((x) & 0xfff0) << 16) | (1 << ((x) & 0x000f)))

#endif /* __PLAT_CMN_X86_XAPIC_DEFS_H__ */
