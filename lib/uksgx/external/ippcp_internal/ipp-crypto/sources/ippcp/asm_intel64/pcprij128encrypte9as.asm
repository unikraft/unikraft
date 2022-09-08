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
;        Encrypt_RIJ128_AES_NI()
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
;* Purpose:    single block RIJ128 Cipher
;*
;* void Encrypt_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                  Ipp32u* outBlk,
;*                                  int nr,
;*                            const Ipp32u* pRKey,
;*                            const Ipp32u Tables[][256])
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsRijndael128EncryptECB
;; Caller = ippsRijndael128EncryptCBC
;; Caller = ippsRijndael128EncryptCFB
;; Caller = ippsRijndael128DecryptCFB
;;
;; Caller = ippsDAARijndael128Update
;; Caller = ippsDAARijndael128Final
;; Caller = ippsDAARijndael128MessageDigest
;;
align IPP_ALIGN_FACTOR
IPPASM Encrypt_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 4

;; rdi:     pInpBlk:  DWORD,    ; input  block address
;; rsi:     pOutBlk:  DWORD,    ; output block address
;; rdx:     nr:           DWORD,    ; number of rounds
;; rcx      pKey:     DWORD     ; key material address

%xdefine SC    (4)


   movdqu   xmm0, oword [rdi]       ; input block

   ;;whitening
   pxor     xmm0, oword [rcx]

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax,[rdx*4]
   lea      rcx,[rcx+rax*4-9*(SC)*4]   ; AES-128-keys

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128
   jz       .key_192

   ;;
   ;; regular rounds
   ;;
.key_256:
   aesenc      xmm0,oword [rcx-4*4*SC]
   aesenc      xmm0,oword [rcx-3*4*SC]
.key_192:
   aesenc      xmm0,oword [rcx-2*4*SC]
   aesenc      xmm0,oword [rcx-1*4*SC]
.key_128:
   aesenc      xmm0,oword [rcx+0*4*SC]
   aesenc      xmm0,oword [rcx+1*4*SC]
   aesenc      xmm0,oword [rcx+2*4*SC]
   aesenc      xmm0,oword [rcx+3*4*SC]
   aesenc      xmm0,oword [rcx+4*4*SC]
   aesenc      xmm0,oword [rcx+5*4*SC]
   aesenc      xmm0,oword [rcx+6*4*SC]
   aesenc      xmm0,oword [rcx+7*4*SC]
   aesenc      xmm0,oword [rcx+8*4*SC]
   ;;
   ;; last rounds
   ;;
   aesenclast  xmm0,oword [rcx+9*4*SC]

   movdqu   oword [rsi], xmm0    ; output block

   REST_XMM
   REST_GPR
   ret
ENDFUNC Encrypt_RIJ128_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_

