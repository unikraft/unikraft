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

%if (_IPP >= _IPP_V8)

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:     RC4 kernel
;*
;* void ARCFourProcessData(const Ipp8u *pSrc, Ipp8u *pDst, int len,
;*                         IppsARCFourState* pCtx)
;*
;***************************************************************

;;
;; Lib = V8
;;
;; Caller = ippsARCFourEncrypt
;; Caller = ippsARCFourDecrypt
;;
align IPP_ALIGN_FACTOR
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

   mov      eax, dword [ebp+4] ; extract x
   mov      ebx, dword [ebp+8] ; extract y

   lea      ebp, [ebp+12]        ; sbox

   add      eax,1                      ; x = (x+1)&0xFF
   and      eax, 0FFh
   mov      ecx, dword [ebp+eax*4] ; tx = S[x]

   lea      edx, [esi+edx]             ; store stop data address
   push     edx

;;
;; main code
;;
align IPP_ALIGN_FACTOR
.main_loop:
   add      ebx, ecx                   ; y = (x+tx)&0xFF
   movzx    ebx, bl
   mov      edx, dword [ebp+ebx*4] ; ty = S[y]

   mov      dword [ebp+ebx*4],ecx  ; S[y] = tx
   add      ecx, edx                   ; tmp_idx = (tx+ty)&0xFF
   movzx    ecx, cl
   mov      dword [ebp+eax*4],edx  ; S[x] = ty

   add      eax, 1                     ; next x = (x+1)&0xFF
   mov      dl, byte [ebp+ecx*4]   ; byte of gamma

   movzx    eax, al

   xor      dl,byte [esi]          ; gamma ^= src
   add      esi, 1
   mov      ecx, dword [ebp+eax*4] ; next tx = S[x]
   mov      byte [edi],dl          ; store result
   add      edi, 1
   cmp      esi, dword [esp]
   jb       .main_loop

   lea      ebp, [ebp-12]           ; pointer to context
   pop      edx                     ; remove local variable

   dec      eax                     ; actual new x counter
   and      eax, 0FFh
   mov      dword [ebp+4], eax   ; update x conter
   mov      dword [ebp+8], ebx   ; updtae y counter

.quit:
   REST_GPR
   ret
ENDFUNC ARCFourProcessData

%endif

