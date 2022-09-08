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
;               Rijndael Inverse Cipher function
;
;     Content:
;        Decrypt_RIJ128_AES_NI()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"


;***************************************************************
;* Purpose:    single block RIJ128 Inverse Cipher
;*
;* void Encrypt_RIJ128(const Ipp32u* inpBlk,
;*                           Ipp32u* outBlk,
;*                           int nr,
;*                     const Ipp32u* pRKey,
;*                     const Ipp32u Tables[][256])
;*
;***************************************************************

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128DecryptECB
;; Caller = ippsRijndael128DecryptCBC
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM Decrypt_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi

%xdefine pInpBlk [esp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [esp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [esp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 3*sizeof(dword)] ; key material address

%xdefine SC    (4)


   mov      esi,pInpBlk       ; input data address
   mov      ecx,pKey          ; key material address
   mov      eax,nr            ; number of rounds
   mov      edi,pOutBlk       ; output data address

   lea      edx,[eax*4]

   movdqu   xmm0, oword [esi] ; input block

   ;;whitening
   pxor     xmm0, oword [ecx+edx*4] ; whitening

   cmp      eax,12            ; switch according to number of rounds
   jl       .key_128
   jz       .key_192

   ;;
   ;; regular rounds
   ;;
.key_256:
   aesdec      xmm0,oword [ecx+9*SC*4+4*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4+3*4*SC]
.key_192:
   aesdec      xmm0,oword [ecx+9*SC*4+2*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4+1*4*SC]
.key_128:
   aesdec      xmm0,oword [ecx+9*SC*4-0*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-1*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-2*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-3*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-4*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-5*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-6*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-7*4*SC]
   aesdec      xmm0,oword [ecx+9*SC*4-8*4*SC]
   ;;
   ;; last rounds
   ;;
   aesdeclast  xmm0,oword [ecx+9*SC*4-9*4*SC]

   movdqu   oword [edi], xmm0    ; output block
   REST_GPR
   ret
ENDFUNC Decrypt_RIJ128_AES_NI
%endif

