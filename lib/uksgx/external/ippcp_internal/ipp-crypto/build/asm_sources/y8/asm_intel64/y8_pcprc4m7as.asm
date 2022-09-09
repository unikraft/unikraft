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
;
;     Purpose:  Cryptography Primitive.
;               ARCFour
;
;     Content:
;        ARCFourKernel()
;
;

%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:     RC4 kernel
;*
;* void ARCFourProcessData(const Ipp8u *pSrc, Ipp8u *pDst, int len,
;*                         IppsARCFourState* pCtx)
;*
;***************************************************************

;;
;; Lib = M7
;;
;; Caller = ippsARCFourEncrypt
;; Caller = ippsARCFourDecrypt
;;
align IPP_ALIGN_FACTOR
IPPASM ARCFourProcessData,PUBLIC
        USES_GPR rsi,rdi,rbx,rbp
        USES_XMM
        COMP_ABI 4
;; rdi:  pSrc:  BYTE,    ; input  stream
;; rsi:  pDst:  BYTE,    ; output stream
;; rdx:  len:      DWORD,    ; stream length
;; rcx:  pCtx:  BYTE     ; context

   movsxd   r8, edx
   test     r8, r8      ; test length
   mov      rbp, rcx    ; copy pointer context
   jz       .quit

   movzx    rax, byte [rbp+4]       ; extract x
   movzx    rbx, byte [rbp+8]       ; extract y

   lea      rbp, [rbp+12]              ; sbox

   add      rax,1                      ; x = (x+1)&0xFF
   movzx    rax, al
   movzx    rcx, byte [rbp+rax*4]  ; tx = S[x]

;;
;; main code
;;
align IPP_ALIGN_FACTOR
.main_loop:
   add      rbx, rcx                   ; y = (x+tx)&0xFF
   movzx    rbx, bl
   add      rdi, 1
   add      rsi, 1
   movzx    rdx, byte [rbp+rbx*4]  ; ty = S[y]

   mov      dword [rbp+rbx*4],ecx  ; S[y] = tx
   add      rcx, rdx                   ; tmp_idx = (tx+ty)&0xFF
   movzx    rcx, cl
   mov      dword [rbp+rax*4],edx  ; S[x] = ty

   mov      dl, byte [rbp+rcx*4]   ; byte of gamma
   add      rax, 1                     ; next x = (x+1)&0xFF
   movzx    rax, al

   xor      dl,byte [rdi-1]        ; gamma ^= src
   sub      r8, 1
   movzx    rcx, byte [rbp+rax*4]  ; next tx = S[x]
   mov      byte [rsi-1],dl        ; store result
   jne      .main_loop

   lea      rbp, [rbp-12]           ; pointer to context

   sub      rax, 1                  ; actual new x counter
   movzx    rax, al
   mov      dword [rbp+4], eax   ; update x conter
   mov      dword [rbp+8], ebx   ; updtae y counter

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC ARCFourProcessData

%endif

