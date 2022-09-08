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
; File: enter_enclave.inc
; Description:
;   The file defines macros used by enter_enclave.asm
;
include se_masm.inc

EENTER_PROLOG MACRO

ifdef I386
    push    ebp
    mov     ebp, esp
    push    ebx
    push    esi
    push    edi
endif

ifdef X64
    mov         [rsp + 1*SE_WORDSIZE], rcx
    mov         [rsp + 2*SE_WORDSIZE], rdx
    mov         [rsp + 3*SE_WORDSIZE], r8
    mov         [rsp + 4*SE_WORDSIZE], r9

    push_reg    rbp

    push_reg    rbx
    push_reg    rsi
    push_reg    rdi
    push_reg    r12
    push_reg    r13
    push_reg    r14
    push_reg    r15

    alloc_stack(40)
    set_frame   rbp, (12 * SE_WORDSIZE)
    .endprolog
endif

    ENDM


EENTER_EPILOG MACRO

    mov          xbx, [xbp - SE_WORDSIZE * 1]
    mov          xsi, [xbp - SE_WORDSIZE * 2]
    mov          xdi, [xbp - SE_WORDSIZE * 3]
ifdef X64
    mov          r12, [xbp - SE_WORDSIZE * 4]
    mov          r13, [xbp - SE_WORDSIZE * 5]
    mov          r14, [xbp - SE_WORDSIZE * 6]
    mov          r15, [xbp - SE_WORDSIZE * 7]
endif
    mov          xsp, xbp
    pop          xbp

;ifdef X64
    ;add         rsp, 40
    ;pop         r15
    ;pop         r14
    ;pop         r13
    ;pop         r12
;endif
    ;pop         xdi
    ;pop         xsi
    ;pop         xbx
    ;mov         xsp, xbp
    ;pop         xbp
;
    ENDM


frame_arg0  EQU    [xbp + 2*SE_WORDSIZE]
frame_arg1  EQU    [xbp + 3*SE_WORDSIZE]
frame_arg2  EQU    [xbp + 4*SE_WORDSIZE]
frame_arg3  EQU    [xbp + 5*SE_WORDSIZE]
frame_arg4  EQU    [xbp + 6*SE_WORDSIZE]

;refer to sgx_error.h
SE_ERROR_READ_LOCK_FAIL EQU 0c0002202h