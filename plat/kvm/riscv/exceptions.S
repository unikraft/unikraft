/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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

#include <uk/arch/limits.h>
#include <uk/arch/lcpu.h>
#include <uk/asm.h>
#include <uk/config.h>

.section .text
.balign 8
GLOBAL(__trap_handler)
    /* Push registers to stack */
    addi sp, sp, -32 * REGBYTES
    sd zero, 31*REGBYTES(sp)
    sd tp, 28*REGBYTES(sp)
    sd ra, 27*REGBYTES(sp)
    sd s11, 26*REGBYTES(sp)
    sd s10, 25*REGBYTES(sp)
    sd s9, 24*REGBYTES(sp)
    sd s8, 23*REGBYTES(sp)
    sd s7, 22*REGBYTES(sp)
    sd s6, 21*REGBYTES(sp)
    sd s5, 20*REGBYTES(sp)
    sd s4, 19*REGBYTES(sp)
    sd s3, 18*REGBYTES(sp)
    sd s2, 17*REGBYTES(sp)
    sd s1, 16*REGBYTES(sp)
    sd s0, 15*REGBYTES(sp)
    sd a7, 14*REGBYTES(sp)
    sd a6, 13*REGBYTES(sp)
    sd a5, 12*REGBYTES(sp)
    sd a4, 11*REGBYTES(sp)
    sd a3, 10*REGBYTES(sp)
    sd a2, 9*REGBYTES(sp)
    sd a1, 8*REGBYTES(sp)
    sd a0, 7*REGBYTES(sp)
    sd t6, 6*REGBYTES(sp)
    sd t5, 5*REGBYTES(sp)
    sd t4, 4*REGBYTES(sp)
    sd t3, 3*REGBYTES(sp)
    sd t2, 2*REGBYTES(sp)
    sd t1, 1*REGBYTES(sp)
    sd t0, 0*REGBYTES(sp)

    /* Save old pc and sp */
    csrr t0, sepc
    sd t0, 30*REGBYTES(sp)
    addi t0, sp, 32 * REGBYTES
    sd t0, 29*REGBYTES(sp)

    mv a0, sp
    call _trap_handler

    /* Pop registers from stack */
    ld t0, 0*REGBYTES(sp)
    ld t1, 1*REGBYTES(sp)
    ld t2, 2*REGBYTES(sp)
    ld t3, 3*REGBYTES(sp)
    ld t4, 4*REGBYTES(sp)
    ld t5, 5*REGBYTES(sp)
    ld t6, 6*REGBYTES(sp)
    ld a0, 7*REGBYTES(sp)
    ld a1, 8*REGBYTES(sp)
    ld a2, 9*REGBYTES(sp)
    ld a3, 10*REGBYTES(sp)
    ld a4, 11*REGBYTES(sp)
    ld a5, 12*REGBYTES(sp)
    ld a6, 13*REGBYTES(sp)
    ld a7, 14*REGBYTES(sp)
    ld s0, 15*REGBYTES(sp)
    ld s1, 16*REGBYTES(sp)
    ld s2, 17*REGBYTES(sp)
    ld s3, 18*REGBYTES(sp)
    ld s4, 19*REGBYTES(sp)
    ld s5, 20*REGBYTES(sp)
    ld s6, 21*REGBYTES(sp)
    ld s7, 22*REGBYTES(sp)
    ld s8, 23*REGBYTES(sp)
    ld s9, 24*REGBYTES(sp)
    ld s10, 25*REGBYTES(sp)
    ld s11, 26*REGBYTES(sp)
    ld ra, 27*REGBYTES(sp)
    ld tp, 28*REGBYTES(sp)
    addi sp, sp, 32 * REGBYTES
    sret


