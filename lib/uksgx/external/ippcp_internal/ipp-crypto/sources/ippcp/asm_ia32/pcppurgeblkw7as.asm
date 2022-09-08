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
%include "ia_emm.inc"

%if (_IPP >= _IPP_W7)

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:     Clear memory block
;*
;* void PurgeBlock(Ipp8u *pDst, int len)
;*
;***************************************************************

;;
;; Lib = W7
;;
align IPP_ALIGN_FACTOR
IPPASM PurgeBlock,PUBLIC
  USES_GPR edi

%xdefine pDst [esp + ARG_1 + 0*sizeof(dword)] ; target address
%xdefine len  [esp + ARG_1 + 1*sizeof(dword)] ; length

   mov      edi, pDst   ; mem address
   mov      ecx, len    ; length
   xor      eax, eax
   sub      ecx, sizeof(dword)
   jl       .test_purge
.purge4:
   mov      dword [edi], eax  ; clear
   add      edi, sizeof(dword)
   sub      ecx, sizeof(dword)
   jge      .purge4

.test_purge:
   add      ecx, sizeof(dword)
   jz       .quit
.purge1:
   mov      byte [edi], al
   add      edi, sizeof(byte)
   sub      ecx, sizeof(byte)
   jg       .purge1

.quit:
   REST_GPR
   ret
ENDFUNC PurgeBlock

%endif

