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
;
;     Content:
;        cpInc_BNU()
;





%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP >= _IPP_W7)
%include "pcpvariant.inc"

%if (_USE_C_cpInc_BNU_ == 0)

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM cpInc_BNU,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pDst  [esp + ARG_1 + 0*sizeof(dword)] ; target address
%xdefine pSrc  [esp + ARG_1 + 1*sizeof(dword)] ; source address
%xdefine len   [esp + ARG_1 + 2*sizeof(dword)] ; length of BNU
%xdefine value [esp + ARG_1 + 3*sizeof(dword)] ; increment val

   mov   edi, pDst   ; dst
   mov   esi, pSrc   ; src
   mov   eax, value  ; value
   movd  mm0, value
   mov   edx,len     ; length
   shl   edx,2

   xor   ecx,ecx

align IPP_ALIGN_FACTOR
.main_loop:
   movd     mm1,DWORD [esi + ecx]
   paddq    mm1,mm0
   movd     DWORD [edi + ecx],mm1
   pshufw   mm0,mm1,11111110b
   movd     eax, mm0

   add      ecx,4
   cmp      ecx,edx
   jl       .main_loop

.exit_loop:
   emms
   REST_GPR
   ret
ENDFUNC cpInc_BNU
%endif

%endif

