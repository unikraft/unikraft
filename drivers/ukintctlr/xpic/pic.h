/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_INTCTLR_PIC_H__
#define __UK_INTCTLR_PIC_H__

int pic_init(struct uk_intctlr_driver_ops **ops);

void pic_ack_irq(unsigned int irq);

#endif /* __UK_INTCTLR_PIC_H__ */
