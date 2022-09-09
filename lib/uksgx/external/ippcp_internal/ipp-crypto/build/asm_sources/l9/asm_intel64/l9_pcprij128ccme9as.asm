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
;        AuthEncrypt_RIJ128_AES_NI()
;        DecryptAuth_RIJ128_AES_NI()
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_ON_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_Y8)

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
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
;; Lib = Y8
;;
;; Caller = ippsAES_CCMEncrypt
;;
align IPP_ALIGN_FACTOR
IPPASM AuthEncrypt_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7
        COMP_ABI 6
;; rdi:     pInpBlk:  BYTE,  ; input  blocks address
;; rsi:     pOutBlk:  BYTE,  ; output blocks address
;; rdx:     nr:          DWORD,  ; number of rounds
;; rcx      pKey:     BYTE,  ; key material address
;; r8d      length:      DWORD,  ; length (bytes)
;; r9       pLocCtx:  BYTE   ; pointer to the localState

%assign BYTES_PER_BLK  (16)

   movdqa   xmm0, oword [r9]                   ; MAC
   movdqa   xmm2, oword [r9+sizeof(oword)]     ; CTRi block
   movdqa   xmm1, oword [r9+sizeof(oword)*2]   ; CTR mask

   movdqa   xmm7, oword [rel u128_str]

   pshufb   xmm2, xmm7     ; CTRi block (LE)
   pshufb   xmm1, xmm7     ; CTR mask

   movdqa   xmm3, xmm1
   pandn    xmm3, xmm2     ; CTR block template
   pand     xmm2, xmm1     ; CTR value

   mov      r8d, r8d       ; expand length
   movsxd   rdx, edx       ; expand number of rounds

   lea      rdx, [rdx*4]   ; nrCounter = -nr*16
   lea      rdx, [rdx*4]   ; pKey += nr*16
   lea      rcx, [rcx+rdx]
   neg      rdx
   mov      rbx, rdx

align IPP_ALIGN_FACTOR
;;
;; block-by-block processing
;;
.blk_loop:
   movdqu   xmm4, oword [rdi]      ; input block src[i]
   pxor     xmm0, xmm4                 ; MAC ^= src[i]

   movdqa   xmm5, xmm3
   paddq    xmm2, oword [rel increment]  ; advance counter bits
   pand     xmm2, xmm1                 ; and mask them
   por      xmm5, xmm2
   pshufb   xmm5, xmm7                 ; CTRi (BE)

   movdqa   xmm6, oword [rcx+rdx]  ; keys for whitening
   add      rdx, 16

   pxor     xmm5, xmm6                 ; whitening (CTRi)
   pxor     xmm0, xmm6                 ; whitening (MAC)

   movdqa   xmm6, oword [rcx+rdx]  ; pre load operation's keys

align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc   xmm5, xmm6                 ; regular round (CTRi)
   aesenc   xmm0, xmm6                 ; regular round (MAC)
   movdqa   xmm6, oword [rcx+rdx+16]
   add      rdx, 16
   jnz      .cipher_loop
   aesenclast  xmm5, xmm6              ; irregular round (CTRi)
   aesenclast  xmm0, xmm6              ; irregular round (MAC)

   pxor     xmm4, xmm5                 ; dst[i] = src[i] ^ ENC(CTRi)
   movdqu   oword [rsi], xmm4

   mov      rdx, rbx
   add      rsi, BYTES_PER_BLK
   add      rdi, BYTES_PER_BLK
   sub      r8,  BYTES_PER_BLK
   jnz      .blk_loop

   movdqu   oword [r9], xmm0                 ; update MAC value
   movdqu   oword [r9+sizeof(oword)], xmm5   ; update ENC(Ctri)

   pxor     xmm6, xmm6

   REST_XMM
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
;; Lib = Y8
;;
;; Caller = ippsAES_CCMDecrypt
;;
align IPP_ALIGN_FACTOR
IPPASM DecryptAuth_RIJ128_AES_NI,PUBLIC
%assign LOCAL_FRAME sizeof(qword)
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7
        COMP_ABI 6
