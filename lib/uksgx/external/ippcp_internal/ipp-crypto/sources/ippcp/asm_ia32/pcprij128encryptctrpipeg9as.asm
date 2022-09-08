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
;        EncryptCTR_RIJ128pipe_AES_NI()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"




;***************************************************************
;* Purpose:    pipelined RIJ128 CTR encryption/decryption
;*
;* void EncryptCTR_RIJ128pipe_AES_NI(const Ipp32u* inpBlk,
;*                                         Ipp32u* outBlk,
;*                                         int nr,
;*                                   const Ipp32u* pRKey,
;*                                         int length,
;*                                         Ipp8u* pCtrValue,
;*                                         Ipp8u* pCtrBitMask)
;***************************************************************

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128EncryptCTR
;;

segment .text align=IPP_ALIGN_FACTOR
align IPP_ALIGN_FACTOR
CONST_TABLE:
u128_str DB 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

align IPP_ALIGN_FACTOR
IPPASM EncryptCTR_RIJ128pipe_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk     [ebp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk     [ebp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr          [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey        [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len         [ebp + ARG_1 + 4*sizeof(dword)] ; number of blocks being processed
%xdefine pCtrValue   [ebp + ARG_1 + 5*sizeof(dword)] ; pointer to the Counter
%xdefine pCtrBitMask [ebp + ARG_1 + 6*sizeof(dword)] ; pointer to the Counter Bit Mask

%xdefine SC             (4)
%assign  BLKS_PER_LOOP  (4)
%assign  BYTES_PER_BLK  (16)
%assign  BYTES_PER_LOOP (BYTES_PER_BLK*BLKS_PER_LOOP)

   mov      esi, pCtrBitMask
   mov      edi, pCtrValue
   movdqu   xmm6, oword [esi]       ; counter bit mask
   movdqu   xmm1, oword [edi]       ; initial counter
   movdqu   xmm5, xmm6                 ; counter bit mask
   pandn    xmm6, xmm1                 ; counter template

   sub      esp, (4*4)                 ; allocate stack

   LD_ADDR  eax, CONST_TABLE           ; load bswap conversion tbl
   movdqa   xmm4, oword [eax+(u128_str - CONST_TABLE)]

   ;;
   ;; init counter
   ;;
   mov      edx, dword [edi]
   mov      ecx, dword [edi+4]
   mov      ebx, dword [edi+8]
   mov      eax, dword [edi+12]
   bswap    edx
   bswap    ecx
   bswap    ebx
   bswap    eax

   mov      dword [esp], eax       ; store counter
   mov      dword [esp+4], ebx
   mov      dword [esp+8], ecx
   mov      dword [esp+12], edx

   mov      esi,pInpBlk       ; input data address
   mov      edi,pOutBlk       ; output data address

   sub      dword len, BYTES_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
.blks_loop:
   mov      eax, dword [esp]        ; get coutnter value
   mov      ebx, dword [esp+4]
   mov      ecx, dword [esp+8]
   mov      edx, dword [esp+12]

   pinsrd   xmm0, eax, 0               ; set counter value
   pinsrd   xmm0, ebx, 1
   pinsrd   xmm0, ecx, 2
   pinsrd   xmm0, edx, 3
   pshufb   xmm0, xmm4                 ; convert int the octet string
   pand     xmm0, xmm5                 ; select counter bits
   por      xmm0, xmm6                 ; add unchanged bits

   add      eax, 1                     ; increment counter
   adc      ebx, 0
   adc      ecx, 0
   adc      edx, 0
   pinsrd   xmm1, eax, 0               ; set counter value
   pinsrd   xmm1, ebx, 1
   pinsrd   xmm1, ecx, 2
   pinsrd   xmm1, edx, 3
   pshufb   xmm1, xmm4                 ; convert int the octet string
   pand     xmm1, xmm5                 ; select counter bits
   por      xmm1, xmm6                 ; add unchanged bits

   add      eax, 1                     ; increment counter
   adc      ebx, 0
   adc      ecx, 0
   adc      edx, 0
   pinsrd   xmm2, eax, 0               ; set counter value
   pinsrd   xmm2, ebx, 1
   pinsrd   xmm2, ecx, 2
   pinsrd   xmm2, edx, 3
   pshufb   xmm2, xmm4                 ; convert int the octet string
   pand     xmm2, xmm5                 ; select counter bits
   por      xmm2, xmm6                 ; add unchanged bits

   add      eax, 1                     ; increment counter
   adc      ebx, 0
   adc      ecx, 0
   adc      edx, 0
   pinsrd   xmm3, eax, 0               ; set counter value
   pinsrd   xmm3, ebx, 1
   pinsrd   xmm3, ecx, 2
   pinsrd   xmm3, edx, 3
   pshufb   xmm3, xmm4                 ; convert int the octet string
   pand     xmm3, xmm5                 ; select counter bits
   por      xmm3, xmm6                 ; add unchanged bits

   add      eax, 1                     ; increment counter
   adc      ebx, 0
   adc      ecx, 0
   adc      edx, 0
   mov      dword [esp], eax        ; and store for next itteration
   mov      dword [esp+4], ebx
   mov      dword [esp+8], ecx
   mov      dword [esp+12], edx

   mov      ecx, pKey                  ; get key material address
   movdqa   xmm7, oword [ecx]
   lea      ebx, [ecx+16]              ; pointer to the round's key material

   pxor     xmm0, xmm7                 ; whitening
   pxor     xmm1, xmm7
   pxor     xmm2, xmm7
   pxor     xmm3, xmm7

   movdqa   xmm7, oword [ebx]       ; pre load round keys
   add      ebx, 16

   mov      eax, nr                    ; counter depending on key length
   sub      eax, 1
.cipher_loop:
   aesenc      xmm0, xmm7              ; regular round
   aesenc      xmm1, xmm7
   aesenc      xmm2, xmm7
   aesenc      xmm3, xmm7
   movdqa      xmm7, oword [ebx]
   add         ebx, 16
   dec         eax
   jnz         .cipher_loop

   aesenclast  xmm0, xmm7              ; irregular round
   aesenclast  xmm1, xmm7
   aesenclast  xmm2, xmm7
   aesenclast  xmm3, xmm7

   movdqu      xmm7, oword [esi+0*BYTES_PER_BLK]  ; xor input blocks
   pxor        xmm0, xmm7
   movdqu      oword [edi+0*BYTES_PER_BLK], xmm0

   movdqu      xmm7, oword [esi+1*BYTES_PER_BLK]
   pxor        xmm1, xmm7
   movdqu      oword [edi+1*BYTES_PER_BLK], xmm1

   movdqu      xmm7, oword [esi+2*BYTES_PER_BLK]
   pxor        xmm2, xmm7
   movdqu      oword [edi+2*BYTES_PER_BLK], xmm2

   movdqu      xmm7, oword [esi+3*BYTES_PER_BLK]
   pxor        xmm3, xmm7
   movdqu      oword [edi+3*BYTES_PER_BLK], xmm3

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

   mov      ecx,pKey          ; key material address

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      eax, nr
   lea      ebx,[eax*4]
   lea      ebx,[ecx+ebx*4-9*(SC)*4]  ; AES-128 round keys

.single_blk_loop:
   movdqu   xmm0, oword [esp]       ; get counter value
   pshufb   xmm0, xmm4                 ; convert int the octet string
   pand     xmm0, xmm5                 ; select counter bits
   por      xmm0, xmm6                 ; add unchanged bits

   pxor     xmm0, oword [ecx]       ; whitening

   cmp      eax,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc      xmm0,oword [ebx-4*4*SC]
   aesenc      xmm0,oword [ebx-3*4*SC]
.key_192_s:
   aesenc      xmm0,oword [ebx-2*4*SC]
   aesenc      xmm0,oword [ebx-1*4*SC]
.key_128_s:
   aesenc      xmm0,oword [ebx+0*4*SC]
   aesenc      xmm0,oword [ebx+1*4*SC]
   aesenc      xmm0,oword [ebx+2*4*SC]
   aesenc      xmm0,oword [ebx+3*4*SC]
   aesenc      xmm0,oword [ebx+4*4*SC]
   aesenc      xmm0,oword [ebx+5*4*SC]
   aesenc      xmm0,oword [ebx+6*4*SC]
   aesenc      xmm0,oword [ebx+7*4*SC]
   aesenc      xmm0,oword [ebx+8*4*SC]
   aesenclast  xmm0,oword [ebx+9*4*SC]

   add         dword [esp], 1       ; advance counter value
   adc         dword [esp+4], 0
   adc         dword [esp+8], 0
   adc         dword [esp+12], 0

   sub         dword len, BYTES_PER_BLK
   jl          .partial_block

   movdqu      xmm1, oword [esi]    ; input block
   add         esi, BYTES_PER_BLK
   pxor        xmm0, xmm1              ; output block
   movdqu      oword [edi], xmm0    ; save output block
   add         edi, BYTES_PER_BLK

   cmp         dword len, 0
   jz          .quit
   jmp         .single_blk_loop

.partial_block:
   add         dword len, BYTES_PER_BLK

.partial_block_loop:
   pextrb      eax, xmm0, 0
   psrldq      xmm0, 1
   movzx       ebx, byte [esi]
   xor         eax, ebx
   mov         byte [edi], al
   inc         esi
   inc         edi
   dec         dword len
   jnz         .partial_block_loop

.quit:
   mov         eax, pCtrValue
   movdqu      xmm0, oword [esp]       ; get counter value
   pshufb      xmm0, xmm4                 ; convert int the octet string
   pand        xmm0, xmm5                 ; select counter bits
   por         xmm0, xmm6                 ; add unchanged bits
   movdqu      oword [eax], xmm0       ; return updated counter

   add      esp, (4*4)                    ; free stack
   REST_GPR
   ret
ENDFUNC EncryptCTR_RIJ128pipe_AES_NI
%endif

