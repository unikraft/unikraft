/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Michalis Pappas <mpappas@fastmail.fm>.
 *                     All rights reserved.
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
#ifndef __UKARCH_RANDOM_H__
#define __UKARCH_RANDOM_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_HAVE_RANDOM

#include <uk/asm/random.h>

/**
 * Initialize the RNG
 *
 * @return 0 on success, negative value on failure
 */
int ukarch_random_init(void);

/**
 * Get a 32-bit random integer
 *
 * @param [out] val Pointer to store the generated value
 * @return 0 on success, negative value on failure
 */
int __check_result ukarch_random_u32(__u32 *val);

/**
 * Get a 64-bit random integer
 *
 * @param [out] val Pointer to store the generated value
 * @return 0 on success, negative value on failure
 */
int __check_result ukarch_random_u64(__u64 *val);

/**
 * Get a 32-bit random integer suitable for seeding
 *
 * @param [out] val Pointer to store the generated value
 * @return 0 on success, negative value on failure
 */
int __check_result ukarch_random_seed_u32(__u32 *val);

/**
 * Get a 64-bit random integer suitable for seeding
 *
 * @param [out] val Pointer to store the generated value
 * @return 0 on success, negative value on failure
 */
int __check_result ukarch_random_seed_u64(__u64 *val);

#else /* CONFIG_HAVE_RANDOM */

static inline int __check_result ukarch_random_init(void)
{
	return -1;
}

static inline int __check_result ukarch_random_u32(__u32 *val __unused)
{
	return -1;
}

static inline int __check_result ukarch_random_u64(__u64 *val __unused)
{
	return -1;
}

static inline int __check_result ukarch_random_seed_u32(__u32 *val __unused)
{
	return -1;
}

static inline int __check_result ukarch_random_seed_u64(__u64 *val __unused)
{
	return -1;
}

#endif /* CONFIG_HAVE_RANDOM */

#ifdef __cplusplus
}
#endif

#endif /* __UKARCH_RANDOM_H__ */
