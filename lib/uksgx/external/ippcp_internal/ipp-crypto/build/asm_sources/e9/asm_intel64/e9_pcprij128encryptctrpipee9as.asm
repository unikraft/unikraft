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
;        EncryptCTR_RIJ128pipe_AES_NI()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "ia_32e_regs.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_ON_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_Y8)

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR
u128_str DB 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

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

;;
;; Lib = Y8
;;
;; Caller = ippsAESEncryptCTR
;;
align IPP_ALIGN_FACTOR
IPPASM EncryptCTR_RIJ128pipe_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7,xmm8,xmm9
        COMP_ABI 7
;; rdi:        pInpBlk:     DWORD,    ; pointer to the input
;; rsi:        pOutBlk:     DWORD,    ; pointer to the output
;; rdx:        nr:              DWORD,    ; number of rounds
;; rcx         pKey:        DWORD     ; key material address
;; r8d         length:          DWORD     ; length of the input
;; r9          pCtrValue:   BYTED     ; pointer to the Counter
;; [rsp+ARG_7] pCtrBitMask: BYTE      ; pointer to the Counter Bit Mask

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP  (BYTES_PER_BLK*BLKS_PER_LOOP)

   mov      rax, [rsp+ARG_7]
   movdqu   xmm8, oword [rax]       ; counter bit mask

   movdqu   xmm0, oword [r9]        ; initial counter
   movdqa   xmm9, xmm8
   pandn    xmm9, xmm0                 ; counter template

   ;;
   ;; init counter
   ;;
   mov      rbx, qword [r9]         ; initial counter (BE)
   mov      rax, qword [r9+8]
   bswap    rbx
   bswap    rax

   movsxd   r8, r8d
   sub      r8, BYTES_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
.blks_loop:
   movdqa   xmm4,oword [rel u128_str]

   pinsrq   xmm0, rax, 0               ; get counter value
   pinsrq   xmm0, rbx, 1
   pshufb   xmm0, xmm4                 ; convert int the octet string
   pand     xmm0, xmm8                 ; select counter bits
   por      xmm0, xmm9                 ; add unchanged bits

   add      rax, 1
   adc      rbx, 0
   pinsrq   xmm1, rax, 0
   pinsrq   xmm1, rbx, 1
   pshufb   xmm1, xmm4
   pand     xmm1, xmm8
   por      xmm1, xmm9

   add      rax, 1
   adc      rbx, 0
   pinsrq   xmm2, rax, 0
   pinsrq   xmm2, rbx, 1
   pshufb   xmm2, xmm4
   pand     xmm2, xmm8
   por      xmm2, xmm9

   add      rax, 1
   adc      rbx, 0
   pinsrq   xmm3, rax, 0
   pinsrq   xmm3, rbx, 1
   pshufb   xmm3, xmm4
   pand     xmm3, xmm8
   por      xmm3, xmm9

   movdqa   xmm4, oword [rcx]
   mov      r10, rcx                   ; set pointer to the key material

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [r10+16]
   add      r10, 16

   mov      r11, rdx                      ; counter depending on key length
   sub      r11, 1
.cipher_loop:
   aesenc      xmm0, xmm4                 ; regular round
   aesenc      xmm1, xmm4
   aesenc      xmm2, xmm4
   aesenc      xmm3, xmm4
   movdqa      xmm4, oword [r10+16]
   add         r10, 16
   dec         r11
   jnz         .cipher_loop

   aesenclast  xmm0, xmm4                 ; irregular round
   aesenclast  xmm1, xmm4
   aesenclast  xmm2, xmm4
   aesenclast  xmm3, xmm4

   movdqu      xmm4, oword [rdi+0*BYTES_PER_BLK]  ; 4 input blocks
   movdqu      xmm5, oword [rdi+1*BYTES_PER_BLK]
   movdqu      xmm6, oword [rdi+2*BYTES_PER_BLK]
   movdqu      xmm7, oword [rdi+3*BYTES_PER_BLK]
   add         rdi, BYTES_PER_LOOP

   pxor        xmm0, xmm4                 ; 4 output blocks
   movdqu      oword [rsi+0*BYTES_PER_BLK], xmm0
   pxor        xmm1, xmm5
   movdqu      oword [rsi+1*BYTES_PER_BLK], xmm1
   pxor        xmm2, xmm6
   movdqu      oword [rsi+2*BYTES_PER_BLK], xmm2
   pxor        xmm3, xmm7
   movdqu      oword [rsi+3*BYTES_PER_BLK], xmm3

   add         rax, 1                     ; advance counter
   adc         rbx, 0

   add         rsi, BYTES_PER_LOOP

   sub         r8, BYTES_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      r8, BYTES_PER_LOOP
   jz       .quit

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      r10,[rdx*4]
   lea      r10, [rcx+r10*4-9*(SC)*4]  ; AES-128 round keys

.single_blk_loop:
   pinsrq   xmm0, rax, 0               ; get counter value
   pinsrq   xmm0, rbx, 1
   pshufb   xmm0, [rel u128_str]   ; convert int the octet string
   pand     xmm0, xmm8                 ; select counter bits
   por      xmm0, xmm9                 ; add unchanged bits

   pxor     xmm0, oword [rcx]       ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc      xmm0,oword [r10-4*4*SC]
   aesenc      xmm0,oword [r10-3*4*SC]
.key_192_s:
   aesenc      xmm0,oword [r10-2*4*SC]
   aesenc      xmm0,oword [r10-1*4*SC]
.key_128_s:
   aesenc      xmm0,oword [r10+0*4*SC]
   aesenc      xmm0,oword [r10+1*4*SC]
   aesenc      xmm0,oword [r10+2*4*SC]
   aesenc      xmm0,oword [r10+3*4*SC]
   aesenc      xmm0,oword [r10+4*4*SC]
   aesenc      xmm0,oword [r10+5*4*SC]
   aesenc      xmm0,oword [r10+6*4*SC]
   aesenc      xmm0,oword [r10+7*4*SC]
   aesenc      xmm0,oword [r10+8*4*SC]
   aesenclast  xmm0,oword [r10+9*4*SC]

   add         rax, 1                  ; update counter
   adc         rbx, 0

   sub         r8, BYTES_PER_BLK
   jl          .partial_block

   movdqu      xmm4, oword [rdi]    ; input block
   pxor        xmm0, xmm4              ; output block
   movdqu      oword [rsi], xmm0    ; save output block

   add         rdi, BYTES_PER_BLK
   add         rsi, BYTES_PER_BLK
   cmp         r8, 0
   jz          .quit
   jmp         .single_blk_loop

.partial_block:
   add         r8, BYTES_PER_BLK

.partial_block_loop:
   pextrb      r10d, xmm0, 0
   psrldq      xmm0, 1
   movzx       r11d, byte [rdi]
   xor         r10, r11
   mov         byte [rsi], r10b
   inc         rdi
   inc         rsi
   dec         r8
   jnz         .partial_block_loop

.quit:
   pinsrq      xmm0, rax, 0               ; get counter value
   pinsrq      xmm0, rbx, 1
   pshufb      xmm0, [rel u128_str]   ; convert int the octet string
   pand        xmm0, xmm8                 ; select counter bits
   por         xmm0, xmm9                 ; add unchanged bits
   movdqu      oword [r9], xmm0        ; return updated counter
   REST_XMM
   REST_GPR
   ret
ENDFUNC EncryptCTR_RIJ128pipe_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_


