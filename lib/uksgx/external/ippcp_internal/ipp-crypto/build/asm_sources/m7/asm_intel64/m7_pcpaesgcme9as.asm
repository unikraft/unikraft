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

%include "asmdefs.inc"
%include "ia_32e.inc"

%assign my_emulator  0; set 1 for emulation
%include "emulator.inc"


; sse_clmul_gcm MACRO to implement: Data*HashKey mod (128,127,126,121,0)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%assign REFLECTED_IN_OUT  1   ; GCM input and output are represented in reflected form naturaly
;USE_BYTE_REFLECTED   = 0   ; set to 1 %if input data is byte-reflected


%macro sse_clmul_gcm_2way 11.nolist
  %xdefine %%HK %1
  %xdefine %%HKforKaratsuba %2
  %xdefine %%GH_1 %3
  %xdefine %%tmpX0_1 %4
  %xdefine %%tmpX1_1 %5
  %xdefine %%tmpX2_1 %6
  %xdefine %%GH_2 %7
  %xdefine %%tmpX0_2 %8
  %xdefine %%tmpX1_2 %9
  %xdefine %%tmpX2_2 %10
  %xdefine %%tmpX3_2 %11

pshufd      %%tmpX2_1, %%GH_1, 01001110b

   ;; Karatsuba Method
   movdqa      %%tmpX1_1, %%GH_1
   pxor        %%tmpX2_1, %%GH_1
my_pclmulqdq   %%GH_1, %%HK, 00h

   pshufd      %%tmpX2_2, %%GH_2, 01001110b

   ;; Karatsuba Method
   movdqa      %%tmpX1_2, %%GH_2
   pxor        %%tmpX2_2, %%GH_2
my_pclmulqdq   %%tmpX1_1, %%HK, 11h
my_pclmulqdq   %%tmpX2_1, %%HKforKaratsuba, 00h
   pxor        %%tmpX2_1, %%GH_1
   pxor        %%tmpX2_1, %%tmpX1_1

   pshufd      %%tmpX0_1, %%tmpX2_1, 78
   movdqa      %%tmpX2_1, %%tmpX0_1
   pand        %%tmpX0_1, oword [rel MASK2]
   pand        %%tmpX2_1, xmm14
   pxor        %%GH_1, %%tmpX0_1
   pxor        %%tmpX1_1, %%tmpX2_1

   ;first phase of the reduction
   movdqa      %%tmpX0_1, %%GH_1

   psllq       %%GH_1, 1

my_pclmulqdq   %%GH_2, %%HK, 00h
   pxor        %%GH_1, %%tmpX0_1
   psllq       %%GH_1, 5
   pxor        %%GH_1, %%tmpX0_1
   psllq       %%GH_1, 57

   pshufd      %%tmpX2_1, %%GH_1, 78
   movdqa      %%GH_1, %%tmpX2_1

my_pclmulqdq   %%tmpX1_2, %%HK, 11h
   pand        %%tmpX2_1, xmm14
   pand        %%GH_1, oword [rel MASK2]
   pxor        %%GH_1, %%tmpX0_1
   pxor        %%tmpX1_1, %%tmpX2_1

   ;second phase of the reduction
   movdqa      %%tmpX2_1, %%GH_1
   psrlq       %%GH_1, 5

my_pclmulqdq   %%tmpX2_2, %%HKforKaratsuba, 00h
   pxor        %%GH_1, %%tmpX2_1
   psrlq       %%GH_1, 1
   pxor        %%GH_1, %%tmpX2_1
   psrlq       %%GH_1, 1

   pxor        %%GH_1, %%tmpX2_1
   pxor        %%GH_1, %%tmpX1_1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   pxor        %%tmpX2_2, %%GH_2
   pxor        %%tmpX2_2, %%tmpX1_2

   pshufd      %%tmpX0_2, %%tmpX2_2, 78
   movdqa      %%tmpX2_2, %%tmpX0_2
   pand        %%tmpX0_2, oword [rel MASK2]
   pand        %%tmpX2_2, xmm14
   pxor        %%GH_2, %%tmpX0_2
   pxor        %%tmpX1_2, %%tmpX2_2

   ;first phase of the reduction
   movdqa      %%tmpX0_2, %%GH_2
   movdqa      %%tmpX2_2, %%GH_2
   movdqa      %%tmpX3_2, %%GH_2     ; move GH_2 into tmpX0_2, tmpX2_2, tmpX3_2

   psllq       %%tmpX0_2, 63       ; packed left shifting << 63
   psllq       %%tmpX2_2, 62       ; packed left shifting shift << 62
   psllq       %%tmpX3_2, 57       ; packed left shifting shift << 57
   pxor        %%tmpX0_2, %%tmpX2_2  ; xor the shifted versions
   pxor        %%tmpX0_2, %%tmpX3_2

   movdqa      %%tmpX2_2, %%tmpX0_2
   pslldq      %%tmpX2_2, 8        ; shift-L tmpX2_2 2 DWs
   psrldq      %%tmpX0_2, 8        ; shift-R xmm2 2 DWs

   pxor        %%GH_2, %%tmpX2_2     ; first phase of the reduction complete
   pxor        %%tmpX1_2, %%tmpX0_2  ; save the lost MS 1-2-7 bits from first phase

   ;second phase of the reduction
   movdqa      %%tmpX2_2, %%GH_2     ; move GH_2 into tmpX3_2
   psrlq       %%tmpX2_2, 5        ; packed right shifting >> 5
   pxor        %%tmpX2_2, %%GH_2     ; xor shifted versions
   psrlq       %%tmpX2_2, 1        ; packed right shifting >> 1
   pxor        %%tmpX2_2, %%GH_2     ; xor shifted versions
   psrlq       %%tmpX2_2, 1        ; packed right shifting >> 1

   pxor        %%GH_2, %%tmpX2_2     ; second phase of the reduction complete
   pxor        %%GH_2, %%tmpX1_2     ; the result is in GH_2
%endmacro

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
   movdqa   %%tmpX0, %%GH   ;; copy GH into tmpX0, tmpX2, xmm15
   movdqa   %%tmpX2, %%GH
   movdqa   xmm15, %%GH

   psllq    %%tmpX0, 63         ; packed left shifting << 63
   psllq    %%tmpX2, 62         ; packed left shifting shift << 62
   psllq    xmm15, 57         ; packed left shifting shift << 57
   pxor     %%tmpX0, %%tmpX2      ; xor the shifted versions
   pxor     %%tmpX0, xmm15

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

%if (_IPP32E >= _IPP32E_Y8)

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
POLY        DQ    00000000000000001h,0C200000000000000h  ;; 0xC2000000000000000000000000000001
TWOONE      DQ    00000000000000001h,00000000100000000h  ;; 0x00000001000000000000000000000001
SHUF_CONST  DQ    008090a0b0c0d0e0fh,00001020304050607h  ;; 0x000102030405060708090a0b0c0d0e0f
MASK1       DQ    0ffffffffffffffffh,00000000000000000h  ;; 0x0000000000000000ffffffffffffffff
MASK2       DQ    00000000000000000h,0ffffffffffffffffh  ;; 0xffffffffffffffff0000000000000000
INC_1       DQ    1,0

%assign sizeof_oword_  (16)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void AesGcmPrecomute_avx(Ipp8u* pPrecomutedData, const Ipp8u* pHKey)
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmPrecompute_avx,PUBLIC
        USES_GPR rdi,rsi
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
        COMP_ABI 2

%xdefine pPrecomData rdi ; (rdi) pointer to the reflected multipliers reflect(hkey),(hkey<<1), (hkey^2)<<1, (hkey^4)<<1,
%xdefine pHKey       rsi ; (rsi) pointer to the Hkey value

   movdqu   xmm0, oword [rel pHKey] ;  xmm0 holds HashKey
   pshufb   xmm0, [rel SHUF_CONST]
  ;movdqu   oword [pPrecomData+sizeof_oword_*0], xmm0

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
   pcmpeqd  xmm4, oword [rel TWOONE]     ; [TWOONE] = 0x00000001000000000000000000000001
   pand     xmm4, oword [rel POLY]
   pxor     xmm0, xmm4                          ; xmm0 holds the HashKey<<1 mod poly

   movdqa         xmm1, xmm0
   sse_clmul_gcm  xmm1, xmm0, xmm3, xmm4, xmm5  ; xmm1 holds (HashKey^2)<<1 mod poly

   movdqa         xmm2, xmm1
   sse_clmul_gcm  xmm2, xmm1, xmm3, xmm4, xmm5  ; xmm2 holds (HashKey^4)<<1 mod poly

   movdqu   oword [pPrecomData+sizeof_oword_*0], xmm0
   movdqu   oword [pPrecomData+sizeof_oword_*1], xmm1
   movdqu   oword [pPrecomData+sizeof_oword_*2], xmm2

   REST_XMM
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
        USES_GPR rsi,rdi
        USES_XMM xmm15
        COMP_ABI 2

