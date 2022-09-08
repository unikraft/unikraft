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
;               ARCFour
;
;     Content:
;        ARCFourKernel()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP >= _IPP_M5) && (_IPP < _IPP_V8)

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:     RC4 kernel
;*
;* void ARCFourProcessData(const Ipp8u *pSrc, Ipp8u *pDst, int len,
;*                         IppsARCFourState* pCtx)
;*
;***************************************************************

;;
;; Lib = W7
;;
;; Caller = ippsARCFourEncrypt
;; Caller = ippsARCFourDecrypt
;;
IPPASM ARCFourProcessData,PUBLIC
  USES_GPR esi,edi,ebx,ebp

%xdefine pSrc [esp + ARG_1 + 0*sizeof(dword)]
%xdefine pDst [esp + ARG_1 + 1*sizeof(dword)]
%xdefine len  [esp + ARG_1 + 2*sizeof(dword)]
%xdefine pCtx [esp + ARG_1 + 3*sizeof(dword)]

   mov      edx, len    ; data length
   mov      esi, pSrc   ; source data
   mov      edi, pDst   ; target data
   mov      ebp, pCtx   ; context

   test     edx, edx    ; test length
   jz       .quit

   xor      eax, eax
   movzx    eax, byte [ebp+4] ; extract x
   xor      ebx, ebx
   movzx    ebx, byte [ebp+8] ; extract y

   lea      ebp, [ebp+12]        ; sbox

   inc      al                      ; x = (x+1)&0xFF
   movzx    ecx, byte [ebp+eax] ; tx = S[x]

   lea      edx, [esi+edx]          ; store stop data address
   push     edx

;;
;; main code
;;
align IPP_ALIGN_FACTOR
.main_loop:
   add      bl,cl                   ; y = (x+tx)&0xFF
   movzx    edx,byte [ebx+ebp]  ; ty = S[y]

   mov      byte [ebx+ebp],cl   ; S[y] = tx
   mov      byte [eax+ebp],dl   ; S[x] = ty
   add      dl, cl                  ; tmp_idx = (tx+ty)&0xFF

   movzx    edx,byte [edx+ebp]  ; byte of gamma

   add      al,1                    ; next x = (x+1)&0xFF
   xor      dl,byte [esi]       ; gamma ^= src
   lea      esi,[esi+1]
   movzx    ecx,byte [eax+ebp]  ; next tx = S[x]
   cmp      esi, dword [esp]
   mov      byte [edi],dl       ; store result
   lea      edi,[edi+1]
   jb       .main_loop

   lea      ebp, [ebp-12]  ; pointer to context
   pop      edx            ; remove local variable

   dec      eax                  ; actual new x counter
   mov      byte [ebp+4], al  ; update x conter
   mov      byte [ebp+8], bl  ; updtae y counter

.quit:
   REST_GPR
   ret
ENDFUNC ARCFourProcessData

%endif

