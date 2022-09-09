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
;        DecryptCBC_RIJ128pipe_AES_NI()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_ON_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_Y8)

segment .text align=IPP_ALIGN_FACTOR


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

;;
;; Lib = Y8
;;
;; Caller = ippsAESDecryptCBC
;;

%assign AES_BLOCK  (16)

align IPP_ALIGN_FACTOR
IPPASM DecryptCBC_RIJ128pipe_AES_NI,PUBLIC
%assign LOCAL_FRAME ((1+8)*AES_BLOCK)
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm15
        COMP_ABI 6
;; rdi:     pInpBlk:  DWORD,    ; input  blocks address
;; rsi:     pOutBlk:  DWORD,    ; output blocks address
;; rdx:     nr:           DWORD,    ; number of rounds
;; rcx      pKey:     DWORD     ; key material address
;; r8d      length:       DWORD     ; length (bytes)
;; r9       pIV       BYTE      ; pointer to the IV

%xdefine SC  (4)

   movdqu   xmm15, oword [r9]      ; IV
   movsxd   r8, r8d                    ; processed length

   lea      rax,[rdx*SC]               ; keys offset

   cmp      r8, (4*AES_BLOCK)
   jl       .short123_input
   cmp      r8, (16*AES_BLOCK)
   jle      .block4x

;;
;; 8-blocks processing
;;
   mov      rbx, rsp                   ; rbx points on cipher's data in stack

   sub      rsp, sizeof(oword)*4       ; allocate stack and xmm registers
   movdqa   oword [rsp+0*sizeof(oword)], xmm6
   movdqa   oword [rsp+1*sizeof(oword)], xmm7
   movdqa   oword [rsp+2*sizeof(oword)], xmm8
   movdqa   oword [rsp+3*sizeof(oword)], xmm9

   sub      r8, (8*AES_BLOCK)

align IPP_ALIGN_FACTOR
.blk8_loop:
   lea      r9,[rcx+rax*sizeof(dword)-AES_BLOCK]; pointer to the key material

   movdqu   xmm0, oword [rdi+0*AES_BLOCK]    ; get input blocks
   movdqu   xmm1, oword [rdi+1*AES_BLOCK]
   movdqu   xmm2, oword [rdi+2*AES_BLOCK]
   movdqu   xmm3, oword [rdi+3*AES_BLOCK]
   movdqu   xmm6, oword [rdi+4*AES_BLOCK]
   movdqu   xmm7, oword [rdi+5*AES_BLOCK]
   movdqu   xmm8, oword [rdi+6*AES_BLOCK]
   movdqu   xmm9, oword [rdi+7*AES_BLOCK]

   movdqa   xmm5, oword [r9+AES_BLOCK]      ; whitening keys
   movdqa   xmm4, oword [r9]                ; pre load operation's keys

   movdqa   oword [rbx+1*AES_BLOCK], xmm0   ; save input into the stack
   pxor     xmm0, xmm5                          ; and do whitening
   movdqa   oword [rbx+2*AES_BLOCK], xmm1
   pxor     xmm1, xmm5
   movdqa   oword [rbx+3*AES_BLOCK], xmm2
   pxor     xmm2, xmm5
   movdqa   oword [rbx+4*AES_BLOCK], xmm3
   pxor     xmm3, xmm5
   movdqa   oword [rbx+5*AES_BLOCK], xmm6
   pxor     xmm6, xmm5
   movdqa   oword [rbx+6*AES_BLOCK], xmm7
   pxor     xmm7, xmm5
   movdqa   oword [rbx+7*AES_BLOCK], xmm8
   pxor     xmm8, xmm5
   movdqa   oword [rbx+8*AES_BLOCK], xmm9
   pxor     xmm9, xmm5

   movdqa   xmm5, oword [r9-AES_BLOCK]      ; pre load operation's keys
   sub      r9, (2*AES_BLOCK)

   lea      r10, [rdx-2]               ; counter = nrounds-2
