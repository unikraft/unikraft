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
;               Purge block
;
;
;

%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:     Clear memory block
;*
;* void PurgeBlock(Ipp8u *pDst, int len)
;*
;***************************************************************

;;
;; Lib = M7
;;
align IPP_ALIGN_FACTOR
IPPASM PurgeBlock,PUBLIC
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 2
;; rdi:  pDst:  BYTE,    ; mem being clear
;; rsi:  len:      DWORD     ; length

   movsxd   rcx, esi    ; store stream length
   xor      rax, rax
   sub      rcx, sizeof(qword)
   jl       .test_purge
.purge8:
   mov      qword [rdi], rax  ; clear
   add      rdi, sizeof(qword)
   sub      rcx, sizeof(qword)
   jge      .purge8

.test_purge:
   add      rcx, sizeof(qword)
   jz       .quit
.purge1:
   mov      byte [rdi], al
   add      rdi, sizeof(byte)
   sub      rcx, sizeof(byte)
   jg       .purge1

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC PurgeBlock

%endif

