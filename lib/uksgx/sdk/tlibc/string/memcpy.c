/*	$OpenBSD: memcpy.c,v 1.2 2015/08/31 02:53:57 guenther Exp $ */
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
#include "sgx_trts.h"
#include <stdbool.h>

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef	long word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)

#ifdef _TLIBC_USE_INTEL_FAST_STRING_
extern void *_intel_fast_memcpy(void *, void *, size_t);
#endif


/*
 * Copy a block of memory, not handling overlap.
 */
void *
__memcpy(void *dst0, const void *src0, size_t length)
{
	char *dst = (char *)dst0;
	const char *src = (const char *)src0;
	size_t t;

	if (length == 0 || dst == src)		/* nothing to do */
		goto done;

	/*
	 * Macros: loop-t-times; and loop-t-times, t>0
	 */
#define	TLOOP(s) if (t) TLOOP1(s)
#define	TLOOP1(s) do { s; } while (--t)

	/*
	 * Copy forward.
	 */
	t = (long)src;	/* only need low bits */
	if ((t | (long)dst) & wmask) {
		/*
		 * Try to align operands.  This cannot be done
		 * unless the low bits match.
		 */
		if ((t ^ (long)dst) & wmask || length < wsize)
			t = length;
		else
			t = wsize - (t & wmask);
		length -= t;
		TLOOP1(*dst++ = *src++);
	}
	/*
	 * Copy whole words, then mop up any trailing bytes.
	 */
	t = length / wsize;
	TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
	t = length & wmask;
	TLOOP(*dst++ = *src++);
done:
	return (dst0);
}

extern void* __memcpy_verw(void *dst0, const void *src0);
extern void* __memcpy_8a(void *dst0, const void *src0);
// use in the enclave when dst is outside the enclave
void* memcpy_verw(void *dst0, const void *src0, size_t len)
{
    char* dst = dst0;
    const char *src = (const char *)src0;
    if(len == 0 || dst == src)
    {
        return dst0;
    }

    while (len >= 8) {
        if((unsigned long long)dst%8 == 0) {
            // 8-byte-aligned - don't need <VERW><MFENCE LFENCE> bracketing
            __memcpy_8a(dst, src);
            src += 8;
            dst += 8;
            len -= 8;
        }
        else{
            // not 8-byte-aligned - need <VERW><MFENCE LFENCE> bracketing
            __memcpy_verw(dst, src);
            src++;
            dst++;
            len--;
        }
    }
    // less than 8 bytes left - need <VERW> <MFENCE LFENCE> bracketing
    for (unsigned i = 0; i < len; i++) {
            __memcpy_verw(dst, src);
            src++;
            dst++;
    }
    return dst0;
}

void *
memcpy_nochecks(void *dst0, const void *src0, size_t length)
{
#ifdef _TLIBC_USE_INTEL_FAST_STRING_
 	return _intel_fast_memcpy(dst0, (void*)src0, length);
#else
	return __memcpy(dst0, src0, length);
#endif
}


//deal the case that src is outside the enclave, count <= 8
static void
copy_external_memory(void* dst, const void* src, size_t count, bool is_dst_external)
{
    unsigned char tmp_buf[16]={0};
    unsigned int off_src = (unsigned long long)src%8;
    if(count == 0)
    {
        return;
    }
    
    //if src is 8-byte-aligned, copy 8 bytes from outside the enclave to the buffer
    //if src is not 8-byte-aligned and off_src + count > 8, copy 16 bytes from outside the enclave to the buffer
    __memcpy_8a(tmp_buf, src - off_src);
    if(off_src != 0 && off_src + count > 8)
    {
        __memcpy_8a(tmp_buf + 8, src - off_src + 8);
    }
    if(is_dst_external)
    {
        memcpy_verw(dst, tmp_buf + off_src, count);
    }
    else
    {
        memcpy_nochecks(dst, tmp_buf + off_src, count);
    }
    return;
}

void *
memcpy(void *dst0, const void *src0, size_t length)
{
    if(length == 0 || dst0 == src0)
    {
        return dst0;
    }

    bool is_src_external = !sgx_is_within_enclave(src0, length);
    bool is_dst_external = !sgx_is_within_enclave(dst0, length);

    //src is inside the enclave
    if(!is_src_external)
    {
        if(is_dst_external)
        {
            return memcpy_verw(dst0, src0, length);
        }
        else
        {
            return memcpy_nochecks(dst0, src0, length);
        }
    }

    //src is outside the enclave
    unsigned int len = 0;
    char* dst = dst0;
    const char *src = (const char *)src0;
    while(length >= 8)
    {
        len = 8 - (unsigned long long)dst%8;
        copy_external_memory(dst, src, len, is_dst_external);
        src += len;
        dst += len;
        length -= len;
    }
    //less than 8 bytes left
    copy_external_memory(dst, src, length, is_dst_external);

    return dst0;
}