align IPP_ALIGN_FACTOR
.cipher_loop8:
   aesdec      xmm0, xmm4              ; regular round
   aesdec      xmm1, xmm4
   aesdec      xmm2, xmm4
   aesdec      xmm3, xmm4
   aesdec      xmm6, xmm4
   aesdec      xmm7, xmm4
   aesdec      xmm8, xmm4
   aesdec      xmm9, xmm4
   movdqa      xmm4, oword [r9]     ; pre load operation's keys

   aesdec      xmm0, xmm5              ; regular round
   aesdec      xmm1, xmm5
   aesdec      xmm2, xmm5
   aesdec      xmm3, xmm5
   aesdec      xmm6, xmm5
   aesdec      xmm7, xmm5
   aesdec      xmm8, xmm5
   aesdec      xmm9, xmm5
   movdqa      xmm5, oword [r9-AES_BLOCK]   ; pre load operation's keys
   sub         r9, (2*AES_BLOCK)

   sub         r10, 2
   jnz         .cipher_loop8

   aesdec      xmm0, xmm4           ; regular round
   aesdec      xmm1, xmm4
   aesdec      xmm2, xmm4
   aesdec      xmm3, xmm4
   aesdec      xmm6, xmm4
   aesdec      xmm7, xmm4
   aesdec      xmm8, xmm4
   aesdec      xmm9, xmm4

   aesdeclast  xmm0, xmm5           ; irregular round, ^IV, and store result
   pxor        xmm0, xmm15
   movdqu      oword [rsi+0*AES_BLOCK], xmm0
   aesdeclast  xmm1, xmm5
   pxor        xmm1, oword [rbx+1*AES_BLOCK]
   movdqu      oword [rsi+1*AES_BLOCK], xmm1
   aesdeclast  xmm2, xmm5
   pxor        xmm2, oword [rbx+2*AES_BLOCK]
   movdqu      oword [rsi+2*AES_BLOCK], xmm2
   aesdeclast  xmm3, xmm5
   pxor        xmm3, oword [rbx+3*AES_BLOCK]
   movdqu      oword [rsi+3*AES_BLOCK], xmm3
   aesdeclast  xmm6, xmm5
   pxor        xmm6, oword [rbx+4*AES_BLOCK]
   movdqu      oword [rsi+4*AES_BLOCK], xmm6
   aesdeclast  xmm7, xmm5
   pxor        xmm7, oword [rbx+5*AES_BLOCK]
   movdqu      oword [rsi+5*AES_BLOCK], xmm7
   aesdeclast  xmm8, xmm5
   pxor        xmm8, oword [rbx+6*AES_BLOCK]
   movdqu      oword [rsi+6*AES_BLOCK], xmm8
   aesdeclast  xmm9, xmm5
   pxor        xmm9, oword [rbx+7*AES_BLOCK]
   movdqu      oword [rsi+7*AES_BLOCK], xmm9

   movdqa   xmm15,oword [rbx+8*AES_BLOCK]     ; update IV

   add      rsi, (8*AES_BLOCK)
   add      rdi, (8*AES_BLOCK)
   sub      r8,  (8*AES_BLOCK)
   jge      .blk8_loop

   movdqa   xmm6, oword [rsp+0*sizeof(oword)]   ; restore xmm registers
   movdqa   xmm7, oword [rsp+1*sizeof(oword)]
   movdqa   xmm8, oword [rsp+2*sizeof(oword)]
   movdqa   xmm9, oword [rsp+3*sizeof(oword)]
   add      rsp, sizeof(oword)*4                   ; and release stack

   add      r8, (8*AES_BLOCK)
   jz       .quit

;;
;; test %if 4-blocks processing alaivalbe
;;
.block4x:
   cmp      r8, (4*AES_BLOCK)
   jl       .short123_input
   sub      r8, (4*AES_BLOCK)

