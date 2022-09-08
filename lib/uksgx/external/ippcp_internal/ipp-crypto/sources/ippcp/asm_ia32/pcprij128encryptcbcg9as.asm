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
;               Rijndael Cipher function
;
;     Content:
;        EncryptCBC_RIJ128pipe_AES_NI()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"

;***************************************************************
;* Purpose:    RIJ128 CBC encryption
;*
;* void EncryptCBC_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                     Ipp32u* outBlk,
;*                                     int nr,
;*                                const Ipp32u* pRKey,
;*                                      int len,
;*                                const Ipp8u* pIV)
;***************************************************************

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128EncryptCBC
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM EncryptCBC_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi

%xdefine pInpBlk [esp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [esp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [esp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [esp + ARG_1 + 4*sizeof(dword)] ; length (bytes)
%xdefine pIV     [esp + ARG_1 + 5*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BYTES_PER_BLK  (16)

   mov      edx, pIV          ; IV address
   mov      esi,pInpBlk       ; input data address
   mov      ecx,pKey          ; key material address
   mov      eax,nr            ; number of rounds
   mov      edi,pOutBlk       ; output data address

   movdqu   xmm0, oword [edx]   ; IV

   mov      edx, len          ; length of stream

align IPP_ALIGN_FACTOR
;;
;; block-by-block processing
;;
.blks_loop:
   movdqu   xmm1, oword [esi]       ; input block

   movdqa   xmm4, oword [ecx]       ; preload key material

   pxor     xmm0, xmm1                 ; src[] ^ iv

   pxor     xmm0, xmm4                 ; whitening

   movdqa   xmm4, oword [ecx+16]    ; preload key material
   add      ecx, 16

   sub      eax, 1                     ; counter depending on key length
align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc      xmm0, xmm4              ; regular round
   movdqa      xmm4, oword [ecx+16]
   add         ecx, 16
   sub         eax, 1
   jnz         .cipher_loop
   aesenclast  xmm0, xmm4              ; irregular round

   movdqu      oword [edi], xmm0    ; store output block

   mov         ecx, pKey               ; restore key pointer
   mov         eax, nr                 ; resrore number of rounds

   add         esi, BYTES_PER_BLK      ; advance pointers
   add         edi, BYTES_PER_BLK
   sub         edx, BYTES_PER_BLK      ; decrease counter
   jnz         .blks_loop

   REST_GPR
   ret
ENDFUNC EncryptCBC_RIJ128_AES_NI
%endif

