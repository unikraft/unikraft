/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2021, University POLITEHNICA of Bucharest. All rights reserved.
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

#ifndef __PLAT_CMN_X86_ACPI_H__
#define __PLAT_CMN_X86_ACPI_H__

#include <x86/acpi/sdt.h>
#include <x86/acpi/madt.h>

struct RSDPDescriptor {
	char Signature[8];
	__u8 Checksum;
	char OEMID[6];
	__u8 Revision;
	__u32 RsdtAddress;
} __packed;

struct RSDPDescriptor20 {
	struct RSDPDescriptor v1;

	__u32 Length;
	__u64 XsdtAddress;
	__u8 ExtendedChecksum;
	__u8 Reserved[3];
} __packed;

/**
 * Detect ACPI version and discover ACPI tables.
 *
 * @return 0 on success, -errno otherwise.
 */
int acpi_init(void);

/**
 * Return the detected ACPI version.
 *
 * @return 0 if ACPI is not initialized or initialization failed, ACPI version
 *    otherwise.
 */
int acpi_get_version(void);

#endif /* __PLAT_CMN_X86_ACPI_H__ */
