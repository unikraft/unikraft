/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _SGX_CET_H_
#define _SGX_CET_H_

#ifdef __CET__
#if defined(__x86_64) || defined(__x86_64__)
#define _CET_ENDBR endbr64
#else
#define _CET_ENDBR endbr32
#endif
#define _CET_NOTRACK notrack
#else
#define _CET_ENDBR
#define _CET_NOTRACK
#endif

#define NT_GNU_PROPERTY_TYPE_0           0x5
#define GNU_PROPERTY_X86_FEATURE_1_AND   0xc0000002
#if defined(__x86_64) || defined(__x86_64__)
#define SECTION_ALIGNMENT                0x3
#else
#define SECTION_ALIGNMENT                0x2
#endif
#define GNU_PROPERTY_X86_FEATURE_1_IBT   0x1
#define GNU_PROPERTY_X86_FEATURE_1_SHSTK 0x2  
#define GNU_PROPERTY_X86_FEATURE_1_CET   (GNU_PROPERTY_X86_FEATURE_1_IBT | GNU_PROPERTY_X86_FEATURE_1_SHSTK)

    .pushsection ".note.gnu.property", "a"
    .p2align SECTION_ALIGNMENT              /* section alignment */
    .long 1f - 0f                           /* name size (not including padding) */
    .long 4f - 1f                           /* desc size (not including padding) */
    .long NT_GNU_PROPERTY_TYPE_0            /* type NT_GNU_PROPERTY_TYPE_0*/
0:  .asciz "GNU"                            /* name */
1:  .long GNU_PROPERTY_X86_FEATURE_1_AND    /* pr_type */
    .long 3f - 2f                           /* pr_datasz */
2:  .long GNU_PROPERTY_X86_FEATURE_1_CET    /* cet feature bits*/
3:  .p2align SECTION_ALIGNMENT
4:
    .popsection

#endif
