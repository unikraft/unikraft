;===============================================================================
; Copyright 2018-2021 Intel Corporation
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
;               Rijndael Cipher function
;
;     Content:
;        cpAESCMAC_Update_AES_NI()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "ia_32e_regs.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_ON_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_Y8)

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:    AES-CMAC update
;*
;* void cpAESCMAC_Update_AES_NI(Ipp8u* digest,
;*                       const  Ipp8u* input,
;*                              int    inpLen,
;*                                     int nr,
;*                               const Ipp32u* pRKey)
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsAES_CMACUpdate
;;
align IPP_ALIGN_FACTOR
IPPASM cpAESCMAC_Update_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 5
;; rdi:     pDigest:  DWORD,    ; input  blocks address
;; rsi:     pInput:   DWORD,    ; output blocks address
;; rdx:     length:       DWORD,    ; lenght in bytes (multiple 16)
;; rcx:     nr:           DWORD     ; number of rounds
;; r8:      pRKey:    DWORD     ; pointer to keys

%xdefine SC  (4)
%assign BYTES_PER_BLK  (16)

   movsxd   rdx, edx                ; input length
   movdqu   xmm0, oword [rdi]   ; digest

align IPP_ALIGN_FACTOR
;;
;; pseudo-pipelined processing
;;
.blks_loop:
   movdqu   xmm1, oword [rsi]       ; input block

   movdqa   xmm4, oword [r8]
   mov      r9, r8                     ; save pointer to the key material

   pxor     xmm0, xmm1                 ; digest ^ src[]

   pxor     xmm0, xmm4                 ; whitening

   movdqa   xmm4, oword [r9+16]
   add      r9, 16

   mov      r10, rcx                   ; counter depending on key length
   sub      r10, 1
align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc      xmm0, xmm4             ; regular round
   movdqa      xmm4, oword [r9+16]
   add         r9, 16
   dec         r10
   jnz         .cipher_loop
   aesenclast  xmm0, xmm4             ; irregular round

   add         rsi, BYTES_PER_BLK     ; advance pointers
   sub         rdx, BYTES_PER_BLK     ; decrease counter
   jnz         .blks_loop

   pxor     xmm4, xmm4
   movdqu   oword [rdi], xmm0     ; store updated digest digest

   REST_XMM
   REST_GPR
   ret
ENDFUNC cpAESCMAC_Update_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_

