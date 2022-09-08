;===============================================================================
; Copyright 2015-2021 Intel Corporation
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
;     Purpose:  Cryptography Primitive.
;               Big Number Arithmetic
;
;     Content:
;        cpMulDgt_BNU()
;        cpAddMulDgt_BNU()
;        cpSubMulDgtBNU()
;





%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP >= _IPP_W7)
%include "pcpvariant.inc"

segment .text align=IPP_ALIGN_FACTOR

%if (_USE_C_cpAddMulDgt_BNU_ == 0)
;;
;; Ipp32u cpAddMulDgt_BNU(Ipp32u* pDst, const Ipp32u* pSrc, cpSize len, Ipp32u dgt)
;;
IPPASM cpAddMulDgt_BNU,PUBLIC
  USES_GPR edi

%xdefine pDst [esp + ARG_1 + 0*sizeof(dword)] ; target address
%xdefine pSrc [esp + ARG_1 + 1*sizeof(dword)] ; source address
%xdefine len  [esp + ARG_1 + 2*sizeof(dword)] ; BNU length
%xdefine dgt  [esp + ARG_1 + 3*sizeof(dword)] ; 32-bit multiplier

   mov      eax,pSrc    ; src
   mov      edx,pDst    ; dst
   mov      edi,len     ; length

   xor      ecx,ecx
   shl      edi,2
   movd     mm0,dgt
   pandn    mm7,mm7 ;c

.main_loop:
   movd     mm1,DWORD [eax + ecx]
   movd     mm2,DWORD [edx + ecx]
   pmuludq  mm1,mm0
   paddq    mm7,mm1
   paddq    mm7,mm2
   movd     DWORD [edx + ecx],mm7
   psrlq    mm7,32
   add      ecx,4
   cmp      ecx,edi
   jl       .main_loop

   movd     eax,mm7

   emms
   REST_GPR
   ret
ENDFUNC cpAddMulDgt_BNU
%endif

%endif

