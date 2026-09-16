#ifndef __UK_PLAT_NATIVE_ARCH_REGS_H__
#define __UK_PLAT_NATIVE_ARCH_REGS_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T0		0
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T1		8
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T2		16
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T3		24
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T4		32
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T5		40
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T6		48
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A0		56
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A1		64
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A2		72
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A3		80
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A4		88
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A5		96
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A6		104
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A7		112
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S0		120
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S1		128
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S2		136
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S3		144
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S4		152
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S5		160
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S6		168
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S7		176
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S8		184
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S9		192
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S10	200
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S11	208
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_RA		216
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_TP		224
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_SP		232
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_PC		240
#define UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_PAD	248

#define UK_PLAT_NATIVE_REGS_OFFSETOF_SP					\
	UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_SP
#define UK_PLAT_NATIVE_REGS_OFFSETOF_PC					\
	UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_PC
#define UK_PLAT_NATIVE_REGS_SIZE			256

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_plat_native_regs {
	__u64 t[7];
	__u64 a[8];
	__u64 s[12];
	__u64 ra;
	__u64 tp;
	__u64 sp;
	__u64 pc;
	__u64 pad;
};

UK_CTASSERT(sizeof(struct uk_plat_native_regs) == UK_PLAT_NATIVE_REGS_SIZE);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, t[0]) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_T0);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, a[0]) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_A0);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, s[0]) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_S0);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, ra) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_RA);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, tp) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_TP);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, sp) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_SP);
UK_CTASSERT(__offsetof(struct uk_plat_native_regs, pc) ==
	    UK_PLAT_NATIVE_RISCV64_REGS_OFFSETOF_PC);

#define uk_plat_native_fn_rarg0				a[0]
#define uk_plat_native_fn_rarg1				a[1]
#define uk_plat_native_fn_rarg2				a[2]
#define uk_plat_native_fn_rarg3				a[3]
#define uk_plat_native_fn_rarg4				a[4]
#define uk_plat_native_fn_rarg5				a[5]

#define uk_plat_native_fn_rret0				a[0]
#define uk_plat_native_fn_rret1				a[1]

__isr static inline __u64
uk_plat_native_regs_get(const struct uk_plat_native_regs *regs, __sz offset)
{
	return *(__u64 *)((const char *)regs + offset);
}

__isr static inline void
uk_plat_native_regs_set(struct uk_plat_native_regs *regs, __sz offset,
			__u64 val)
{
	*(__u64 *)((char *)regs + offset) = val;
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_REGS_H__ */
