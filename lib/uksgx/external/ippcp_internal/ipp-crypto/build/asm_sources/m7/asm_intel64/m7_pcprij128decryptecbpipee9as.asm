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
;        DecryptECB_RIJ128pipe_AES_NI()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_ON_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_Y8)


segment .text align=IPP_ALIGN_FACTOR

;***************************************************************
;* Purpose:    pipelined RIJ128 ECB decryption
;*
;* void DecryptECB_RIJ128pipe_AES_NI(const Ipp32u* inpBlk,
;*                                         Ipp32u* outBlk,
;*                                         int nr,
;*                                   const Ipp32u* pRKey,
;*                                         int len)
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsAESDecryptECB
;;
align IPP_ALIGN_FACTOR
IPPASM DecryptECB_RIJ128pipe_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 5
;; rdi:     pInpBlk:  DWORD,    ; input  blocks address
;; rsi:     pOutBlk:  DWORD,    ; output blocks address
;; rdx:     nr:           DWORD,    ; number of rounds
;; rcx      pKey:     DWORD     ; key material address
;; r8d      length:       DWORD     ; length (bytes)

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP  (BYTES_PER_BLK*BLKS_PER_LOOP)

   lea      rax,[rdx*SC]               ; keys offset

   movsxd   r8, r8d
   sub      r8, BYTES_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
;ALIGN IPP_ALIGN_FACTOR
.blks_loop:
   lea      r9,[rcx+rax*4]             ; set pointer to the key material
   movdqa   xmm4, oword [r9]       ; keys for whitening
   sub      r9, 16

   movdqu   xmm0, oword [rdi+0*BYTES_PER_BLK]  ; get input blocks
   movdqu   xmm1, oword [rdi+1*BYTES_PER_BLK]
   movdqu   xmm2, oword [rdi+2*BYTES_PER_BLK]
   movdqu   xmm3, oword [rdi+3*BYTES_PER_BLK]
   add      rdi, BYTES_PER_LOOP

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [r9]       ; pre load operation's keys
   sub      r9, 16

   mov      r10, rdx                   ; counter depending on key length
   sub      r10, 1
;ALIGN IPP_ALIGN_FACTOR
.cipher_loop:
   aesdec      xmm0, xmm4              ; regular round
   aesdec      xmm1, xmm4
   aesdec      xmm2, xmm4
   aesdec      xmm3, xmm4
   movdqa      xmm4, oword [r9]    ; pre load operation's keys
   sub         r9, 16
   dec         r10
   jnz         .cipher_loop

   aesdeclast  xmm0, xmm4                 ; irregular round
   movdqu      oword [rsi+0*BYTES_PER_BLK], xmm0  ; store output blocks

   aesdeclast  xmm1, xmm4
   movdqu      oword [rsi+1*BYTES_PER_BLK], xmm1

   aesdeclast  xmm2, xmm4
   movdqu      oword [rsi+2*BYTES_PER_BLK], xmm2

   aesdeclast  xmm3, xmm4
   movdqu      oword [rsi+3*BYTES_PER_BLK], xmm3

   add         rsi, BYTES_PER_LOOP
   sub         r8, BYTES_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      r8, BYTES_PER_LOOP
   jz       .quit

   lea      r9,[rcx+rax*4]             ; set pointer to the key material
align IPP_ALIGN_FACTOR
.single_blk_loop:
   movdqu   xmm0, oword [rdi]       ; get input block
   add      rdi,  BYTES_PER_BLK
   pxor     xmm0, oword [r9]       ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesdec      xmm0, oword [rcx+9*SC*4+4*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4+3*SC*4]
.key_192_s:
   aesdec      xmm0, oword [rcx+9*SC*4+2*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4+1*SC*4]
.key_128_s:
   aesdec      xmm0, oword [rcx+9*SC*4-0*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-1*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-2*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-3*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-4*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-5*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-6*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-7*SC*4]
   aesdec      xmm0, oword [rcx+9*SC*4-8*SC*4]
   aesdeclast  xmm0, oword [rcx+9*SC*4-9*SC*4]

   movdqu      oword [rsi], xmm0    ; save output block
   add         rsi, BYTES_PER_BLK

   sub         r8, BYTES_PER_BLK
   jnz         .single_blk_loop

.quit:
   pxor  xmm4, xmm4

   REST_XMM
   REST_GPR
   ret
ENDFUNC DecryptECB_RIJ128pipe_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_

