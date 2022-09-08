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
;        AuthEncrypt_RIJ128_AES_NI()
;        DecryptAuth_RIJ128_AES_NI()
;




%include "asmdefs.inc"
%include "ia_emm.inc"


%if (_IPP >= _IPP_P8)

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
ENCODE_DATA:
u128_str    DB 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
increment   DQ 1,0

;***************************************************************
;* Purpose:    Authenticate and Encrypt
;*
;* void AuthEncrypt_RIJ128_AES_NI(Ipp8u* outBlk,
;*                          const Ipp8u* inpBlk,
;*                                int nr,
;*                          const Ipp8u* pRKey,
;*                                Ipp32u len,
;*                                Ipp8u* pLocalState)
;* inp localCtx:
;*    MAC
;*    CTRi
;*    CTRi mask
;*
;* out localCtx:
;*    new MAC
;*    S = enc(CTRi)
;***************************************************************

;;
;; Lib = P8
;;
;; Caller = ippsRijndael128CCMEncrypt
;; Caller = ippsRijndael128CCMEncryptMessage
;;
align IPP_ALIGN_FACTOR
IPPASM AuthEncrypt_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pInpBlk [esp + ARG_1 + 0*sizeof(dword)] ; input  blocks address
%xdefine pOutBlk [esp + ARG_1 + 1*sizeof(dword)] ; output blocks address
%xdefine nr      [esp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [esp + ARG_1 + 4*sizeof(dword)] ; length (bytes)
%xdefine pLocCtx [esp + ARG_1 + 5*sizeof(dword)] ; pointer to the localState

%assign BYTES_PER_BLK  (16)

   mov      eax, pLocCtx
   movdqa   xmm0, oword [eax]                  ; MAC
   movdqa   xmm2, oword [eax+sizeof(oword)]    ; CTRi block
   movdqa   xmm1, oword [eax+sizeof(oword)*2]  ; CTR mask

   LD_ADDR  eax, ENCODE_DATA

   movdqa   xmm7, oword [eax+(u128_str-ENCODE_DATA)]

   pshufb   xmm2, xmm7     ; CTRi block (LE)
   pshufb   xmm1, xmm7     ; CTR mask

   movdqa   xmm3, xmm1
   pandn    xmm3, xmm2     ; CTR block template
   pand     xmm2, xmm1     ; CTR value

   mov      edx, nr        ; number of rounds
   mov      ecx, pKey      ; and keys

   lea      edx, [edx*4]   ; nrCounter = -nr*16
   lea      edx, [edx*4]   ; pKey += nr*16
   lea      ecx, [ecx+edx]
   neg      edx
   mov      ebx, edx

   mov      esi, pInpBlk
   mov      edi, pOutBlk

align IPP_ALIGN_FACTOR
;;
;; block-by-block processing
;;
.blk_loop:
   movdqu   xmm4, oword [esi]      ; input block src[i]
   pxor     xmm0, xmm4                 ; MAC ^= src[i]

   movdqa   xmm5, xmm3
   paddq    xmm2, oword [eax+(increment-ENCODE_DATA)] ; advance counter bits
   pand     xmm2, xmm1                 ; and mask them
   por      xmm5, xmm2
   pshufb   xmm5, xmm7                 ; CTRi (BE)

   movdqa   xmm6, oword [ecx+edx]  ; keys for whitening
   add      edx, 16

   pxor     xmm5, xmm6                 ; whitening (CTRi)
   pxor     xmm0, xmm6                 ; whitening (MAC)

   movdqa   xmm6, oword [ecx+edx]  ; pre load operation's keys

align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc   xmm5, xmm6                 ; regular round (CTRi)
   aesenc   xmm0, xmm6                 ; regular round (MAC)
   movdqa   xmm6, oword [ecx+edx+16]
   add      edx, 16
   jnz      .cipher_loop
   aesenclast  xmm5, xmm6              ; irregular round (CTRi)
   aesenclast  xmm0, xmm6              ; irregular round (MAC)

   pxor     xmm4, xmm5                 ; dst[i] = src[i] ^ ENC(CTRi)
   movdqu   oword [edi], xmm4

   mov      edx, ebx
   add      esi, BYTES_PER_BLK
   add      edi, BYTES_PER_BLK
   sub      dword len, BYTES_PER_BLK
   jnz      .blk_loop

   mov      eax, pLocCtx
   movdqu   oword [eax], xmm0                ; update MAC value
   movdqu   oword [eax+sizeof(oword)], xmm5  ; update ENC(Ctri)

   REST_GPR
   ret
ENDFUNC AuthEncrypt_RIJ128_AES_NI


;***************************************************************
;* Purpose:    Decrypt and Authenticate
;*
;* void DecryptAuth_RIJ128_AES_NI(Ipp8u* outBlk,
;*                          const Ipp8u* inpBlk,
;*                                int nr,
;*                          const Ipp8u* pRKey,
;*                                Ipp32u len,
;*                                Ipp8u* pLocalState)
;* inp localCtx:
;*    MAC
;*    CTRi
;*    CTRi mask
;*
;* out localCtx:
;*    new MAC
;*    S = enc(CTRi)
;***************************************************************

;;
;; Lib = P8
;;
;; Caller = ippsRijndael128CCMDecrypt
;; Caller = ippsRijndael128CCMDecryptMessage
;;
align IPP_ALIGN_FACTOR
IPPASM DecryptAuth_RIJ128_AES_NI,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pInpBlk [esp + ARG_1 + 0*sizeof(dword)] ; input  blocks address
%xdefine pOutBlk [esp + ARG_1 + 1*sizeof(dword)] ; output blocks address
%xdefine nr      [esp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pKey    [esp + ARG_1 + 3*sizeof(dword)] ; key material address
%xdefine len     [esp + ARG_1 + 4*sizeof(dword)] ; length (bytes)
%xdefine pLocCtx [esp + ARG_1 + 5*sizeof(dword)] ; pointer to the localState

%assign BYTES_PER_BLK  (16)

   mov      eax, pLocCtx
   movdqa   xmm0, oword [eax]                  ; MAC
   movdqa   xmm2, oword [eax+sizeof(oword)]    ; CTRi block
   movdqa   xmm1, oword [eax+sizeof(oword)*2]  ; CTR mask

   LD_ADDR  eax, ENCODE_DATA

   movdqa   xmm7, oword [eax+(u128_str-ENCODE_DATA)]

   pshufb   xmm2, xmm7     ; CTRi block (LE)
   pshufb   xmm1, xmm7     ; CTR mask

   movdqa   xmm3, xmm1
   pandn    xmm3, xmm2     ; CTR block template
   pand     xmm2, xmm1     ; CTR value

   mov      edx, nr        ; number of rounds
   mov      ecx, pKey      ; and keys

   lea      edx, [edx*4]   ; nrCounter = -nr*16
   lea      edx, [edx*4]   ; pKey += nr*16
   lea      ecx, [ecx+edx]
   neg      edx
   mov      ebx, edx

   mov      esi, pInpBlk
   mov      edi, pOutBlk

align IPP_ALIGN_FACTOR
;;
;; block-by-block processing
;;
.blk_loop:

   ;;;;;;;;;;;;;;;;;
   ;; decryption
   ;;;;;;;;;;;;;;;;;
   movdqu   xmm4, oword [esi]      ; input block src[i]

   movdqa   xmm5, xmm3
   paddq    xmm2, oword [eax+(increment-ENCODE_DATA)] ; advance counter bits
   pand     xmm2, xmm1                 ; and mask them
   por      xmm5, xmm2
   pshufb   xmm5, xmm7                 ; CTRi (BE)

   movdqa   xmm6, oword [ecx+edx]  ; keys for whitening
   add      edx, 16

   pxor     xmm5, xmm6                 ; whitening (CTRi)
   movdqa   xmm6, oword [ecx+edx]  ; pre load operation's keys

align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc   xmm5, xmm6                 ; regular round (CTRi)
   movdqa   xmm6, oword [ecx+edx+16]
   add      edx, 16
   jnz      .cipher_loop
   aesenclast  xmm5, xmm6              ; irregular round (CTRi)

   pxor     xmm4, xmm5                 ; dst[i] = src[i] ^ ENC(CTRi)
   movdqu   oword [edi], xmm4

   ;;;;;;;;;;;;;;;;;
   ;; update MAC
   ;;;;;;;;;;;;;;;;;
   mov      edx, ebx

   movdqa   xmm6, oword [ecx+edx]  ; keys for whitening
   add      edx, 16

   pxor     xmm0, xmm4                 ; MAC ^= dst[i]
   pxor     xmm0, xmm6                 ; whitening (MAC)

   movdqa   xmm6, oword [ecx+edx]  ; pre load operation's keys

align IPP_ALIGN_FACTOR
.auth_loop:
   aesenc   xmm0, xmm6                 ; regular round (MAC)
   movdqa   xmm6, oword [ecx+edx+16]
   add      edx, 16
   jnz      .auth_loop
   aesenclast  xmm0, xmm6              ; irregular round (MAC)


   mov      edx, ebx
   add      esi, BYTES_PER_BLK
   add      edi, BYTES_PER_BLK
   sub      dword len, BYTES_PER_BLK
   jnz      .blk_loop

   mov      eax, pLocCtx
   movdqu   oword [eax], xmm0                ; update MAC value
   movdqu   oword [eax+sizeof(oword)], xmm6  ; update ENC(Ctri)

   REST_GPR
   ret
ENDFUNC DecryptAuth_RIJ128_AES_NI

%endif


