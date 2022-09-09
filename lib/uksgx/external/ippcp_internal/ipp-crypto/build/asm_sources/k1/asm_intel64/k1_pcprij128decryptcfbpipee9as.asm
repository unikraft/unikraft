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


%macro COPY_8U 4.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%limit %3
  %xdefine %%tmp %4

   xor   rcx, rcx
%%next_byte:
   mov   %%tmp, byte [%%src+rcx]
   mov   byte [%%dst+rcx], %%tmp
   add   rcx, 1
   cmp   rcx, %%limit
   jl    %%next_byte
%endmacro

%macro COPY_32U 4.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%limit %3
  %xdefine %%tmp %4

   xor   rcx, rcx
%%next_dword:
   mov   %%tmp, dword [%%src+rcx]
   mov   dword [%%dst+rcx], %%tmp
   add   rcx, 4
   cmp   rcx, %%limit
   jl    %%next_dword
%endmacro

%macro COPY_128U 4.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%limit %3
  %xdefine %%tmp %4

   xor   rcx, rcx
%%next_oword:
   movdqu   %%tmp, oword [%%src+rcx]
   movdqu   oword [%%dst+rcx], %%tmp
   add   rcx, 16
   cmp   rcx, %%limit
   jl    %%next_oword
%endmacro

segment .text align=IPP_ALIGN_FACTOR


;***************************************************************
;* Purpose:    pipelined RIJ128 CFB decryption
;*
;* void DecryptCFB_RIJ128pipe_AES_NI(const Ipp32u* inpBlk,
;*                                         Ipp32u* outBlk,
;*                                         int nr,
;*                                   const Ipp32u* pRKey,
;*                                         int cfbBlks,
;*                                         int cfbSize,
;*                                   const Ipp8u* pIV)
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsRijndael128DecryptCFB
;;
align IPP_ALIGN_FACTOR
IPPASM DecryptCFB_RIJ128pipe_AES_NI,PUBLIC
%assign LOCAL_FRAME (1+4+4)*16
        USES_GPR rsi,rdi,r13,r14,r15
        USES_XMM xmm6,xmm7
        COMP_ABI 7
;; rdi:        pInpBlk:  DWORD,    ; input  blocks address
;; rsi:        pOutBlk:  DWORD,    ; output blocks address
;; rdx:        nr:           DWORD,    ; number of rounds
;; rcx         pKey:     DWORD     ; key material address
;; r8d         cfbBlks:      DWORD     ; length of stream in cfbSize
;; r9d         cfbSize:      DWORD     ; cfb blk size
;; [rsp+ARG_7] pIV       BYTE      ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   mov      rax, [rsp+ARG_7]           ; IV address
   movdqu   xmm4, oword [rax]       ; get IV
   movdqa   oword [rsp+0*16], xmm4 ; into the stack

   mov      r13, rdi
   mov      r14, rsi
   mov      r15, rcx

   movsxd   r8, r8d                    ; length of stream
   movsxd   r9, r9d                    ; cfb blk size

   sub      r8, BLKS_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
   lea      r10, [r9*BLKS_PER_LOOP]
.blks_loop:
   COPY_32U {rsp+16}, r13, r10, r11d   ; move 4 input blocks to stack

   movdqa   xmm4, oword [r15]

   lea      r10, [r9+r9*2]
   movdqa   xmm0, oword [rsp]      ; get encoded blocks
   movdqu   xmm1, oword [rsp+r9]
   movdqu   xmm2, oword [rsp+r9*2]
   movdqu   xmm3, oword [rsp+r10]

   mov      r10, r15                   ; set pointer to the key material

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [r10+16]    ; pre load operation's keys
   add      r10, 16

   mov      r11, rdx                   ; counter depending on key length
   sub      r11, 1
