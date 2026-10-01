#include <uk/essentials.h>
#include <uk/lcpu.h>

#if CONFIG_LIBUKNOFAULT
#include <uk/nofault.h>
#endif /* CONFIG_LIBUKNOFAULT */

#include "../../crashdump.h"

static inline __u64 cdmp_arch_read_gp(void)
{
	__u64 gp;

	__asm__ __volatile__("mv %0, gp" : "=r" (gp));

	return gp;
}

void cdmp_arch_print_registers(struct uk_lcpu_regs *regs)
{
	crash_printk("Registers:\n");

	crash_printk("PC: %016lx RA: %016lx SP: %016lx GP: %016lx\n",
		     uk_lcpu_regs_get(regs, PC),
		     uk_lcpu_regs_get(regs, RA),
		     uk_lcpu_regs_get(regs, SP),
		     cdmp_arch_read_gp());
	crash_printk("TP: %016lx T0: %016lx T1: %016lx T2: %016lx\n",
		     uk_lcpu_regs_get(regs, TP),
		     uk_lcpu_regs_get(regs, T0),
		     uk_lcpu_regs_get(regs, T1),
		     uk_lcpu_regs_get(regs, T2));
	crash_printk("S0: %016lx S1: %016lx A0: %016lx A1: %016lx\n",
		     uk_lcpu_regs_get(regs, S0),
		     uk_lcpu_regs_get(regs, S1),
		     uk_lcpu_regs_get(regs, A0),
		     uk_lcpu_regs_get(regs, A1));
	crash_printk("A2: %016lx A3: %016lx A4: %016lx A5: %016lx\n",
		     uk_lcpu_regs_get(regs, A2),
		     uk_lcpu_regs_get(regs, A3),
		     uk_lcpu_regs_get(regs, A4),
		     uk_lcpu_regs_get(regs, A5));
	crash_printk("A6: %016lx A7: %016lx S2: %016lx S3: %016lx\n",
		     uk_lcpu_regs_get(regs, A6),
		     uk_lcpu_regs_get(regs, A7),
		     uk_lcpu_regs_get(regs, S2),
		     uk_lcpu_regs_get(regs, S3));
	crash_printk("S4: %016lx S5: %016lx S6: %016lx S7: %016lx\n",
		     uk_lcpu_regs_get(regs, S4),
		     uk_lcpu_regs_get(regs, S5),
		     uk_lcpu_regs_get(regs, S6),
		     uk_lcpu_regs_get(regs, S7));
	crash_printk("S8: %016lx S9: %016lx S10:%016lx S11:%016lx\n",
		     uk_lcpu_regs_get(regs, S8),
		     uk_lcpu_regs_get(regs, S9),
		     uk_lcpu_regs_get(regs, S10),
		     uk_lcpu_regs_get(regs, S11));
	crash_printk("T3: %016lx T4: %016lx T5: %016lx T6: %016lx\n",
		     uk_lcpu_regs_get(regs, T3),
		     uk_lcpu_regs_get(regs, T4),
		     uk_lcpu_regs_get(regs, T5),
		     uk_lcpu_regs_get(regs, T6));
}

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK
void cdmp_arch_print_stack(struct uk_lcpu_regs *regs)
{
	cdmp_gen_print_stack(uk_lcpu_regs_get(regs, SP));
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_STACK */

#if CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__
void cdmp_arch_print_call_trace(struct uk_lcpu_regs *regs)
{
	__u64 fp = uk_lcpu_regs_get(regs, FP);
	__u64 *frame;
	int depth_left = 32;
	__sz frame_len = sizeof(unsigned long) * 2;

	crash_printk("Call Trace:\n");

	cdmp_gen_print_call_trace_entry(uk_lcpu_regs_get(regs, PC));

	while (fp && depth_left-- > 0) {
		if (unlikely(fp < frame_len)) {
			crash_printk(" Bad frame pointer\n");
			break;
		}

		frame = (void *)(fp - frame_len);

#if CONFIG_LIBUKNOFAULT
		if (uk_nofault_probe_r((__uptr)frame, frame_len, 0) != frame_len) {
			crash_printk(" Bad frame pointer\n");
			break;
		}
#endif /* CONFIG_LIBUKNOFAULT */

		cdmp_gen_print_call_trace_entry(frame[1] > 0 ?
						frame[1] - 1 :
						frame[1]);
		fp = frame[0];
	}
}
#endif /* CONFIG_LIBUKDEBUG_CRASH_PRINT_CALL_TRACE && !__OMIT_FRAMEPOINTER__ */
