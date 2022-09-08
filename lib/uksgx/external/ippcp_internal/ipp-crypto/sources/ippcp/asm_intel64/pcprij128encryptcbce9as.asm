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
;               Rijndael Cipher function
;
;     Content:
;        EncryptCBC_RIJ128_AES_NI()
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
;* Purpose:    RIJ128 CBC encryption
;*
;* void EncryptCBC_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                     Ipp32u* outBlk,
;*                                     int nr,
;*                               const Ipp32u* pRKey,
;*                                     int len,
;*                               const Ipp8u* pIV)
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsAESEncryptCBC
;;
align IPP_ALIGN_FACTOR
IPPASM EncryptCBC_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 6
;; rdi:     pInpBlk:  DWORD,    ; input  blocks address
;; rsi:     pOutBlk:  DWORD,    ; output blocks address
;; rdx:     nr:           DWORD,    ; number of rounds
;; rcx      pKey:     DWORD     ; key material address
;; r8d      length:       DWORD     ; length (bytes)
;; r9       pIV:      DWORD     ; IV address

%xdefine SC  (4)
%assign BYTES_PER_BLK  (16)

   movsxd   r8, r8d                 ; input length
   movdqu   xmm0, oword [r9]    ; IV

align IPP_ALIGN_FACTOR
;;
;; pseudo-pipelined processing
;;
.blks_loop:
   movdqu   xmm1, oword [rdi]       ; input block

   movdqa   xmm4, oword [rcx]
   mov      r9, rcx                    ; set pointer to the key material

   pxor     xmm0, xmm1                 ; src[] ^ iv

   pxor     xmm0, xmm4                 ; whitening

   movdqa   xmm4, oword [r9+16]
   add      r9, 16

   mov      r10, rdx                   ; counter depending on key length
   sub      r10, 1
align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc      xmm0, xmm4             ; regular round
   movdqa      xmm4, oword [r9+16]
   add         r9, 16
   dec         r10
   jnz         .cipher_loop
   aesenclast  xmm0, xmm4             ; irregular round

   movdqu      oword [rsi], xmm0    ; store output block

   add         rdi, BYTES_PER_BLK      ; advance pointers
   add         rsi, BYTES_PER_BLK
   sub         r8, BYTES_PER_BLK       ; decrease counter
   jnz         .blks_loop

   pxor  xmm4, xmm4

   REST_XMM
   REST_GPR
   ret
ENDFUNC EncryptCBC_RIJ128_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_

