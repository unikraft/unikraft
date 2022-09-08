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
%include "ia_emm.inc"


;***************************************************************
;* Purpose:    AES-CMAC update
;*
;* void cpAESCMAC_Update_AES_NI(Ipp8u* digest,
;*                       const  Ipp8u* input,
;*                              int    inpLen,
;*                                     int nr,
;*                               const Ipp32u* pRKey)
;***************************************************************

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsAES_CMACUpdate
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM cpAESCMAC_Update_AES_NI,PUBLIC
  USES_GPR esi,edi

%xdefine pDigest [esp + ARG_1 + 0*sizeof(dword)] ; input/output digest
%xdefine pInpBlk [esp + ARG_1 + 1*sizeof(dword)] ; input blocks
%xdefine len     [esp + ARG_1 + 2*sizeof(dword)] ; length (bytes)
%xdefine nr      [esp + ARG_1 + 3*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 4*sizeof(dword)] ; key material address

%xdefine SC  (4)
%assign BYTES_PER_BLK  (16)

   mov      edi, pDigest      ; pointer to digest
   mov      esi,pInpBlk       ; input data address
   mov      ecx,pKey          ; key material address
   mov      eax,nr            ; number of rounds

   movdqu   xmm0, oword [edi]   ; digest

   mov      edx, len          ; length of stream

align IPP_ALIGN_FACTOR
;;
;; block-by-block processing
;;
.blks_loop:
   movdqu   xmm1, oword [esi]       ; input block

   movdqa   xmm4, oword [ecx]       ; preload key material

   pxor     xmm0, xmm1                 ; digest ^ src[]

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

   mov         ecx, pKey               ; restore key pointer
   mov         eax, nr                 ; resrore number of rounds

   add         esi, BYTES_PER_BLK      ; advance pointers
   sub         edx, BYTES_PER_BLK      ; decrease counter
   jnz         .blks_loop

   pxor        xmm4, xmm4
   movdqu      oword [edi], xmm0    ; store output block

   REST_GPR
   ret
ENDFUNC cpAESCMAC_Update_AES_NI
%endif

