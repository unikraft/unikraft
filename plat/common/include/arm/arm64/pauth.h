/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, Michalis Pappas <mpappas@fastmail.fm>.
 * All rights reserved.
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
#ifndef __PLAT_COMMON_ARM64_PAUTH_H__
#define __PLAT_COMMON_ARM64_PAUTH_H__

/* Generate a PAuth key
 *
 * Generate an 128-bit key for PAC generation and verification. This
 * function should be implemented by the platform. Failure to generate
 * a key must be handled internally to this function in a platform-specific
 * way.
 *
 * NOTICE: Definition of this function must use the __no_pauth attribute.
 *         See arch/arm/arm64/compiler.h
 *
 * @param [in/out] key_hi upper 64 key bits
 * @param [in/out] key_lo lower 64 key bits
 */
void ukplat_pauth_gen_key(__u64 *key_hi, __u64 *key_lo);

/* Initialize and enable Pointer Authentication
 *
 * Initializes and enables Pointer Authentication. This function
 * generates keys by calling ukplat_pauth_gen_key(), programs keys,
 * and enables pointer authentication.
 *
 * @return 0 on success, -1 if FEAT_PAUTH is not implemented.
 */
int ukplat_pauth_init(void);

#endif /* __PLAT_COMMON_ARM64_PAUTH_H__ */
