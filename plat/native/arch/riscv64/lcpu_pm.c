#include <uk/assert.h>
#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#endif /* CONFIG_LIBUKBOOT */
#include <uk/lcpu/pm.h>
#include <uk/plat/native/except.h>
#include <uk/prio.h>

#if CONFIG_UKPLAT_CPU_MAXCOUNT != 1
#error "plat/native/riscv64 currently supports exactly one hart"
#endif

static void plat_native_lcpu_halt(void)
{
	uk_plat_native_halt();
}

static void plat_native_lcpu_halt_irq(void)
{
	UK_ASSERT(uk_plat_native_irqs_disabled());
	uk_plat_native_halt_irq();
}

static const struct uk_lcpu_pm_ops plat_native_pm_ops = {
	.halt = plat_native_lcpu_halt,
	.halt_irq = plat_native_lcpu_halt_irq,
};

#if CONFIG_LIBUKBOOT
__isr static int
plat_native_lcpu_pm_ops_register(struct ukplat_bootinfo *bi __unused)
{
	return uk_lcpu_pm_ops_register(&plat_native_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(plat_native_lcpu_pm_ops_register, UK_PRIO_EARLIEST);
#endif /* CONFIG_LIBUKBOOT */