;; rdi:     pInpBlk:  BYTE,  ; input  blocks address
;; rsi:     pOutBlk:  BYTE,  ; output blocks address
;; rdx:     nr:          DWORD,  ; number of rounds
;; rcx      pKey:     BYTE,  ; key material address
;; r8d      length:      DWORD,  ; length (bytes)
;; r9       pLocCtx:  BYTE   ; pointer to the localState

%assign BYTES_PER_BLK  (16)

   movdqa   xmm0, oword [r9]                   ; MAC
   movdqa   xmm2, oword [r9+sizeof(oword)]     ; CTRi block
   movdqa   xmm1, oword [r9+sizeof(oword)*2]   ; CTR mask

   movdqa   xmm7, oword [rel u128_str]

   pshufb   xmm2, xmm7     ; CTRi block (LE)
   pshufb   xmm1, xmm7     ; CTR mask

   movdqa   xmm3, xmm1
   pandn    xmm3, xmm2     ; CTR block template
   pand     xmm2, xmm1     ; CTR value

   mov      r8d, r8d       ; expand length
   movsxd   rdx, edx       ; expand number of rounds

   lea      rdx, [rdx*4]   ; nrCounter = -nr*16
   lea      rdx, [rdx*4]   ; pKey += nr*16
   lea      rcx, [rcx+rdx]
   neg      rdx
   mov      rbx, rdx

align IPP_ALIGN_FACTOR
;;
;; block-by-block processing
;;
.blk_loop:

   ;;;;;;;;;;;;;;;;;
   ;; decryption
   ;;;;;;;;;;;;;;;;;
   movdqu   xmm4, oword [rdi]      ; input block src[i]

   movdqa   xmm5, xmm3
   paddq    xmm2, oword [rel increment]  ; advance counter bits
   pand     xmm2, xmm1                 ; and mask them
   por      xmm5, xmm2
   pshufb   xmm5, xmm7                 ; CTRi (BE)

   movdqa   xmm6, oword [rcx+rdx]  ; keys for whitening
   add      rdx, 16

   pxor     xmm5, xmm6                 ; whitening (CTRi)
   movdqa   xmm6, oword [rcx+rdx]  ; pre load operation's keys

align IPP_ALIGN_FACTOR
.cipher_loop:
   aesenc   xmm5, xmm6                 ; regular round (CTRi)
   movdqa   xmm6, oword [rcx+rdx+16]
   add      rdx, 16
   jnz      .cipher_loop
   aesenclast  xmm5, xmm6              ; irregular round (CTRi)

   pxor     xmm4, xmm5                 ; dst[i] = src[i] ^ ENC(CTRi)
   movdqu   oword [rsi], xmm4

   ;;;;;;;;;;;;;;;;;
   ;; update MAC
   ;;;;;;;;;;;;;;;;;
   mov      rdx, rbx

   movdqa   xmm6, oword [rcx+rdx]  ; keys for whitening
   add      rdx, 16

   pxor     xmm0, xmm4                 ; MAC ^= dst[i]
   pxor     xmm0, xmm6                 ; whitening (MAC)

   movdqa   xmm6, oword [rcx+rdx]  ; pre load operation's keys

align IPP_ALIGN_FACTOR
.auth_loop:
   aesenc   xmm0, xmm6                 ; regular round (MAC)
   movdqa   xmm6, oword [rcx+rdx+16]
   add      rdx, 16
   jnz      .auth_loop
   aesenclast  xmm0, xmm6              ; irregular round (MAC)


   mov      rdx, rbx
   add      rsi, BYTES_PER_BLK
   add      rdi, BYTES_PER_BLK
   sub      r8,  BYTES_PER_BLK
   jnz      .blk_loop

   movdqu   oword [r9], xmm0                 ; update MAC value
   movdqu   oword [r9+sizeof(oword)], xmm6   ; update ENC(Ctri)

   pxor     xmm6, xmm6

   REST_XMM
   REST_GPR
   ret
ENDFUNC DecryptAuth_RIJ128_AES_NI

%endif
%endif ;; _AES_NI_ENABLING_


