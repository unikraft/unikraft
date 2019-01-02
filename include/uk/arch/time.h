/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UKARCH_TIME_H__
#define __UKARCH_TIME_H__

#include <uk/arch/types.h>
#include <uk/arch/limits.h>

/*
 * System Time
 * 64 bit value containing the nanoseconds elapsed since boot time.
 * This value is adjusted by frequency drift.
 * NOW() returns the current time.
 * The other macros are for convenience to approximate short intervals
 * of real time into system time
 */
typedef __u64 __nsec;
typedef __s64 __snsec;

#define __PRInsec __PRIu64
#define __PRIsnsec __PRIs64

#define __NSEC_MAX (__U64_MAX)
#define __SNSEC_MAX (__S64_MAX)
#define __SNSEC_MIN (__S64_MIN)

#define UKARCH_NSEC_PER_SEC 1000000000ULL

#define ukarch_time_nsec_to_sec(ns)      ((ns) / UKARCH_NSEC_PER_SEC)
#define ukarch_time_nsec_to_msec(ns)     ((ns) / 1000000ULL)
#define ukarch_time_nsec_to_usec(ns)     ((ns) / 1000UL)
#define ukarch_time_subsec(ns)           ((ns) % 1000000000ULL)

#define ukarch_time_sec_to_nsec(sec)     ((sec)  * UKARCH_NSEC_PER_SEC)
#define ukarch_time_msec_to_nsec(msec)   ((msec) * 1000000UL)
#define ukarch_time_usec_to_nsec(usec)   ((usec) * 1000UL)

#endif /* __UKARCH_TIME_H__ */
