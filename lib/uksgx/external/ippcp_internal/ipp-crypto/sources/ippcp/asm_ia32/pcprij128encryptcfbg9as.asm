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
;        Encrypt_RIJ128_AES_NI()
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
;* Purpose:    RIJ128 CFB encryption
;*
;* void EncryptCFB_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                     Ipp32u* outBlk,
;*                                     int nr,
;*                               const Ipp32u* pRKey,
;*                                     int len,
;*                                     int cfbSize,
;*                               const Ipp8u* pIV)
;***************************************************************

%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128EncryptCFB
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM EncryptCFB_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp
  
  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  blocks address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output blocks address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in bytes
%xdefine cfbSize [ebp + ARG_1 + 5*sizeof(dword)] ; cfb blk size
%xdefine pIV     [ebp + ARG_1 + 6*sizeof(dword)] ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   sub      esp,16*(1+4+4)    ; allocate stask:
                              ; +0*16  IV
                              ; +1*16  inp0, inp1, inp2, inp3
                              ; +5*16  out0, out1, out2, out3

   mov      eax, pIV                   ; IV address
   movdqu   xmm4, oword [eax]       ; get IV
   movdqu   oword [esp+0*16], xmm4 ; into the stack

;;
;; processing
;;
.blks_loop:
   mov      esi,pInpBlk                ; input data address

   mov      edx,cfbSize                ; size of block
   lea      ebx, [edx*BLKS_PER_LOOP]   ; 4 cfb block
   mov      edx, len
   cmp      edx, ebx
   cmovl    ebx, edx
   COPY_8U {esp+5*16}, esi, ebx, dl    ; move 1-4 input blocks to stack

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      ecx, pKey
   mov      edx, nr
   lea      eax,[edx*4]
   lea      eax, [ecx+eax*4-9*(SC)*4]  ; AES-128 round keys

   xor      esi, esi                   ; index
   mov      edi, ebx
.single_blk:
   movdqu   xmm0, oword [esp+esi]  ; get processing blocks

   pxor     xmm0, oword [ecx]      ; whitening

   cmp      edx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s
                                       ; do encryption
