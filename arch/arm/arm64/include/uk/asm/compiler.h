/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Michalis Pappas <mpappas@fastmail.fm>.
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
#ifndef __UK_ESSENTIALS_H__
#error Do not include this header directly
#endif

#if CONFIG_ARM64_FEAT_PAUTH
#if CONFIG_ARM64_FEAT_BTI
#define __no_pauth __attribute__((target("branch-protection=bti")))
#else
#define __no_pauth __attribute__((target("branch-protection=none")))
#endif /* CONFIG_ARM64_FEAT_BTI */
#else
#define __no_pauth
#endif /* CONFIG_ARM64_FEAT_PAUTH */

#if CONFIG_ARM64_FEAT_BTI
#if CONFIG_ARM64_FEAT_PAUTH
#define __no_bti __attribute__((target("branch-protection=pac-ret+leaf")))
#else
#define __no_bti __attribute__((target("branch-protection=none")))
#endif /* CONFIG_ARM64_FEAT_PAUTH */
#else
#define __no_bti
#endif /* CONFIG_ARM64_FEAT_BTI */

#if defined(CONFIG_ARM64_FEAT_PAUTH) || defined(CONFIG_ARM64_FEAT_BTI)
#define __no_branch_protection __attribute__((target("branch-protection=none")))
#else
#define __no_branch_protection
#endif /* CONFIG_ARM64_FEAT_PAUTH || CONFIG_ARM64_FEAT_BTI */

