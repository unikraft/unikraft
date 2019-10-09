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

#endif /* __ARM_ARM_TIME_H */