.key_256_s:
   aesenc     xmm0, oword [eax-4*4*SC]
   aesenc     xmm0, oword [eax-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [eax-2*4*SC]
   aesenc     xmm0, oword [eax-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [eax+0*4*SC]
   aesenc     xmm0, oword [eax+1*4*SC]
   aesenc     xmm0, oword [eax+2*4*SC]
   aesenc     xmm0, oword [eax+3*4*SC]
   aesenc     xmm0, oword [eax+4*4*SC]
   aesenc     xmm0, oword [eax+5*4*SC]
   aesenc     xmm0, oword [eax+6*4*SC]
   aesenc     xmm0, oword [eax+7*4*SC]
   aesenc     xmm0, oword [eax+8*4*SC]
   aesenclast xmm0, oword [eax+9*4*SC]

   movdqu      xmm1, oword [esp+5*16+esi] ; get src blocks from the stack
   pxor        xmm0, xmm1                    ; xor src
   movdqu      oword [esp+1*16+esi],xmm0  ;and store into the stack

   add         esi, cfbSize                  ; advance index
   sub         edi, cfbSize                  ; decrease length
   jg          .single_blk

   mov         edi,pOutBlk                   ; output data address
   COPY_8U     edi, {esp+1*16}, ebx, dl      ; move 1-4 blocks to output

   movdqu      xmm0, oword [esp+ebx]; update IV
   movdqu      oword [esp], xmm0

   add         pInpBlk, ebx
   add         pOutBlk, ebx
   sub         len, ebx
   jg          .blks_loop

   add         esp, 16*(1+4+4)
   REST_GPR
   ret
ENDFUNC EncryptCFB_RIJ128_AES_NI

align IPP_ALIGN_FACTOR
IPPASM EncryptCFB32_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp
  
  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  blocks address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output blocks address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in bytes
%xdefine cfbSize [ebp + ARG_1 + 5*sizeof(dword)] ; cfb blk size
%xdefine pIV     [ebp + ARG_1 + 6*sizeof(dword)] ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   sub      esp,16*(1+4+4)    ; allocate stask:
                              ; +0*16  IV
                              ; +1*16  inp0, inp1, inp2, inp3
                              ; +5*16  out0, out1, out2, out3

   mov      eax, pIV                   ; IV address
   movdqu   xmm4, oword [eax]       ; get IV
   movdqu   oword [esp+0*16], xmm4 ; into the stack

;;
;; processing
;;
.blks_loop:
   mov      esi,pInpBlk                ; input data address

   mov      edx,cfbSize                ; size of block
   lea      ebx, [edx*BLKS_PER_LOOP]   ; 4 cfb block
   mov      edx, len
   cmp      edx, ebx
   cmovl    ebx, edx
   COPY_32U {esp+5*16}, esi, ebx, edx  ; move 1-4 input blocks to stack

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      ecx, pKey
   mov      edx, nr
   lea      eax,[edx*4]
   lea      eax, [ecx+eax*4-9*(SC)*4]  ; AES-128 round keys

   xor      esi, esi                   ; index
   mov      edi, ebx
.single_blk:
   movdqu   xmm0, oword [esp+esi]  ; get processing blocks

   pxor     xmm0, oword [ecx]      ; whitening

   cmp      edx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s
                                       ; do encryption
.key_256_s:
   aesenc     xmm0, oword [eax-4*4*SC]
   aesenc     xmm0, oword [eax-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [eax-2*4*SC]
   aesenc     xmm0, oword [eax-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [eax+0*4*SC]
   aesenc     xmm0, oword [eax+1*4*SC]
   aesenc     xmm0, oword [eax+2*4*SC]
   aesenc     xmm0, oword [eax+3*4*SC]
   aesenc     xmm0, oword [eax+4*4*SC]
   aesenc     xmm0, oword [eax+5*4*SC]
   aesenc     xmm0, oword [eax+6*4*SC]
   aesenc     xmm0, oword [eax+7*4*SC]
   aesenc     xmm0, oword [eax+8*4*SC]
   aesenclast xmm0, oword [eax+9*4*SC]

   movdqu      xmm1, oword [esp+5*16+esi] ; get src blocks from the stack
   pxor        xmm0, xmm1                    ; xor src
   movdqu      oword [esp+1*16+esi],xmm0  ;and store into the stack

   add         esi, cfbSize                  ; advance index
   sub         edi, cfbSize                  ; decrease length
   jg          .single_blk

   mov         edi,pOutBlk                   ; output data address
   COPY_32U    edi, {esp+1*16}, ebx, edx     ; move 1-4 blocks to output

   movdqu      xmm0, oword [esp+ebx]      ; update IV
   movdqu      oword [esp], xmm0

   add         pInpBlk, ebx
   add         pOutBlk, ebx
   sub         len, ebx
   jg          .blks_loop

   add         esp, 16*(1+4+4)
   REST_GPR
   ret
ENDFUNC EncryptCFB32_RIJ128_AES_NI


align IPP_ALIGN_FACTOR
IPPASM EncryptCFB128_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  blocks address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output blocks address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in bytes
%xdefine pIV     [ebp + ARG_1 + 5*sizeof(dword)] ; pointer to the IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   mov      eax, pIV                   ; IV address
   movdqu   xmm0, oword [eax]       ; get IV

   mov      esi,pInpBlk                ; input data address
   mov      edi,pOutBlk                ; output data address
   mov      ebx, len

   ; get actual address of key material: pRKeys += (nr-9) * SC
   mov      ecx, pKey
   mov      edx, nr
   lea      eax,[edx*4]
   lea      eax, [ecx+eax*4-9*(SC)*4]  ; AES-128 round keys


;;
;; processing
;;
.blks_loop:
   pxor     xmm0, oword [ecx]      ; whitening

   movdqu   xmm1, oword [esi]       ; input blocks


   cmp      edx,12                     ; switch according to number of rounds
   jl       .key_128_s
   jz       .key_192_s
                                       ; do encryption
.key_256_s:
   aesenc     xmm0, oword [eax-4*4*SC]
   aesenc     xmm0, oword [eax-3*4*SC]
.key_192_s:
   aesenc     xmm0, oword [eax-2*4*SC]
   aesenc     xmm0, oword [eax-1*4*SC]
.key_128_s:
   aesenc     xmm0, oword [eax+0*4*SC]
   aesenc     xmm0, oword [eax+1*4*SC]
   aesenc     xmm0, oword [eax+2*4*SC]
   aesenc     xmm0, oword [eax+3*4*SC]
   aesenc     xmm0, oword [eax+4*4*SC]
   aesenc     xmm0, oword [eax+5*4*SC]
   aesenc     xmm0, oword [eax+6*4*SC]
   aesenc     xmm0, oword [eax+7*4*SC]
   aesenc     xmm0, oword [eax+8*4*SC]
   aesenclast xmm0, oword [eax+9*4*SC]

   pxor        xmm0, xmm1                    ; xor src
   movdqu      oword [edi],xmm0           ;and store into the dst

   add         esi, 16
   add         edi, 16
   sub         ebx, 16
   jg          .blks_loop

   REST_GPR
   ret
ENDFUNC EncryptCFB128_RIJ128_AES_NI

%endif


