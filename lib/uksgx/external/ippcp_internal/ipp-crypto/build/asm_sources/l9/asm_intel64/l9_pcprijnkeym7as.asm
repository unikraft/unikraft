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
;               Rijndael Key Expansion Support
;
;     Content:
;        SubsDword_8uT()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

segment .text align=IPP_ALIGN_FACTOR

%xdefine CACHE_LINE_SIZE  (64)

;***************************************************************
;* Purpose:    Mitigation of the Key Expansion procedure
;*
;* Ipp32u Touch_SubsDword_8uT(Ipp32u inp,
;*                      const Ipp8u* pTbl,
;*                            int tblBytes)
;***************************************************************
align IPP_ALIGN_FACTOR
IPPASM Touch_SubsDword_8uT,PUBLIC
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 3
;; rdi:     inp:      DWORD,     ; input dword
;; rsi:     pTbl: BYTE,      ; Rijndael's S-box
;; edx      tblLen:   DWORD      ; length of table (bytes)

   movsxd   r8, edx              ; length
   xor      rcx, rcx
.touch_tbl:
   mov      rax, [rsi+rcx]
   add      rcx, CACHE_LINE_SIZE
   cmp      rcx, r8
   jl       .touch_tbl

   mov      rax, rdi
   and      rax, 0FFh         ; b[0]
   movzx    rax, BYTE [rsi+rax]

   shr      rdi, 8
   mov      r9, rdi
   and      r9, 0FFh          ; b[1]
   movzx    r9, BYTE [rsi+r9]
   shl      r9, 8

   shr      rdi, 8
   mov      rcx, rdi
   and      rcx, 0FFh         ; b[2]
   movzx    rcx, BYTE [rsi+rcx]
   shl      rcx, 16

   shr      rdi, 8
   mov      rdx, rdi
   and      rdx, 0FFh         ; b[3]
   movzx    rdx, BYTE [rsi+rdx]
   shl      rdx, 24

   or       rax, r9
   or       rax, rcx
   or       rax, rdx
   REST_XMM
   REST_GPR
   ret
ENDFUNC Touch_SubsDword_8uT

%endif

