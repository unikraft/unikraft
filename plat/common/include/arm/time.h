#ifndef __ARM_ARM_TIME_H
#define __ARM_ARM_TIME_H

/* Bits definition of cntv_ctl register */
#define GT_TIMER_ENABLE        0x01
#define GT_TIMER_MASK_IRQ      0x02
#define GT_TIMER_IRQ_STATUS    0x04

#ifdef CONFIG_ARCH_ARM_64
#include <arm/arm64/time.h>
#else
#include <arm/arm/time.h>
#endif /* CONFIG_ARCH_ARM_64 */

void generic_timer_enable(void);
void generic_timer_mask_irq(void);
void generic_timer_unmask_irq(void);
__u64 generic_timer_get_ticks(void);
__u32 generic_timer_get_frequency(int fdt_timer);
int generic_timer_init(int fdt_timer);
int generic_timer_irq_handler(void *arg __unused);
void generic_timer_cpu_block_until(__u64 until_ns);
void generic_timer_update_boot_ticks(void);

#endif /* __ARM_ARM_TIME_H */
