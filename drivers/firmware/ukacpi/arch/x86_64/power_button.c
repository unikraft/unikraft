/* SPDX-License-Identifier: BSD-3-Clause */

/* ACPI fixed-hardware power button, using the firmware's legacy I/O blocks.
 * Control-method buttons, hardware-reduced ACPI and MMIO GAS are not handled.
 * The interrupt only requests shutdown; application termination and writeback
 * belong to the existing power-management and process termination contracts.
 */
#include <errno.h>
#include <uk/acpi.h>
#include <uk/arch/util.h>
#include <uk/bitops.h>
#include <uk/init.h>
#include <uk/intctlr.h>
#include <uk/plat/time.h>
#include <uk/print.h>

#define PM1_PWRBTN UK_BIT(8)
#define PM1_SCI_EN 1U

static struct {
	__u16 events[2];
	__u16 enables[2];
	unsigned int irq;
	int registered;
} button;

static int power_button_irq(void *arg __unused)
{
	unsigned int pressed = 0;

	for (unsigned int i = 0; i < 2; ++i) {
		if (!button.events[i])
			continue;
		if (uk_arch_x86_64_inw(button.events[i]) & PM1_PWRBTN) {
			/* Write-one-to-clear only our own event. */
			uk_arch_x86_64_outw(button.events[i], PM1_PWRBTN);
			pressed = 1;
		}
	}
	if (!pressed)
		return 0;
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	uk_pr_info("ACPI fixed power button received on SCI %u\n", button.irq);
#endif
	uk_pm_raise_shutdown_event(UK_PM_SHUTDOWN_OP_SYSHALT);
	return 1;
}

static int power_button_init(struct uk_init_ctx *ctx __unused)
{
	const struct uk_acpi_fadt *fadt = uk_acpi_get_fadt();
	__nsec deadline;
	__u32 ports[2];
	int rc;

	if (!fadt || fadt->hdr.tab_len < __offsetof(struct uk_acpi_fadt, flags)
					 + sizeof(fadt->flags))
		return -ENODEV;
	if (fadt->flags & (UK_ACPI_FADT_FLAGS_PWR_BUTTON |
			   UK_ACPI_FADT_FLAGS_HW_REDUCED))
		return -ENOTSUP;
	if (!fadt->sci_irq || fadt->sci_irq >= 16 || fadt->pm1_evt_sz != 4 ||
	    fadt->pm1_ctlr_sz < 2 || !fadt->pm1a_ctlr_blk ||
	    fadt->pm1a_ctlr_blk > 0xfffe)
		return -ENOTSUP;
	ports[0] = fadt->pm1a_evt_blk;
	ports[1] = fadt->pm1b_evt_blk;
	if (!ports[0])
		return -ENOTSUP;
	for (unsigned int i = 0; i < 2; ++i) {
		if (ports[i] > 0xfffc)
			return -ENOTSUP;
		button.events[i] = ports[i];
	}
	button.irq = fadt->sci_irq;
	if (!(uk_arch_x86_64_inw(fadt->pm1a_ctlr_blk) & PM1_SCI_EN)) {
		if (!fadt->smi_cmd || fadt->smi_cmd > 0xffff ||
		    !fadt->acpi_enable)
			return -ENOTSUP;
		uk_arch_x86_64_outb(fadt->smi_cmd, fadt->acpi_enable);
		deadline = ukplat_monotonic_clock() + 1000000000ULL;
		while (!(uk_arch_x86_64_inw(fadt->pm1a_ctlr_blk) & PM1_SCI_EN))
			if (ukplat_monotonic_clock() >= deadline)
				return -ETIMEDOUT;
	}
	rc = uk_intctlr_irq_register(button.irq, power_button_irq, &button);
	if (rc)
		return rc;
	button.registered = 1;
	for (unsigned int i = 0; i < 2; ++i) {
		if (!button.events[i])
			continue;
		button.enables[i] = uk_arch_x86_64_inw(button.events[i] + 2);
		uk_arch_x86_64_outw(button.events[i], PM1_PWRBTN);
		uk_arch_x86_64_outw(button.events[i] + 2,
				    button.enables[i] | PM1_PWRBTN);
	}
#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	uk_pr_info("ACPI fixed power button armed: SCI=%u PM1=%x,%x\n",
		   button.irq, button.events[0], button.events[1]);
#endif
	return 0;
}

static void power_button_term(struct uk_term_ctx *ctx __unused)
{
	if (!button.registered)
		return;
	for (unsigned int i = 0; i < 2; ++i)
		if (button.events[i])
			uk_arch_x86_64_outw(button.events[i] + 2,
					    button.enables[i]);
	uk_intctlr_irq_unregister_arg(button.irq, power_button_irq, &button);
	button.registered = 0;
}

/* Late enough for the controller, before application startup. Reverse-order
 * teardown leaves the process termination phase responsible for its own I/O.
 */
uk_sys_initcall(power_button_init, power_button_term);