.cipher_loop:
   aesenc      xmm0, xmm4              ; regular round
   aesenc      xmm1, xmm4
   aesenc      xmm2, xmm4
   aesenc      xmm3, xmm4
   movdqa      xmm4, oword [r10+16]; pre load operation's keys
   add         r10, 16
   dec         r11
   jnz         .cipher_loop

   aesenclast  xmm0, xmm4              ; irregular round and IV
   aesenclast  xmm1, xmm4
   aesenclast  xmm2, xmm4
   aesenclast  xmm3, xmm4

   lea         r10, [r9+r9*2]          ; get src blocks from the stack
   movdqa      xmm4, oword [rsp+16]
   movdqu      xmm5, oword [rsp+16+r9]
   movdqu      xmm6, oword [rsp+16+r9*2]
   movdqu      xmm7, oword [rsp+16+r10]

   pxor        xmm0, xmm4              ; xor src
   movdqa      oword [rsp+5*16],xmm0;and store into the stack
   pxor        xmm1, xmm5
   movdqu      oword [rsp+5*16+r9], xmm1
   pxor        xmm2, xmm6
   movdqu      oword [rsp+5*16+r9*2], xmm2
   pxor        xmm3, xmm7
   movdqu      oword [rsp+5*16+r10], xmm3

   lea         r10, [r9*BLKS_PER_LOOP]
  ;COPY_8U     r14, {rsp+5*16}, r10    ; move 4 blocks to output
   COPY_32U    r14, {rsp+5*16}, r10, r11d ; move 4 blocks to output

   movdqu      xmm0, oword [rsp+r10]; update IV
   movdqu      oword [rsp], xmm0

   add         r13, r10
   add         r14, r10
   sub         r8, BLKS_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      r8, BLKS_PER_LOOP
   jz       .quit

   lea      r10, [r9*2]
   lea      r11, [r9+r9*2]
   cmp      r8, 2
   cmovl    r10, r9
   cmovg    r10, r11
   COPY_8U  {rsp+16}, r13, r10, al     ; move recent input blocks to stack

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax,[rdx*4]
   lea      rax, [r15+rax*4-9*(SC)*4]  ; AES-128 round keys

   xor      r11, r11                   ; index
