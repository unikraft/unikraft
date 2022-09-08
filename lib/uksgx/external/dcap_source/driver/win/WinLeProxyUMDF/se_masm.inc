;
; Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
;   * Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
;   * Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in
;     the documentation and/or other materials provided with the
;     distribution.
;   * Neither the name of Intel Corporation nor the names of its
;     contributors may be used to endorse or promote products derived
;     from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
; A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
; OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
; LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;


;
; File: se_masm.inc
; Description:
;   The file defines macros used by both uRTS and tRTS masm code.
;

ifdef X64
ifndef __TLIBC__
;; c.f. Macros defined in ksamd64.inc
;; http://msdn.microsoft.com/en-us/library/ms235217.aspx
include  ksamd64.inc
endif
endif

ifdef I386
    UNDER   EQU <_>
    _FRAME_ EQU <>
    SE3func EQU __SE3
endif

ifdef X64
    UNDER   EQU <>
    _FRAME_ EQU <FRAME>
    SE3func EQU _SE3
endif

BEGIN_FUNC MACRO   func
    PUBLIC @catstr(%UNDER,<func>)
    @catstr(%UNDER,<func>) PROC _FRAME_
ENDM

BEGIN_FUNC_NAKED    MACRO   func
    PUBLIC @catstr(%UNDER,<func>)
    @catstr(%UNDER,<func>) PROC
ENDM

END_FUNC    MACRO   func
    @catstr(%UNDER,<func>) ENDP
ENDM

ifdef I386
    xax equ     <eax>
    xbx equ     <ebx>
    xcx equ     <ecx>
    xdx equ     <edx>
    xsi equ     <esi>
    xdi equ     <edi>
    xsp equ     <esp>
    xbp equ     <ebp>

    SE_WORDSIZE equ 4
endif

ifdef X64
    xax equ     <rax>
    xbx equ     <rbx>
    xcx equ     <rcx>
    xdx equ     <rdx>
    xsi equ     <rsi>
    xdi equ     <rdi>
    xsp equ     <rsp>
    xbp equ     <rbp>

    SE_WORDSIZE equ 8
endif

    SE_PAGESIZE equ 1000h

;; Generic argument picker for "naked" functions
ifdef I386
    naked_arg0  EQU     4[esp]
    naked_arg1  EQU     8[esp]
    naked_arg2  EQU    12[esp]
    naked_arg3  EQU    16[esp]
endif

;; In Win64, the first four parameters are passed by RCX, RDX, R8, R9.
ifdef X64
    naked_arg0  EQU     rcx
    naked_arg1  EQU     rdx
    naked_arg2  EQU     r8
    naked_arg3  EQU     r9
endif

;; Prolog and epilog for SE instructions, such as EREPORT, EGETKEY, ECREATE...
;; set xbx = arg0, xcx = arg1, xdx = arg2
SE_PROLOG MACRO
ifdef I386
    push        ebp
    mov         ebp, esp

    push        ebx

    mov         ebx, [xbp + 2*SE_WORDSIZE]
    mov         ecx, [xbp + 3*SE_WORDSIZE]
    mov         edx, [xbp + 4*SE_WORDSIZE]
endif

ifdef X64
    push_reg    rbx
    .endprolog

    mov         rbx, rcx
    mov         rcx, rdx
    mov         rdx, r8
endif
    ENDM

SE_EPILOG MACRO
ifdef I386
    pop         ebx
    mov         esp, ebp
    pop         ebp
endif

ifdef X64
    pop         rbx
endif
    ENDM

;; SE leaf codes - need to sync with inst.h
SE_EREPORT      EQU     0
SE_EGETKEY      EQU     1
SE_EENTER       EQU     2
SE_ERESUME      EQU     3
SE_EEXIT        EQU     4
SE_EACCEPT      EQU     5
SE_EMODPE       EQU     6
SE_EACCEPTCOPY  EQU     7

SE_ECREATE  EQU     0
SE_EADD     EQU     1
SE_EINIT    EQU     2
SE_EREMOVE  EQU     3
SE_EDBGRD   EQU     4
SE_EDBGWR   EQU     5
SE_EEXTEND  EQU     6

ifdef SE_SIM
EXTRN   SE3func:PROC
SE_SIM_EXCEPTION EQU 1
endif

ifdef SE_SIM
SGX_ENCLU   MACRO
        local @call_SE3func, @se3_generic, @check_EEXIT
ifdef SE_SIM_EXCEPTION
        cmp  eax, SE_ERESUME
        jne  @se3_generic
        ;; arrage se instruction here is for align to _do_ecall_filter
        ;; so that _do_ecall_filter can detect it is EREUSME exception
        ;; or exception within enclave.
        DB      00Fh
        DB      001h
        DB      0D7h
;       DB      0F3h
@se3_generic:
endif

        cmp eax, SE_EENTER
        jne @check_EEXIT
        mov xdx, xbp
        jmp @call_SE3func
@check_EEXIT:
        cmp  eax, SE_EEXIT
        jne  @call_SE3func

        ; if inst is EEXIT, xbp and xsp need to be passed by xdx and xcx
        mov xdx, xbp
        mov xcx, xsp

@call_SE3func:
ifdef I386
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
endif

ifdef X64
        sub  rsp, 030h
        mov  [rsp + 4*SE_WORDSIZE], rsi
        mov  [rsp + 5*SE_WORDSIZE], rdi
        mov  r9, rdx
        mov  r8, rcx
        mov  rdx, rbx
        mov  rcx, rax
endif

        call SE3func

        add  xsp, SE_WORDSIZE * 6
        ENDM
else
SGX_ENCLU   MACRO
        DB      00Fh
        DB      001h
        DB      0D7h
;		DB      0F3h
        ENDM
endif

;; vim: set syntax=asm ts=4 sw=4 ai et: