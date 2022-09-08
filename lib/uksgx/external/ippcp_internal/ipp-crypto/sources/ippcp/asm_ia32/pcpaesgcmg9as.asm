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
;               AES-GCM function
;
;     Content:
;      AesGcmPrecompute_avx()
;      AesGcmMulGcm_avx()
;      AesGcmAuth_avx()
;      AesGcmEnc_avx()
;      AesGcmDec_avx()
;
;
;

%include "asmdefs.inc"
%include "ia_emm.inc"

%assign my_emulator  0; set 1 for emulation
%include "emulator.inc"

;;
;; a = a*b mod g(x), g(x) = x^128 + x^7 + x^2 +x +1
;;
%macro sse_clmul_gcm 5.nolist
  %xdefine %%GH %1
  %xdefine %%HK %2
  %xdefine %%tmpX0 %3
  %xdefine %%tmpX1 %4
  %xdefine %%tmpX2 %5

   ;; GH, HK hold the values for the two operands which are carry-less multiplied
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; Karatsuba Method
   ;;
   ;; GH = [GH1:GH0]
   ;; HK = [HK1:HK0]
   ;;
   pshufd      %%tmpX2, %%GH, 01001110b ;; xmm2 = {GH0:GH1}
   pshufd      %%tmpX0, %%HK, 01001110b ;; xmm0 = {HK0:HK1}
   pxor        %%tmpX2, %%GH            ;; xmm2 = {GH0+GH1:GH1+GH0}
   pxor        %%tmpX0, %%HK            ;; xmm0 = {HK0+HK1:HK1+HK0}

my_pclmulqdq   %%tmpX2, %%tmpX0,00h     ;; tmpX2 = (a1+a0)*(b1+b0)     xmm2 = (GH1+GH0)*(HK1+HK0)
   movdqa      %%tmpX1, %%GH
my_pclmulqdq   %%GH,    %%HK,   00h     ;; GH = a0*b0                  GH   = GH0*HK0
   pxor        %%tmpX0, %%tmpX0
my_pclmulqdq   %%tmpX1, %%HK,   11h     ;; tmpX1 = a1*b1               xmm1 = GH1*HK1
   pxor        %%tmpX2, %%GH            ;;                             xmm2 = (GH1+GH0)*(HK1+HK0) + GH0*HK0
   pxor        %%tmpX2, %%tmpX1         ;; tmpX2 = a0*b1+a1*b0         xmm2 = (GH1+GH0)*(HK1+HK0) + GH0*HK0 + GH1*HK1 = GH0*HK1+GH1*HK0

   palignr     %%tmpX0, %%tmpX2, 8      ;; tmpX0 = {Zeros : HI(a0*b1+a1*b0)}
   pslldq      %%tmpX2, 8             ;; tmpX2 = {LO(HI(a0*b1+a1*b0)) : Zeros}
   pxor        %%tmpX1, %%tmpX0         ;; <xmm1:GH> holds the result of the carry-less multiplication of GH by HK
   pxor        %%GH,    %%tmpX2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;first phase of the reduction:
   ;  Most( (product_H * g1), 128))    product_H = GH
   ;                                   g1 = 2^256/g = g = 1+x+x^2+x^7+x^128
   ;
   movdqa   %%tmpX0, %%GH
   psllq    %%tmpX0, 1          ; GH<<1
   pxor     %%tmpX0, %%GH
   psllq    %%tmpX0, 5          ; ((GH<<1) ^ GH)<<5
   pxor     %%tmpX0, %%GH
   psllq    %%tmpX0, 57         ; (((GH<<1) ^ GH)<<5) ^ GH)<<57     <==>    GH<<63 ^ GH<<62 ^ GH<<57

   movdqa   %%tmpX2, %%tmpX0
   pslldq   %%tmpX2, 8          ; shift-L tmpX2 2 DWs
   psrldq   %%tmpX0, 8          ; shift-R xmm2 2 DWs

   pxor     %%GH, %%tmpX2         ; first phase of the reduction complete
   pxor     %%tmpX1, %%tmpX0      ; save the lost MS 1-2-7 bits from first phase

   ;second phase of the reduction
   movdqa   %%tmpX2, %%GH         ; move GH into xmm15
   psrlq    %%tmpX2, 5          ; packed right shifting >> 5
   pxor     %%tmpX2, %%GH         ; xor shifted versions
   psrlq    %%tmpX2, 1          ; packed right shifting >> 1
   pxor     %%tmpX2, %%GH         ; xor shifted versions
   psrlq    %%tmpX2, 1          ; packed right shifting >> 1

   pxor     %%GH, %%tmpX2         ; second phase of the reduction complete
   pxor     %%GH, %%tmpX1         ; the result is in GH
%endmacro

%if (_IPP >= _IPP_P8)

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
CONST_TABLE:
_poly       DQ    00000000000000001h,0C200000000000000h  ;; 0xC2000000000000000000000000000001
_twoone     DQ    00000000000000001h,00000000100000000h  ;; 0x00000001000000000000000000000001
_u128_str   DB    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
_mask1      DQ    0ffffffffffffffffh,00000000000000000h  ;; 0x0000000000000000ffffffffffffffff
_mask2      DQ    00000000000000000h,0ffffffffffffffffh  ;; 0xffffffffffffffff0000000000000000
_inc1       DQ 1,0

%xdefine POLY     [esi+(_poly     - CONST_TABLE)]
%xdefine TWOONE   [esi+(_twoone   - CONST_TABLE)]
%xdefine u128_str [esi+(_u128_str - CONST_TABLE)]
%xdefine MASK1    [esi+(_mask1    - CONST_TABLE)]
%xdefine MASK2    [esi+(_mask2    - CONST_TABLE)]
%xdefine inc1     [esi+(_inc1     - CONST_TABLE)]

%assign sizeof_oword_  (16)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void GCMpipePrecomute(const Ipp8u* pRefHkey, Ipp8u* pMultipliers);
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmPrecompute_avx,PUBLIC
  USES_GPR esi

%xdefine pHkey        [esp + ARG_1 + 0*sizeof(dword)] ; pointer to the reflected hkey
%xdefine pMultipliers [esp + ARG_1 + 1*sizeof(dword)] ; output to the precomputed multipliers

   LD_ADDR  esi, CONST_TABLE

   mov      eax, pHkey
   movdqu   xmm0, oword [eax]   ;  xmm0 holds HashKey
   pshufb   xmm0, u128_str

   ; precompute HashKey<<1 mod poly from the HashKey
   movdqa   xmm4, xmm0
   psllq    xmm0, 1
   psrlq    xmm4, 63
   movdqa   xmm3, xmm4
   pslldq   xmm4, 8
   psrldq   xmm3, 8
   por      xmm0, xmm4
   ;reduction
   pshufd   xmm4, xmm3, 00100100b
   pcmpeqd  xmm4, oword TWOONE  ; TWOONE = 0x00000001000000000000000000000001
   pand     xmm4, oword POLY
   pxor     xmm0, xmm4              ; xmm0 holds the HashKey<<1 mod poly

   movdqa         xmm1, xmm0
   sse_clmul_gcm  xmm1, xmm0, xmm3, xmm4, xmm5  ; xmm1 holds (HashKey^2)<<1 mod poly

   movdqa         xmm2, xmm1
   sse_clmul_gcm  xmm2, xmm1, xmm3, xmm4, xmm5  ; xmm2 holds (HashKey^4)<<1 mod poly

   mov      eax, pMultipliers
   movdqu   oword [eax+sizeof_oword_*0], xmm0
   movdqu   oword [eax+sizeof_oword_*1], xmm1
   movdqu   oword [eax+sizeof_oword_*2], xmm2

   REST_GPR
   ret
ENDFUNC AesGcmPrecompute_avx


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void AesGcmMulGcm_avx(Ipp8u* pHash, const Ipp8u* pHKey)
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmMulGcm_avx,PUBLIC
  USES_GPR esi,edi

%xdefine pHash [esp + ARG_1 + 0*sizeof(dword)]
%xdefine pHKey [esp + ARG_1 + 1*sizeof(dword)]

   LD_ADDR  esi, CONST_TABLE

   mov   edi, pHash        ; (edi) pointer to the Hash value
   mov   eax, pHKey        ; (eax) pointer to the (hkey<<1) value

   movdqa   xmm0, oword [edi]
   pshufb   xmm0, u128_str
   movdqa   xmm1, oword [eax]

   sse_clmul_gcm  xmm0, xmm1, xmm2, xmm3, xmm4  ; xmm0 holds Hash*HKey mod poly

   pshufb   xmm0, u128_str
   movdqa   oword [edi], xmm0

   REST_GPR
   ret
ENDFUNC AesGcmMulGcm_avx


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void AesGcmAuth_avx(Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pHKey
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmAuth_avx,PUBLIC
  USES_GPR esi,edi

%xdefine pHash [esp + ARG_1 + 0*sizeof(dword)]
%xdefine pSrc  [esp + ARG_1 + 1*sizeof(dword)]
%xdefine len   [esp + ARG_1 + 2*sizeof(dword)]
%xdefine pHKey [esp + ARG_1 + 3*sizeof(dword)]

%assign BYTES_PER_BLK  (16)

   LD_ADDR  esi, CONST_TABLE

   mov      edi, pHash
   movdqa   xmm0, oword [edi]
   pshufb   xmm0, u128_str
   mov      eax, pHKey
   movdqa   xmm1, oword [eax]

   mov      ecx, pSrc
   mov      edx, len

align IPP_ALIGN_FACTOR
.auth_loop:
   movdqu   xmm2, oword [ecx]  ; src[]
   pshufb   xmm2, u128_str
   add      ecx, BYTES_PER_BLK
   pxor     xmm0, xmm2              ; hash ^= src[]

   sse_clmul_gcm  xmm0, xmm1, xmm2, xmm3, xmm4  ; xmm0 holds Hash*HKey mod poly

   sub      edx, BYTES_PER_BLK
   jnz      .auth_loop

   pshufb   xmm0, u128_str
   movdqa   oword [edi], xmm0

   REST_GPR
   ret
ENDFUNC AesGcmAuth_avx


;***************************************************************
;* Purpose:    pipelined AES-GCM encryption
;*
;* void AesGcmEnc_avx(Ipp8u* pDst,
;*              const Ipp8u* pSrc,
;*                    int length,
;*              RijnCipher cipher,
;*                    int nr,
;*              const Ipp8u* pRKey,
;*                    Ipp8u* pGhash,
;*                    Ipp8u* pCtrValue,
;*                    Ipp8u* pEncCtrValue,
;*              const Ipp8u* pPrecomData)
;***************************************************************

;;
;; Lib = P8, G9
;;
;; Caller = ippsRijndael128GCMEncrypt
;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmEnc_avx,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pDst        [ebp + ARG_1 + 0*sizeof(dword)] ; output block address
%xdefine pSrc        [ebp + ARG_1 + 1*sizeof(dword)] ; input  block address
%xdefine len         [ebp + ARG_1 + 2*sizeof(dword)] ; length(byte)
%xdefine cipher      [ebp + ARG_1 + 3*sizeof(dword)]
%xdefine nr          [ebp + ARG_1 + 4*sizeof(dword)] ; number of rounds
%xdefine pKey        [ebp + ARG_1 + 5*sizeof(dword)] ; key material address
%xdefine pGhash      [ebp + ARG_1 + 6*sizeof(dword)] ; hash
%xdefine pCounter    [ebp + ARG_1 + 7*sizeof(dword)] ; counter
%xdefine pEcounter   [ebp + ARG_1 + 8*sizeof(dword)] ; enc. counter
%xdefine pPrecomData [ebp + ARG_1 + 9*sizeof(dword)] ; const multipliers

%xdefine SC             (4)
%assign  BLKS_PER_LOOP  (4)
%assign  BYTES_PER_BLK  (16)
%assign  BYTES_PER_LOOP (BYTES_PER_BLK*BLKS_PER_LOOP)

;;
;; stack structure:
%assign CNT        (0)
%assign ECNT       (CNT+sizeof_oword_)
%assign GHASH      (ECNT+sizeof_oword_)

%assign GHASH0     (GHASH)
%assign GHASH1     (GHASH0+sizeof_oword_)
%assign GHASH2     (GHASH1+sizeof_oword_)
%assign GHASH3     (GHASH2+sizeof_oword_)

%assign SHUF_CONST (GHASH3+sizeof_oword_)
%assign INC_1      (SHUF_CONST+sizeof_oword_)

%assign BLKS4      (INC_1+sizeof_oword_)
%assign BLKS       (BLKS4+sizeof(dword))
%assign STACK_SIZE (BLKS+sizeof(dword)+sizeof_oword_)

   sub      esp, STACK_SIZE             ; alocate stack
   lea      ebx, [esp+sizeof_oword_]    ; align stack
   and      ebx, -sizeof_oword_
   mov      eax, cipher                 ; due to bug in ml12 - dummy instruction
   LD_ADDR  esi, CONST_TABLE
   movdqa   xmm4, oword u128_str
   movdqa   xmm5, oword inc1

   mov      eax, pCounter              ; address of the counter
   mov      ecx, pEcounter             ; address of the encrypted counter
   mov      edx, pGhash                ; address of hash value

   movdqu   xmm0, oword [eax]       ; counter value
   movdqu   xmm1, oword [ecx]       ; encrypted counter value
   movdqu   xmm2, oword [edx]       ; hash value

my_pshufb   xmm0, xmm4                 ; convert counter and
   movdqa   oword [ebx+CNT], xmm0  ; and store into the stack
   movdqa   oword [ebx+ECNT], xmm1 ; store encrypted counter into the stack

my_pshufb   xmm2, xmm4                 ; convert hash value
   pxor     xmm1, xmm1
   movdqa   oword [ebx+GHASH0], xmm2  ; store hash into the stack
   movdqa   oword [ebx+GHASH1], xmm1  ;
   movdqa   oword [ebx+GHASH2], xmm1  ;
   movdqa   oword [ebx+GHASH3], xmm1  ;

   movdqa   oword [ebx+SHUF_CONST], xmm4 ; store constants into the stack
   movdqa   oword [ebx+INC_1], xmm5

   mov      ecx, pKey                  ; key marerial
   mov      esi, pSrc                  ; src/dst pointers
   mov      edi, pDst

   mov      eax, len
   mov      edx, BYTES_PER_LOOP-1
   and      edx, eax
   and      eax,-BYTES_PER_LOOP
   mov      dword [ebx+BLKS4], eax ; 4-blks counter
   mov      dword [ebx+BLKS], edx  ; rest counter
   jz       .single_block_proc

;;
;; pipelined processing
;;
align IPP_ALIGN_FACTOR
.blks4_loop:
   ;;
   ;; ctr encryption
   ;;
   movdqa   xmm5, oword [ebx+INC_1]

   movdqa   xmm1, xmm0                 ; counter+1
   paddd    xmm1, xmm5
   movdqa   xmm2, xmm1                 ; counter+2
   paddd    xmm2, xmm5
   movdqa   xmm3, xmm2                 ; counter+3
   paddd    xmm3, xmm5
   movdqa   xmm4, xmm3                 ; counter+4
   paddd    xmm4, xmm5
   movdqa   oword [ebx+CNT], xmm4

   movdqa   xmm5,oword [ebx+SHUF_CONST]

   movdqa   xmm0, oword [ecx]       ; pre-load whitening keys
   lea      eax, [ecx+16]              ; pointer to the round's key material

my_pshufb   xmm1, xmm5                 ; counter, counter+1, counter+2, counter+3
my_pshufb   xmm2, xmm5                 ; ready to be encrypted
my_pshufb   xmm3, xmm5
my_pshufb   xmm4, xmm5

   pxor     xmm1, xmm0                 ; whitening
   pxor     xmm2, xmm0
   pxor     xmm3, xmm0
   pxor     xmm4, xmm0

   movdqa   xmm0, oword [eax]       ; pre load round keys
   add      eax, 16

   mov      edx, nr                    ; counter depending on key length
   sub      edx, 1

align IPP_ALIGN_FACTOR
.cipher4_loop:
my_aesenc      xmm1, xmm0              ; regular round
my_aesenc      xmm2, xmm0
my_aesenc      xmm3, xmm0
my_aesenc      xmm4, xmm0
   movdqa      xmm0, oword [eax]
   add         eax, 16
   dec         edx
   jnz         .cipher4_loop
my_aesenclast  xmm1, xmm0
my_aesenclast  xmm2, xmm0
my_aesenclast  xmm3, xmm0
my_aesenclast  xmm4, xmm0

   movdqa      xmm0, oword [ebx+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [ebx+ECNT], xmm4    ; save encrypted counter+4

   movdqu      xmm4, oword [esi+0*BYTES_PER_BLK]   ; ctr encryption of 4 input blocks
   movdqu      xmm5, oword [esi+1*BYTES_PER_BLK]
   movdqu      xmm6, oword [esi+2*BYTES_PER_BLK]
   movdqu      xmm7, oword [esi+3*BYTES_PER_BLK]
   add         esi, BYTES_PER_LOOP

   pxor        xmm0, xmm4                             ; ctr encryption
   movdqu      oword [edi+0*BYTES_PER_BLK], xmm0   ; store result
my_pshufbM     xmm0, [ebx+SHUF_CONST]            ; convert for multiplication and
   pxor        xmm0, oword [ebx+GHASH0]

   pxor        xmm1, xmm5
   movdqu      oword [edi+1*BYTES_PER_BLK], xmm1
my_pshufbM     xmm1, [ebx+SHUF_CONST]
   pxor        xmm1, oword [ebx+GHASH1]

   pxor        xmm2, xmm6
   movdqu      oword [edi+2*BYTES_PER_BLK], xmm2
my_pshufbM     xmm2, [ebx+SHUF_CONST]
   pxor        xmm2, oword [ebx+GHASH2]

   pxor        xmm3, xmm7
   movdqu      oword [edi+3*BYTES_PER_BLK], xmm3
my_pshufbM     xmm3, [ebx+SHUF_CONST]
   pxor        xmm3, oword [ebx+GHASH3]

   add         edi, BYTES_PER_LOOP

   mov         eax, pPrecomData                        ; pointer to the {hk<<1,hk^2<<1,kh^4<<1} multipliers
   movdqa      xmm7,oword [eax+sizeof_oword_*2]

   cmp         dword [ebx+BLKS4], BYTES_PER_LOOP
   je          .combine_hash

   ;;
   ;; update hash value
   ;;
   sse_clmul_gcm  xmm0, xmm7, xmm4, xmm5, xmm6       ; gHash0 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm1, xmm7, xmm4, xmm5, xmm6       ; gHash1 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm2, xmm7, xmm4, xmm5, xmm6       ; gHash2 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm3, xmm7, xmm4, xmm5, xmm6       ; gHash3 = gHash0 * (HashKey^4)<<1 mod poly

   movdqa      oword [ebx+GHASH0], xmm0
   movdqa      oword [ebx+GHASH1], xmm1
   movdqa      oword [ebx+GHASH2], xmm2
   movdqa      oword [ebx+GHASH3], xmm3

   movdqa      xmm0, oword [ebx+CNT]     ; next counter value
   sub         dword [ebx+BLKS4], BYTES_PER_LOOP
   jge         .blks4_loop

.combine_hash:
   sse_clmul_gcm  xmm0, xmm7, xmm4, xmm5, xmm6       ; gHash0 = gHash0 * (HashKey^4)<<1 mod poly
   movdqa         xmm7,oword [eax+sizeof_oword_*1]
   sse_clmul_gcm  xmm1, xmm7, xmm4, xmm5, xmm6       ; gHash1 = gHash1 * (HashKey^2)<<1 mod poly
   movdqa         xmm7,oword [eax+sizeof_oword_*0]
   sse_clmul_gcm  xmm2, xmm7, xmm4, xmm5, xmm6       ; gHash2 = gHash2 * (HashKey^1)<<1 mod poly

   pxor           xmm3, xmm1
   pxor           xmm3, xmm2
   sse_clmul_gcm  xmm3, xmm7, xmm4, xmm5, xmm6        ; gHash3 = gHash3 * (HashKey)<<1 mod poly

   pxor           xmm3, xmm0
   movdqa         oword [ebx+GHASH0], xmm3         ; store ghash

;;
;; rest of input processing (1-3 blocks)
;;
.single_block_proc:
   cmp      dword [ebx+BLKS],0
   jz       .quit

align IPP_ALIGN_FACTOR
.blk_loop:
   movdqa   xmm0, oword [ebx+CNT]  ; advance counter value
   movdqa   xmm1, xmm0
   paddd    xmm1, oword [ebx+INC_1]
   movdqa   oword [ebx+CNT], xmm1

   movdqa   xmm0, oword [ecx]       ; pre-load whitening keys
   lea      eax, [ecx+16]

my_pshufb   xmm1, [ebx+SHUF_CONST] ; counter is ready to be encrypted

   pxor     xmm1, xmm0                 ; whitening

   movdqa   xmm0, oword [eax]
   add      eax, 16

   mov      edx, nr                    ; counter depending on key length
   sub      edx, 1

align IPP_ALIGN_FACTOR
.cipher_loop:
my_aesenc      xmm1, xmm0              ; regular round
   movdqa      xmm0, oword [eax]
   add         eax, 16
   dec         edx
   jnz         .cipher_loop
my_aesenclast  xmm1, xmm0

   movdqa      xmm0, oword [ebx+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [ebx+ECNT], xmm1    ; save encrypted counter

   movdqu      xmm1, oword [esi]          ; input block
   add         esi, BYTES_PER_BLK
   pxor        xmm0, xmm1                    ; ctr encryption
   movdqu      oword [edi], xmm0
   add         edi, BYTES_PER_BLK

   mov         eax, pPrecomData

   pshufb      xmm0, [ebx+SHUF_CONST]
   pxor        xmm0, oword [ebx+GHASH0]
   movdqa      xmm1, oword [eax]
   sse_clmul_gcm   xmm0, xmm1, xmm2, xmm3, xmm4 ; update hash value
   movdqa      oword [ebx+GHASH0], xmm0

   sub         dword [ebx+BLKS], BYTES_PER_BLK
   jg          .blk_loop

;;
;; exit
;;
.quit:
   movdqa   xmm4, oword [ebx+SHUF_CONST]

   movdqa   xmm0, oword [ebx+CNT]       ; counter
   movdqa   xmm1, oword [ebx+ECNT]      ; encrypted counter
   movdqa   xmm2, oword [ebx+GHASH0]    ; hash

   mov      eax, pCounter              ; address of the counter
   mov      ecx, pEcounter             ; address of the encrypted counter
   mov      edx, pGhash                ; address of hash value

my_pshufb   xmm0, xmm4                 ; convert counter back and
   movdqu   oword [eax], xmm0      ; and store

   movdqu   oword [ecx], xmm1      ; store encrypted counter into the context

my_pshufb   xmm2, xmm4                 ; convert hash value back
   movdqu   oword [edx], xmm2      ; store hash into the context

   add      esp, STACK_SIZE            ; free stack
   REST_GPR
   ret
ENDFUNC AesGcmEnc_avx


;***************************************************************
;* Purpose:    pipelined AES-GCM decryption
;*
;* void AesGcmEnc_avx(Ipp8u* pDst,
;*              const Ipp8u* pSrc,
;*                    int length,
;*              RijnCipher cipher,
;*                    int nr,
;*              const Ipp8u* pRKey,
;*                    Ipp8u* pGhash,
;*                    Ipp8u* pCtrValue,
;*                    Ipp8u* pEncCtrValue,
;*              const Ipp8u* pPrecomData)
;***************************************************************

;;
;; Lib = P8, G9
;;
;; Caller = ippsRijndael128GCMDecrypt
;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmDec_avx,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pDst        [ebp + ARG_1 + 0*sizeof(dword)] ; output block address
%xdefine pSrc        [ebp + ARG_1 + 1*sizeof(dword)] ; input  block address
%xdefine len         [ebp + ARG_1 + 2*sizeof(dword)] ; length(byte)
%xdefine cipher      [ebp + ARG_1 + 3*sizeof(dword)]
%xdefine nr          [ebp + ARG_1 + 4*sizeof(dword)] ; number of rounds
%xdefine pKey        [ebp + ARG_1 + 5*sizeof(dword)] ; key material address
%xdefine pGhash      [ebp + ARG_1 + 6*sizeof(dword)] ; hash
%xdefine pCounter    [ebp + ARG_1 + 7*sizeof(dword)] ; counter
%xdefine pEcounter   [ebp + ARG_1 + 8*sizeof(dword)] ; enc. counter
%xdefine pPrecomData [ebp + ARG_1 + 9*sizeof(dword)] ; const multipliers

%xdefine SC             (4)
%assign  BLKS_PER_LOOP  (4)
%assign  BYTES_PER_BLK  (16)
%assign  BYTES_PER_LOOP (BYTES_PER_BLK*BLKS_PER_LOOP)

;;
;; stack structure:
%assign CNT        (0)
%assign ECNT       (CNT+sizeof_oword_)
%assign GHASH      (ECNT+sizeof_oword_)

%assign GHASH0     (GHASH)
%assign GHASH1     (GHASH0+sizeof_oword_)
%assign GHASH2     (GHASH1+sizeof_oword_)
%assign GHASH3     (GHASH2+sizeof_oword_)

%assign SHUF_CONST (GHASH3+sizeof_oword_)
%assign INC_1      (SHUF_CONST+sizeof_oword_)

%assign BLKS4      (INC_1+sizeof_oword_)
%assign BLKS       (BLKS4+sizeof(dword))
%assign STACK_SIZE (BLKS+sizeof(dword)+sizeof_oword_)

   sub      esp, STACK_SIZE            ; alocate stack
   lea      ebx, [esp+sizeof_oword_]   ; align stack
   and      ebx, -sizeof_oword_
   mov      eax, cipher                 ; due to bug in ml12 - dummy instruction

   LD_ADDR  esi, CONST_TABLE
   movdqa   xmm4, oword u128_str
   movdqa   xmm5, oword inc1

   mov      eax, pCounter              ; address of the counter
   mov      ecx, pEcounter             ; address of the encrypted counter
   mov      edx, pGhash                ; address of hash value

   movdqu   xmm0, oword [eax]       ; counter value
   movdqu   xmm1, oword [ecx]       ; encrypted counter value
   movdqu   xmm2, oword [edx]       ; hash value

my_pshufb   xmm0, xmm4                 ; convert counter and
   movdqa   oword [ebx+CNT], xmm0  ; and store into the stack
   movdqa   oword [ebx+ECNT], xmm1 ; store encrypted counter into the stack

my_pshufb   xmm2, xmm4                 ; convert hash value
   pxor     xmm1, xmm1
   movdqa   oword [ebx+GHASH0], xmm2  ; store hash into the stack
   movdqa   oword [ebx+GHASH1], xmm1  ;
   movdqa   oword [ebx+GHASH2], xmm1  ;
   movdqa   oword [ebx+GHASH3], xmm1  ;

   movdqa   oword [ebx+SHUF_CONST], xmm4 ; store constants into the stack
   movdqa   oword [ebx+INC_1], xmm5

   mov      ecx, pKey                  ; key marerial
   mov      esi, pSrc                  ; src/dst pointers
   mov      edi, pDst

   mov      eax, len
   mov      edx, BYTES_PER_LOOP-1
   and      edx, eax
   and      eax,-BYTES_PER_LOOP
   mov      dword [ebx+BLKS4], eax ; 4-blks counter
   mov      dword [ebx+BLKS], edx  ; rest counter
   jz       .single_block_proc

;;
;; pipelined processing
;;
align IPP_ALIGN_FACTOR
.blks4_loop:
   ;;
   ;; ctr encryption
   ;;
   movdqa   xmm5, oword [ebx+INC_1]

   movdqa   xmm1, xmm0                 ; counter+1
   paddd    xmm1, xmm5
   movdqa   xmm2, xmm1                 ; counter+2
   paddd    xmm2, xmm5
   movdqa   xmm3, xmm2                 ; counter+3
   paddd    xmm3, xmm5
   movdqa   xmm4, xmm3                 ; counter+4
   paddd    xmm4, xmm5
   movdqa   oword [ebx+CNT], xmm4

   movdqa   xmm5,oword [ebx+SHUF_CONST]

   movdqa   xmm0, oword [ecx]       ; pre-load whitening keys
   lea      eax, [ecx+16]              ; pointer to the round's key material

my_pshufb   xmm1, xmm5                 ; counter, counter+1, counter+2, counter+3
my_pshufb   xmm2, xmm5                 ; ready to be encrypted
my_pshufb   xmm3, xmm5
my_pshufb   xmm4, xmm5

   pxor     xmm1, xmm0                 ; whitening
   pxor     xmm2, xmm0
   pxor     xmm3, xmm0
   pxor     xmm4, xmm0

   movdqa   xmm0, oword [eax]       ; pre load round keys
   add      eax, 16

   mov      edx, nr                    ; counter depending on key length
   sub      edx, 1

align IPP_ALIGN_FACTOR
.cipher4_loop:
my_aesenc      xmm1, xmm0              ; regular round
my_aesenc      xmm2, xmm0
my_aesenc      xmm3, xmm0
my_aesenc      xmm4, xmm0
   movdqa      xmm0, oword [eax]
   add         eax, 16
   dec         edx
   jnz         .cipher4_loop
my_aesenclast  xmm1, xmm0
my_aesenclast  xmm2, xmm0
my_aesenclast  xmm3, xmm0
my_aesenclast  xmm4, xmm0

   movdqa      xmm0, oword [ebx+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [ebx+ECNT], xmm4    ; save encrypted counter+4

   movdqu      xmm4, oword [esi+0*BYTES_PER_BLK]   ; ctr encryption of 4 input blocks
   movdqu      xmm5, oword [esi+1*BYTES_PER_BLK]
   movdqu      xmm6, oword [esi+2*BYTES_PER_BLK]
   movdqu      xmm7, oword [esi+3*BYTES_PER_BLK]
   add         esi, BYTES_PER_LOOP

   pxor        xmm0, xmm4                             ; ctr encryption
   movdqu      oword [edi+0*BYTES_PER_BLK], xmm0   ; store result
my_pshufbM     xmm4, [ebx+SHUF_CONST]       ; convert for multiplication and
   pxor        xmm4, oword [ebx+GHASH0]

   pxor        xmm1, xmm5
   movdqu      oword [edi+1*BYTES_PER_BLK], xmm1
my_pshufbM     xmm5, [ebx+SHUF_CONST]
   pxor        xmm5, oword [ebx+GHASH1]

   pxor        xmm2, xmm6
   movdqu      oword [edi+2*BYTES_PER_BLK], xmm2
  ;pshufb      xmm6, [ebx+SHUF_CONST]
my_pshufbM     xmm6, [ebx+SHUF_CONST]
   pxor        xmm6, oword [ebx+GHASH2]

   pxor        xmm3, xmm7
   movdqu      oword [edi+3*BYTES_PER_BLK], xmm3
my_pshufbM     xmm7, [ebx+SHUF_CONST]
   pxor        xmm7, oword [ebx+GHASH3]

   add         edi, BYTES_PER_LOOP

   mov         eax, pPrecomData                 ; pointer to the const multipliers (c^1, c^2, c^4)
   movdqa      xmm0,oword [eax+sizeof_oword_*2]

   cmp         dword [ebx+BLKS4], BYTES_PER_LOOP
   je          .combine_hash

   ;;
   ;; update hash value
   ;;
   sse_clmul_gcm  xmm4, xmm0, xmm1, xmm2, xmm3       ; gHash0 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm5, xmm0, xmm1, xmm2, xmm3       ; gHash1 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm6, xmm0, xmm1, xmm2, xmm3       ; gHash2 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm7, xmm0, xmm1, xmm2, xmm3       ; gHash3 = gHash0 * (HashKey^4)<<1 mod poly

   movdqa      oword [ebx+GHASH0], xmm4
   movdqa      oword [ebx+GHASH1], xmm5
   movdqa      oword [ebx+GHASH2], xmm6
   movdqa      oword [ebx+GHASH3], xmm7

   movdqa      xmm0, oword [ebx+CNT]     ; next counter value
   sub         dword [ebx+BLKS4], BYTES_PER_LOOP
   jge         .blks4_loop

.combine_hash:
   sse_clmul_gcm  xmm4, xmm0, xmm1, xmm2, xmm3       ; gHash0 = gHash0 * (HashKey^4)<<1 mod poly
   movdqa         xmm0, oword [eax+sizeof_oword_*1]
   sse_clmul_gcm  xmm5, xmm0, xmm1, xmm2, xmm3       ; gHash1 = gHash1 * (HashKey^2)<<1 mod poly
   movdqa         xmm0, oword [eax+sizeof_oword_*0]
   sse_clmul_gcm  xmm6, xmm0, xmm1, xmm2, xmm3       ; gHash2 = gHash2 * (HashKey^1)<<1 mod poly

   pxor           xmm7, xmm5
   pxor           xmm7, xmm6
   sse_clmul_gcm  xmm7, xmm0, xmm1, xmm2, xmm3        ; gHash3 = gHash3 * (HashKey)<<1 mod poly

   pxor           xmm7, xmm4
   movdqa         oword [ebx+GHASH0], xmm7         ; store ghash

;;
;; rest of input processing (1-3 blocks)
;;
.single_block_proc:
   cmp      dword [ebx+BLKS],0
   jz       .quit

align IPP_ALIGN_FACTOR
.blk_loop:
   movdqa   xmm0, oword [ebx+CNT]  ; advance counter value
   movdqa   xmm1, xmm0
   paddd    xmm1, oword [ebx+INC_1]
   movdqa   oword [ebx+CNT], xmm1

   movdqa   xmm0, oword [ecx]       ; pre-load whitening keys
   lea      eax, [ecx+16]

my_pshufb   xmm1, [ebx+SHUF_CONST] ; counter is ready to be encrypted

   pxor     xmm1, xmm0                 ; whitening

   movdqa   xmm0, oword [eax]
   add      eax, 16

   mov      edx, nr                    ; counter depending on key length
   sub      edx, 1

align IPP_ALIGN_FACTOR
.cipher_loop:
my_aesenc      xmm1, xmm0              ; regular round
   movdqa      xmm0, oword [eax]
   add         eax, 16
   dec         edx
   jnz         .cipher_loop
my_aesenclast  xmm1, xmm0

   movdqa      xmm0, oword [ebx+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [ebx+ECNT], xmm1    ; save encrypted counter

   movdqu      xmm1, oword [esi]          ; input block
   add         esi, BYTES_PER_BLK
   pxor        xmm0, xmm1                    ; ctr encryption
   movdqu      oword [edi], xmm0
   add         edi, BYTES_PER_BLK

   mov         eax, pPrecomData

   pshufb      xmm1, [ebx+SHUF_CONST]
   pxor        xmm1, oword [ebx+GHASH0]
   movdqa      xmm0, oword [eax]
   sse_clmul_gcm   xmm1, xmm0, xmm2, xmm3, xmm4 ; update hash value
   movdqa      oword [ebx+GHASH0], xmm1

   sub         dword [ebx+BLKS], BYTES_PER_BLK
   jg          .blk_loop

;;
;; exit
;;
.quit:
   movdqa   xmm4, oword [ebx+SHUF_CONST]

   movdqa   xmm0, oword [ebx+CNT]     ; counter
   movdqa   xmm1, oword [ebx+ECNT]    ; encrypted counter
   movdqa   xmm2, oword [ebx+GHASH0]  ; hash

   mov      eax, pCounter              ; address of the counter
   mov      ecx, pEcounter             ; address of the encrypted counter
   mov      edx, pGhash                ; address of hash value

my_pshufb   xmm0, xmm4                 ; convert counter back and
   movdqu   oword [eax], xmm0      ; and store

   movdqu   oword [ecx], xmm1      ; store encrypted counter into the context

my_pshufb   xmm2, xmm4                 ; convert hash value back
   movdqu   oword [edx], xmm2      ; store hash into the context

   add      esp, STACK_SIZE            ; free stack
   REST_GPR
   ret
ENDFUNC AesGcmDec_avx

%endif

