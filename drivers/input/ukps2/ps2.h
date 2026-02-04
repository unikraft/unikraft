/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __PS2_H__
#define __PS2_H__

#include <uk/bitops.h>

#define PS2_DATA_REG				0x60

#define PS2_STATUS_REG				0x64
#define PS2_STATUS_RECV_FULL			UK_BIT(0)
#define PS2_STATUS_SEND_FULL			UK_BIT(1)

#define PS2_CMD_REG				0x64
#define PS2_CMD_READ_CFG			0x20
#define PS2_CMD_WRITE_CFG			0x60
#define PS2_CMD_DIS_KBD				0xAD
#define PS2_CMD_EN_KBD				0xAE
#define PS2_CMD_CPU_RESET			0xFE

#define PS2_CFG_REG_EN_KBD_IRQ			UK_BIT(0)
#define PS2_CFG_REG_DIS_KBD_CLK			UK_BIT(4)

/* ACPI scan code set - enough for Firecracker "Send CtrlAltDelete" */
#define PS2_KEY_CTRL				0x0014
#define PS2_KEY_ALT				0x0011
#define PS2_KEY_DEL				0xE071

#endif /*  __PS2_H__ */
