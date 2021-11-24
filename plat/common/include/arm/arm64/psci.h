/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Jia He <justin.he@arm.com>
 *          Răzvan Vîrtan <virtanrazvan@gmail.com>
 *
 * Copyright (c) 2021, Arm Ltd., University Politehnica of Bucharest. All rights reserved.
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
 */

#ifndef __ARM64_PSCI_H__
#define __ARM64_PSCI_H__

/*
 * PSCI function codes (as per PSCI v0.2).
 */
#define PSCI_FNID_VERSION               0x84000000
#define PSCI_FNID_CPU_SUSPEND           0xc4000001
#define PSCI_FNID_CPU_OFF               0x84000002
#define PSCI_FNID_CPU_ON                0xc4000003
#define PSCI_FNID_AFFINITY_INFO         0xc4000004
#define PSCI_FNID_MIGRATE               0xc4000005
#define PSCI_FNID_MIGRATE_INFO_TYPE     0x84000006
#define PSCI_FNID_MIGRATE_INFO_UP_CPU   0xc4000007
#define PSCI_FNID_SYSTEM_OFF            0x84000008
#define PSCI_FNID_SYSTEM_RESET          0x84000009
#define PSCI_FNID_FEATURES              0x8400000a

#endif /* __ARM64_PSCI_H__ */
