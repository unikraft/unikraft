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
;        Decrypt_RIJ128_AES_NI()
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"


%macro COPY_8U 4.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%limit %3
  %xdefine %%tmp %4

   xor   ecx, ecx
%%next_byte:
   mov   %%tmp, byte [%%src+ecx]
   mov   byte [%%dst+ecx], %%tmp
   add   ecx, 1
   cmp   ecx, %%limit
   jl    %%next_byte
%endmacro


%macro COPY_32U 4.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%limit %3
  %xdefine %%tmp %4

   xor   ecx, ecx
%%next_dword:
   mov   %%tmp, dword [%%src+ecx]
   mov   dword [%%dst+ecx], %%tmp
   add   ecx, 4
   cmp   ecx, %%limit
   jl    %%next_dword
%endmacro


%macro COPY_128U 4.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%limit %3
  %xdefine %%tmp %4

   xor   ecx, ecx
%%next_oword:
   movdqu   %%tmp, oword [%%src+ecx]
   movdqu   oword [%%dst+ecx], %%tmp
   add   ecx, 16
   cmp   ecx, %%limit
   jl    %%next_oword
%endmacro


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

;%if (_IPP >= _IPP_P8) && (_IPP < _IPP_G9)
%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128DecryptCFB
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM DecryptCFB_RIJ128pipe_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine cfbBlks [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in cfbSize
%xdefine cfbSize [ebp + ARG_1 + 5*sizeof(dword)] ; cfb blk size
%xdefine pIV     [ebp + ARG_1 + 6*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   sub      esp,16*(1+4+4)    ; allocate stask:
                              ; +0*16  IV
                              ; +1*16  inp0, inp1, inp2, inp3
                              ; +5*16  out0, out1, out2, out3

   mov      eax, pIV                   ; IV address
   movdqu   xmm4, oword [eax]       ; get IV
   movdqu   oword [esp+0*16], xmm4 ; into the stack

   mov      esi,pInpBlk       ; input data address
   mov      edi,pOutBlk       ; output data address
   mov      edx,cfbSize       ; size of block

   sub      dword cfbBlks, BLKS_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
.blks_loop:
   lea      eax, [edx*BLKS_PER_LOOP]
   COPY_32U {esp+16}, esi, eax, ebx    ; move 4 input blocks to stack

   mov      ecx, pKey
   movdqa   xmm4, oword [ecx]       ; keys for whitening

   lea      ebx, [edx+edx*2]
   movdqu   xmm0, oword [esp]      ; get encoded blocks
   movdqu   xmm1, oword [esp+edx]
   movdqu   xmm2, oword [esp+edx*2]
   movdqu   xmm3, oword [esp+ebx]

   lea      ebx, [ecx+16]              ; pointer to the round's key material

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [ebx]       ; pre load operation's keys
   add      ebx, 16

   mov      eax, nr                    ; counter depending on key length
   sub      eax, 1
.cipher_loop:
   aesenc      xmm0, xmm4              ; regular round
   aesenc      xmm1, xmm4
   aesenc      xmm2, xmm4
   aesenc      xmm3, xmm4
   movdqa      xmm4, oword [ebx]   ; pre load operation's keys
   add         ebx, 16
   dec         eax
   jnz         .cipher_loop

   aesenclast  xmm0, xmm4              ; irregular round and IV
   aesenclast  xmm1, xmm4
   aesenclast  xmm2, xmm4
   aesenclast  xmm3, xmm4

   lea         ebx, [edx+edx*2]        ; get src blocks from the stack
   movdqu      xmm4, oword [esp+16]
   movdqu      xmm5, oword [esp+16+edx]
   movdqu      xmm6, oword [esp+16+edx*2]
   movdqu      xmm7, oword [esp+16+ebx]

   pxor        xmm0, xmm4              ; xor src
   movdqu      oword [esp+5*16],xmm0;and store into the stack
   pxor        xmm1, xmm5
   movdqu      oword [esp+5*16+edx], xmm1
   pxor        xmm2, xmm6
   movdqu      oword [esp+5*16+edx*2], xmm2
   pxor        xmm3, xmm7
   movdqu      oword [esp+5*16+ebx], xmm3

   lea         eax, [edx*BLKS_PER_LOOP]
   COPY_32U    edi, {esp+5*16}, eax, ebx  ; move 4 blocks to output

   movdqu      xmm0, oword [esp+eax]   ; update IV
   movdqu      oword [esp], xmm0


   add         esi, eax
   add         edi, eax
   sub         dword cfbBlks, BLKS_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      dword cfbBlks, BLKS_PER_LOOP
   jz       .quit

   lea      ebx, [edx*2]
   lea      ecx, [edx+edx*2]
   cmp      dword cfbBlks, 2
   cmovl    ebx, edx
   cmovg    ebx, ecx
   COPY_8U  {esp+16}, esi, ebx, al     ; move recent input blocks to stack

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      ecx, pKey
   mov      eax, nr
   lea      esi,[eax*4]
   lea      esi, [ecx+esi*4-9*(SC)*4]  ; AES-128 round keys

   xor      eax, eax                   ; index
.single_blk_loop:
   movdqu   xmm0, oword [esp+eax]   ; get encoded block

   pxor     xmm0, oword [ecx]      ; whitening

   cmp      dword nr, 12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc     xmm0, oword [esi-4*4*SC]
   aesenc     xmm0, oword [esi-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [esi-2*4*SC]
   aesenc     xmm0, oword [esi-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [esi+0*4*SC]
   aesenc     xmm0, oword [esi+1*4*SC]
   aesenc     xmm0, oword [esi+2*4*SC]
   aesenc     xmm0, oword [esi+3*4*SC]
   aesenc     xmm0, oword [esi+4*4*SC]
   aesenc     xmm0, oword [esi+5*4*SC]
   aesenc     xmm0, oword [esi+6*4*SC]
   aesenc     xmm0, oword [esi+7*4*SC]
   aesenc     xmm0, oword [esi+8*4*SC]
   aesenclast xmm0, oword [esi+9*4*SC]

   movdqu   xmm1, oword [esp+eax+16]   ; get input block from the stack
   pxor     xmm0, xmm1                    ; xor src
   movdqu   oword [esp+5*16+eax], xmm0 ; and save output

   add      eax, edx
   dec      dword cfbBlks
   jnz      .single_blk_loop

   COPY_8U  edi, {esp+5*16}, ebx, al      ; copy rest output from the stack

.quit:
   add      esp, 16*(1+4+4)               ; free stack
   REST_GPR
   ret
ENDFUNC DecryptCFB_RIJ128pipe_AES_NI

align IPP_ALIGN_FACTOR
IPPASM DecryptCFB32_RIJ128pipe_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine cfbBlks [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in cfbSize
%xdefine cfbSize [ebp + ARG_1 + 5*sizeof(dword)] ; cfb blk size (4 bytes multible)
%xdefine pIV     [ebp + ARG_1 + 6*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   sub      esp,16*(1+4+4)    ; allocate stask:
                              ; +0*16  IV
                              ; +1*16  inp0, inp1, inp2, inp3
                              ; +5*16  out0, out1, out2, out3

   mov      eax, pIV                   ; IV address
   movdqu   xmm4, oword [eax]       ; get IV
   movdqu   oword [esp+0*16], xmm4 ; into the stack

   mov      esi,pInpBlk       ; input data address
   mov      edi,pOutBlk       ; output data address
   mov      edx,cfbSize       ; size of block

   sub      dword cfbBlks, BLKS_PER_LOOP
   jl       .short_input

;;
;; pipelined processing
;;
.blks_loop:
   lea      eax, [edx*BLKS_PER_LOOP]
   COPY_128U {esp+16}, esi, eax, xmm0  ; move 4 input blocks to stack

   mov      ecx, pKey
   movdqa   xmm4, oword [ecx]       ; keys for whitening

   lea      ebx, [edx+edx*2]
   movdqu   xmm0, oword [esp]      ; get encoded blocks
   movdqu   xmm1, oword [esp+edx]
   movdqu   xmm2, oword [esp+edx*2]
   movdqu   xmm3, oword [esp+ebx]

   lea      ebx, [ecx+16]              ; pointer to the round's key material

   pxor     xmm0, xmm4                 ; whitening
   pxor     xmm1, xmm4
   pxor     xmm2, xmm4
   pxor     xmm3, xmm4

   movdqa   xmm4, oword [ebx]       ; pre load operation's keys
   add      ebx, 16

   mov      eax, nr                    ; counter depending on key length
   sub      eax, 1
.cipher_loop:
   aesenc      xmm0, xmm4              ; regular round
   aesenc      xmm1, xmm4
   aesenc      xmm2, xmm4
   aesenc      xmm3, xmm4
   movdqa      xmm4, oword [ebx]   ; pre load operation's keys
   add         ebx, 16
   dec         eax
   jnz         .cipher_loop

   aesenclast  xmm0, xmm4              ; irregular round and IV
   aesenclast  xmm1, xmm4
   aesenclast  xmm2, xmm4
   aesenclast  xmm3, xmm4

   lea         ebx, [edx+edx*2]        ; get src blocks from the stack
   movdqu      xmm4, oword [esp+16]
   movdqu      xmm5, oword [esp+16+edx]
   movdqu      xmm6, oword [esp+16+edx*2]
   movdqu      xmm7, oword [esp+16+ebx]

   pxor        xmm0, xmm4              ; xor src
   movdqu      oword [esp+5*16],xmm0;and store into the stack
   pxor        xmm1, xmm5
   movdqu      oword [esp+5*16+edx], xmm1
   pxor        xmm2, xmm6
   movdqu      oword [esp+5*16+edx*2], xmm2
   pxor        xmm3, xmm7
   movdqu      oword [esp+5*16+ebx], xmm3

   lea         eax, [edx*BLKS_PER_LOOP]
   COPY_128U   edi, {esp+5*16}, eax, xmm0 ; move 4 blocks to output

   movdqu      xmm0, oword [esp+eax]   ; update IV
   movdqu      oword [esp], xmm0


   add         esi, eax
   add         edi, eax
   sub         dword cfbBlks, BLKS_PER_LOOP
   jge         .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      dword cfbBlks, BLKS_PER_LOOP
   jz       .quit

   lea      ebx, [edx*2]
   lea      ecx, [edx+edx*2]
   cmp      dword cfbBlks, 2
   cmovl    ebx, edx
   cmovg    ebx, ecx
   COPY_32U {esp+16}, esi, ebx, eax    ; move recent input blocks to stack

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      ecx, pKey
   mov      eax, nr
   lea      esi,[eax*4]
   lea      esi, [ecx+esi*4-9*(SC)*4]  ; AES-128 round keys

   xor      eax, eax                   ; index
.single_blk_loop:
   movdqu   xmm0, oword [esp+eax]   ; get encoded block

   pxor     xmm0, oword [ecx]      ; whitening

   cmp      dword nr, 12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s

.key_256_s:
   aesenc     xmm0, oword [esi-4*4*SC]
   aesenc     xmm0, oword [esi-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [esi-2*4*SC]
   aesenc     xmm0, oword [esi-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [esi+0*4*SC]
   aesenc     xmm0, oword [esi+1*4*SC]
   aesenc     xmm0, oword [esi+2*4*SC]
   aesenc     xmm0, oword [esi+3*4*SC]
   aesenc     xmm0, oword [esi+4*4*SC]
   aesenc     xmm0, oword [esi+5*4*SC]
   aesenc     xmm0, oword [esi+6*4*SC]
   aesenc     xmm0, oword [esi+7*4*SC]
   aesenc     xmm0, oword [esi+8*4*SC]
   aesenclast xmm0, oword [esi+9*4*SC]

   movdqu   xmm1, oword [esp+eax+16]   ; get input block from the stack
   pxor     xmm0, xmm1                    ; xor src
   movdqu   oword [esp+5*16+eax], xmm0 ; and save output

   add      eax, edx
   dec      dword cfbBlks
   jnz      .single_blk_loop

   COPY_32U edi, {esp+5*16}, ebx, eax     ; copy rest output from the stack

.quit:
   add      esp, 16*(1+4+4)               ; free stack
   REST_GPR
   ret
ENDFUNC DecryptCFB32_RIJ128pipe_AES_NI

;;
;; Lib = G9
;;
;; Caller = ippsRijndael128DecryptCFB
;;
align IPP_ALIGN_FACTOR
IPPASM DecryptCFB128_RIJ128pipe_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in bytes
%xdefine pIV     [ebp + ARG_1 + 5*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP  (BYTES_PER_BLK*BLKS_PER_LOOP)

   mov      esi,pInpBlk       ; input data address
   mov      edi,pOutBlk       ; output data address
   mov      ecx,pKey          ; keys
   mov      edx, len

   mov      eax, pIV
   movdqu   xmm0, oword [eax]       ; get IV

   sub      edx, BYTES_PER_LOOP        ; test length of the stream
   jl       .short_input

;;
;; pipelined processing
;;
.blks_loop:
   movdqa   xmm7, oword [ecx]       ; keys for whitening
   lea      ebx, [ecx+16]              ; pointer to the round's key material

   movdqu   xmm1, oword [esi+0*BYTES_PER_BLK] ; get another encoded cblocks
   movdqu   xmm2, oword [esi+1*BYTES_PER_BLK]
   movdqu   xmm3, oword [esi+2*BYTES_PER_BLK]

   pxor     xmm0, xmm7                 ; whitening
   pxor     xmm1, xmm7
   pxor     xmm2, xmm7
   pxor     xmm3, xmm7

   movdqa   xmm7, oword [ebx]       ; pre load operation's keys
   add      ebx, 16

   mov      eax, nr                    ; counter depending on key length
   sub      eax, 1
.cipher_loop:
   aesenc      xmm0, xmm7              ; regular round
   aesenc      xmm1, xmm7
   aesenc      xmm2, xmm7
   aesenc      xmm3, xmm7
   movdqa      xmm7, oword [ebx]   ; pre load operation's keys
   add         ebx, 16
   dec         eax
   jnz         .cipher_loop

   aesenclast  xmm0, xmm7              ; irregular round and IV for 4 input blocks
   movdqu      xmm4, oword [esi+0*BYTES_PER_BLK]
   aesenclast  xmm1, xmm7
   movdqu      xmm5, oword [esi+1*BYTES_PER_BLK]
   aesenclast  xmm2, xmm7
   movdqu      xmm6, oword [esi+2*BYTES_PER_BLK]
   aesenclast  xmm3, xmm7
   movdqu      xmm7, oword [esi+3*BYTES_PER_BLK]
   add         esi, BYTES_PER_LOOP

   pxor     xmm0, xmm4                 ; 4 output blocks
   movdqu   oword [edi+0*BYTES_PER_BLK], xmm0
   pxor     xmm1, xmm5
   movdqu   oword [edi+1*BYTES_PER_BLK], xmm1
   pxor     xmm2, xmm6
   movdqu   oword [edi+2*BYTES_PER_BLK], xmm2
   pxor     xmm3, xmm7
   movdqu   oword [edi+3*BYTES_PER_BLK], xmm3
   add      edi, BYTES_PER_LOOP

   movdqa   xmm0, xmm7                 ; update IV
   sub      edx, BYTES_PER_LOOP
   jge      .blks_loop

;;
;; block-by-block processing
;;
.short_input:
   add      edx, BYTES_PER_LOOP
   jz       .quit

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      eax, nr
   lea      ebx,[eax*4]
   lea      ebx, [ecx+ebx*4-9*(SC)*4]  ; AES-128 round keys

.single_blk_loop:
   pxor        xmm0, oword [ecx]      ; whitening

   cmp         eax,12                     ; switch according to number of rounds
   jl          .key_128_s
   jz          .key_192_s

.key_256_s:
   aesenc      xmm0, oword [ebx-4*4*SC]
   aesenc      xmm0, oword [ebx-3*4*SC]
.key_192_s:
   aesenc      xmm0, oword [ebx-2*4*SC]
   aesenc      xmm0, oword [ebx-1*4*SC]
.key_128_s:
   aesenc      xmm0, oword [ebx+0*4*SC]
   aesenc      xmm0, oword [ebx+1*4*SC]
   aesenc      xmm0, oword [ebx+2*4*SC]
   aesenc      xmm0, oword [ebx+3*4*SC]
   aesenc      xmm0, oword [ebx+4*4*SC]
   aesenc      xmm0, oword [ebx+5*4*SC]
   aesenc      xmm0, oword [ebx+6*4*SC]
   aesenc      xmm0, oword [ebx+7*4*SC]
   aesenc      xmm0, oword [ebx+8*4*SC]
   aesenclast  xmm0, oword [ebx+9*4*SC]

   movdqu      xmm1, oword [esi]       ; input block from the stream
   add         esi, BYTES_PER_BLK

   pxor        xmm0, xmm1                 ; xor src
   movdqu      oword [edi], xmm0       ; and save output
   add         edi, BYTES_PER_BLK

   movdqa      xmm0, xmm1                 ; update IV
   sub         edx, BYTES_PER_BLK
   jnz         .single_blk_loop

.quit:
   REST_GPR
   ret
ENDFUNC DecryptCFB128_RIJ128pipe_AES_NI
%endif

