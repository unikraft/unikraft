#ifndef __PLAT_CMN_X86_XAPIC_H__
#define __PLAT_CMN_X86_XAPIC_H__

#include <x86/cpu.h>
#include <x86/apic_defs.h>

#define x2apic_logical_dest(x) ((((x)&0xfff0) << 16) | (1 << ((x)&0x000f)))

static inline void x2apic_send_ipi(int irqno, int dest)
{
	int eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_FIXED
	      | (32 + irqno);

	wrmsr(x2APIC_ICR, eax, dest);
}

static inline void x2apic_send_nmi(int dest)
{
	int eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_NMI;

	wrmsr(x2APIC_ICR, eax, dest);
}

static inline void x2apic_send_sipi(int addr, int dest)
{
	int eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_SUP | (addr >> 12);

	wrmsr(x2APIC_ICR, eax, dest);
}

static inline void x2apic_send_iipi(int dest)
{
	int eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_LEVEL_ASSERT
	      | APIC_ICR_DESTMODE_PHYSICAL | APIC_ICR_DMODE_INIT;

	wrmsr(x2APIC_ICR, eax, dest);
}

static inline void x2apic_send_iipi_deassert(int dest)
{
	int eax;

	eax = APIC_ICR_TRIGGER_LEVEL | APIC_ICR_DESTMODE_PHYSICAL
	      | APIC_ICR_DMODE_INIT;

	wrmsr(x2APIC_ICR, eax, dest);
}

static inline void x2apic_clear_errors(void)
{
	wrmsr(x2APIC_ESR, 0, 0);
}

static inline void x2apic_ack_interrupt(void)
{
	wrmsr(x2APIC_EOI, 0, 0);
}


#endif /* __PLAT_CMN_X86_XAPIC_H__ */
