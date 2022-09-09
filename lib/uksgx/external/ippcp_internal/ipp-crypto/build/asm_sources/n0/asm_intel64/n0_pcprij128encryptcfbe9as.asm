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
;     Purpose:  Cryptography Primitive.
;               Rijndael Inverse Cipher function
;
;     Content:
;        Encrypt_RIJ128_AES_NI()
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
;* Purpose:    RIJ128 CFB encryption
;*
;* void EncryptCFB_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                     Ipp32u* outBlk,
;*                                     int nr,
;*                               const Ipp32u* pRKey,
;*                                     int cfbBlks,
;*                                     int cfbSize,
;*                               const Ipp8u* pIV)
;***************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsAESEncryptCFB
;;
align IPP_ALIGN_FACTOR
IPPASM EncryptCFB_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME (1+4+4)*16
        USES_GPR rsi,rdi,r12,r15
        USES_XMM
        COMP_ABI 7
;; rdi:        pInpBlk:  DWORD,    ; input  blocks address
;; rsi:        pOutBlk:  DWORD,    ; output blocks address
;; rdx:        nr:           DWORD,    ; number of rounds
;; rcx         pKey:     DWORD     ; key material address
;; r8d         cfbBlks:      DWORD     ; length of stream in bytes
;; r9d         cfbSize:      DWORD     ; cfb blk size
;; [rsp+ARG_7] pIV       BYTE      ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   mov      rax, [rsp+ARG_7]           ; IV address
   movdqu   xmm4, oword [rax]       ; get IV
   movdqa   oword [rsp+0*16], xmm4 ; into the stack

   movsxd   r8, r8d                    ; length of stream
   movsxd   r9, r9d                    ; cfb blk size

   mov      r15, rcx                   ; save key material address

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax,[rdx*4]
   lea      rax, [r15+rax*4-9*(SC)*4]  ; AES-128 round keys

;;
;; processing
;;
   lea      r10, [r9*BLKS_PER_LOOP]    ; 4 cfb block
.blks_loop:
   cmp      r8, r10
   cmovl    r10, r8
   COPY_8U {rsp+5*16}, rdi, r10, r11b  ; move 1-4 input blocks to stack

   mov      r12, r10                   ; copy length to be processed
   xor      r11, r11                   ; index
.single_blk:
   movdqu   xmm0, oword [rsp+r11]  ; get processing blocks

   pxor     xmm0, oword [r15]      ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s
                                       ; do encryption
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

   movdqu      xmm1, oword [rsp+5*16+r11] ; get src blocks from the stack
   pxor        xmm0, xmm1                    ; xor src
   movdqu      oword [rsp+1*16+r11],xmm0  ;and store into the stack

   add         r11, r9                       ; advance index
   sub         r12, r9                       ; decrease length
   jg          .single_blk

   COPY_8U     rsi, {rsp+1*16}, r10, r11b    ; move 1-4 blocks to output

   movdqu      xmm0, oword [rsp+r10]; update IV
   movdqa      oword [rsp], xmm0

   add         rdi, r10
   add         rsi, r10
   sub         r8, r10
   jg          .blks_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC EncryptCFB_RIJ128_AES_NI

align IPP_ALIGN_FACTOR
IPPASM EncryptCFB32_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME (1+4+4)*16
        USES_GPR rsi,rdi,r12,r15
        USES_XMM
        COMP_ABI 7
;; rdi:        pInpBlk:  DWORD,    ; input  blocks address
;; rsi:        pOutBlk:  DWORD,    ; output blocks address
;; rdx:        nr:           DWORD,    ; number of rounds
;; rcx         pKey:     DWORD     ; key material address
;; r8d         cfbBlks:      DWORD     ; length of stream in bytes
;; r9d         cfbSize:      DWORD     ; cfb blk size
;; [rsp+ARG_7] pIV       BYTE      ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   mov      rax, [rsp+ARG_7]           ; IV address
   movdqu   xmm4, oword [rax]       ; get IV
   movdqa   oword [rsp+0*16], xmm4 ; into the stack

   movsxd   r8, r8d                    ; length of stream
   movsxd   r9, r9d                    ; cfb blk size

   mov      r15, rcx                   ; save key material address

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax,[rdx*4]
   lea      rax, [r15+rax*4-9*(SC)*4]  ; AES-128 round keys

;;
;; processing
;;
   lea      r10, [r9*BLKS_PER_LOOP]    ; 4 cfb block
.blks_loop:
   cmp      r8, r10
   cmovl    r10, r8
   COPY_32U {rsp+5*16}, rdi, r10, r11d ; move 1-4 input blocks to stack

   mov      r12, r10                   ; copy length to be processed
   xor      r11, r11                   ; index
.single_blk:
   movdqu   xmm0, oword [rsp+r11]  ; get processing blocks

   pxor     xmm0, oword [r15]      ; whitening

   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s
                                       ; do encryption
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

   movdqu      xmm1, oword [rsp+5*16+r11] ; get src blocks from the stack
   pxor        xmm0, xmm1                    ; xor src
   movdqu      oword [rsp+1*16+r11],xmm0  ;and store into the stack

   add         r11, r9                       ; advance index
   sub         r12, r9                       ; decrease length
   jg          .single_blk

   COPY_32U    rsi, {rsp+1*16}, r10, r11d    ; move 1-4 blocks to output

   movdqu      xmm0, oword [rsp+r10]; update IV
   movdqa      oword [rsp], xmm0

   add         rdi, r10
   add         rsi, r10
   sub         r8, r10
   jg          .blks_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC EncryptCFB32_RIJ128_AES_NI

align IPP_ALIGN_FACTOR
IPPASM EncryptCFB128_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM
        COMP_ABI 6
;; rdi:        pInpBlk:  DWORD,    ; input  blocks address
;; rsi:        pOutBlk:  DWORD,    ; output blocks address
;; rdx:        nr:           DWORD,    ; number of rounds
;; rcx         pKey:     DWORD     ; key material address
;; r8d         cfbBlks:      DWORD     ; length of stream in bytes
;; r9d         pIV:      BYTE      ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   movdqu   xmm0, oword [r9]        ; get IV

   movsxd   r8, r8d                    ; length of stream
   movsxd   r9, r9d                    ; cfb blk size

   ; get actual address of key material: pRKeys += (nr-9) * SC
   lea      rax,[rdx*4]
   lea      rax, [rcx+rax*4-9*(SC)*4]  ; AES-128 round keys

;;
;; processing
;;
.blks_loop:
   pxor     xmm0, oword [rcx]      ; whitening

   movdqu   xmm1, oword [rdi]       ; input blocks


   cmp      rdx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s
                                       ; do encryption
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

   pxor        xmm0, xmm1                    ; xor src
   movdqu      oword [rsi],xmm0           ;and store into the dst

   add         rdi, 16
   add         rsi, 16
   sub         r8, 16
   jg          .blks_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC EncryptCFB128_RIJ128_AES_NI

%endif

%endif ;; _AES_NI_ENABLING_


