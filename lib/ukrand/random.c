/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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
#include <uk/config.h>
#include <uk/init.h>
#include <uk/syscall.h>
#include <sys/random.h>
#include <uk/essentials.h>
#include <uk/swrand.h>
#include <uk/hwrand.h>
#include <uk/entropy.h>

static int _uk_random_generator_init() {
	int ret;
	#ifdef CONFIG_LIBUKRAND_HARDWARE_RANDOMNESS
		ret = _uk_hwrand_init();
		if (ret) {
			goto exit;
		}
	#endif

	#ifdef CONFIG_LIBUKRAND_SOFTWARE_RANDOMNESS
		ret = _uk_swrand_init();
		if (ret) {
			goto exit;
		}
	#endif

exit:
	return ret;

}

__u32 uk_get_estimated_entropy(void) {
	#ifdef CONFIG_LIBUKRAND_HARDWARE_RANDOMNESS
		return uk_entropy_get_estimated_entropy();
	#endif

	#ifdef CONFIG_LIBUKRAND_SOFTWARE_RANDOMNESS
		return 0;
	#endif	
}

UK_SYSCALL_R_DEFINE(ssize_t, getrandom,
		    void *, buf, size_t, buflen,
		    unsigned int, flags)
{	
	size_t offset = 0;
	#ifdef CONFIG_LIBUKRAND_HARDWARE_RANDOMNESS
		offset = uk_hwrand_generate_bytes(buf, buflen);

	#endif

	#ifdef CONFIG_LIBUKRAND_SOFTWARE_RANDOMNESS
		if(offset < buflen) {
			offset += uk_swrand_generate_bytes(buf + offset, buflen - offset);

		}
	#endif

	return offset;
}

uk_early_initcall(_uk_random_generator_init);
