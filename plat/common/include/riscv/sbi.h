/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
 *
 * Authors:
 *	Anup Patel <anup.patel@wdc.com>
 *	Eduard Vintila <eduard.vintila47@gmail.com>
 */

struct sbiret {
	long error;
	long value;
};

/* SBI system reset extension ID */
#define SBI_EXT_SRST 0x53525354

/* SBI reset function ID */
#define SBI_EXT_SRST_RESET 0x0

/* Reset type */
#define SBI_SRST_RESET_TYPE_SHUTDOWN 0x0

/* Reset reason */
#define SBI_SRST_RESET_REASON_NONE 0x0
#define SBI_SRST_RESET_REASON_SYSFAIL 0x1

/* SBI time extension ID */
#define SBI_EXT_TIME 0x54494D45

/* SBI set timer function ID */
#define SBI_EXT_TIME_SET_TIMER 0x0

#define SBI_ECALL(__eid, __fid, __a0, __a1, __a2)                              \
	({                                                                     \
		register unsigned long a0 asm("a0") = (unsigned long)(__a0);   \
		register unsigned long a1 asm("a1") = (unsigned long)(__a1);   \
		register unsigned long a2 asm("a2") = (unsigned long)(__a2);   \
		register unsigned long a6 asm("a6") = (unsigned long)(__fid);  \
		register unsigned long a7 asm("a7") = (unsigned long)(__eid);  \
		asm volatile("ecall"                                           \
			     : "+r"(a0)                                        \
			     : "r"(a1), "r"(a2), "r"(a6), "r"(a7)              \
			     : "memory");                                      \
		a0;                                                            \
	})

#define SBI_ECALL_0(__eid, __fid) SBI_ECALL(__eid, __fid, 0, 0, 0)
#define SBI_ECALL_1(__eid, __fid, __a0) SBI_ECALL(__eid, __fid, __a0, 0, 0)
#define SBI_ECALL_2(__eid, __fid, __a0, __a1)                                  \
	SBI_ECALL(__eid, __fid, __a0, __a1, 0)

#define sbi_system_reset(reset_type, reset_reason)                             \
	SBI_ECALL_2(SBI_EXT_SRST, SBI_EXT_SRST_RESET, reset_type, reset_reason)

#define sbi_set_timer(stime_value)                                             \
	SBI_ECALL_1(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, stime_value)
