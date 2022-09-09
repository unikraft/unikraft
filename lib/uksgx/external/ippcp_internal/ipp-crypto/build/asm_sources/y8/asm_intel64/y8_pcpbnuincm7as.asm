;===============================================================================
; Copyright 2014-2021 Intel Corporation
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

;
;
;     Purpose:  Cryptography Primitive.
;               Big Number Operations
;
;     Content:
;        cpInc_BNU()
;        cpDec_BNU()
;
;

%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

segment .text align=IPP_ALIGN_FACTOR


;*************************************************************
;* Ipp64u cpInc_BNU(Ipp64u* pDst,
;*             const Ipp64u* pSrc, int len,
;*                   Ipp64u increment)
;* returns carry
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpInc_BNU,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 4

; rdi = pDst
; rsi = pSrc
; rdx = len
; rcx = increment

   movsxd   rdx, edx    ; length

   mov      r8, qword [rsi]     ; r[0] = r[0]+increment
   add      r8, rcx
   mov      qword [rdi], r8

   lea      rsi, [rsi+rdx*sizeof(qword)]
   lea      rdi, [rdi+rdx*sizeof(qword)]
   lea      rcx, [rdx*sizeof(qword)]

   sbb      rax, rax                ; save cf
   neg      rcx                     ; rcx = negative length (bytes)
   add      rcx, sizeof(qword)
   jrcxz    .exit
   add      rax, rax                ; restore cf
   jnc      .copy

align IPP_ALIGN_FACTOR
.inc_loop:
   mov      r8, qword [rsi+rcx]
   adc      r8, 0
   mov      qword [rdi+rcx], r8
   lea      rcx, [rcx+sizeof(qword)]
   jrcxz    .exit_loop
   jnc      .exit_loop
   jmp      .inc_loop
.exit_loop:
   sbb      rax, rax                ; save cf

.copy:
   cmp      rsi, rdi
   jz       .exit
   jrcxz    .exit
.copy_loop:
   mov      r8, qword [rsi+rcx]
   mov      qword [rdi+rcx], r8
   add      rcx, sizeof(qword)
   jnz      .copy_loop

.exit:
   neg      rax
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpInc_BNU



;*************************************************************
;* Ipp64u cpDec_BNU(Ipp64u* pDst,
;*             const Ipp64u* pSrc, int len,
;*                   Ipp64u increment)
;* returns borrow
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpDec_BNU,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 4

; rdi = pDst
; rsi = pSrc
; rdx = len
; rcx = increment

   movsxd   rdx, edx    ; length

   mov      r8, qword [rsi]     ; r[0] = r[0]+increment
   sub      r8, rcx
   mov      qword [rdi], r8

   lea      rsi, [rsi+rdx*sizeof(qword)]
   lea      rdi, [rdi+rdx*sizeof(qword)]
   lea      rcx, [rdx*sizeof(qword)]

   sbb      rax, rax                ; save cf
   neg      rcx                     ; rcx = negative length (bytes)
   add      rcx, sizeof(qword)
   jrcxz    .exit
   add      rax, rax                ; restore cf
   jnc      .copy

align IPP_ALIGN_FACTOR
.inc_loop:
   mov      r8, qword [rsi+rcx]
   sbb      r8, 0
   mov      qword [rdi+rcx], r8
   lea      rcx, [rcx+sizeof(qword)]
   jrcxz    .exit_loop
   jnc      .exit_loop
   jmp      .inc_loop
.exit_loop:
   sbb      rax, rax                ; save cf

.copy:
   cmp      rsi, rdi
   jz       .exit
   jrcxz    .exit
.copy_loop:
   mov      r8, qword [rsi+rcx]
   mov      qword [rdi+rcx], r8
   add      rcx, sizeof(qword)
   jnz      .copy_loop

.exit:
   neg      rax
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpDec_BNU

%endif

