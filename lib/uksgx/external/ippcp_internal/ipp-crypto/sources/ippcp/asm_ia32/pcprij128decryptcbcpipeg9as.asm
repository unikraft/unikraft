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
;        DecryptCBC_RIJ128pipe_AES_NI()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"


;***************************************************************
;* Purpose:    pipelined RIJ128 CBC decryption
;*
;* void DecryptCBC_RIJ128pipe_AES_NI(const Ipp32u* inpBlk,
;*                                         Ipp32u* outBlk,
;*                                         int nr,
;*                                   const Ipp32u* pRKey,
;*                                         int len,
;*                                   const Ipp8u* pIV)
;***************************************************************

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = G9
;;
;; Caller = ippsRijndael128DecryptCBC
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM DecryptCBC_RIJ128pipe_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [ebp + ARG_1 + 4*sizeof(dword)] ; length(byte)
%xdefine pIV     [ebp + ARG_1 + 5*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP  (BYTES_PER_BLK*BLKS_PER_LOOP)

   mov      esi,pInpBlk       ; input data address
   mov      edi,pOutBlk       ; output data address
   mov      ecx,pKey          ; key material address
   mov      eax,nr            ; number of rounds

   sub      esp, 16*(1+4+1)   ; allocate stack
   lea      edx, [esp+16]
   and      edx, -16

   mov      ebx, pIV
   movdqu   xmm4, oword [ebx]      ; save IV
   movdqa   oword [edx+0*16], xmm4 ; into the stack

   sub      dword len, BYTES_PER_LOOP
   jl       .short_input

   lea      ebx,[eax*SC]      ; keys offset
   lea      ecx,[ecx+ebx*4]
;;
;; pipelined processing
;;
.blks_loop:
   movdqa   xmm4, oword [ecx]      ; keys for whitening
   lea      ebx, [ecx-16]              ; set pointer to the round's key material

   movdqu   xmm0, oword [esi+0*BYTES_PER_BLK]   ; get input blocks
   movdqu   xmm1, oword [esi+1*BYTES_PER_BLK]
   movdqu   xmm2, oword [esi+2*BYTES_PER_BLK]
   movdqu   xmm3, oword [esi+3*BYTES_PER_BLK]
   movdqa   oword [edx+1*16], xmm0             ; and save as IVx
   movdqa   oword [edx+2*16], xmm1             ; for next operations
   movdqa   oword [edx+3*16], xmm2             ; into the stack
   movdqa   oword [edx+4*16], xmm3

   pxor     xmm0, xmm4                 ;whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [ebx]      ; pre load operation's keys
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

   aesdeclast  xmm0, xmm4                 ; irregular round and IV
   pxor        xmm0, oword [edx+0*16]  ; xor with IV
   movdqu      oword [edi+0*16], xmm0  ; and store output blocls

   aesdeclast  xmm1, xmm4
   pxor        xmm1, oword [edx+1*16]
   movdqu      oword [edi+1*16], xmm1

   aesdeclast  xmm2, xmm4
   pxor        xmm2, oword [edx+2*16]
   movdqu      oword [edi+2*16], xmm2

   aesdeclast  xmm3, xmm4
   pxor        xmm3, oword [edx+3*16]
   movdqu      oword [edi+3*16], xmm3

   movdqa      xmm4, oword [edx+4*16]     ; update IV
   movdqa      oword [edx+0*16], xmm4

   add         esi, BYTES_PER_LOOP
   add         edi, BYTES_PER_LOOP
   sub         dword len, BYTES_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      dword len, BYTES_PER_LOOP
   jz       .quit

   mov      eax, nr
   mov      ecx, pKey
   lea      ebx,[eax*SC]               ; set pointer to the key material
   lea      ebx,[ecx+ebx*4]

.single_blk_loop:
   movdqu   xmm0, oword [esi]       ; get input block
   movdqa   oword [edx+16], xmm0    ; and save as IV for future
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

   pxor        xmm0, oword [edx+0*16]  ; add IV
   movdqu      oword [edi], xmm0       ; and save output blocl

   movdqa      xmm4, oword [edx+1*16]  ; update IV
   movdqa      oword [edx+0*16], xmm4

   add         esi, BYTES_PER_BLK
   add         edi, BYTES_PER_BLK
   sub         dword len, BYTES_PER_BLK
   jnz         .single_blk_loop

.quit:
   add      esp, 16*(1+4+1)   ; free stack
   REST_GPR
   ret
ENDFUNC DecryptCBC_RIJ128pipe_AES_NI
%endif

