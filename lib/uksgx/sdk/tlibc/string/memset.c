/*	$OpenBSD: memset.c,v 1.7 2015/08/31 02:53:57 guenther Exp $ */
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef _TLIBC_USE_INTEL_FAST_STRING_
extern void *_intel_fast_memset(void *, void *, size_t);
#else
extern void *__memset(void *dst, int c, size_t n);
#endif

extern void* __memcpy_verw(void *dst0, const void *src0);
extern void* __memcpy_8a(void *dst0, const void *src0);

void *
memset_verw(void *dst, int c, size_t len)
{
    char* dst0 = dst;
    if (len == 0 || dst == NULL)
    {
        return dst;
    }
    unsigned char tt = (unsigned char)c;
    uint64_t tmp = 0;
    memset((void*)&tmp, c, 8);
    while (len >= 8) {
        if((unsigned long long)dst0%8 == 0) {
            // 8-byte-aligned - don't need <VERW><MFENCE LFENCE> bracketing
            __memcpy_8a(dst0, (void*)&tmp);
            dst0 += 8;
            len -= 8;
        }
        else{
            // not 8-byte-aligned - need <VERW><MFENCE LFENCE> bracketing
            __memcpy_verw(dst0, (void *)&tt);
            dst0++;
            len--;
        }
    }
    // less than 8 bytes left - need <VERW> <MFENCE LFENCE> bracketing
    for (unsigned i = 0; i < len; i++) {
            __memcpy_verw(dst0, (void *)&tt);
            dst0++;
    }
    return dst;
}

void *
memset(void *dst, int c, size_t n)
{
#ifdef _TLIBC_USE_INTEL_FAST_STRING_
	return _intel_fast_memset(dst, (void*)c, n);
#else
	return __memset(dst, c, n);
#endif /* !_TLIBC_USE_INTEL_FAST_STRING_ */	
}
