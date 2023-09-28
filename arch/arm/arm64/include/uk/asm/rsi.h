/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_RSI_H__
#define __UKARCH_RSI_H__

#define RSI_CMD_ATTESTATION_TOKEN_CONTINUE	0xC4000195
#define RSI_CMD_ATTESTATION_TOKEN_INIT		0xC4000194
#define RSI_CMD_HOST_CALL			0xC4000199
#define RSI_CMD_IPA_STATE_GET			0xC4000198
#define RSI_CMD_IPA_STATE_SET			0xC4000197
#define RSI_CMD_MEASUREMENT_EXTEND		0xC4000193
#define RSI_CMD_MEASUREMENT_READ		0xC4000192
#define RSI_CMD_REALM_CONFIG			0xC4000196
#define RSI_CMD_VERSION				0xC4000190

/**
 * RsiCommandReturnCode type
 * Represents a return code from an RSI command.
 */
#define RSI_SUCCESS				0x0
#define RSI_ERROR_INPUT				0x1
#define RSI_ERROR_STATE				0x2
#define RSI_INCOMPLETE				0x3

/* RIPAS values */
#define RSI_RIPAS_EMPTY				0x0
#define RSI_RIPAS_RAM				0x1
#define RSI_RIPAS_DESTROYED			0x2

/* version shift */
#define RSI_VERSION_MAJOR_SHIFT			16
#define RSI_VERSION_MAJOR_MASK			0x7fff
#define RSI_VERSION_MINOR_SHIFT			0
#define RSI_VERSION_MINOR_MASK			0xffff

#endif /* __UKARCH_RSI_H__ */
