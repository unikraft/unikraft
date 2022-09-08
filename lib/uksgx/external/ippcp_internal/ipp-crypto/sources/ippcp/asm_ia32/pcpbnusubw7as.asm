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
;     Purpose:  Cryptography Primitive.
;               Big Number Arithmetic
;
;     Content:
;        cpSubMul_BNU()
;





%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP >= _IPP_W7)
%include "pcpvariant.inc"

segment .text align=IPP_ALIGN_FACTOR

%if (_USE_C_cpSub_BNU_ == 0)

align IPP_ALIGN_FACTOR
IPPASM cpSub_BNU,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pDst  [esp + ARG_1 + 0*sizeof(dword)] ; target address
%xdefine pSrc1 [esp + ARG_1 + 1*sizeof(dword)] ; source address
%xdefine pSrc2 [esp + ARG_1 + 2*sizeof(dword)] ; source address
%xdefine len   [esp + ARG_1 + 3*sizeof(dword)] ; length of BNU

   mov   eax,pSrc1   ; src1
   mov   ebx,pSrc2   ; src2
   mov   edx,pDst    ; dst
   mov   edi,len     ; length
   shl   edi,2

   xor ecx,ecx
   pandn mm0,mm0

align IPP_ALIGN_FACTOR
.main_loop:
   movd     mm1,DWORD [eax + ecx]
   movd     mm2,DWORD [ebx + ecx]

   paddq    mm0,mm1
   psubq    mm0,mm2
   movd     DWORD [edx + ecx],mm0
   pshufw   mm0,mm0,11111110b

   add      ecx,4
   cmp      ecx,edi
   jl       .main_loop

   movd     eax,mm0
   neg      eax

   emms
   REST_GPR
   ret
ENDFUNC cpSub_BNU
%endif

%endif