%xdefine pHash   rdi         ; (rdi) pointer to the Hash value
%xdefine pHKey   rsi         ; (rsi) pointer to the (hkey<<1) value

   movdqa   xmm0, oword [rel pHash]
   pshufb   xmm0, [rel SHUF_CONST]
   movdqa   xmm1, oword [rel pHKey]

   sse_clmul_gcm  xmm0, xmm1, xmm2, xmm3, xmm4  ; xmm0 holds Hash*HKey mod poly

   pshufb   xmm0, [rel SHUF_CONST]
   movdqa   oword [rel pHash], xmm0

   REST_XMM
   REST_GPR
   ret
ENDFUNC AesGcmMulGcm_avx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void AesGcmAuth_avx(Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pHKey, const void* pParam
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM AesGcmAuth_avx,PUBLIC
        USES_GPR rsi,rdi
        USES_XMM xmm15
        COMP_ABI 5

%xdefine pHash       rdi ; (rdi) pointer to the Hash value
%xdefine pSrc        rsi ; (rsi) pointer to the input data
%xdefine len         rdx ; (rdx) length of data (multiplr by AES_DATA_BLOCK)
%xdefine pHKey       rcx ; (rcx) pointer to the (hkey<<1) value

%assign  BYTES_PER_BLK (16)

   movdqa   xmm0, oword [rel pHash]
   pshufb   xmm0, [rel SHUF_CONST]
   movdqa   xmm1, oword [rel pHKey]

   movsxd   rdx, edx

align IPP_ALIGN_FACTOR
.auth_loop:
   movdqu   xmm2, oword [rel pSrc]  ; src[]
   pshufb   xmm2, [rel SHUF_CONST]
   add      pSrc, BYTES_PER_BLK
   pxor     xmm0, xmm2              ; hash ^= src[]

   sse_clmul_gcm  xmm0, xmm1, xmm2, xmm3, xmm4  ; xmm0 holds Hash*HKey mod poly

   sub      len, BYTES_PER_BLK
   jnz      .auth_loop

   pshufb   xmm0, [rel SHUF_CONST]
   movdqa   oword [pHash], xmm0

   REST_XMM
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
align IPP_ALIGN_FACTOR
;;
;; Lib = Y8, E9
;;
;; Caller = ippsRijndael128GCMEncrypt
;;
IPPASM AesGcmEnc_avx,PUBLIC
%assign LOCAL_FRAME (8*16)
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
        COMP_ABI 10

%xdefine pDst         rdi         ; pointer to the encrypted data
%xdefine pSrc         rsi         ; pointer to the plane data
%xdefine len          rdx         ; data length in bytes (multiple by BYTES_PER_BLK)
%xdefine cipher       rcx         ; ciper function (don't need in fact)
%xdefine nr           r8d         ; number of cipher's rounds
%xdefine pRKey        r9          ; pointer to the cipher's round keys
%xdefine pGhash       [rsp+ARG_7] ; pointer to the Hash value
%xdefine pCtrValue    [rsp+ARG_8] ; pointer to the counter value
%xdefine pEncCtrValue [rsp+ARG_9] ; pointer to the encrypted counter
%xdefine pPrecomData  [rsp+ARG_10]; pointer to the precomputed data

%assign SC             (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP (BYTES_PER_BLK*BLKS_PER_LOOP)

;;
;; stack structure:
%assign CNT           (0)
%assign ECNT          (CNT+sizeof_oword_)
%assign GHASH         (ECNT+sizeof_oword_)

%assign GHASH0        (GHASH)
%assign GHASH1        (GHASH0+sizeof_oword_)
%assign GHASH2        (GHASH1+sizeof_oword_)
%assign GHASH3        (GHASH2+sizeof_oword_)
%assign HKeyKaratsuba (GHASH3+sizeof_oword_)

   mov      rax, pCtrValue             ; address of the counter
   mov      rbx, pEncCtrValue          ; address of the encrypted counter
   mov      rcx, pGhash                ; address of hash value
   movdqa   xmm4,oword [rel SHUF_CONST]

   movdqu   xmm0, oword [rax]       ; counter value
   movdqu   xmm1, oword [rbx]       ; encrypted counter value
   movdqu   xmm2, oword [rcx]       ; hash value

   pshufb   xmm0, xmm4                 ; convert counter and
   movdqa   oword [rsp+CNT], xmm0  ; store into the stack
   movdqa   oword [rsp+ECNT], xmm1 ; store encrypted counter into the stack

   pshufb   xmm2, xmm4                 ; convert hash value
   pxor     xmm1, xmm1
   movdqa   oword [rsp+GHASH0], xmm2  ; store hash into the stack
   movdqa   oword [rsp+GHASH1], xmm1  ;
   movdqa   oword [rsp+GHASH2], xmm1  ;
   movdqa   oword [rsp+GHASH3], xmm1  ;

   mov         rbx, [rsp+ARG_10]          ; pointer to the {hk<<1,hk^2<<1,kh^4<<1} multipliers
   movdqa      xmm10,oword [rbx+sizeof_oword_*2]
   pshufd      xmm9, xmm10, 01001110b     ; xmm9 holds qword-swapped version of (HashKey^4)<<1 mod poly for Karatsuba
   pxor        xmm9, xmm10
   movdqa      oword [rsp+HKeyKaratsuba], xmm9

   movsxd      len, edx                   ; data length
   mov         rcx, pRKey                 ; rcx point to the chipher's round keys

   mov         rax, len
   and         rax, BYTES_PER_LOOP-1
   and         len, -BYTES_PER_LOOP
   jz          .single_block_proc

;;
;; pipelined processing (4 blocks)
;;
align IPP_ALIGN_FACTOR
.blks4_loop:
   ;;
   ;; ctr encryption
   ;;
   movdqa   xmm6,oword [rel INC_1]
   movdqa   xmm5,oword [rel SHUF_CONST]

   movdqa   xmm1, xmm0                 ; counter+1
   paddd    xmm1, xmm6
   movdqa   xmm2, xmm1                 ; counter+2
   paddd    xmm2, xmm6
   movdqa   xmm3, xmm2                 ; counter+3
   paddd    xmm3, xmm6
   movdqa   xmm4, xmm3                 ; counter+4
   paddd    xmm4, xmm6
   movdqa   oword [rsp+CNT], xmm4

   movdqa   xmm0, oword [rcx]       ; pre-load whitening keys
   mov      r10, rcx

   pshufb   xmm1, xmm5                 ; counter, counter+1, counter+2, counter+3
   pshufb   xmm2, xmm5                 ; ready to be encrypted
   pshufb   xmm3, xmm5
   pshufb   xmm4, xmm5

   pxor     xmm1, xmm0                 ; whitening
   pxor     xmm2, xmm0
   pxor     xmm3, xmm0
   pxor     xmm4, xmm0

   movdqa   xmm0, oword [r10+16]
   add      r10, 16

   mov      r11d, nr                   ; counter depending on key length
   sub      r11, 1

align IPP_ALIGN_FACTOR
.cipher4_loop:
my_aesenc      xmm1, xmm0              ; regular round
my_aesenc      xmm2, xmm0
my_aesenc      xmm3, xmm0
my_aesenc      xmm4, xmm0
   movdqa      xmm0, oword [r10+16]
   add         r10, 16
   dec         r11
   jnz         .cipher4_loop
my_aesenclast  xmm1, xmm0
my_aesenclast  xmm2, xmm0
my_aesenclast  xmm3, xmm0
my_aesenclast  xmm4, xmm0

   movdqa      xmm0, oword [rsp+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [rsp+ECNT], xmm4    ; save encrypted counter+4

   movdqu      xmm4, oword [pSrc+0*BYTES_PER_BLK]  ; 4 input blocks
   movdqu      xmm5, oword [pSrc+1*BYTES_PER_BLK]
   movdqu      xmm6, oword [pSrc+2*BYTES_PER_BLK]
   movdqu      xmm7, oword [pSrc+3*BYTES_PER_BLK]
   add         pSrc, BYTES_PER_LOOP

   pxor        xmm0, xmm4                             ; ctr encryption
   movdqu      oword [pDst+0*BYTES_PER_BLK], xmm0  ; store result
   pshufb      xmm0, [rel SHUF_CONST]             ; convert for multiplication and
   pxor        xmm0, oword [rsp+GHASH0]

   pxor        xmm1, xmm5
   movdqu      oword [pDst+1*BYTES_PER_BLK], xmm1
   pshufb      xmm1, [rel SHUF_CONST]
   pxor        xmm1, oword [rsp+GHASH1]

   pxor        xmm2, xmm6
   movdqu      oword [pDst+2*BYTES_PER_BLK], xmm2
   pshufb      xmm2, [rel SHUF_CONST]
   pxor        xmm2, oword [rsp+GHASH2]

   pxor        xmm3, xmm7
   movdqu      oword [pDst+3*BYTES_PER_BLK], xmm3
   pshufb      xmm3, [rel SHUF_CONST]
   pxor        xmm3, oword [rsp+GHASH3]

   add         pDst, BYTES_PER_LOOP

   cmp         len, BYTES_PER_LOOP
   je          .combine_hash

   ;;
   ;; update hash value
   ;;
   movdqa        xmm14, oword [rel MASK1]
   sse_clmul_gcm_2way   xmm10, xmm9, xmm0, xmm4, xmm5, xmm6, xmm1, xmm11, xmm12, xmm13, xmm15
   sse_clmul_gcm_2way   xmm10, xmm9, xmm2, xmm4, xmm5, xmm6, xmm3, xmm11, xmm12, xmm13, xmm15

   movdqa      oword [rsp+GHASH0], xmm0
   movdqa      oword [rsp+GHASH1], xmm1
   movdqa      oword [rsp+GHASH2], xmm2
   movdqa      oword [rsp+GHASH3], xmm3

   sub         len, BYTES_PER_LOOP
   movdqa      xmm0, oword [rsp+CNT]     ; next counter value
   cmp         len, BYTES_PER_LOOP
   jge         .blks4_loop

.combine_hash:
   movdqa      xmm8,oword [rbx]                   ; hk<<1
   movdqa      xmm9,oword [rbx+sizeof_oword_]     ; (hk^2)<<1

   sse_clmul_gcm  xmm0, xmm10, xmm6, xmm4, xmm5       ; gHash0 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm1,  xmm9, xmm6, xmm4, xmm5       ; gHash1 = gHash1 * (HashKey^2)<<1 mod poly
   sse_clmul_gcm  xmm2,  xmm8, xmm6, xmm4, xmm5       ; gHash2 = gHash2 * (HashKey^1)<<1 mod poly

   pxor           xmm3, xmm1
   pxor           xmm3, xmm2
   sse_clmul_gcm  xmm3, xmm8, xmm6, xmm4, xmm5        ; gHash3 = gHash3 * (HashKey)<<1 mod poly
   pxor           xmm3, xmm0
   movdqa         oword [rsp+GHASH0], xmm3        ; store gHash

;;
;; rest of input processing (1-3 blocks)
;;
.single_block_proc:
   test     rax, rax
   jz       .quit

align IPP_ALIGN_FACTOR
.blk_loop:
   movdqa   xmm0, oword [rsp+CNT]  ; advance counter value
   movdqa   xmm1, xmm0
   paddd    xmm1, oword [rel INC_1]
   movdqa   oword [rsp+CNT], xmm1

   movdqa   xmm0, oword [rcx]       ; pre-load whitening keys
   mov      r10, rcx

   pshufb   xmm1, [rel SHUF_CONST] ; counter is ready to be encrypted

   pxor     xmm1, xmm0                 ; whitening

   movdqa   xmm0, oword [r10+16]
   add      r10, 16

   mov      r11d, nr                   ; counter depending on key length
   sub      r11, 1

align IPP_ALIGN_FACTOR
.cipher_loop:
my_aesenc      xmm1, xmm0              ; regular round
   movdqa      xmm0, oword [r10+16]
   add         r10, 16
   dec         r11
   jnz         .cipher_loop
my_aesenclast  xmm1, xmm0

   movdqa      xmm0, oword [rsp+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [rsp+ECNT], xmm1    ; save encrypted counter

   movdqu      xmm1, oword [pSrc]         ; input block
   add         pSrc, BYTES_PER_BLK
   pxor        xmm0, xmm1                    ; ctr encryption
   movdqu      oword [pDst], xmm0
   add         pDst, BYTES_PER_BLK

   pshufb      xmm0, [rel SHUF_CONST]
   pxor        xmm0, oword [rsp+GHASH0]
   movdqa      xmm1, oword [rbx]
   sse_clmul_gcm   xmm0, xmm1, xmm2, xmm3, xmm4 ; update hash value
   movdqa      oword [rsp+GHASH0], xmm0

   sub         rax, BYTES_PER_BLK
   jg          .blk_loop

;;
;; exit
;;
.quit:
   movdqa   xmm0, oword [rsp+CNT]     ; counter
   movdqa   xmm1, oword [rsp+ECNT]    ; encrypted counter
   movdqa   xmm2, oword [rsp+GHASH0]  ; hash

   mov      rax, pCtrValue             ; address of the counter
   mov      rbx, pEncCtrValue          ; address of the encrypted counter
   mov      rcx, pGhash                ; address of hash value

   pshufb   xmm0, [rel SHUF_CONST] ; convert counter back and
   movdqu   oword [rax], xmm0      ; store counter into the context

   movdqu   oword [rbx], xmm1      ; store encrypted counter into the context

   pshufb   xmm2, [rel SHUF_CONST] ; convert hach value back
   movdqu   oword [rcx], xmm2      ; store hash into the context

   REST_XMM
   REST_GPR
   ret
ENDFUNC AesGcmEnc_avx

;***************************************************************
;* Purpose:    pipelined AES-GCM decryption
;*
;* void AesGcmDec_avx(Ipp8u* pDst,
;*              const Ipp8u* pSrc,,
;*                    int length,
;*              RijnCipher cipher,
;*                    int nr,
;*              const Ipp8u* pRKey,
;*                    Ipp8u* pGhash,
;*                    Ipp8u* pCtrValue,
;*                    Ipp8u* pEncCtrValue,
;*              const Ipp8u* pPrecomData)
;***************************************************************
align IPP_ALIGN_FACTOR
;;
;; Lib = Y8, E9
;;
;; Caller = ippsRijndael128GCMDecrypt
;;
IPPASM AesGcmDec_avx,PUBLIC
%assign LOCAL_FRAME (8*16)
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
        COMP_ABI 10

%xdefine pDst         rdi         ; pointer to the encrypted data
%xdefine pSrc         rsi         ; pointer to the plane data
%xdefine len          rdx         ; data length in bytes (multiple by BYTES_PER_BLK)
%xdefine cipher       rcx         ; ciper function (don't need in fact)
%xdefine nr           r8d         ; number of cipher's rounds
%xdefine pRKey        r9          ; pointer to the cipher's round keys
%xdefine pGhash       [rsp+ARG_7] ; pointer to the Hash value
%xdefine pCtrValue    [rsp+ARG_8] ; pointer to the counter value
%xdefine pEncCtrValue [rsp+ARG_9] ; pointer to the encrypted counter
%xdefine pPrecomData  [rsp+ARG_10]; pointer to the precomputed data

%assign SC             (4)
%assign BLKS_PER_LOOP  (4)
%assign BYTES_PER_BLK  (16)
%assign BYTES_PER_LOOP (BYTES_PER_BLK*BLKS_PER_LOOP)

;;
;; stack structure:
%assign CNT           (0)
%assign ECNT          (CNT+sizeof_oword_)
%assign GHASH         (ECNT+sizeof_oword_)

%assign GHASH0        (GHASH)
%assign GHASH1        (GHASH0+sizeof_oword_)
%assign GHASH2        (GHASH1+sizeof_oword_)
%assign GHASH3        (GHASH2+sizeof_oword_)
%assign HKeyKaratsuba (GHASH3+sizeof_oword_)

   mov      rax, pCtrValue             ; address of the counter
   mov      rbx, pEncCtrValue          ; address of the encrypted counter
   mov      rcx, pGhash                ; address of hash value
   movdqa   xmm4,oword [rel SHUF_CONST]

   movdqu   xmm0, oword [rax]       ; counter value
   movdqu   xmm1, oword [rbx]       ; encrypted counter value
   movdqu   xmm2, oword [rcx]       ; hash value

   pshufb   xmm0, xmm4                 ; convert counter and
   movdqa   oword [rsp+CNT], xmm0  ; store into the stack
   movdqa   oword [rsp+ECNT], xmm1 ; store encrypted counter into the stack

   pshufb   xmm2, xmm4                 ; convert hash value
   pxor     xmm1, xmm1
   movdqa   oword [rsp+GHASH0], xmm2  ; store hash into the stack
   movdqa   oword [rsp+GHASH1], xmm1  ;
   movdqa   oword [rsp+GHASH2], xmm1  ;
   movdqa   oword [rsp+GHASH3], xmm1  ;

   mov         rbx, [rsp+ARG_10]          ; pointer to the {hk<<1,hk^2<<1,kh^4<<1} multipliers
   movdqa      xmm10,oword [rbx+sizeof_oword_*2]
   pshufd      xmm9, xmm10, 01001110b  ; xmm0 holds qword-swapped version of (HashKey^4)<<1 mod poly for Karatsuba
   pxor        xmm9, xmm10
   movdqa      oword [rsp+HKeyKaratsuba], xmm9

   movsxd      len, edx                   ; data length
   mov         rcx, pRKey                 ; rcx point to the chipher's round keys

   mov         rax, len
   and         rax, BYTES_PER_LOOP-1
   and         len, -BYTES_PER_LOOP
   jz          .single_block_proc

;;
;; pipelined processing (4 blocks)
;;
align IPP_ALIGN_FACTOR
.blks4_loop:
   ;;
   ;; ctr encryption
   ;;
   movdqa   xmm6,oword [rel INC_1]
   movdqa   xmm5,oword [rel SHUF_CONST]

   movdqa   xmm1, xmm0                 ; counter+1
   paddd    xmm1, oword [rel INC_1]
   movdqa   xmm2, xmm1                 ; counter+2
   paddd    xmm2, oword [rel INC_1]
   movdqa   xmm3, xmm2                 ; counter+3
   paddd    xmm3, oword [rel INC_1]
   movdqa   xmm4, xmm3                 ; counter+4
   paddd    xmm4, oword [rel INC_1]
   movdqa   oword [rsp+CNT], xmm4

   movdqa   xmm0, oword [rcx]       ; pre-load whitening keys
   mov      r10, rcx

   pshufb   xmm1, xmm5                 ; counter, counter+1, counter+2, counter+3
   pshufb   xmm2, xmm5                 ; ready to be encrypted
   pshufb   xmm3, xmm5
   pshufb   xmm4, xmm5

   pxor     xmm1, xmm0                 ; whitening
   pxor     xmm2, xmm0
   pxor     xmm3, xmm0
   pxor     xmm4, xmm0

   movdqa   xmm0, oword [r10+16]
   add      r10, 16

   mov      r11d, nr                   ; counter depending on key length
   sub      r11, 1

align IPP_ALIGN_FACTOR
.cipher4_loop:
my_aesenc      xmm1, xmm0              ; regular round
my_aesenc      xmm2, xmm0
my_aesenc      xmm3, xmm0
my_aesenc      xmm4, xmm0
   movdqa      xmm0, oword [r10+16]
   add         r10, 16
   dec         r11
   jnz         .cipher4_loop
my_aesenclast  xmm1, xmm0
my_aesenclast  xmm2, xmm0
my_aesenclast  xmm3, xmm0
my_aesenclast  xmm4, xmm0

   movdqa   xmm0, oword [rsp+ECNT]    ; load pre-calculated encrypted counter
   movdqa   oword [rsp+ECNT], xmm4    ; save encrypted counter+4

   movdqu      xmm4, oword [pSrc+0*BYTES_PER_BLK]  ; 4 input blocks
   movdqu      xmm5, oword [pSrc+1*BYTES_PER_BLK]
   movdqu      xmm6, oword [pSrc+2*BYTES_PER_BLK]
   movdqu      xmm7, oword [pSrc+3*BYTES_PER_BLK]
   add         pSrc, BYTES_PER_LOOP

   pxor        xmm0, xmm4                             ; ctr encryption
   movdqu      oword [pDst+0*BYTES_PER_BLK], xmm0  ; store result
   pshufb      xmm4, [rel SHUF_CONST]             ; convert for multiplication and
   pxor        xmm4, oword [rsp+GHASH0]

   pxor        xmm1, xmm5
   movdqu      oword [pDst+1*BYTES_PER_BLK], xmm1
   pshufb      xmm5, [rel SHUF_CONST]
   pxor        xmm5, oword [rsp+GHASH1]

   pxor        xmm2, xmm6
   movdqu      oword [pDst+2*BYTES_PER_BLK], xmm2
   pshufb      xmm6, [rel SHUF_CONST]
   pxor        xmm6, oword [rsp+GHASH2]

   pxor        xmm3, xmm7
   movdqu      oword [pDst+3*BYTES_PER_BLK], xmm3
   pshufb      xmm7, [rel SHUF_CONST]
   pxor        xmm7, oword [rsp+GHASH3]

   add         pDst, BYTES_PER_LOOP

   cmp         len, BYTES_PER_LOOP
   je          .combine_hash

   ;;
   ;; update hash value
   ;;
   movdqa        xmm14, oword [rel MASK1]
   sse_clmul_gcm_2way   xmm10, xmm9, xmm4, xmm0, xmm1, xmm2, xmm5, xmm11, xmm12, xmm13, xmm15
   sse_clmul_gcm_2way   xmm10, xmm9, xmm6, xmm0, xmm1, xmm2, xmm7, xmm11, xmm12, xmm13, xmm15

   movdqa      oword [rsp+GHASH0], xmm4
   movdqa      oword [rsp+GHASH1], xmm5
   movdqa      oword [rsp+GHASH2], xmm6
   movdqa      oword [rsp+GHASH3], xmm7

   sub         len, BYTES_PER_LOOP
   movdqa      xmm0, oword [rsp+CNT]     ; next counter value
   cmp         len, BYTES_PER_LOOP
   jge         .blks4_loop

.combine_hash:
   movdqa      xmm8,oword [rbx]                   ; hk<<1
   movdqa      xmm9,oword [rbx+sizeof_oword_]     ; (hk^2)<<1

   sse_clmul_gcm  xmm4, xmm10, xmm0, xmm1, xmm2       ; gHash0 = gHash0 * (HashKey^4)<<1 mod poly
   sse_clmul_gcm  xmm5,  xmm9, xmm0, xmm1, xmm2       ; gHash1 = gHash1 * (HashKey^2)<<1 mod poly
   sse_clmul_gcm  xmm6,  xmm8, xmm0, xmm1, xmm2       ; gHash2 = gHash2 * (HashKey^1)<<1 mod poly

   pxor           xmm7, xmm5
   pxor           xmm7, xmm6
   sse_clmul_gcm  xmm7, xmm8, xmm0, xmm1, xmm2        ; gHash3 = gHash3 * (HashKey)<<1 mod poly
   pxor           xmm7, xmm4
   movdqa         oword [rsp+GHASH0], xmm7        ; store gHash

;;
;; rest of input processing (1-3 blocks)
;;
.single_block_proc:
   test     rax, rax
   jz       .quit

align IPP_ALIGN_FACTOR
.blk_loop:
   movdqa   xmm0, oword [rsp+CNT]  ; advance counter value
   movdqa   xmm1, xmm0
   paddd    xmm1, oword [rel INC_1]
   movdqa   oword [rsp+CNT], xmm1

   movdqa   xmm0, oword [rcx]       ; pre-load whitening keys
   mov      r10, rcx

   pshufb   xmm1, [rel SHUF_CONST] ; counter is ready to be encrypted

   pxor     xmm1, xmm0                 ; whitening

   movdqa   xmm0, oword [r10+16]
   add      r10, 16

   mov      r11d, nr                   ; counter depending on key length
   sub      r11, 1

align IPP_ALIGN_FACTOR
.cipher_loop:
my_aesenc      xmm1, xmm0              ; regular round
   movdqa      xmm0, oword [r10+16]
   add         r10, 16
   dec         r11
   jnz         .cipher_loop
my_aesenclast  xmm1, xmm0

   movdqa      xmm0, oword [rsp+ECNT]    ; load pre-calculated encrypted counter
   movdqa      oword [rsp+ECNT], xmm1    ; save encrypted counter

   movdqu      xmm1, oword [pSrc]         ; input block
   add         pSrc, BYTES_PER_BLK
   pxor        xmm0, xmm1                    ; ctr encryption
   movdqu      oword [pDst], xmm0
   add         pDst, BYTES_PER_BLK

   pshufb      xmm1, [rel SHUF_CONST]
   pxor        xmm1, oword [rsp+GHASH0]
   movdqa      xmm0, oword [rbx]
   sse_clmul_gcm   xmm1, xmm0, xmm2, xmm3, xmm4 ; update hash value
   movdqa      oword [rsp+GHASH0], xmm1

   sub         rax, BYTES_PER_BLK
   jg          .blk_loop

;;
;; exit
;;
.quit:
   movdqa   xmm0, oword [rsp+CNT]     ; counter
   movdqa   xmm1, oword [rsp+ECNT]    ; encrypted counter
   movdqa   xmm2, oword [rsp+GHASH0]  ; hash

   mov      rax, pCtrValue             ; address of the counter
   mov      rbx, pEncCtrValue          ; address of the encrypted counter
   mov      rcx, pGhash                ; address of hash value

   pshufb   xmm0, [rel SHUF_CONST] ; convert counter back and
   movdqu   oword [rax], xmm0      ; store counter into the context

   movdqu   oword [rbx], xmm1      ; store encrypted counter into the context

   pshufb   xmm2, [rel SHUF_CONST] ; convert hach value back
   movdqu   oword [rcx], xmm2      ; store hash into the context

   REST_XMM
   REST_GPR
   ret
ENDFUNC AesGcmDec_avx

%endif

