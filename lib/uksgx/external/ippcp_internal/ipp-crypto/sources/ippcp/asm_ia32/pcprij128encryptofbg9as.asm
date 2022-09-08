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


;***************************************************************
;* Purpose:    RIJ128 OFB encryption
;*
;* void EncryptOFB_RIJ128_AES_NI(const Ipp32u* inpBlk,
;*                                     Ipp32u* outBlk,
;*                                     int nr,
;*                               const Ipp32u* pRKey,
;*                                     int length,
;*                                     int ofbBlks,
;*                               const Ipp8u* pIV)
;***************************************************************

%if (_IPP >= _IPP_P8)
;;
;; Lib = P8
;;
;; Caller = ippsRijndael128DecryptOFB
;;

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
IPPASM EncryptOFB_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInpBlk [ebp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [ebp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [ebp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [ebp + ARG_1 + 4*sizeof(dword)] ; length of stream in bytes
%xdefine ofbSize [ebp + ARG_1 + 5*sizeof(dword)] ; ofb blk size
%xdefine pIV     [ebp + ARG_1 + 6*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

%xdefine tmpInp   esp
%xdefine tmpOut   tmpInp+sizeof(oword)
%xdefine locDst   tmpOut+sizeof(oword)
%xdefine locSrc   locDst+sizeof(oword)*4
%xdefine locLen   locSrc+sizeof(oword)*4
%xdefine stackLen sizeof(oword)+sizeof(oword)+sizeof(oword)*4+sizeof(oword)*4+sizeof(dword)

   sub      esp,stackLen   ; allocate stack

   mov      eax, pIV                   ; get IV
   movdqu   xmm0, oword [eax]       ;
   movdqu   oword [tmpInp], xmm0   ; and save into the stack

   mov      eax, nr                    ; number of rounds
   mov      ecx, pKey                  ; key material address
   lea      eax, [eax*4]               ; nr*16 offset (bytes) to end of key material
   lea      eax, [eax*4]
   lea      ecx, [ecx+eax]
   neg      eax                        ; save -nr*16
   mov      nr, eax
   mov      pKey, ecx                  ; save key material address

   mov      esi, pInpBlk               ; input stream
   mov      edi, pOutBlk               ; output stream
   mov      ebx, ofbSize               ; cfb blk size

align IPP_ALIGN_FACTOR
;;
;; processing
;;
.blks_loop:
   lea      ebx, [ebx*BLKS_PER_LOOP]   ; 4 cfb block

   cmp      len, ebx
   cmovl    ebx, len
   COPY_8U  {locSrc}, esi, ebx, dl     ; move 1-4 input blocks to stack

   mov      ecx, pKey
   mov      eax, nr

   mov      dword [locLen], ebx
   xor      edx, edx                   ; index

align IPP_ALIGN_FACTOR
.single_blk:
   movdqa   xmm3, oword [ecx+eax]   ; preload key material
   add      eax, 16

   movdqa   xmm4, oword [ecx+eax]   ; preload next key material
   pxor     xmm0, xmm3                 ; whitening

align IPP_ALIGN_FACTOR
.cipher_loop:
   add         eax, 16
   aesenc      xmm0, xmm4                 ; regular round
   movdqa      xmm4, oword [ecx+eax]
   jnz         .cipher_loop
   aesenclast  xmm0, xmm4                 ; irregular round
   movdqu      oword [tmpOut], xmm0    ; save chipher output

   mov         eax, ofbSize                  ; cfb blk size

   movdqu      xmm1, oword [locSrc+edx]   ; get src blocks from the stack
   pxor        xmm1, xmm0                    ; xor src
   movdqu      oword [locDst+edx],xmm1    ;and store into the stack

   movdqu      xmm0, oword [tmpInp+eax]   ; update chiper input (IV)
   movdqu      oword [tmpInp], xmm0

   add         edx, eax                      ; advance index
   mov         eax, nr
   cmp         edx, ebx
   jl          .single_blk

   COPY_8U     edi, {locDst}, edx, bl     ; move 1-4 blocks to output

   mov         ebx, ofbSize               ; restore cfb blk size
   add         esi, edx                   ; advance pointers
   add         edi, edx
   sub         len, edx                   ; decrease stream counter
   jg          .blks_loop

   mov      eax, pIV                   ; IV address
   movdqu   xmm0, oword [tmpInp]    ; update IV before return
   movdqu   oword [eax], xmm0

   add      esp,stackLen   ; remove local variables
   REST_GPR
   ret
ENDFUNC EncryptOFB_RIJ128_AES_NI


align IPP_ALIGN_FACTOR
IPPASM EncryptOFB128_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi

%xdefine pInpBlk [esp + ARG_1 + 0*sizeof(dword)] ; input  block address
%xdefine pOutBlk [esp + ARG_1 + 1*sizeof(dword)] ; output block address
%xdefine nr      [esp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [esp + ARG_1 + 4*sizeof(dword)] ; length of stream in bytes
%xdefine pIV     [esp + ARG_1 + 5*sizeof(dword)] ; IV

%xdefine SC  (4)
%assign BLKS_PER_LOOP  (4)

   mov      eax, pIV                   ; get IV
   movdqu   xmm0, oword [eax]       ;

   mov      eax, nr                    ; number of rounds
   mov      ecx, pKey                  ; key material address
   lea      eax, [eax*4]               ; nr*16 offset (bytes) to end of key material
   lea      eax, [eax*4]
   lea      ecx, [ecx+eax]
   neg      eax                        ; save -nr*16
   mov      nr, eax

   mov      esi, pInpBlk               ; input stream
   mov      edi, pOutBlk               ; output stream
   mov      edx, len                   ; length of stream

align IPP_ALIGN_FACTOR
;;
;; processing
;;
.blks_loop:
   movdqa   xmm3, oword [ecx+eax]   ; preload key material
   add      eax, 16

align IPP_ALIGN_FACTOR
.single_blk:
   movdqa   xmm4, oword [ecx+eax]   ; preload next key material
   pxor     xmm0, xmm3                 ; whitening

   movdqu      xmm1, oword [esi]    ; input block

align IPP_ALIGN_FACTOR
.cipher_loop:
   add         eax, 16
   aesenc      xmm0, xmm4              ; regular round
   movdqa      xmm4, oword [ecx+eax]
   jnz         .cipher_loop
   aesenclast  xmm0, xmm4              ; irregular round

   pxor        xmm1, xmm0              ; xor src
   movdqu      oword [edi],xmm1     ; and store into the dst

   mov         eax, nr                 ; restore key material counter
   add         esi, 16                 ; advance pointers
   add         edi, 16
   sub         edx, 16                 ; decrease stream counter
   jg          .blks_loop

   mov      eax, pIV                   ; get IV address
   movdqu   oword [eax], xmm0       ; update IV before return

   REST_GPR
   ret
ENDFUNC EncryptOFB128_RIJ128_AES_NI

%endif


