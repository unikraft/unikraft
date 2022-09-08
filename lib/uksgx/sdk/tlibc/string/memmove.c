/*	$OpenBSD: memmove.c,v 1.2 2015/08/31 02:53:57 guenther Exp $ */
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

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef	long word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)


#ifdef _TLIBC_USE_INTEL_FAST_STRING_
extern void *_intel_fast_memmove(void *, void *, size_t);
#else
extern void *__memmove(void *, void *, size_t);
#endif

/*
 * Copy a block of memory, handling overlap.
 */
void *
memmove(void *dst0, const void *src0, size_t length)
{
#ifdef _TLIBC_USE_INTEL_FAST_STRING_
 	return _intel_fast_memmove(dst0, (void*)src0, length);
#else
	return __memmove(dst0, src0, length);
#endif
}

extern void* __memcpy_verw(void *dst0, const void *src0);
extern void* __memcpy_8a(void *dst0, const void *src0);
void *
memmove_verw(void *dst, const void *src, size_t count)
{
    if(count == 0 || dst == src)
    {
        return dst;
    }

    if (dst < src)
    {
        // if src is above dst, we have to copy front to back
        // to avoid overwriting the data we want to copy.
        char* dst0 = (char*)dst;
        const char *src0 = (const char *)src;
        while (count >= 8) {
            if((unsigned long long)dst0%8 == 0) {
                // 8-byte-aligned - don't need <VERW><MFENCE LFENCE> bracketing
                __memcpy_8a(dst0, src0);
                src0 += 8;
                dst0 += 8;
                count -= 8;
            }
            else{
                // not 8-byte-aligned - need <VERW><MFENCE LFENCE> bracketing
                __memcpy_verw(dst0, src0);
                src0++;
                dst0++;
                count--;
            }
        }
        // less than 8 bytes left - need <VERW> <MFENCE LFENCE> bracketing
        for (unsigned i = 0; i < count; i++) {
                __memcpy_verw(dst0, src0);
                src0++;
                dst0++;
        }
    }
    else
    {
        // If dst is above src, we have to copy back to front
        // to avoid overwriting the data we want to copy.
        char* dst0 = (char*)dst + count -1;
        const char *src0 = (const char *)src + count -1;
        while (count >= 8) {
            if((unsigned long long)dst0%8 == 7) {
                // 8-byte-aligned - don't need <VERW><MFENCE LFENCE> bracketing
                __memcpy_8a(dst0 - 7, src0 - 7);
                src0 -= 8;
                dst0 -= 8;
                count -= 8;
            }
            else{
                // not 8-byte-aligned - need <VERW><MFENCE LFENCE> bracketing
                __memcpy_verw(dst0, src0);
                src0--;
                dst0--;
                count--;
            }
        }
        // less than 8 bytes left - need <VERW> <MFENCE LFENCE> bracketing
        for (unsigned i = 0; i < count; i++) {
                __memcpy_verw(dst0, src0);
                src0--;
                dst0--;
        }
    }

    return dst;
}