.single_blk_loop:
   movdqu   xmm0, oword [rsp+r11]   ; get encoded block

   pxor     xmm0, oword [r15]      ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc     xmm0, oword [rax-4*4*SC]
   aesenc     xmm0, oword [rax-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [rax-2*4*SC]
   aesenc     xmm0, oword [rax-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [rax+0*4*SC]
   aesenc     xmm0, oword [rax+1*4*SC]
   aesenc     xmm0, oword [rax+2*4*SC]
   aesenc     xmm0, oword [rax+3*4*SC]
   aesenc     xmm0, oword [rax+4*4*SC]
   aesenc     xmm0, oword [rax+5*4*SC]
   aesenc     xmm0, oword [rax+6*4*SC]
   aesenc     xmm0, oword [rax+7*4*SC]
   aesenc     xmm0, oword [rax+8*4*SC]
   aesenclast xmm0, oword [rax+9*4*SC]

   movdqu   xmm1, oword [rsp+r11+16]   ; get input block from the stack
   pxor     xmm0, xmm1                    ; xor src
   movdqu   oword [rsp+5*16+r11], xmm0 ; and save output

   add      r11, r9
   dec      r8
   jnz      .single_blk_loop

   COPY_8U  r14, {rsp+5*16}, r10, al     ; copy rest output from the stack

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC DecryptCFB_RIJ128pipe_AES_NI

align IPP_ALIGN_FACTOR
IPPASM DecryptCFB32_RIJ128pipe_AES_NI,PUBLIC
%assign LOCAL_FRAME (1+4+4)*16
        USES_GPR rsi,rdi,r13,r14,r15
        USES_XMM xmm6,xmm7
        COMP_ABI 7
;; rdi:        pInpBlk:  DWORD,    ; input  blocks address
;; rsi:        pOutBlk:  DWORD,    ; output blocks address
;; rdx:        nr:           DWORD,    ; number of rounds
;; rcx         pKey:     DWORD     ; key material address
;; r8d         cfbBlks:      DWORD     ; length of stream in cfbSize
;; r9d         cfbSize:      DWORD     ; cfb blk size (4 bytes multible)
;; [rsp+ARG_7] pIV       BYTE      ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   mov      rax, [rsp+ARG_7]           ; IV address
   movdqu   xmm4, oword [rax]       ; get IV
   movdqa   oword [rsp+0*16], xmm4 ; into the stack

   mov      r13, rdi
   mov      r14, rsi
   mov      r15, rcx

   movsxd   r8, r8d                    ; length of stream
   movsxd   r9, r9d                    ; cfb blk size

   sub      r8, BLKS_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
   lea      r10, [r9*BLKS_PER_LOOP]
.blks_loop:
   COPY_128U {rsp+16}, r13, r10, xmm0  ; move 4 input blocks to stack

   movdqa   xmm4, oword [r15]

   lea      r10, [r9+r9*2]
   movdqa   xmm0, oword [rsp]      ; get encoded blocks
   movdqu   xmm1, oword [rsp+r9]
   movdqu   xmm2, oword [rsp+r9*2]
   movdqu   xmm3, oword [rsp+r10]

   mov      r10, r15                   ; set pointer to the key material

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [r10+16]    ; pre load operation's keys
   add      r10, 16

   mov      r11, rdx                   ; counter depending on key length
   sub      r11, 1
.cipher_loop:
   aesenc      xmm0, xmm4              ; regular round
   aesenc      xmm1, xmm4
   aesenc      xmm2, xmm4
   aesenc      xmm3, xmm4
   movdqa      xmm4, oword [r10+16]; pre load operation's keys
   add         r10, 16
   dec         r11
   jnz         .cipher_loop

   aesenclast  xmm0, xmm4              ; irregular round and IV
   aesenclast  xmm1, xmm4
   aesenclast  xmm2, xmm4
   aesenclast  xmm3, xmm4

   lea         r10, [r9+r9*2]          ; get src blocks from the stack
   movdqa      xmm4, oword [rsp+16]
   movdqu      xmm5, oword [rsp+16+r9]
   movdqu      xmm6, oword [rsp+16+r9*2]
   movdqu      xmm7, oword [rsp+16+r10]

   pxor        xmm0, xmm4              ; xor src
   movdqa      oword [rsp+5*16],xmm0;and store into the stack
   pxor        xmm1, xmm5
   movdqu      oword [rsp+5*16+r9], xmm1
   pxor        xmm2, xmm6
   movdqu      oword [rsp+5*16+r9*2], xmm2
   pxor        xmm3, xmm7
   movdqu      oword [rsp+5*16+r10], xmm3

   lea         r10, [r9*BLKS_PER_LOOP]
   COPY_128U   r14, {rsp+5*16}, r10, xmm0 ; move 4 blocks to output

   movdqu      xmm0, oword [rsp+r10]   ; update IV
   movdqu      oword [rsp], xmm0

   add         r13, r10
   add         r14, r10
   sub         r8, BLKS_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      r8, BLKS_PER_LOOP
   jz       .quit

   lea      r10, [r9*2]
   lea      r11, [r9+r9*2]
   cmp      r8, 2
   cmovl    r10, r9
   cmovg    r10, r11
   COPY_32U {rsp+16}, r13, r10, eax    ; move recent input blocks to stack

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax,[rdx*4]
   lea      rax, [r15+rax*4-9*(SC)*4]  ; AES-128 round keys

   xor      r11, r11                   ; index
.single_blk_loop:
   movdqu   xmm0, oword [rsp+r11]   ; get encoded block

   pxor     xmm0, oword [r15]      ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc     xmm0, oword [rax-4*4*SC]
   aesenc     xmm0, oword [rax-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [rax-2*4*SC]
   aesenc     xmm0, oword [rax-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [rax+0*4*SC]
   aesenc     xmm0, oword [rax+1*4*SC]
   aesenc     xmm0, oword [rax+2*4*SC]
   aesenc     xmm0, oword [rax+3*4*SC]
   aesenc     xmm0, oword [rax+4*4*SC]
   aesenc     xmm0, oword [rax+5*4*SC]
   aesenc     xmm0, oword [rax+6*4*SC]
   aesenc     xmm0, oword [rax+7*4*SC]
   aesenc     xmm0, oword [rax+8*4*SC]
   aesenclast xmm0, oword [rax+9*4*SC]

   movdqu   xmm1, oword [rsp+r11+16]   ; get input block from the stack
   pxor     xmm0, xmm1                    ; xor src
   movdqu   oword [rsp+5*16+r11], xmm0 ; and save output

   add      r11, r9
   dec      r8
   jnz      .single_blk_loop

   COPY_32U r14, {rsp+5*16}, r10, eax    ; copy rest output from the stack

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC DecryptCFB32_RIJ128pipe_AES_NI

;;
;; Lib = Y8
;;
;; Caller = ippsRijndael128DecryptCFB
;;
align IPP_ALIGN_FACTOR
IPPASM DecryptCFB128_RIJ128pipe_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM xmm6,xmm7
        COMP_ABI 6
;; rdi:        pInpBlk:  DWORD,    ; input  blocks address
;; rsi:        pOutBlk:  DWORD,    ; output blocks address
;; rdx:        nr:           DWORD,    ; number of rounds
;; rcx         pKey:     DWORD     ; key material address
;; r8d         lenBytes:     DWORD     ; length of stream in bytes
;; r9          pIV       BYTE      ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP  (BYTES_PER_BLK*BLKS_PER_LOOP)

   movdqu   xmm0, oword [r9]        ; get IV

   movsxd   r8, r8d                    ; length of the stream
   sub      r8, BYTES_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
.blks_loop:
   movdqa   xmm7, oword [rcx]       ; get initial key material
   mov      r10, rcx                   ; set pointer to the key material

   movdqu   xmm1, oword [rdi+0*BYTES_PER_BLK] ; get another encoded cblocks
   movdqu   xmm2, oword [rdi+1*BYTES_PER_BLK]
   movdqu   xmm3, oword [rdi+2*BYTES_PER_BLK]

   pxor     xmm0, xmm7                 ; whitening
   pxor     xmm1, xmm7
   pxor     xmm2, xmm7
   pxor     xmm3, xmm7

   movdqa   xmm7, oword [r10+16]    ; pre load operation's keys
   add      r10, 16

   mov      r11, rdx                      ; counter depending on key length
   sub      r11, 1
.cipher_loop:
   aesenc      xmm0, xmm7                 ; regular round
   aesenc      xmm1, xmm7
   aesenc      xmm2, xmm7
   aesenc      xmm3, xmm7
   movdqa      xmm7, oword [r10+16]   ; pre load operation's keys
   add         r10, 16
   dec         r11
   jnz         .cipher_loop

   aesenclast  xmm0, xmm7                 ; irregular round and IV
   movdqu      xmm4, oword [rdi+0*BYTES_PER_BLK]  ; 4 input blocks
   aesenclast  xmm1, xmm7
   movdqu      xmm5, oword [rdi+1*BYTES_PER_BLK]
   aesenclast  xmm2, xmm7
   movdqu      xmm6, oword [rdi+2*BYTES_PER_BLK]
   aesenclast  xmm3, xmm7
   movdqu      xmm7, oword [rdi+3*BYTES_PER_BLK]
   add         rdi, BYTES_PER_LOOP

   pxor     xmm0, xmm4                 ; 4 output blocks
   movdqu   oword [rsi+0*16], xmm0
   pxor     xmm1, xmm5
   movdqu   oword [rsi+1*16], xmm1
   pxor     xmm2, xmm6
   movdqu   oword [rsi+2*16], xmm2
   pxor     xmm3, xmm7
   movdqu   oword [rsi+3*16], xmm3
   add      rsi, BYTES_PER_LOOP

   movdqa   xmm0, xmm7                 ; update IV
   sub      r8, BYTES_PER_LOOP
   jge      .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      r8, BYTES_PER_LOOP
   jz       .quit

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax, [rdx*4]
   lea      rax, [rcx+rax*4-9*(SC)*4]  ; AES-128 round keys

.single_blk_loop:
   pxor     xmm0, oword [rcx]      ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc     xmm0, oword [rax-4*4*SC]
   aesenc     xmm0, oword [rax-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [rax-2*4*SC]
   aesenc     xmm0, oword [rax-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [rax+0*4*SC]
   aesenc     xmm0, oword [rax+1*4*SC]
   aesenc     xmm0, oword [rax+2*4*SC]
   aesenc     xmm0, oword [rax+3*4*SC]
   aesenc     xmm0, oword [rax+4*4*SC]
   aesenc     xmm0, oword [rax+5*4*SC]
   aesenc     xmm0, oword [rax+6*4*SC]
   aesenc     xmm0, oword [rax+7*4*SC]
   aesenc     xmm0, oword [rax+8*4*SC]
   aesenclast xmm0, oword [rax+9*4*SC]

   movdqu      xmm1, oword [rdi]       ; input block from the stream
   add         rdi, BYTES_PER_BLK
   pxor        xmm0, xmm1                 ; xor src
   movdqu      oword [rsi], xmm0       ; and save output
   add         rsi, BYTES_PER_BLK

   movdqa      xmm0, xmm1                 ; update IV
   sub         r8, BYTES_PER_BLK
   jnz         .single_blk_loop

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC DecryptCFB128_RIJ128pipe_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_

