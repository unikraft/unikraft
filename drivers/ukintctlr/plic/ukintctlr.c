#include <errno.h>
#include <uk/config.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/intctlr.h>
#include <uk/asm/lcpu.h>
#include <uk/intctlr/plic.h>
#include <uk/assert.h>

static struct uk_intctlr_desc intctlr;

static struct uk_intctlr_driver_ops plic_ops = {
	.fdt_xlat = __NULL,
	.mask_irq = plic_mask_irq,
	.unmask_irq = plic_clear_irq,
};

static int configure_irq(struct uk_intctlr_irq *irq __unused)
{
	return 0;
}

// used interally
int uk_intctlr_probe(void)
{
	int rc = -ENODEV;
	struct ukplat_bootinfo *bi;

	bi = ukplat_bootinfo_get();

	rc = init_plic((void *)bi->dtb);
	if (rc < 0)
		UK_CRASH("Interrupt controller not found, crashing...\n");

	intctlr.name = "PLIC";
	intctlr.ops = &plic_ops;
	intctlr.ops->configure_irq = configure_irq;

	_csr_set(UK_ARCH_RISCV64_CSR_SIE, UK_ARCH_RISCV64_SIP_SEIP);

	return uk_intctlr_register(&intctlr);
}
