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
;        DecryptECB_RIJ128pipe_AES_NI()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"

;***************************************************************
;* Purpose:    pipelined RIJ128 ECB decryption
;*
;* void DecryptECB_RIJ128pipe_AES_NI(const Ipp32u* inpBlk,
;*                                         Ipp32u* outBlk,
;*                                         int nr,
;*                                   const Ipp32u* pRKey,
;*                                         int len)
;***************************************************************

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128DecryptECB
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM DecryptECB_RIJ128pipe_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pInpBlk [esp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [esp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [esp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [esp + ARG_1 + 4*sizeof(dword)] ; length(byte)

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP  (BYTES_PER_BLK*BLKS_PER_LOOP)

   mov      esi,pInpBlk       ; input data address
   mov      edi,pOutBlk       ; output data address
   mov      ecx,pKey          ; key material address
   mov      edx,len           ; length
   mov      eax,nr            ; number of rounds

   sub      edx, BYTES_PER_LOOP
   jl       .short_input

   lea      ebx,[eax*SC]      ; keys offset
   lea      ecx,[ecx+ebx*4]
;;
;; pipelined processing
;;
.blks_loop:
   movdqa   xmm4, oword [ecx]      ; keys for whitening
   lea      ebx, [ecx-16]              ; set pointer to the round's key material

   movdqu   xmm0, oword [esi+0*BYTES_PER_BLK]  ; get input blocks
   movdqu   xmm1, oword [esi+1*BYTES_PER_BLK]
   movdqu   xmm2, oword [esi+2*BYTES_PER_BLK]
   movdqu   xmm3, oword [esi+3*BYTES_PER_BLK]
   add      esi, BYTES_PER_LOOP

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [ebx]      ; pre load round keys
   sub      ebx, 16

   mov      eax, nr                    ; counter depending on key length
   sub      eax, 1
.cipher_loop:
   aesdec      xmm0, xmm4              ; regular round
   aesdec      xmm1, xmm4
   aesdec      xmm2, xmm4
   aesdec      xmm3, xmm4
   movdqa      xmm4, oword [ebx]   ; pre load operation's keys
   sub         ebx, 16
   dec         eax
   jnz         .cipher_loop

   aesdeclast  xmm0, xmm4              ; irregular round
   movdqu      oword [edi+0*BYTES_PER_BLK], xmm0  ; store output blocks
   aesdeclast  xmm1, xmm4
   movdqu      oword [edi+1*BYTES_PER_BLK], xmm1
   aesdeclast  xmm2, xmm4
   movdqu      oword [edi+2*BYTES_PER_BLK], xmm2
   aesdeclast  xmm3, xmm4
   movdqu      oword [edi+3*BYTES_PER_BLK], xmm3

   add         edi, BYTES_PER_LOOP
   sub         edx, BYTES_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      edx, BYTES_PER_LOOP
   jz       .quit

   mov      eax, nr
   mov      ecx, pKey
   lea      ebx,[eax*SC]               ; set pointer to the key material
   lea      ebx,[ecx+ebx*4]

.single_blk_loop:
   movdqu   xmm0, oword [esi]       ; get input block
   add      esi,  BYTES_PER_BLK
   pxor     xmm0, oword [ebx]      ; whitening

   cmp      eax,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesdec      xmm0, oword [ecx+9*SC*4+4*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4+3*SC*4]
.key_192_s:
   aesdec      xmm0, oword [ecx+9*SC*4+2*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4+1*SC*4]
.key_128_s:
   aesdec      xmm0, oword [ecx+9*SC*4-0*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-1*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-2*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-3*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-4*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-5*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-6*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-7*SC*4]
   aesdec      xmm0, oword [ecx+9*SC*4-8*SC*4]
   aesdeclast  xmm0, oword [ecx+9*SC*4-9*SC*4]

   movdqu      oword [edi], xmm0    ; save output block
   add         edi, BYTES_PER_BLK

   sub         edx, BYTES_PER_BLK
   jnz         .single_blk_loop

.quit:
   REST_GPR
   ret
ENDFUNC DecryptECB_RIJ128pipe_AES_NI
%endif

