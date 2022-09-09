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
;               Rijndael Inverse Cipher function
;
;     Content:
;        Decrypt_RIJ128_AES_NI()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_ON_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_Y8)


segment .text align=IPP_ALIGN_FACTOR



;***************************************************************
;* Purpose:    single block RIJ128 Inverse Cipher
;*
;* void Decrypt_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                  Ipp32u* outBlk,
;*                                  int nr,
;*                            const Ipp32u* pRKey,
;*                            const Ipp32u Tables[][256])
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsRijndael128DecryptECB
;; Caller = ippsRijndael128DecryptCBC
;;
align IPP_ALIGN_FACTOR
IPPASM Decrypt_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 4
;; rdi:     pInpBlk:  DWORD,    ; input  block address
;; rsi:     pOutBlk:  DWORD,    ; output block address
;; rdx:     nr:           DWORD,    ; number of rounds
;; rcx      pKey:     DWORD     ; key material address

%xdefine SC    (4)


   lea      rax,[rdx*SC]
   lea      rax,[rax*4]

   movdqu   xmm0, oword [rdi]       ; input block

   ;;whitening
   pxor     xmm0, oword [rcx+rax]

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128
   jz       .key_192

   ;;
   ;; regular rounds
   ;;
.key_256:
   aesdec      xmm0,oword [rcx+9*SC*4+4*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4+3*SC*4]
.key_192:
   aesdec      xmm0,oword [rcx+9*SC*4+2*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4+1*SC*4]
.key_128:
   aesdec      xmm0,oword [rcx+9*SC*4-0*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-1*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-2*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-3*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-4*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-5*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-6*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-7*SC*4]
   aesdec      xmm0,oword [rcx+9*SC*4-8*SC*4]
   ;;
   ;; last rounds
   ;;
   aesdeclast  xmm0,oword [rcx+9*SC*4-9*SC*4]

   movdqu   oword [rsi], xmm0    ; output block

   REST_XMM
   REST_GPR
   ret
ENDFUNC Decrypt_RIJ128_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_