align IPP_ALIGN_FACTOR
.blk4_loop:
   lea      r9,[rcx+rax*sizeof(dword)-AES_BLOCK]; pointer to the key material

   movdqu   xmm0, oword [rdi+0*AES_BLOCK]    ; get input blocks
   movdqu   xmm1, oword [rdi+1*AES_BLOCK]
   movdqu   xmm2, oword [rdi+2*AES_BLOCK]
   movdqu   xmm3, oword [rdi+3*AES_BLOCK]

   movdqa   xmm5, oword [r9+AES_BLOCK]      ; whitening keys
   movdqa   xmm4, oword [r9]                ; pre load operation's keys

   movdqa   oword [rsp+1*AES_BLOCK], xmm0   ; save input into the stack
   pxor     xmm0, xmm5                          ; and do whitening
   movdqa   oword [rsp+2*AES_BLOCK], xmm1
   pxor     xmm1, xmm5
   movdqa   oword [rsp+3*AES_BLOCK], xmm2
   pxor     xmm2, xmm5
   movdqa   oword [rsp+4*AES_BLOCK], xmm3
   pxor     xmm3, xmm5

   movdqa   xmm5, oword [r9-AES_BLOCK]      ; pre load operation's keys
   sub      r9, (2*AES_BLOCK)

   lea      r10, [rdx-2]               ; counter = nrounds-2
align IPP_ALIGN_FACTOR
.cipher_loop4:
   aesdec      xmm0, xmm4              ; regular round
   aesdec      xmm1, xmm4
   aesdec      xmm2, xmm4
   aesdec      xmm3, xmm4
   movdqa      xmm4, oword [r9]     ; pre load operation's keys

   aesdec      xmm0, xmm5              ; regular round
   aesdec      xmm1, xmm5
   aesdec      xmm2, xmm5
   aesdec      xmm3, xmm5
   movdqa      xmm5, oword [r9-AES_BLOCK]   ; pre load operation's keys
   sub         r9, (2*AES_BLOCK)

   sub         r10, 2
   jnz         .cipher_loop4

   aesdec      xmm0, xmm4           ; regular round
   aesdec      xmm1, xmm4
   aesdec      xmm2, xmm4
   aesdec      xmm3, xmm4

   aesdeclast  xmm0, xmm5           ; irregular round, ^IV, and store result
   pxor        xmm0, xmm15
   movdqu      oword [rsi+0*AES_BLOCK], xmm0
   aesdeclast  xmm1, xmm5
   pxor        xmm1, oword [rsp+1*AES_BLOCK]
   movdqu      oword [rsi+1*AES_BLOCK], xmm1
   aesdeclast  xmm2, xmm5
   pxor        xmm2, oword [rsp+2*AES_BLOCK]
   movdqu      oword [rsi+2*AES_BLOCK], xmm2
   aesdeclast  xmm3, xmm5
   pxor        xmm3, oword [rsp+3*AES_BLOCK]
   movdqu      oword [rsi+3*AES_BLOCK], xmm3

   movdqa      xmm15,oword [rsp+4*AES_BLOCK]     ; update IV

   add      rsi, (4*AES_BLOCK)
   add      rdi, (4*AES_BLOCK)
   sub      r8,  (4*AES_BLOCK)
   jge      .blk4_loop

   add      r8, (4*AES_BLOCK)
   jz       .quit

;;
;; block-by-block processing
;;
.short123_input:
   lea      r9,[rcx+rax*sizeof(dword)] ; pointer to the key material (whitening)

align IPP_ALIGN_FACTOR
.single_blk_loop:
   movdqu   xmm0, oword [rdi]       ; get input block
   add      rdi, AES_BLOCK
   movdqa   xmm1, xmm0                 ; and save as IV for future

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

   pxor        xmm0, xmm15                ; add IV
   movdqu      oword [rsi], xmm0       ; and save output blocl
   add         rsi, AES_BLOCK

   sub         r8,  AES_BLOCK
   movdqa      xmm15, xmm1                ; update IV

   jnz         .single_blk_loop

.quit:
   pxor  xmm4, xmm4
   pxor  xmm5, xmm5

   REST_XMM
   REST_GPR
   ret
ENDFUNC DecryptCBC_RIJ128pipe_AES_NI

%endif ;; _IPP32E >= _IPP32E_Y8
%endif ;; _AES_NI_ENABLING_

