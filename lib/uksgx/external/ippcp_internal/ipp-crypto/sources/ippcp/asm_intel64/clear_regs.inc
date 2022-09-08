;===============================================================================
; Copyright 2020-2021 Intel Corporation
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;===============================================================================

%ifndef _CLEAR_REGS_ASM_
%define _CLEAR_REGS_ASM_

%include "os.inc"

;
; This macro clears any GP registers passed
;
%macro clear_gps 1-16
%define %%NUM_REGS %0
%rep %%NUM_REGS
        xor %1, %1
%rotate 1
%endrep
%endmacro

;
; This macro clears any XMM registers passed on SSE
;
%macro clear_xmms_sse 1-16
%define %%NUM_REGS %0
%rep %%NUM_REGS
        pxor    %1, %1
%rotate 1
%endrep
%endmacro

;
; This macro clears any XMM registers passed on AVX
;
%macro clear_xmms_avx 1-16
%define %%NUM_REGS %0
%rep %%NUM_REGS
        vpxor   %1, %1
%rotate 1
%endrep
%endmacro

;
; This macro clears any YMM registers passed
;
%macro clear_ymms 1-16
%define %%NUM_REGS %0
%rep %%NUM_REGS
        vpxor   %1, %1
%rotate 1
%endrep
%endmacro

;
; This macro clears any ZMM registers passed
;
%macro clear_zmms 1-32
%define %%NUM_REGS %0
%rep %%NUM_REGS
        vpxorq  %1, %1
%rotate 1
%endrep
%endmacro

;
; This macro clears all scratch GP registers
; for Windows or Linux
;
%macro clear_scratch_gps_asm 0
        clear_gps rax, rcx, rdx, r8, r9, r10, r11
%ifdef LINUX
        clear_gps rdi, rsi
%endif
%endmacro

;
; This macro clears all scratch XMM registers on SSE
;
%macro clear_scratch_xmms_sse_asm 0
%ifdef LINUX
%assign i 0
%rep 16
        pxor    xmm %+ i, xmm %+ i
%assign i (i+1)
%endrep
; On Windows, XMM0-XMM5 registers are scratch registers
%else
%assign i 0
%rep 6
        pxor    xmm %+ i, xmm %+ i
%assign i (i+1)
%endrep
%endif ; LINUX
%endmacro

;
; This macro clears all scratch XMM registers on AVX
;
%macro clear_scratch_xmms_avx_asm 0
%ifdef LINUX
        vzeroall
; On Windows, XMM0-XMM5 registers are scratch registers
%else
%assign i 0
%rep 6
        vpxor   xmm %+ i, xmm %+ i
%assign i (i+1)
%endrep
%endif ; LINUX
%endmacro

;
; This macro clears all scratch YMM registers
;
; It should be called before restoring the XMM registers
; for Windows (XMM6-XMM15)
;
%macro clear_scratch_ymms_asm 0
; On Linux, all YMM registers are scratch registers
%ifdef LINUX
        vzeroall
; On Windows, YMM0-YMM5 registers are scratch registers.
; YMM6-YMM15 upper 128 bits are scratch registers too, but
; the lower 128 bits are to be restored after calling these function
; which clears the upper bits too.
%else
%assign i 0
%rep 6
        vpxor   ymm %+ i, ymm %+ i
%assign i (i+1)
%endrep
%endif ; LINUX
%endmacro

;
; This macro clears all scratch ZMM registers
;
; It should be called before restoring the XMM registers
; for Windows (XMM6-XMM15). YMM registers are used
; on purpose, since XOR'ing YMM registers is faster
; than XOR'ing ZMM registers, and the operation clears
; also the upper 256 bits
;
%macro clear_scratch_zmms_asm 0
; On Linux, all ZMM registers are scratch registers
%ifdef LINUX
        vzeroall
        ;; vzeroall only clears the first 16 ZMM registers
%assign i 16
%rep 16
        vpxorq  ymm %+ i, ymm %+ i
%assign i (i+1)
%endrep
; On Windows, ZMM0-ZMM5 and ZMM16-ZMM31 registers are scratch registers.
; ZMM6-ZMM15 upper 384 bits are scratch registers too, but
; the lower 128 bits are to be restored after calling these function
; which clears the upper bits too.
%else
%assign i 0
%rep 6
        vpxorq  ymm %+ i, ymm %+ i
%assign i (i+1)
%endrep

%assign i 16
%rep 16
        vpxorq  ymm %+ i, ymm %+ i
%assign i (i+1)
%endrep
%endif ; LINUX
%endmacro

%endif ;; _CLEAR_REGS_ASM
