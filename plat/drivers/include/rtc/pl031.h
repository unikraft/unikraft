/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author: Razvan Deaconescu <razvan.deaconescu@cs.pub.ro>
 *
 * Copyright (c) 2022, University POLITEHNICA of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __PLAT_DRV_RTC_PL031_H__
#define __PLAT_DRV_RTC_PL031_H__

#define PL031_STATUS_ENABLED	1
#define PL031_STATUS_DISABLED	0

#include <rtc/rtc.h>

/**
 * Read time from device and store in RTC structure.
 *
 * @param[out] rt Pointer to time structure storing result
 */
void pl031_read_time(struct rtc_time *rt);

/**
 * Write time from RTC structure to device.
 *
 * @param[in] rt Pointer to time structure storing time
 */
void pl031_write_time(struct rtc_time *rt);

/**
 * Set alarm timeout. Interrupt will be delivered at timeout expiry.
 *
 * @param[in] rt Pointer to time structure storing timeout
 */
void pl031_write_alarm(struct rtc_time *rt);

/**
 * Read alarm timeout.
 *
 * @param[out] rt Pointer to time structure storing result
 */
void pl031_read_alarm(struct rtc_time *rt);

/**
 * Enable PL031 device.
 */
void pl031_enable(void);

/**
 * Disable PL031 device.
 */
void pl031_disable(void);

/**
 * Get PL031 device status (enabled or disabled).
 *
 * @return PL031_STATUS_ENABLED or PL031_STATUS_DISABLED if device
 *         is enabled or not
 */
int pl031_get_status(void);

/**
 * Enable interrupt for device.
 */
void pl031_enable_intr(void);

/**
 * Disable interrupt for device.
 */
void pl031_disable_intr(void);

/**
 * Clear interrupt for device.
 */
void pl031_clear_intr(void);

/**
 * Register alarm handler to be called when interrupt is delivered
 * (at alarm expiry).
 *
 * @param[in] handler Alarm handler as function pointer
 * @return <0 for error, 0 for success
 */
int pl031_register_alarm_handler(int (*handler)(void *));

/**
 * Initialize PL031 device. Parse device tree blob (DTB) to extract
 * base address and IRQ.
 *
 * @param[in] dtb Pointer to DTB structure
 * @return <0 for error, 0 for success
 */
int pl031_init_rtc(void *dtb);

#endif /* __PLAT_DRV_RTC_PL031_H__ */
