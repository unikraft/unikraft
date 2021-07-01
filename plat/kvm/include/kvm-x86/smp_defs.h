#ifndef __PLAT_KVM_X86_SMP_DEFS_H__
#define __PLAT_KVM_X86_SMP_DEFS_H__

#define X86_LCPU_RUN_FLAG_NMI		1

#define UKPLAT_X86_IPI_RUN_IRQ		5
#define UKPLAT_X86_IPI_WAKEUP_IRQ	6

#define LCPU_STATE_OFFLINE		0
#define LCPU_STATE_BUSY			1
#define LCPU_STATE_IDLE			2
#define LCPU_STATE_WAITING		3

/* When changing the x86_cpu struct, these values must be changed accordingly */
#define X86_CPU_SIZE			80
#define X86_CPU_ENTRY_OFFSET		0
#define X86_CPU_STACKP_OFFSET		8
#define X86_CPU_STATE_OFFSET		16
#define X86_CPU_APICID_OFFSET		20
#define X86_CPU_INTRS_OFFSET		24
#define X86_CPU_TRAPS_OFFSET		32
#define X86_CPU_PANICS_OFFSET		40
#define X86_CPU_FNLIST_OFFSET		48
#define X86_CPU_NMILIST_OFFSET		64

#endif /* __PLAT_KVM_X86_SMP_DEFS_H__ */
