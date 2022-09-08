;===============================================================================
; Copyright 2020-2021 Intel Corporation
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
; Authors:
;       Erdinc Ozturk
;       Vinodh Gopal
;       James Guilford
;       Tomasz Kantecki
;
;
; References:
;       This code was derived and highly optimized from the code described in paper:
;               Vinodh Gopal et. al. Optimized Galois-Counter-Mode Implementation on Intel Architecture Processors. August, 2010
;       The details of the implementation is explained in:
;               Erdinc Ozturk et. al. Enabling High-Performance Galois-Counter-Mode on Intel Architecture Processors. October, 2012.
;
;
;
;
; Assumptions:
;
;
;
; iv:
;       0                   1                   2                   3
;       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                             Salt  (From the SA)               |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                     Initialization Vector                     |
;       |         (This is the sequence number from IPSec header)       |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                              0x1                              |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;
;
;
; AAD:
;       AAD will be padded with 0 to the next 16byte multiple
;       for example, assume AAD is a u32 vector
;
;       if AAD is 8 bytes:
;       AAD[3] = {A0, A1};
;       padded AAD in xmm register = {A1 A0 0 0}
;
;       0                   1                   2                   3
;       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                               SPI (A1)                        |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                     32-bit Sequence Number (A0)               |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                              0x0                              |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;
;                                       AAD Format with 32-bit Sequence Number
;
;       if AAD is 12 bytes:
;       AAD[3] = {A0, A1, A2};
;       padded AAD in xmm register = {A2 A1 A0 0}
;
;       0                   1                   2                   3
;       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                               SPI (A2)                        |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                 64-bit Extended Sequence Number {A1,A0}       |
;       |                                                               |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;       |                              0x0                              |
;       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
;
;        AAD Format with 64-bit Extended Sequence Number
;
;
; aadLen:
;       Must be a multiple of 4 bytes and from the definition of the spec.
;       The code additionally supports any aadLen length.
;
; TLen:
;       from the definition of the spec, TLen can only be 8, 12 or 16 bytes.
;
; poly = x^128 + x^127 + x^126 + x^121 + 1
; throughout the code, one tab and two tab indentations are used. one tab is for GHASH part, two tabs is for AES part.
;

%ifndef _GCM_VAES_AVX512_INC_
%define _GCM_VAES_AVX512_INC_

%include "os.inc"
%include "reg_sizes.inc"
%include "clear_regs.inc"
%include "gcm_defines.inc"
%include "gcm_keys_vaes_avx512.inc"
%include "memcpy.inc"
%include "aes_common.inc"

%include "asmdefs.inc"
%include "ia_32e.inc"

%ifndef GCM128_MODE
%ifndef GCM192_MODE
%ifndef GCM256_MODE
%error "No GCM mode selected for gcm_avx512.asm!"
%endif
%endif
%endif

;; Decide on AES-GCM key size to compile for
%ifdef GCM128_MODE
%define NROUNDS 9
%define FN_NAME(x,y) aes_gcm_ %+ x %+ _128 %+ y %+ vaes_avx512
%endif

%ifdef GCM192_MODE
%define NROUNDS 11
%define FN_NAME(x,y) aes_gcm_ %+ x %+ _192 %+ y %+ vaes_avx512
%endif

%ifdef GCM256_MODE
%define NROUNDS 13
%define FN_NAME(x,y) aes_gcm_ %+ x %+ _256 %+ y %+ vaes_avx512
%endif

section .text
default rel

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Stack frame definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%ifidn __OUTPUT_FORMAT__, win64
        %define XMM_STORAGE     (10*16)      ; space for 10 XMM registers
        %define GP_STORAGE      ((9*8) + 24) ; space for 9 GP registers + 24 bytes for 64 byte alignment
%else
        %define XMM_STORAGE     0
        %define GP_STORAGE      (8*8)   ; space for 7 GP registers + 1 for alignment
%endif
%define LOCAL_STORAGE           (48*16) ; space for up to 48 AES blocks

;;; sequence is (bottom-up): GP, XMM, local
%define STACK_GP_OFFSET         0
%define STACK_XMM_OFFSET        (STACK_GP_OFFSET + GP_STORAGE)
%define STACK_LOCAL_OFFSET      (STACK_XMM_OFFSET + XMM_STORAGE)
%define STACK_FRAME_SIZE        (STACK_LOCAL_OFFSET + LOCAL_STORAGE)

;; for compatibility with stack argument definitions in gcm_defines.asm
%define STACK_OFFSET 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Utility Macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; ===========================================================================
;;; ===========================================================================
;;; Horizontal XOR - 4 x 128bits xored together
%macro VHPXORI4x128 2
%define %%REG   %1      ; [in/out] ZMM with 4x128bits to xor; 128bit output
%define %%TMP   %2      ; [clobbered] ZMM temporary register
        vextracti64x4   YWORD(%%TMP), %%REG, 1
        vpxorq          YWORD(%%REG), YWORD(%%REG), YWORD(%%TMP)
        vextracti32x4   XWORD(%%TMP), YWORD(%%REG), 1
        vpxorq          XWORD(%%REG), XWORD(%%REG), XWORD(%%TMP)
%endmacro               ; VHPXORI4x128

;;; ===========================================================================
;;; ===========================================================================
;;; Horizontal XOR - 2 x 128bits xored together
%macro VHPXORI2x128 2
%define %%REG   %1      ; [in/out] YMM/ZMM with 2x128bits to xor; 128bit output
%define %%TMP   %2      ; [clobbered] XMM/YMM/ZMM temporary register
        vextracti32x4   XWORD(%%TMP), %%REG, 1
        vpxorq          XWORD(%%REG), XWORD(%%REG), XWORD(%%TMP)
%endmacro               ; VHPXORI2x128

;;; ===========================================================================
;;; ===========================================================================
;;; schoolbook multiply - 1st step
%macro VCLMUL_STEP1 6-7
%define %%KP    %1      ; [in] key pointer
%define %%HI    %2      ; [in] previous blocks 4 to 7
%define %%TMP   %3      ; [clobbered] ZMM/YMM/XMM temporary
%define %%TH    %4      ; [out] high product
%define %%TM    %5      ; [out] medium product
%define %%TL    %6      ; [out] low product
%define %%HKEY  %7      ; [in/optional] hash key for multiplication

%if %0 == 6
        vmovdqu64       %%TMP, [%%KP + HashKey_4]
%else
        vmovdqa64       %%TMP, %%HKEY
%endif
        vpclmulqdq      %%TH, %%HI, %%TMP, 0x11     ; %%T5 = a1*b1
        vpclmulqdq      %%TL, %%HI, %%TMP, 0x00     ; %%T7 = a0*b0
        vpclmulqdq      %%TM, %%HI, %%TMP, 0x01     ; %%T6 = a1*b0
        vpclmulqdq      %%TMP, %%HI, %%TMP, 0x10    ; %%T4 = a0*b1
        vpxorq          %%TM, %%TM, %%TMP           ; [%%TH : %%TM : %%TL]
%endmacro               ; VCLMUL_STEP1

;;; ===========================================================================
;;; ===========================================================================
;;; schoolbook multiply - 2nd step
%macro VCLMUL_STEP2 9-11
%define %%KP    %1      ; [in] key pointer
%define %%HI    %2      ; [out] ghash high 128 bits
%define %%LO    %3      ; [in/out] cipher text blocks 0-3 (in); ghash low 128 bits (out)
%define %%TMP0  %4      ; [clobbered] ZMM/YMM/XMM temporary
%define %%TMP1  %5      ; [clobbered] ZMM/YMM/XMM temporary
%define %%TMP2  %6      ; [clobbered] ZMM/YMM/XMM temporary
%define %%TH    %7      ; [in] high product
%define %%TM    %8      ; [in] medium product
%define %%TL    %9      ; [in] low product
%define %%HKEY  %10     ; [in/optional] hash key for multiplication
%define %%HXOR  %11     ; [in/optional] type of horizontal xor (4 - 4x128; 2 - 2x128; 1 - none)

%if %0 == 9
        vmovdqu64       %%TMP0, [%%KP + HashKey_8]
%else
        vmovdqa64       %%TMP0, %%HKEY
%endif
        vpclmulqdq      %%TMP1, %%LO, %%TMP0, 0x10     ; %%TMP1 = a0*b1
        vpclmulqdq      %%TMP2, %%LO, %%TMP0, 0x11     ; %%TMP2 = a1*b1
        vpxorq          %%TH, %%TH, %%TMP2
        vpclmulqdq      %%TMP2, %%LO, %%TMP0, 0x00     ; %%TMP2 = a0*b0
        vpxorq          %%TL, %%TL, %%TMP2
        vpclmulqdq      %%TMP0, %%LO, %%TMP0, 0x01     ; %%TMP0 = a1*b0
        vpternlogq      %%TM, %%TMP1, %%TMP0, 0x96     ; %%TM = TM xor TMP1 xor TMP0

        ;; finish multiplications
        vpsrldq         %%TMP2, %%TM, 8
        vpxorq          %%HI, %%TH, %%TMP2
        vpslldq         %%TMP2, %%TM, 8
        vpxorq          %%LO, %%TL, %%TMP2

        ;; xor 128bit words horizontally and compute [(X8*H1) + (X7*H2) + ... ((X1+Y0)*H8]
        ;; note: (X1+Y0) handled elsewhere
%if %0 < 11
        VHPXORI4x128    %%HI, %%TMP2
        VHPXORI4x128    %%LO, %%TMP1
%else
%if %%HXOR == 4
        VHPXORI4x128    %%HI, %%TMP2
        VHPXORI4x128    %%LO, %%TMP1
%elif %%HXOR == 2
        VHPXORI2x128    %%HI, %%TMP2
        VHPXORI2x128    %%LO, %%TMP1
%endif                          ; HXOR
        ;; for HXOR == 1 there is nothing to be done
%endif                          ; !(%0 < 11)
        ;; HIx holds top 128 bits
        ;; LOx holds low 128 bits
        ;; - further reductions to follow
%endmacro               ; VCLMUL_STEP2

;;; ===========================================================================
;;; ===========================================================================
;;; AVX512 reduction macro
%macro VCLMUL_REDUCE 6
%define %%OUT   %1      ; [out] zmm/ymm/xmm: result (must not be %%TMP1 or %%HI128)
%define %%POLY  %2      ; [in] zmm/ymm/xmm: polynomial
%define %%HI128 %3      ; [in] zmm/ymm/xmm: high 128b of hash to reduce
%define %%LO128 %4      ; [in] zmm/ymm/xmm: low 128b of hash to reduce
%define %%TMP0  %5      ; [in] zmm/ymm/xmm: temporary register
%define %%TMP1  %6      ; [in] zmm/ymm/xmm: temporary register

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; first phase of the reduction
        vpclmulqdq      %%TMP0, %%POLY, %%LO128, 0x01
        vpslldq         %%TMP0, %%TMP0, 8       ; shift-L 2 DWs
        vpxorq          %%TMP0, %%LO128, %%TMP0 ; first phase of the reduction complete

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; second phase of the reduction
        vpclmulqdq      %%TMP1, %%POLY, %%TMP0, 0x00
        vpsrldq         %%TMP1, %%TMP1, 4       ; shift-R only 1-DW to obtain 2-DWs shift-R

        vpclmulqdq      %%OUT, %%POLY, %%TMP0, 0x10
        vpslldq         %%OUT, %%OUT, 4         ; shift-L 1-DW to obtain result with no shifts

        vpternlogq      %%OUT, %%TMP1, %%HI128, 0x96    ; OUT/GHASH = OUT xor TMP1 xor HI128
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%endmacro

;;; ===========================================================================
;;; ===========================================================================
;;; schoolbook multiply (1 to 8 blocks) - 1st step
%macro VCLMUL_1_TO_8_STEP1 8
%define %%KP      %1    ; [in] key pointer
%define %%HI      %2    ; [in] ZMM ciphered blocks 4 to 7
%define %%TMP1    %3    ; [clobbered] ZMM temporary
%define %%TMP2    %4    ; [clobbered] ZMM temporary
%define %%TH      %5    ; [out] ZMM high product
%define %%TM      %6    ; [out] ZMM medium product
%define %%TL      %7    ; [out] ZMM low product
%define %%NBLOCKS %8    ; [in] number of blocks to ghash (0 to 8)

%if %%NBLOCKS == 8
        VCLMUL_STEP1    %%KP, %%HI, %%TMP1, %%TH, %%TM, %%TL
%elif  %%NBLOCKS == 7
        vmovdqu64       %%TMP2, [%%KP + HashKey_3]
        vmovdqa64       %%TMP1, [rel mask_out_top_block]
        vpandq          %%TMP2, %%TMP1
        vpandq          %%HI, %%TMP1
        VCLMUL_STEP1    NULL, %%HI, %%TMP1, %%TH, %%TM, %%TL, %%TMP2
%elif  %%NBLOCKS == 6
        vmovdqu64       YWORD(%%TMP2), [%%KP + HashKey_2]
        VCLMUL_STEP1    NULL, YWORD(%%HI), YWORD(%%TMP1), \
                YWORD(%%TH), YWORD(%%TM), YWORD(%%TL), YWORD(%%TMP2)
%elif  %%NBLOCKS == 5
        vmovdqu64       XWORD(%%TMP2), [%%KP + HashKey_1]
        VCLMUL_STEP1    NULL, XWORD(%%HI), XWORD(%%TMP1), \
                XWORD(%%TH), XWORD(%%TM), XWORD(%%TL), XWORD(%%TMP2)
%else
        vpxorq          %%TH, %%TH
        vpxorq          %%TM, %%TM
        vpxorq          %%TL, %%TL
%endif
%endmacro               ; VCLMUL_1_TO_8_STEP1

;;; ===========================================================================
;;; ===========================================================================
;;; schoolbook multiply (1 to 8 blocks) - 2nd step
%macro VCLMUL_1_TO_8_STEP2 10
%define %%KP      %1    ; [in] key pointer
%define %%HI      %2    ; [out] ZMM ghash high 128bits
%define %%LO      %3    ; [in/out] ZMM ciphered blocks 0 to 3 (in); ghash low 128bits (out)
%define %%TMP0    %4    ; [clobbered] ZMM temporary
%define %%TMP1    %5    ; [clobbered] ZMM temporary
%define %%TMP2    %6    ; [clobbered] ZMM temporary
%define %%TH      %7    ; [in/clobbered] ZMM high sum
%define %%TM      %8    ; [in/clobbered] ZMM medium sum
%define %%TL      %9    ; [in/clobbered] ZMM low sum
%define %%NBLOCKS %10   ; [in] number of blocks to ghash (0 to 8)

%if %%NBLOCKS == 8
        VCLMUL_STEP2    %%KP, %%HI, %%LO, %%TMP0, %%TMP1, %%TMP2, %%TH, %%TM, %%TL
%elif %%NBLOCKS == 7
        vmovdqu64       %%TMP2, [%%KP + HashKey_7]
        VCLMUL_STEP2    NULL, %%HI, %%LO, %%TMP0, %%TMP1, %%TMP2, %%TH, %%TM, %%TL, %%TMP2, 4
%elif %%NBLOCKS == 6
        vmovdqu64       %%TMP2, [%%KP + HashKey_6]
        VCLMUL_STEP2    NULL, %%HI, %%LO, %%TMP0, %%TMP1, %%TMP2, %%TH, %%TM, %%TL, %%TMP2, 4
%elif %%NBLOCKS == 5
        vmovdqu64       %%TMP2, [%%KP + HashKey_5]
        VCLMUL_STEP2    NULL, %%HI, %%LO, %%TMP0, %%TMP1, %%TMP2, %%TH, %%TM, %%TL, %%TMP2, 4
%elif %%NBLOCKS == 4
        vmovdqu64       %%TMP2, [%%KP + HashKey_4]
        VCLMUL_STEP2    NULL, %%HI, %%LO, %%TMP0, %%TMP1, %%TMP2, %%TH, %%TM, %%TL, %%TMP2, 4
%elif %%NBLOCKS == 3
        vmovdqu64       %%TMP2, [%%KP + HashKey_3]
        vmovdqa64       %%TMP1, [rel mask_out_top_block]
        vpandq          %%TMP2, %%TMP1
        vpandq          %%LO, %%TMP1
        VCLMUL_STEP2    NULL, %%HI, %%LO, %%TMP0, %%TMP1, %%TMP2, %%TH, %%TM, %%TL, %%TMP2, 4
%elif %%NBLOCKS == 2
        vmovdqu64       YWORD(%%TMP2), [%%KP + HashKey_2]
        VCLMUL_STEP2    NULL, YWORD(%%HI), YWORD(%%LO), \
                YWORD(%%TMP0), YWORD(%%TMP1), YWORD(%%TMP2), \
                YWORD(%%TH), YWORD(%%TM), YWORD(%%TL), YWORD(%%TMP2), 2
%elif %%NBLOCKS == 1
        vmovdqu64       XWORD(%%TMP2), [%%KP + HashKey_1]
        VCLMUL_STEP2    NULL, XWORD(%%HI), XWORD(%%LO), \
                XWORD(%%TMP0), XWORD(%%TMP1), XWORD(%%TMP2), \
                XWORD(%%TH), XWORD(%%TM), XWORD(%%TL), XWORD(%%TMP2), 1
%else
        vpxorq          %%HI, %%HI
        vpxorq          %%LO, %%LO
%endif
%endmacro               ; VCLMUL_1_TO_8_STEP2

;;; ===========================================================================
;;; ===========================================================================
;;; GHASH 1 to 16 blocks of cipher text
;;; - performs reduction at the end
;;; - can take intermediate GHASH sums as input
%macro  GHASH_1_TO_16 20
%define %%KP            %1      ; [in] pointer to expanded keys
%define %%GHASH         %2      ; [out] ghash output
%define %%T1            %3      ; [clobbered] temporary ZMM
%define %%T2            %4      ; [clobbered] temporary ZMM
%define %%T3            %5      ; [clobbered] temporary ZMM
%define %%T4            %6      ; [clobbered] temporary ZMM
%define %%T5            %7      ; [clobbered] temporary ZMM
%define %%T6            %8      ; [clobbered] temporary ZMM
%define %%T7            %9      ; [clobbered] temporary ZMM
%define %%T8            %10     ; [clobbered] temporary ZMM
%define %%T9            %11     ; [clobbered] temporary ZMM
%define %%GH            %12     ; [in/cloberred] ghash sum (high) or "no_zmm"
%define %%GL            %13     ; [in/cloberred] ghash sum (low) or "no_zmm"
%define %%GM            %14     ; [in/cloberred] ghash sum (medium) or "no_zmm"
%define %%AAD_HASH_IN   %15     ; [in] input hash value
%define %%CIPHER_IN0    %16     ; [in] ZMM with cipher text blocks 0-3
%define %%CIPHER_IN1    %17     ; [in] ZMM with cipher text blocks 4-7
%define %%CIPHER_IN2    %18     ; [in] ZMM with cipher text blocks 8-11
%define %%CIPHER_IN3    %19     ; [in] ZMM with cipher text blocks 12-15
%define %%NUM_BLOCKS    %20     ; [in] numerical value, number of blocks

%define %%T0H           %%T1
%define %%T0L           %%T2
%define %%T0M1          %%T3
%define %%T0M2          %%T4

%define %%T1H           %%T5
%define %%T1L           %%T6
%define %%T1M1          %%T7
%define %%T1M2          %%T8

%define %%HK            %%T9

%assign hashk       HashKey_ %+ %%NUM_BLOCKS
%assign reg_idx     0
%assign blocks_left %%NUM_BLOCKS

        vpxorq          %%CIPHER_IN0, %%CIPHER_IN0, %%AAD_HASH_IN

%assign first_result 1

%ifnidn %%GH, no_zmm
%ifnidn %%GM, no_zmm
%ifnidn %%GL, no_zmm
        ;; GHASH sums passed in to be updated and
        ;; reduced at the end
        vmovdqa64       %%T0H, %%GH
        vmovdqa64       %%T0L, %%GL
        vmovdqa64       %%T0M1, %%GM
        vpxorq          %%T0M2, %%T0M2
%assign first_result 0
%endif
%endif
%endif

%rep (blocks_left / 4)
%xdefine %%REG_IN %%CIPHER_IN %+ reg_idx
        vmovdqu64       %%HK, [%%KP + hashk]
%if first_result == 1
        vpclmulqdq      %%T0H, %%REG_IN, %%HK, 0x11      ; H = a1*b1
        vpclmulqdq      %%T0L, %%REG_IN, %%HK, 0x00      ; L = a0*b0
        vpclmulqdq      %%T0M1, %%REG_IN, %%HK, 0x01     ; M1 = a1*b0
        vpclmulqdq      %%T0M2, %%REG_IN, %%HK, 0x10     ; TM2 = a0*b1
%assign first_result 0
%else
        vpclmulqdq      %%T1H, %%REG_IN, %%HK, 0x11      ; H = a1*b1
        vpclmulqdq      %%T1L, %%REG_IN, %%HK, 0x00      ; L = a0*b0
        vpclmulqdq      %%T1M1, %%REG_IN, %%HK, 0x01     ; M1 = a1*b0
        vpclmulqdq      %%T1M2, %%REG_IN, %%HK, 0x10     ; M2 = a0*b1
        vpxorq          %%T0H, %%T0H, %%T1H
        vpxorq          %%T0L, %%T0L, %%T1L
        vpxorq          %%T0M1, %%T0M1, %%T1M1
        vpxorq          %%T0M2, %%T0M2, %%T1M2
%endif
%undef %%REG_IN
%assign reg_idx     (reg_idx + 1)
%assign hashk       (hashk + 64)
%assign blocks_left (blocks_left - 4)
%endrep

%if blocks_left > 0
;; There are 1, 2 or 3 blocks left to process.
;; It may also be that they are the only blocks to process.

%xdefine %%REG_IN %%CIPHER_IN %+ reg_idx

%if first_result == 1
;; Case where %%NUM_BLOCKS = 1, 2 or 3
%xdefine %%OUT_H  %%T0H
%xdefine %%OUT_L  %%T0L
%xdefine %%OUT_M1 %%T0M1
%xdefine %%OUT_M2 %%T0M2
%else
%xdefine %%OUT_H  %%T1H
%xdefine %%OUT_L  %%T1L
%xdefine %%OUT_M1 %%T1M1
%xdefine %%OUT_M2 %%T1M2
%endif

%if blocks_left == 1
        vmovdqu64       XWORD(%%HK), [%%KP + hashk]
        vpclmulqdq      XWORD(%%OUT_H), XWORD(%%REG_IN), XWORD(%%HK), 0x11      ; %%TH = a1*b1
        vpclmulqdq      XWORD(%%OUT_L), XWORD(%%REG_IN), XWORD(%%HK), 0x00      ; %%TL = a0*b0
        vpclmulqdq      XWORD(%%OUT_M1), XWORD(%%REG_IN), XWORD(%%HK), 0x01     ; %%TM1 = a1*b0
        vpclmulqdq      XWORD(%%OUT_M2), XWORD(%%REG_IN), XWORD(%%HK), 0x10     ; %%TM2 = a0*b1
%elif blocks_left == 2
        vmovdqu64       YWORD(%%HK), [%%KP + hashk]
        vpclmulqdq      YWORD(%%OUT_H), YWORD(%%REG_IN), YWORD(%%HK), 0x11      ; %%TH = a1*b1
        vpclmulqdq      YWORD(%%OUT_L), YWORD(%%REG_IN), YWORD(%%HK), 0x00      ; %%TL = a0*b0
        vpclmulqdq      YWORD(%%OUT_M1), YWORD(%%REG_IN), YWORD(%%HK), 0x01     ; %%TM1 = a1*b0
        vpclmulqdq      YWORD(%%OUT_M2), YWORD(%%REG_IN), YWORD(%%HK), 0x10     ; %%TM2 = a0*b1
%else ; blocks_left == 3
        vmovdqu64       YWORD(%%HK), [%%KP + hashk]
        vinserti64x2    %%HK, [%%KP + hashk + 32], 2
        vpclmulqdq      %%OUT_H, %%REG_IN, %%HK, 0x11      ; %%TH = a1*b1
        vpclmulqdq      %%OUT_L, %%REG_IN, %%HK, 0x00      ; %%TL = a0*b0
        vpclmulqdq      %%OUT_M1, %%REG_IN, %%HK, 0x01     ; %%TM1 = a1*b0
        vpclmulqdq      %%OUT_M2, %%REG_IN, %%HK, 0x10     ; %%TM2 = a0*b1
%endif ; blocks_left

%undef %%REG_IN
%undef %%OUT_H
%undef %%OUT_L
%undef %%OUT_M1
%undef %%OUT_M2

%if first_result != 1
        vpxorq          %%T0H, %%T0H, %%T1H
        vpxorq          %%T0L, %%T0L, %%T1L
        vpxorq          %%T0M1, %%T0M1, %%T1M1
        vpxorq          %%T0M2, %%T0M2, %%T1M2
%endif

%endif ; blocks_left > 0

        ;; integrate TM into TH and TL
        vpxorq          %%T0M1, %%T0M1, %%T0M2
        vpsrldq         %%T1M1, %%T0M1, 8
        vpslldq         %%T1M2, %%T0M1, 8
        vpxorq          %%T0H, %%T0H, %%T1M1
        vpxorq          %%T0L, %%T0L, %%T1M2

        ;; add TH and TL 128-bit words horizontally
        VHPXORI4x128    %%T0H, %%T1M1
        VHPXORI4x128    %%T0L, %%T1M2

        ;; reduction
        vmovdqa64       XWORD(%%HK), [rel POLY2]
        VCLMUL_REDUCE   XWORD(%%GHASH), XWORD(%%HK), \
                        XWORD(%%T0H), XWORD(%%T0L), XWORD(%%T0M1), XWORD(%%T0M2)
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GHASH_MUL MACRO to implement: Data*HashKey mod (128,127,126,121,0)
;;; Input: A and B (128-bits each, bit-reflected)
;;; Output: C = A*B*x mod poly, (i.e. >>1 )
;;; To compute GH = GH*HashKey mod poly, give HK = HashKey<<1 mod poly as input
;;; GH = GH * HK * x mod poly which is equivalent to GH*HashKey mod poly.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GHASH_MUL  7
%define %%GH %1         ; 16 Bytes
%define %%HK %2         ; 16 Bytes
%define %%T1 %3
%define %%T2 %4
%define %%T3 %5
%define %%T4 %6
%define %%T5 %7
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        vpclmulqdq      %%T1, %%GH, %%HK, 0x11  ; %%T1 = a1*b1
        vpclmulqdq      %%T2, %%GH, %%HK, 0x00  ; %%T2 = a0*b0
        vpclmulqdq      %%T3, %%GH, %%HK, 0x01  ; %%T3 = a1*b0
        vpclmulqdq      %%GH, %%GH, %%HK, 0x10  ; %%GH = a0*b1
        vpxorq          %%GH, %%GH, %%T3


        vpsrldq         %%T3, %%GH, 8           ; shift-R %%GH 2 DWs
        vpslldq         %%GH, %%GH, 8           ; shift-L %%GH 2 DWs

        vpxorq          %%T1, %%T1, %%T3
        vpxorq          %%GH, %%GH, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqu64       %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%GH, 0x01
        vpslldq         %%T2, %%T2, 8           ; shift-L %%T2 2 DWs

        vpxorq          %%GH, %%GH, %%T2        ; first phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%T2, %%T3, %%GH, 0x00
        vpsrldq         %%T2, %%T2, 4           ; shift-R only 1-DW to obtain 2-DWs shift-R

        vpclmulqdq      %%GH, %%T3, %%GH, 0x10
        vpslldq         %%GH, %%GH, 4           ; Shift-L 1-DW to obtain result with no shifts

        ; second phase of the reduction complete, the result is in %%GH
        vpternlogq      %%GH, %%T1, %%T2, 0x96  ; GH = GH xor T1 xor T2
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; In PRECOMPUTE, the commands filling Hashkey_i_k are not required for avx512
;;; functions, but are kept to allow users to switch cpu architectures between calls
;;; of pre, init, update, and finalize.
%macro  PRECOMPUTE 8
%define %%GDATA %1
%define %%HK    %2
%define %%T1    %3
%define %%T2    %4
%define %%T3    %5
%define %%T4    %6
%define %%T5    %7
%define %%T6    %8

        vmovdqa  %%T5, %%HK

        ;; GHASH keys 2 to 48
%assign max_hkey_idx 48

%assign i 2
%rep (max_hkey_idx - 1)
        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2 ;  %%T5 = HashKey^i<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_ %+ i], %%T5           ;  [HashKey_i] = %%T5
%assign i (i + 1)
%endrep

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; READ_SMALL_DATA_INPUT
;;; Packs xmm register with data when data input is less or equal to 16 bytes
;;; Returns 0 if data has length 0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro READ_SMALL_DATA_INPUT    5
%define %%OUTPUT        %1 ; [out] xmm register
%define %%INPUT         %2 ; [in] buffer pointer to read from
%define %%LENGTH        %3 ; [in] number of bytes to read
%define %%TMP1          %4 ; [clobbered]
%define %%MASK          %5 ; [out] k1 to k7 register to store the partial block mask

        cmp             %%LENGTH, 16
        jge             %%_read_small_data_ge16
        lea             %%TMP1, [rel byte_len_to_mask_table]
%ifidn __OUTPUT_FORMAT__, win64
        add             %%TMP1, %%LENGTH
        add             %%TMP1, %%LENGTH
        kmovw           %%MASK, [%%TMP1]
%else
        kmovw           %%MASK, [%%TMP1 + %%LENGTH*2]
%endif
        vmovdqu8        %%OUTPUT{%%MASK}{z}, [%%INPUT]
        jmp             %%_read_small_data_end
%%_read_small_data_ge16:
        VX512LDR        %%OUTPUT, [%%INPUT]
        mov             %%TMP1, 0xffff
        kmovq           %%MASK, %%TMP1
%%_read_small_data_end:
%endmacro ; READ_SMALL_DATA_INPUT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CALC_AAD_HASH: Calculates the hash of the data which will not be encrypted.
; Input: The input data (A_IN), that data's length (A_LEN), and the hash key (HASH_KEY).
; Output: The hash of the data (AAD_HASH).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  CALC_AAD_HASH   18
%define %%A_IN          %1      ; [in] AAD text pointer
%define %%A_LEN         %2      ; [in] AAD length
%define %%AAD_HASH      %3      ; [out] xmm ghash value
%define %%GDATA_KEY     %4      ; [in] pointer to keys
%define %%ZT0           %5      ; [clobbered] ZMM register
%define %%ZT1           %6      ; [clobbered] ZMM register
%define %%ZT2           %7      ; [clobbered] ZMM register
%define %%ZT3           %8      ; [clobbered] ZMM register
%define %%ZT4           %9      ; [clobbered] ZMM register
%define %%ZT5           %10     ; [clobbered] ZMM register
%define %%ZT6           %11     ; [clobbered] ZMM register
%define %%ZT7           %12     ; [clobbered] ZMM register
%define %%ZT8           %13     ; [clobbered] ZMM register
%define %%ZT9           %14     ; [clobbered] ZMM register
%define %%T1            %15     ; [clobbered] GP register
%define %%T2            %16     ; [clobbered] GP register
%define %%T3            %17     ; [clobbered] GP register
%define %%MASKREG       %18     ; [clobbered] mask register

%define %%SHFMSK %%ZT9
%define %%POLY   %%ZT8
%define %%TH     %%ZT7
%define %%TM     %%ZT6
%define %%TL     %%ZT5

        mov             %%T1, %%A_IN            ; T1 = AAD
        mov             %%T2, %%A_LEN           ; T2 = aadLen
        vpxorq          %%AAD_HASH, %%AAD_HASH

        vmovdqa64       %%SHFMSK, [rel SHUF_MASK]
        vmovdqa64       %%POLY, [rel POLY2]

%%_get_AAD_loop128:
        cmp             %%T2, 128
        jl              %%_exit_AAD_loop128

        vmovdqu64       %%ZT2, [%%T1 + 64*0]  ; LO blocks (0-3)
        vmovdqu64       %%ZT1, [%%T1 + 64*1]  ; HI blocks (4-7)
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         %%ZT1, %%SHFMSK

        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)

        VCLMUL_STEP1    %%GDATA_KEY, %%ZT1, %%ZT0, %%TH, %%TM, %%TL
        VCLMUL_STEP2    %%GDATA_KEY, %%ZT1, %%ZT2, %%ZT0, %%ZT3, %%ZT4, %%TH, %%TM, %%TL

        ;; result in %%ZT1(H):%%ZT2(L)
        ;; reduce and put the result in AAD_HASH
        VCLMUL_REDUCE   %%AAD_HASH, XWORD(%%POLY), XWORD(%%ZT1), XWORD(%%ZT2), \
                XWORD(%%ZT0), XWORD(%%ZT3)

        sub             %%T2, 128
        je              %%_CALC_AAD_done

        add             %%T1, 128
        jmp             %%_get_AAD_loop128

%%_exit_AAD_loop128:
        or              %%T2, %%T2
        jz              %%_CALC_AAD_done

        ;; prep mask source address
        lea             %%T3, [rel byte64_len_to_mask_table]
        lea             %%T3, [%%T3 + %%T2*8]

        ;; calculate number of blocks to ghash (including partial bytes)
        add             %%T2, 15
        and             %%T2, -16       ; 1 to 8 blocks possible here
        shr             %%T2, 4
        cmp             %%T2, 7
        je              %%_AAD_blocks_7
        cmp             %%T2, 6
        je              %%_AAD_blocks_6
        cmp             %%T2, 5
        je              %%_AAD_blocks_5
        cmp             %%T2, 4
        je              %%_AAD_blocks_4
        cmp             %%T2, 3
        je              %%_AAD_blocks_3
        cmp             %%T2, 2
        je              %%_AAD_blocks_2
        cmp             %%T2, 1
        je              %%_AAD_blocks_1
        ;; fall through for 8 blocks

        ;; The flow of each of these cases is identical:
        ;; - load blocks plain text
        ;; - shuffle loaded blocks
        ;; - xor in current hash value into block 0
        ;; - perform up multiplications with ghash keys
        ;; - jump to reduction code
%%_AAD_blocks_8:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        %%ZT1{%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         %%ZT1, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH) ; xor in current ghash
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 8
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 8
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_7:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        %%ZT1{%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         %%ZT1, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH) ; xor in current ghash
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 7
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 7
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_6:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        YWORD(%%ZT1){%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         YWORD(%%ZT1), YWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 6
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 6
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_5:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        XWORD(%%ZT1){%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         XWORD(%%ZT1), XWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 5
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 5
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_4:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2{%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         %%ZT2, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 4
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 4
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_3:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2{%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         %%ZT2, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 3
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 3
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_2:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        YWORD(%%ZT2){%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         YWORD(%%ZT2), YWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 2
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 2
        jmp             %%_AAD_blocks_done

%%_AAD_blocks_1:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        XWORD(%%ZT2){%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         XWORD(%%ZT2), XWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%AAD_HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 1
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 1

%%_AAD_blocks_done:
        ;; Multiplications have been done. Do the reduction now
        VCLMUL_REDUCE   %%AAD_HASH, XWORD(%%POLY), XWORD(%%ZT1), XWORD(%%ZT2), \
                        XWORD(%%ZT0), XWORD(%%ZT3)
%%_CALC_AAD_done:
        ;; result in AAD_HASH

%endmacro ; CALC_AAD_HASH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; PARTIAL_BLOCK
;;; Handles encryption/decryption and the tag partial blocks between
;;; update calls.
;;; Requires the input data be at least 1 byte long.
;;; Output:
;;; A cipher/plain of the first partial block (CYPH_PLAIN_OUT),
;;; AAD_HASH and updated GDATA_CTX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro PARTIAL_BLOCK 22
%define %%GDATA_KEY             %1 ; [in] key pointer
%define %%GDATA_CTX             %2 ; [in] context pointer
%define %%CYPH_PLAIN_OUT        %3 ; [in] output buffer
%define %%PLAIN_CYPH_IN         %4 ; [in] input buffer
%define %%PLAIN_CYPH_LEN        %5 ; [in] buffer length
%define %%DATA_OFFSET           %6 ; [in/out] data offset (gets updated)
%define %%AAD_HASH              %7 ; [out] updated GHASH value
%define %%ENC_DEC               %8 ; [in] cipher direction
%define %%GPTMP0                %9 ; [clobbered] GP temporary register
%define %%GPTMP1                %10 ; [clobbered] GP temporary register
%define %%GPTMP2                %11 ; [clobbered] GP temporary register
%define %%ZTMP0                 %12 ; [clobbered] ZMM temporary register
%define %%ZTMP1                 %13 ; [clobbered] ZMM temporary register
%define %%ZTMP2                 %14 ; [clobbered] ZMM temporary register
%define %%ZTMP3                 %15 ; [clobbered] ZMM temporary register
%define %%ZTMP4                 %16 ; [clobbered] ZMM temporary register
%define %%ZTMP5                 %17 ; [clobbered] ZMM temporary register
%define %%ZTMP6                 %18 ; [clobbered] ZMM temporary register
%define %%ZTMP7                 %19 ; [clobbered] ZMM temporary register
%define %%ZTMP8                 %20 ; [clobbered] ZMM temporary register
%define %%ZTMP9                 %21 ; [clobbered] ZMM temporary register
%define %%MASKREG               %22 ; [clobbered] mask temporary register

%define %%XTMP0 XWORD(%%ZTMP0)
%define %%XTMP1 XWORD(%%ZTMP1)
%define %%XTMP2 XWORD(%%ZTMP2)
%define %%XTMP3 XWORD(%%ZTMP3)
%define %%XTMP4 XWORD(%%ZTMP4)
%define %%XTMP5 XWORD(%%ZTMP5)
%define %%XTMP6 XWORD(%%ZTMP6)
%define %%XTMP7 XWORD(%%ZTMP7)
%define %%XTMP8 XWORD(%%ZTMP8)
%define %%XTMP9 XWORD(%%ZTMP9)

%define %%LENGTH        %%GPTMP0
%define %%IA0           %%GPTMP1
%define %%IA1           %%GPTMP2

        mov             %%LENGTH, [%%GDATA_CTX + PBlockLen]
        or              %%LENGTH, %%LENGTH
        je              %%_partial_block_done           ;Leave Macro if no partial blocks

        READ_SMALL_DATA_INPUT   %%XTMP0, %%PLAIN_CYPH_IN, %%PLAIN_CYPH_LEN, %%IA0, %%MASKREG

        ;; XTMP1 = my_ctx_data.partial_block_enc_key
        vmovdqu64       %%XTMP1, [%%GDATA_CTX + PBlockEncKey]
        vmovdqu64       %%XTMP2, [%%GDATA_KEY + HashKey]

        ;; adjust the shuffle mask pointer to be able to shift right %%LENGTH bytes
        ;; (16 - %%LENGTH) is the number of bytes in plaintext mod 16)
        lea             %%IA0, [rel SHIFT_MASK]
        add             %%IA0, %%LENGTH
        vmovdqu64       %%XTMP3, [%%IA0]   ; shift right shuffle mask
        vpshufb         %%XTMP1, %%XTMP3

%ifidn  %%ENC_DEC, DEC
        ;;  keep copy of cipher text in %%XTMP4
        vmovdqa64       %%XTMP4, %%XTMP0
%endif
        vpxorq          %%XTMP1, %%XTMP0      ; Cyphertext XOR E(K, Yn)

        ;; Set %%IA1 to be the amount of data left in CYPH_PLAIN_IN after filling the block
        ;; Determine if partial block is not being filled and shift mask accordingly
        mov             %%IA1, %%PLAIN_CYPH_LEN
        add             %%IA1, %%LENGTH
        sub             %%IA1, 16
        jge             %%_no_extra_mask
        sub             %%IA0, %%IA1
%%_no_extra_mask:
        ;; get the appropriate mask to mask out bottom %%LENGTH bytes of %%XTMP1
        ;; - mask out bottom %%LENGTH bytes of %%XTMP1
        vmovdqu64       %%XTMP0, [%%IA0 + ALL_F - SHIFT_MASK]
        vpand           %%XTMP1, %%XTMP0

%ifidn  %%ENC_DEC, DEC
        vpand           %%XTMP4, %%XTMP0
        vpshufb         %%XTMP4, [rel SHUF_MASK]
        vpshufb         %%XTMP4, %%XTMP3
        vpxorq          %%AAD_HASH, %%XTMP4
%else
        vpshufb         %%XTMP1, [rel SHUF_MASK]
        vpshufb         %%XTMP1, %%XTMP3
        vpxorq          %%AAD_HASH, %%XTMP1
%endif
        cmp             %%IA1, 0
        jl              %%_partial_incomplete

        ;; GHASH computation for the last <16 Byte block
        GHASH_MUL       %%AAD_HASH, %%XTMP2, %%XTMP5, %%XTMP6, %%XTMP7, %%XTMP8, %%XTMP9

        mov             qword [%%GDATA_CTX + PBlockLen], 0

        ;;  Set %%IA1 to be the number of bytes to write out
        mov             %%IA0, %%LENGTH
        mov             %%LENGTH, 16
        sub             %%LENGTH, %%IA0
        jmp             %%_enc_dec_done

%%_partial_incomplete:
%ifidn __OUTPUT_FORMAT__, win64
        mov             %%IA0, %%PLAIN_CYPH_LEN
        add             [%%GDATA_CTX + PBlockLen], %%IA0
%else
        add             [%%GDATA_CTX + PBlockLen], %%PLAIN_CYPH_LEN
%endif
        mov             %%LENGTH, %%PLAIN_CYPH_LEN

%%_enc_dec_done:
        ;; output encrypted Bytes

        lea             %%IA0, [rel byte_len_to_mask_table]
        kmovw           %%MASKREG, [%%IA0 + %%LENGTH*2]
        vmovdqu64       [%%GDATA_CTX + AadHash], %%AAD_HASH

%ifidn  %%ENC_DEC, ENC
        ;; shuffle XTMP1 back to output as ciphertext
        vpshufb         %%XTMP1, [rel SHUF_MASK]
        vpshufb         %%XTMP1, %%XTMP3
%endif
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET]{%%MASKREG}, %%XTMP1
        add             %%DATA_OFFSET, %%LENGTH
%%_partial_block_done:
%endmacro ; PARTIAL_BLOCK


%macro GHASH_SINGLE_MUL 9
%define %%GDATA                 %1
%define %%HASHKEY               %2
%define %%CIPHER                %3
%define %%STATE_11              %4
%define %%STATE_00              %5
%define %%STATE_MID             %6
%define %%T1                    %7
%define %%T2                    %8
%define %%FIRST                 %9

        vmovdqu         %%T1, [%%GDATA + %%HASHKEY]
%ifidn %%FIRST, first
        vpclmulqdq      %%STATE_11, %%CIPHER, %%T1, 0x11         ; %%T4 = a1*b1
        vpclmulqdq      %%STATE_00, %%CIPHER, %%T1, 0x00         ; %%T4_2 = a0*b0
        vpclmulqdq      %%STATE_MID, %%CIPHER, %%T1, 0x01        ; %%T6 = a1*b0
        vpclmulqdq      %%T2, %%CIPHER, %%T1, 0x10               ; %%T5 = a0*b1
        vpxor           %%STATE_MID, %%STATE_MID, %%T2
%else
        vpclmulqdq      %%T2, %%CIPHER, %%T1, 0x11
        vpxor           %%STATE_11, %%STATE_11, %%T2

        vpclmulqdq      %%T2, %%CIPHER, %%T1, 0x00
        vpxor           %%STATE_00, %%STATE_00, %%T2

        vpclmulqdq      %%T2, %%CIPHER, %%T1, 0x01
        vpxor           %%STATE_MID, %%STATE_MID, %%T2

        vpclmulqdq      %%T2, %%CIPHER, %%T1, 0x10
        vpxor           %%STATE_MID, %%STATE_MID, %%T2
%endif

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; This macro is used to "warm-up" pipeline for GHASH_8_ENCRYPT_8_PARALLEL
;;; macro code. It is called only for data lenghts 128 and above.
;;; The flow is as follows:
;;; - encrypt the initial %%num_initial_blocks blocks (can be 0)
;;; - encrypt the next 8 blocks and stitch with
;;;   GHASH for the first %%num_initial_blocks
;;;   - the last 8th block can be partial (lengths between 129 and 239)
;;;   - partial block ciphering is handled within this macro
;;;     - top bytes of such block are cleared for
;;;       the subsequent GHASH calculations
;;;   - PBlockEncKey needs to be setup in case of multi-call
;;;     - top bytes of the block need to include encrypted counter block so that
;;;       when handling partial block case text is read and XOR'ed against it.
;;;       This needs to be in un-shuffled format.

%macro INITIAL_BLOCKS 26-27
%define %%GDATA_KEY             %1      ; [in] pointer to GCM keys
%define %%GDATA_CTX             %2      ; [in] pointer to GCM context
%define %%CYPH_PLAIN_OUT        %3      ; [in] output buffer
%define %%PLAIN_CYPH_IN         %4      ; [in] input buffer
%define %%LENGTH                %5      ; [in/out] number of bytes to process
%define %%DATA_OFFSET           %6      ; [in/out] data offset
%define %%num_initial_blocks    %7      ; [in] can be 0, 1, 2, 3, 4, 5, 6 or 7
%define %%CTR                   %8      ; [in/out] XMM counter block
%define %%AAD_HASH              %9      ; [in/out] ZMM with AAD hash
%define %%ZT1                   %10     ; [out] ZMM cipher blocks 0-3 for GHASH
%define %%ZT2                   %11     ; [out] ZMM cipher blocks 4-7 for GHASH
%define %%ZT3                   %12     ; [clobbered] ZMM temporary
%define %%ZT4                   %13     ; [clobbered] ZMM temporary
%define %%ZT5                   %14     ; [clobbered] ZMM temporary
%define %%ZT6                   %15     ; [clobbered] ZMM temporary
%define %%ZT7                   %16     ; [clobbered] ZMM temporary
%define %%ZT8                   %17     ; [clobbered] ZMM temporary
%define %%ZT9                   %18     ; [clobbered] ZMM temporary
%define %%ZT10                  %19     ; [clobbered] ZMM temporary
%define %%ZT11                  %20     ; [clobbered] ZMM temporary
%define %%ZT12                  %21     ; [clobbered] ZMM temporary
%define %%IA0                   %22     ; [clobbered] GP temporary
%define %%IA1                   %23     ; [clobbered] GP temporary
%define %%ENC_DEC               %24     ; [in] ENC/DEC selector
%define %%MASKREG               %25     ; [clobbered] mask register
%define %%SHUFMASK              %26     ; [in] ZMM with BE/LE shuffle mask
%define %%PARTIAL_PRESENT       %27     ; [in] "no_partial_block" option can be passed here (if length is guaranteed to be > 15*16 bytes)

%define %%T1 XWORD(%%ZT1)
%define %%T2 XWORD(%%ZT2)
%define %%T3 XWORD(%%ZT3)
%define %%T4 XWORD(%%ZT4)
%define %%T5 XWORD(%%ZT5)
%define %%T6 XWORD(%%ZT6)
%define %%T7 XWORD(%%ZT7)
%define %%T8 XWORD(%%ZT8)
%define %%T9 XWORD(%%ZT9)

%define %%TH %%ZT10
%define %%TM %%ZT11
%define %%TL %%ZT12

;; determine if partial block code needs to be added
%assign partial_block_possible 1
%if %0 > 26
%ifidn %%PARTIAL_PRESENT, no_partial_block
%assign partial_block_possible 0
%endif
%endif

%if %%num_initial_blocks > 0
        ;; prepare AES counter blocks
%if %%num_initial_blocks == 1
        vpaddd          %%T3, %%CTR, [rel ONE]
%elif %%num_initial_blocks == 2
        vshufi64x2      YWORD(%%ZT3), YWORD(%%CTR), YWORD(%%CTR), 0
        vpaddd          YWORD(%%ZT3), YWORD(%%ZT3), [rel ddq_add_1234]
%else
        vshufi64x2      ZWORD(%%CTR), ZWORD(%%CTR), ZWORD(%%CTR), 0
        vpaddd          %%ZT3, ZWORD(%%CTR), [rel ddq_add_1234]
        vpaddd          %%ZT4, ZWORD(%%CTR), [rel ddq_add_5678]
%endif

        ;; extract new counter value (%%T3)
        ;; shuffle the counters for AES rounds
%if %%num_initial_blocks <= 4
        vextracti32x4   %%CTR, %%ZT3, (%%num_initial_blocks - 1)
%else
        vextracti32x4   %%CTR, %%ZT4, (%%num_initial_blocks - 5)
%endif
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%num_initial_blocks, vpshufb, \
                        %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%SHUFMASK, %%SHUFMASK, %%SHUFMASK, %%SHUFMASK

        ;; load plain/cipher text
        ZMM_LOAD_BLOCKS_0_16 %%num_initial_blocks, %%PLAIN_CYPH_IN, %%DATA_OFFSET, \
                        %%ZT5, %%ZT6, no_zmm, no_zmm

        ;; AES rounds and XOR with plain/cipher text
%assign j 0
%rep (NROUNDS + 2)
        vbroadcastf64x2 %%ZT1, [%%GDATA_KEY + (j * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%ZT1, j, \
                        %%ZT5, %%ZT6, no_zmm, no_zmm, \
                        %%num_initial_blocks, NROUNDS
%assign j (j + 1)
%endrep

        ;; write cipher/plain text back to output and
        ;; zero bytes outside the mask before hashing
        ZMM_STORE_BLOCKS_0_16 %%num_initial_blocks, %%CYPH_PLAIN_OUT, %%DATA_OFFSET, \
                        %%ZT3, %%ZT4, no_zmm, no_zmm

        ;; Shuffle the cipher text blocks for hashing part
        ;; ZT5 and ZT6 are expected outputs with blocks for hashing
%ifidn  %%ENC_DEC, DEC
        ;; Decrypt case
        ;; - cipher blocks are in ZT5 & ZT6
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%num_initial_blocks, vpshufb, \
                        %%ZT5, %%ZT6, no_zmm, no_zmm, \
                        %%ZT5, %%ZT6, no_zmm, no_zmm, \
                        %%SHUFMASK, %%SHUFMASK, %%SHUFMASK, %%SHUFMASK
%else
        ;; Encrypt case
        ;; - cipher blocks are in ZT3 & ZT4
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%num_initial_blocks, vpshufb, \
                        %%ZT5, %%ZT6, no_zmm, no_zmm, \
                        %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%SHUFMASK, %%SHUFMASK, %%SHUFMASK, %%SHUFMASK
%endif                          ; Encrypt

        ;; adjust data offset and length
        sub             %%LENGTH, (%%num_initial_blocks * 16)
        add             %%DATA_OFFSET, (%%num_initial_blocks * 16)

        ;; At this stage
        ;; - ZT5:ZT6 include cipher blocks to be GHASH'ed

%endif                          ;  %%num_initial_blocks > 0

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; - cipher of %%num_initial_blocks is done
        ;; - prepare counter blocks for the next 8 blocks (ZT3 & ZT4)
        ;;   - save the last block in %%CTR
        ;;   - shuffle the blocks for AES
        ;; - stitch encryption of the new blocks with
        ;;   GHASHING the previous blocks
        vshufi64x2      ZWORD(%%CTR), ZWORD(%%CTR), ZWORD(%%CTR), 0
        vpaddd          %%ZT3, ZWORD(%%CTR), [rel ddq_add_1234]
        vpaddd          %%ZT4, ZWORD(%%CTR), [rel ddq_add_5678]
        vextracti32x4   %%CTR, %%ZT4, 3

        vpshufb         %%ZT3, %%SHUFMASK
        vpshufb         %%ZT4, %%SHUFMASK

%if partial_block_possible != 0
        ;; get text load/store mask (assume full mask by default)
        mov             %%IA0, 0xffff_ffff_ffff_ffff
%if %%num_initial_blocks > 0
        ;; NOTE: 'jge' is always taken for %%num_initial_blocks = 0
        ;;      This macro is executed for lenght 128 and up,
        ;;      zero length is checked in GCM_ENC_DEC.
        ;; We know there is partial block if:
        ;;      LENGTH - 16*num_initial_blocks < 128
        cmp             %%LENGTH, 128
        jge             %%_initial_partial_block_continue
        mov             %%IA1, rcx
        mov             rcx, 128
        sub             rcx, %%LENGTH
        shr             %%IA0, cl
        mov             rcx, %%IA1
%%_initial_partial_block_continue:
%endif
        kmovq           %%MASKREG, %%IA0
        ;; load plain or cipher text (masked)
        ZMM_LOAD_MASKED_BLOCKS_0_16 8, %%PLAIN_CYPH_IN, %%DATA_OFFSET, \
                        %%ZT1, %%ZT2, no_zmm, no_zmm, %%MASKREG
%else
        ;; load plain or cipher text
        ZMM_LOAD_BLOCKS_0_16 8, %%PLAIN_CYPH_IN, %%DATA_OFFSET, \
                        %%ZT1, %%ZT2, no_zmm, no_zmm
%endif  ;;  partial_block_possible

        ;; === AES ROUND 0
%assign aes_round 0
        vbroadcastf64x2 %%ZT8, [%%GDATA_KEY + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%ZT8, aes_round, \
                        %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)

        ;; ===  GHASH blocks 4-7
%if (%%num_initial_blocks > 0)
        ;; Hash in AES state
        vpxorq          %%ZT5, %%ZT5, %%AAD_HASH

        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT6, %%ZT8, %%ZT9, \
                        %%TH, %%TM, %%TL, %%num_initial_blocks
%endif

        ;; === [1/3] of AES rounds

%rep ((NROUNDS + 1) / 3)
        vbroadcastf64x2 %%ZT8, [%%GDATA_KEY + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%ZT8, aes_round, \
                        %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endrep                         ; %rep ((NROUNDS + 1) / 2)

        ;; ===  GHASH blocks 0-3 and gather
%if (%%num_initial_blocks > 0)
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT6, %%ZT5, \
                %%ZT7, %%ZT8, %%ZT9, \
                %%TH, %%TM, %%TL, %%num_initial_blocks
%endif

        ;; === [2/3] of AES rounds

%rep ((NROUNDS + 1) / 3)
        vbroadcastf64x2 %%ZT8, [%%GDATA_KEY + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%ZT8, aes_round, \
                        %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endrep                         ; %rep ((NROUNDS + 1) / 2)

        ;; ===  GHASH reduction

%if (%%num_initial_blocks > 0)
        ;; [out] AAD_HASH - hash output
        ;; [in]  T8 - polynomial
        ;; [in]  T6 - high, T5 - low
        ;; [clobbered] T9, T7 - temporary
        vmovdqu64       %%T8, [rel POLY2]
        VCLMUL_REDUCE   XWORD(%%AAD_HASH), %%T8, %%T6, %%T5, %%T7, %%T9
%endif

        ;; === [3/3] of AES rounds

%rep (((NROUNDS + 1) / 3) + 2)
%if aes_round < (NROUNDS + 2)
        vbroadcastf64x2 %%ZT8, [%%GDATA_KEY + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT3, %%ZT4, no_zmm, no_zmm, \
                        %%ZT8, aes_round, \
                        %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endif
%endrep                         ; %rep ((NROUNDS + 1) / 2)

%if partial_block_possible != 0
        ;; write cipher/plain text back to output and
        ;; zero bytes outside the mask before hashing
        ZMM_STORE_MASKED_BLOCKS_0_16 8, %%CYPH_PLAIN_OUT, %%DATA_OFFSET, \
                        %%ZT3, %%ZT4, no_zmm, no_zmm, %%MASKREG
        ;; check if there is partial block
        cmp             %%LENGTH, 128
        jl              %%_initial_save_partial
        ;; adjust offset and length
        add             %%DATA_OFFSET, 128
        sub             %%LENGTH, 128
        jmp             %%_initial_blocks_done
%%_initial_save_partial:
        ;; partial block case
        ;; - save the partial block in unshuffled format
        ;;   - ZT4 is partially XOR'ed with data and top bytes contain
        ;;     encrypted counter block only
        ;; - save number of bytes process in the partial block
        ;; - adjust offset and zero the length
        ;; - clear top bytes of the partial block for subsequent GHASH calculations
        vextracti32x4   [%%GDATA_CTX + PBlockEncKey], %%ZT4, 3
        add             %%DATA_OFFSET, %%LENGTH
        sub             %%LENGTH, (128 - 16)
        mov             [%%GDATA_CTX + PBlockLen], %%LENGTH
        xor             %%LENGTH, %%LENGTH
        vmovdqu8        %%ZT4{%%MASKREG}{z}, %%ZT4
%%_initial_blocks_done:
%else
        ZMM_STORE_BLOCKS_0_16 8, %%CYPH_PLAIN_OUT, %%DATA_OFFSET, \
                        %%ZT3, %%ZT4, no_zmm, no_zmm
        add             %%DATA_OFFSET, 128
        sub             %%LENGTH, 128
%endif  ;; partial_block_possible

        ;; Shuffle AES result for GHASH.
%ifidn  %%ENC_DEC, DEC
        ;; Decrypt case
        ;; - cipher blocks are in ZT1 & ZT2
        vpshufb         %%ZT1, %%SHUFMASK
        vpshufb         %%ZT2, %%SHUFMASK
%else
        ;; Encrypt case
        ;; - cipher blocks are in ZT3 & ZT4
        vpshufb         %%ZT1, %%ZT3, %%SHUFMASK
        vpshufb         %%ZT2, %%ZT4, %%SHUFMASK
%endif                          ; Encrypt

        ;; Current hash value is in AAD_HASH

        ;; Combine GHASHed value with the corresponding ciphertext
        vpxorq          %%ZT1, %%ZT1, %%AAD_HASH

%endmacro                       ; INITIAL_BLOCKS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; INITIAL_BLOCKS_PARTIAL macro with support for a partial final block.
;;; It may look similar to INITIAL_BLOCKS but its usage is different:
;;; - first encrypts/decrypts required number of blocks and then
;;;   ghashes these blocks
;;; - Small packets or left over data chunks (<256 bytes)
;;;     - single or multi call
;;; - Remaining data chunks below 256 bytes (multi buffer code)
;;;
;;; num_initial_blocks is expected to include the partial final block
;;; in the count.
%macro INITIAL_BLOCKS_PARTIAL 41
%define %%GDATA_KEY             %1  ; [in] key pointer
%define %%GDATA_CTX             %2  ; [in] context pointer
%define %%CYPH_PLAIN_OUT        %3  ; [in] text out pointer
%define %%PLAIN_CYPH_IN         %4  ; [in] text out pointer
%define %%LENGTH                %5  ; [in/clobbered] length in bytes
%define %%DATA_OFFSET           %6  ; [in/out] current data offset (updated)
%define %%num_initial_blocks    %7  ; [in] can only be 1, 2, 3, 4, 5, ..., 15 or 16 (not 0)
%define %%CTR                   %8  ; [in/out] current counter value
%define %%HASH_IN_OUT           %9  ; [in/out] XMM ghash in/out value
%define %%ENC_DEC               %10 ; [in] cipher direction (ENC/DEC)
%define %%INSTANCE_TYPE         %11 ; [in] multi_call or single_call
%define %%ZT0                   %12 ; [clobbered] ZMM temporary
%define %%ZT1                   %13 ; [clobbered] ZMM temporary
%define %%ZT2                   %14 ; [clobbered] ZMM temporary
%define %%ZT3                   %15 ; [clobbered] ZMM temporary
%define %%ZT4                   %16 ; [clobbered] ZMM temporary
%define %%ZT5                   %17 ; [clobbered] ZMM temporary
%define %%ZT6                   %18 ; [clobbered] ZMM temporary
%define %%ZT7                   %19 ; [clobbered] ZMM temporary
%define %%ZT8                   %20 ; [clobbered] ZMM temporary
%define %%ZT9                   %21 ; [clobbered] ZMM temporary
%define %%ZT10                  %22 ; [clobbered] ZMM temporary
%define %%ZT11                  %23 ; [clobbered] ZMM temporary
%define %%ZT12                  %24 ; [clobbered] ZMM temporary
%define %%ZT13                  %25 ; [clobbered] ZMM temporary
%define %%ZT14                  %26 ; [clobbered] ZMM temporary
%define %%ZT15                  %27 ; [clobbered] ZMM temporary
%define %%ZT16                  %28 ; [clobbered] ZMM temporary
%define %%ZT17                  %29 ; [clobbered] ZMM temporary
%define %%ZT18                  %30 ; [clobbered] ZMM temporary
%define %%ZT19                  %31 ; [clobbered] ZMM temporary
%define %%ZT20                  %32 ; [clobbered] ZMM temporary
%define %%ZT21                  %33 ; [clobbered] ZMM temporary
%define %%ZT22                  %34 ; [clobbered] ZMM temporary
%define %%GH                    %35 ; [in] ZMM ghash sum (high)
%define %%GL                    %36 ; [in] ZMM ghash sum (low)
%define %%GM                    %37 ; [in] ZMM ghash sum (middle)
%define %%IA0                   %38 ; [clobbered] GP temporary
%define %%IA1                   %39 ; [clobbered] GP temporary
%define %%MASKREG               %40 ; [clobbered] mask register
%define %%SHUFMASK              %41 ; [in] ZMM with BE/LE shuffle mask

%define %%T1 XWORD(%%ZT1)
%define %%T2 XWORD(%%ZT2)
%define %%T7 XWORD(%%ZT7)

%define %%CTR0 %%ZT3
%define %%CTR1 %%ZT4
%define %%CTR2 %%ZT8
%define %%CTR3 %%ZT9

%define %%DAT0 %%ZT5
%define %%DAT1 %%ZT6
%define %%DAT2 %%ZT10
%define %%DAT3 %%ZT11

%ifnidn %%GH, no_zmm
%ifnidn %%GL, no_zmm
%ifnidn %%GM, no_zmm
        ;; when temporary sums are passed then zero HASH IN value
        ;; - whatever it holds it is invalid in this case
        vpxorq          %%HASH_IN_OUT, %%HASH_IN_OUT
%endif
%endif
%endif
        ;; Copy ghash to temp reg
        vmovdqa64       %%T2, %%HASH_IN_OUT

        ;; prepare AES counter blocks
%if %%num_initial_blocks == 1
        vpaddd          XWORD(%%CTR0), %%CTR, [rel ONE]
%elif %%num_initial_blocks == 2
        vshufi64x2      YWORD(%%CTR0), YWORD(%%CTR), YWORD(%%CTR), 0
        vpaddd          YWORD(%%CTR0), YWORD(%%CTR0), [rel ddq_add_1234]
%else
        vshufi64x2      ZWORD(%%CTR), ZWORD(%%CTR), ZWORD(%%CTR), 0
        vpaddd          %%CTR0, ZWORD(%%CTR), [rel ddq_add_1234]
%if %%num_initial_blocks > 4
        vpaddd          %%CTR1, ZWORD(%%CTR), [rel ddq_add_5678]
%endif
%if %%num_initial_blocks > 8
        vpaddd          %%CTR2, %%CTR0, [rel ddq_add_8888]
%endif
%if %%num_initial_blocks > 12
        vpaddd          %%CTR3, %%CTR1, [rel ddq_add_8888]
%endif
%endif

        ;; get load/store mask
        lea             %%IA0, [rel byte64_len_to_mask_table]
        mov             %%IA1, %%LENGTH
%if %%num_initial_blocks > 12
        sub             %%IA1, 3 * 64
%elif %%num_initial_blocks > 8
        sub             %%IA1, 2 * 64
%elif %%num_initial_blocks > 4
        sub             %%IA1, 64
%endif
        kmovq           %%MASKREG, [%%IA0 + %%IA1*8]

        ;; extract new counter value
        ;; shuffle the counters for AES rounds
%if %%num_initial_blocks <= 4
        vextracti32x4   %%CTR, %%CTR0, (%%num_initial_blocks - 1)
%elif %%num_initial_blocks <= 8
        vextracti32x4   %%CTR, %%CTR1, (%%num_initial_blocks - 5)
%elif %%num_initial_blocks <= 12
        vextracti32x4   %%CTR, %%CTR2, (%%num_initial_blocks - 9)
%else
        vextracti32x4   %%CTR, %%CTR3, (%%num_initial_blocks - 13)
%endif
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%num_initial_blocks, vpshufb, \
                        %%CTR0, %%CTR1, %%CTR2, %%CTR3, \
                        %%CTR0, %%CTR1, %%CTR2, %%CTR3, \
                        %%SHUFMASK, %%SHUFMASK, %%SHUFMASK, %%SHUFMASK

        ;; load plain/cipher text
       ZMM_LOAD_MASKED_BLOCKS_0_16 %%num_initial_blocks, %%PLAIN_CYPH_IN, %%DATA_OFFSET, \
                        %%DAT0, %%DAT1, %%DAT2, %%DAT3, %%MASKREG

        ;; AES rounds and XOR with plain/cipher text
%assign j 0
%rep (NROUNDS + 2)
        vbroadcastf64x2 %%ZT1, [%%GDATA_KEY + (j * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%CTR0, %%CTR1, %%CTR2, %%CTR3, \
                        %%ZT1, j, \
                        %%DAT0, %%DAT1, %%DAT2, %%DAT3, \
                        %%num_initial_blocks, NROUNDS
%assign j (j + 1)
%endrep

        ;; retrieve the last cipher counter block (partially XOR'ed with text)
        ;; - this is needed for partial block cases
%if %%num_initial_blocks <= 4
        vextracti32x4   %%T1, %%CTR0, (%%num_initial_blocks - 1)
%elif %%num_initial_blocks <= 8
        vextracti32x4   %%T1, %%CTR1, (%%num_initial_blocks - 5)
%elif %%num_initial_blocks <= 12
        vextracti32x4   %%T1, %%CTR2, (%%num_initial_blocks - 9)
%else
        vextracti32x4   %%T1, %%CTR3, (%%num_initial_blocks - 13)
%endif

        ;; write cipher/plain text back to output and
        ZMM_STORE_MASKED_BLOCKS_0_16 %%num_initial_blocks, %%CYPH_PLAIN_OUT, %%DATA_OFFSET, \
                        %%CTR0, %%CTR1, %%CTR2, %%CTR3, %%MASKREG

        ;; zero bytes outside the mask before hashing
%if %%num_initial_blocks <= 4
        vmovdqu8        %%CTR0{%%MASKREG}{z}, %%CTR0
%elif %%num_initial_blocks <= 8
        vmovdqu8        %%CTR1{%%MASKREG}{z}, %%CTR1
%elif %%num_initial_blocks <= 12
        vmovdqu8        %%CTR2{%%MASKREG}{z}, %%CTR2
%else
        vmovdqu8        %%CTR3{%%MASKREG}{z}, %%CTR3
%endif

        ;; Shuffle the cipher text blocks for hashing part
        ;; ZT5 and ZT6 are expected outputs with blocks for hashing
%ifidn  %%ENC_DEC, DEC
        ;; Decrypt case
        ;; - cipher blocks are in ZT5 & ZT6
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%num_initial_blocks, vpshufb, \
                        %%DAT0, %%DAT1, %%DAT2, %%DAT3, \
                        %%DAT0, %%DAT1, %%DAT2, %%DAT3, \
                        %%SHUFMASK, %%SHUFMASK, %%SHUFMASK, %%SHUFMASK
%else
        ;; Encrypt case
        ;; - cipher blocks are in CTR0-CTR3
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%num_initial_blocks, vpshufb, \
                        %%DAT0, %%DAT1, %%DAT2, %%DAT3, \
                        %%CTR0, %%CTR1, %%CTR2, %%CTR3, \
                        %%SHUFMASK, %%SHUFMASK, %%SHUFMASK, %%SHUFMASK
%endif                          ; Encrypt

        ;; Extract the last block for partials and multi_call cases
%if %%num_initial_blocks <= 4
        vextracti32x4   %%T7, %%DAT0, %%num_initial_blocks - 1
%elif %%num_initial_blocks <= 8
        vextracti32x4   %%T7, %%DAT1, %%num_initial_blocks - 5
%elif %%num_initial_blocks <= 12
        vextracti32x4   %%T7, %%DAT2, %%num_initial_blocks - 9
%else
        vextracti32x4   %%T7, %%DAT3, %%num_initial_blocks - 13
%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Hash all but the last block of data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;; update data offset
%if %%num_initial_blocks > 1
        ;; The final block of data may be <16B
        add     %%DATA_OFFSET, 16 * (%%num_initial_blocks - 1)
        sub     %%LENGTH, 16 * (%%num_initial_blocks - 1)
%endif

%if %%num_initial_blocks < 16
        ;; NOTE: the 'jl' is always taken for num_initial_blocks = 16.
        ;;      This is run in the context of GCM_ENC_DEC_SMALL for length < 256.
        cmp     %%LENGTH, 16
        jl      %%_small_initial_partial_block

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Handle a full length final block - encrypt and hash all blocks
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        sub     %%LENGTH, 16
        add     %%DATA_OFFSET, 16
        mov     [%%GDATA_CTX + PBlockLen], %%LENGTH

        ;; Hash all of the data

        ;; ZT2 - incoming AAD hash (low 128bits)
        ;; ZT12-ZT20 - temporary registers
        GHASH_1_TO_16 %%GDATA_KEY, %%HASH_IN_OUT, \
                        %%ZT12, %%ZT13, %%ZT14, %%ZT15, %%ZT16, \
                        %%ZT17, %%ZT18, %%ZT19, %%ZT20, \
                        %%GH, %%GL, %%GM, \
                        %%ZT2, %%DAT0, %%DAT1, %%DAT2, %%DAT3, \
                        %%num_initial_blocks

        jmp             %%_small_initial_compute_done
%endif                          ; %if %%num_initial_blocks < 16

%%_small_initial_partial_block:

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;;; Handle ghash for a <16B final block
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;; In this case if it's a single call to encrypt we can
        ;; hash all of the data but if it's an init / update / finalize
        ;; series of call we need to leave the last block if it's
        ;; less than a full block of data.

        mov             [%%GDATA_CTX + PBlockLen], %%LENGTH
        ;; %%T1 is ciphered counter block
        vmovdqu64       [%%GDATA_CTX + PBlockEncKey], %%T1

%ifidn %%INSTANCE_TYPE, multi_call
%assign k (%%num_initial_blocks - 1)
%assign last_block_to_hash 1
%else
%assign k (%%num_initial_blocks)
%assign last_block_to_hash 0
%endif

%if (%%num_initial_blocks > last_block_to_hash)

        ;; ZT12-ZT20 - temporary registers
        GHASH_1_TO_16 %%GDATA_KEY, %%HASH_IN_OUT, \
                        %%ZT12, %%ZT13, %%ZT14, %%ZT15, %%ZT16, \
                        %%ZT17, %%ZT18, %%ZT19, %%ZT20, \
                        %%GH, %%GL, %%GM, \
                        %%ZT2, %%DAT0, %%DAT1, %%DAT2, %%DAT3, k

        ;; just fall through no jmp needed
%else
        ;; Record that a reduction is not needed -
        ;; In this case no hashes are computed because there
        ;; is only one initial block and it is < 16B in length.
        ;; We only need to check if a reduction is needed if
        ;; initial_blocks == 1 and init/update/final is being used.
        ;; In this case we may just have a partial block, and that
        ;; gets hashed in finalize.

%assign need_for_reduction 1
%ifidn %%GH, no_zmm
%ifidn %%GL, no_zmm
%ifidn %%GM, no_zmm
;; if %%GH, %%GL & %%GM not passed then reduction is not required
%assign need_for_reduction 0
%endif
%endif
%endif

%if need_for_reduction == 0
        ;; The hash should end up in HASH_IN_OUT.
        ;; The only way we should get here is if there is
        ;; a partial block of data, so xor that into the hash.
        vpxorq          %%HASH_IN_OUT, %%T2, %%T7
%else
        ;; right - here we have nothing to ghash in the small data but
        ;; we have GHASH sums passed through that we need to gather and reduce

        ;; integrate TM into TH and TL
        vpsrldq         %%ZT12, %%GM, 8
        vpslldq         %%ZT13, %%GM, 8
        vpxorq          %%GH, %%GH, %%ZT12
        vpxorq          %%GL, %%GL, %%ZT13

        ;; add TH and TL 128-bit words horizontally
        VHPXORI4x128    %%GH, %%ZT12
        VHPXORI4x128    %%GL, %%ZT13

        ;; reduction
        vmovdqa64       XWORD(%%ZT12), [rel POLY2]
        VCLMUL_REDUCE   %%HASH_IN_OUT, XWORD(%%ZT12), \
                        XWORD(%%GH), XWORD(%%GL), XWORD(%%ZT13), XWORD(%%ZT14)

        vpxorq          %%HASH_IN_OUT, %%HASH_IN_OUT, %%T7
%endif
        ;; The result is in %%HASH_IN_OUT
        jmp             %%_after_reduction
%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; After GHASH reduction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%%_small_initial_compute_done:

%ifidn %%INSTANCE_TYPE, multi_call
        ;; If using init/update/finalize, we need to xor any partial block data
        ;; into the hash.
%if %%num_initial_blocks > 1
        ;; NOTE: for %%num_initial_blocks = 0 the xor never takes place
%if %%num_initial_blocks != 16
        ;; NOTE: for %%num_initial_blocks = 16, %%LENGTH, stored in [PBlockLen] is never zero
        or              %%LENGTH, %%LENGTH
        je              %%_after_reduction
%endif                          ; %%num_initial_blocks != 16
        vpxorq          %%HASH_IN_OUT, %%HASH_IN_OUT, %%T7
%endif                          ; %%num_initial_blocks > 1
%endif                          ; %%INSTANCE_TYPE, multi_call

%%_after_reduction:
        ;; Final hash is now in HASH_IN_OUT

%endmacro                       ; INITIAL_BLOCKS_PARTIAL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Main GCM macro stitching cipher with GHASH
;;; - operates on single stream
;;; - encrypts 8 blocks at a time
;;; - ghash the 8 previously encrypted ciphertext blocks
;;; For partial block case and multi_call , AES_PARTIAL_BLOCK on output
;;; contains encrypted counter block.
%macro  GHASH_8_ENCRYPT_8_PARALLEL 34-37
%define %%GDATA                 %1  ; [in] key pointer
%define %%CYPH_PLAIN_OUT        %2  ; [in] pointer to output buffer
%define %%PLAIN_CYPH_IN         %3  ; [in] pointer to input buffer
%define %%DATA_OFFSET           %4  ; [in] data offset
%define %%CTR1                  %5  ; [in/out] ZMM counter blocks 0 to 3
%define %%CTR2                  %6  ; [in/out] ZMM counter blocks 4 to 7
%define %%GHASHIN_AESOUT_B03    %7  ; [in/out] ZMM ghash in / aes out blocks 0 to 3
%define %%GHASHIN_AESOUT_B47    %8  ; [in/out] ZMM ghash in / aes out blocks 4 to 7
%define %%AES_PARTIAL_BLOCK     %9  ; [out] XMM partial block (AES)
%define %%loop_idx              %10 ; [in] counter block prep selection "add+shuffle" or "add"
%define %%ENC_DEC               %11 ; [in] cipher direction
%define %%FULL_PARTIAL          %12 ; [in] last block type selection "full" or "partial"
%define %%IA0                   %13 ; [clobbered] temporary GP register
%define %%IA1                   %14 ; [clobbered] temporary GP register
%define %%LENGTH                %15 ; [in] length
%define %%INSTANCE_TYPE         %16 ; [in] 'single_call' or 'multi_call' selection
%define %%GH4KEY                %17 ; [in] ZMM with GHASH keys 4 to 1
%define %%GH8KEY                %18 ; [in] ZMM with GHASH keys 8 to 5
%define %%SHFMSK                %19 ; [in] ZMM with byte swap mask for pshufb
%define %%ZT1                   %20 ; [clobbered] temporary ZMM (cipher)
%define %%ZT2                   %21 ; [clobbered] temporary ZMM (cipher)
%define %%ZT3                   %22 ; [clobbered] temporary ZMM (cipher)
%define %%ZT4                   %23 ; [clobbered] temporary ZMM (cipher)
%define %%ZT5                   %24 ; [clobbered] temporary ZMM (cipher)
%define %%ZT10                  %25 ; [clobbered] temporary ZMM (ghash)
%define %%ZT11                  %26 ; [clobbered] temporary ZMM (ghash)
%define %%ZT12                  %27 ; [clobbered] temporary ZMM (ghash)
%define %%ZT13                  %28 ; [clobbered] temporary ZMM (ghash)
%define %%ZT14                  %29 ; [clobbered] temporary ZMM (ghash)
%define %%ZT15                  %30 ; [clobbered] temporary ZMM (ghash)
%define %%ZT16                  %31 ; [clobbered] temporary ZMM (ghash)
%define %%ZT17                  %32 ; [clobbered] temporary ZMM (ghash)
%define %%MASKREG               %33 ; [clobbered] mask register for partial loads/stores
%define %%DO_REDUCTION          %34 ; [in] "reduction", "no_reduction", "final_reduction"
%define %%TO_REDUCE_L           %35 ; [in/out] ZMM for low 4x128-bit in case of "no_reduction"
%define %%TO_REDUCE_H           %36 ; [in/out] ZMM for hi 4x128-bit in case of "no_reduction"
%define %%TO_REDUCE_M           %37 ; [in/out] ZMM for medium 4x128-bit in case of "no_reduction"

%define %%GH1H  %%ZT10
%define %%GH1L  %%ZT11
%define %%GH1M1 %%ZT12
%define %%GH1M2 %%ZT13

%define %%GH2H  %%ZT14
%define %%GH2L  %%ZT15
%define %%GH2M1 %%ZT16
%define %%GH2M2 %%ZT17

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; populate counter blocks for cipher part
%ifidn %%loop_idx, in_order
        ;; %%CTR1 & %%CTR2 are shuffled outside the scope of this macro
        ;; it has to be kept in unshuffled format
        vpshufb         %%ZT1, %%CTR1, %%SHFMSK
        vpshufb         %%ZT2, %%CTR2, %%SHFMSK
%else
        vmovdqa64       %%ZT1, %%CTR1
        vmovdqa64       %%ZT2, %%CTR2
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; stitch AES rounds with GHASH

%assign aes_round 0

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 0 - ARK
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)

        ;;==================================================
        ;; GHASH 4 blocks
        vpclmulqdq      %%GH1H,  %%GHASHIN_AESOUT_B47, %%GH4KEY, 0x11     ; a1*b1
        vpclmulqdq      %%GH1L,  %%GHASHIN_AESOUT_B47, %%GH4KEY, 0x00     ; a0*b0
        vpclmulqdq      %%GH1M1, %%GHASHIN_AESOUT_B47, %%GH4KEY, 0x01     ; a1*b0
        vpclmulqdq      %%GH1M2, %%GHASHIN_AESOUT_B47, %%GH4KEY, 0x10     ; a0*b1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; 3 AES rounds
%rep 3
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endrep                         ; 3 x AES ROUND

        ;; =================================================
        ;; GHASH 4 blocks
        vpclmulqdq      %%GH2M1, %%GHASHIN_AESOUT_B03, %%GH8KEY, 0x10     ; a0*b1
        vpclmulqdq      %%GH2M2, %%GHASHIN_AESOUT_B03, %%GH8KEY, 0x01     ; a1*b0
        vpclmulqdq      %%GH2H,  %%GHASHIN_AESOUT_B03, %%GH8KEY, 0x11     ; a1*b1
        vpclmulqdq      %%GH2L,  %%GHASHIN_AESOUT_B03, %%GH8KEY, 0x00     ; a0*b0

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; 3 AES rounds
%rep 3
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endrep                         ; 3 x AES ROUND

        ;; =================================================
        ;; gather GHASH in GH1L (low) and GH1H (high)
%ifidn %%DO_REDUCTION, no_reduction
        vpternlogq      %%GH1M1, %%GH1M2, %%GH2M1, 0x96       ; TM: GH1M1 ^= GH1M2 ^ GH2M1
        vpternlogq      %%TO_REDUCE_M, %%GH1M1, %%GH2M2, 0x96 ; TM: TO_REDUCE_M ^= GH1M1 ^ GH2M2
        vpternlogq      %%TO_REDUCE_H, %%GH1H, %%GH2H, 0x96   ; TH: TO_REDUCE_H ^= GH1H ^ GH2H
        vpternlogq      %%TO_REDUCE_L, %%GH1L, %%GH2L, 0x96   ; TL: TO_REDUCE_L ^= GH1L ^ GH2L
%endif
%ifidn %%DO_REDUCTION, do_reduction
        ;; phase 1: add mid products together
        vpternlogq      %%GH1M1, %%GH1M2, %%GH2M1, 0x96 ; TM: GH1M1 ^= GH1M2 ^ GH2M1
        vpxorq          %%GH1M1, %%GH1M1, %%GH2M2

        vpsrldq         %%GH2M1, %%GH1M1, 8
        vpslldq         %%GH1M1, %%GH1M1, 8
%endif
%ifidn %%DO_REDUCTION, final_reduction
        ;; phase 1: add mid products together
        vpternlogq      %%GH1M1, %%GH1M2, %%GH2M1, 0x96       ; TM: GH1M1 ^= GH1M2 ^ GH2M1
        vpternlogq      %%GH1M1, %%TO_REDUCE_M, %%GH2M2, 0x96 ; TM: GH1M1 ^= TO_REDUCE_M ^ GH2M2

        vpsrldq         %%GH2M1, %%GH1M1, 8
        vpslldq         %%GH1M1, %%GH1M1, 8
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; 2 AES rounds
%rep 2
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endrep                         ; 2 x AES ROUND

        ;; =================================================
        ;; Add mid product to high and low then
        ;; horizontal xor of low and high 4x128
%ifidn %%DO_REDUCTION, final_reduction
        vpternlogq      %%GH1H, %%GH2H, %%GH2M1, 0x96   ; TH = TH1 + TH2 + TM>>64
        vpxorq          %%GH1H, %%TO_REDUCE_H
        vpternlogq      %%GH1L, %%GH2L, %%GH1M1, 0x96   ; TL = TL1 + TL2 + TM<<64
        vpxorq          %%GH1L, %%TO_REDUCE_L
%endif
%ifidn %%DO_REDUCTION, do_reduction
        vpternlogq      %%GH1H, %%GH2H, %%GH2M1, 0x96   ; TH = TH1 + TH2 + TM>>64
        vpternlogq      %%GH1L, %%GH2L, %%GH1M1, 0x96   ; TL = TL1 + TL2 + TM<<64
%endif
%ifnidn %%DO_REDUCTION, no_reduction
        VHPXORI4x128    %%GH1H, %%GH2H
        VHPXORI4x128    %%GH1L, %%GH2L
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; 2 AES rounds
%rep 2
%if (aes_round < (NROUNDS + 1))
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endif                          ; aes_round < (NROUNDS + 1)
%endrep

        ;; =================================================
        ;; first phase of reduction
%ifnidn %%DO_REDUCTION, no_reduction
        vmovdqu64       XWORD(%%GH2M2), [rel POLY2]
        vpclmulqdq      XWORD(%%ZT15), XWORD(%%GH2M2), XWORD(%%GH1L), 0x01
        vpslldq         XWORD(%%ZT15), XWORD(%%ZT15), 8             ; shift-L 2 DWs
        vpxorq          XWORD(%%ZT15), XWORD(%%GH1L), XWORD(%%ZT15) ; first phase of the reduct
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; 2 AES rounds
%rep 2
%if (aes_round < (NROUNDS + 1))
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endif                          ; aes_round < (NROUNDS + 1)
%endrep

        ;; =================================================
        ;; second phase of the reduction
%ifnidn %%DO_REDUCTION, no_reduction
        vpclmulqdq      XWORD(%%ZT16), XWORD(%%GH2M2), XWORD(%%ZT15), 0x00
        vpsrldq         XWORD(%%ZT16), XWORD(%%ZT16), 4 ; shift-R 1-DW to obtain 2-DWs shift-R

        vpclmulqdq      XWORD(%%ZT13), XWORD(%%GH2M2), XWORD(%%ZT15), 0x10
        vpslldq         XWORD(%%ZT13), XWORD(%%ZT13), 4 ; shift-L 1-DW for result without shifts
        ;; ZT13 = ZT13 xor ZT16 xor GH1H
        vpternlogq      XWORD(%%ZT13), XWORD(%%ZT16), XWORD(%%GH1H), 0x96
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; all remaining AES rounds but the last
%rep (NROUNDS + 2)
%if (aes_round < (NROUNDS + 1))
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS
%assign aes_round (aes_round + 1)
%endif                          ; aes_round < (NROUNDS + 1)
%endrep

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; load/store mask (partial case) and load the text data
%ifidn %%FULL_PARTIAL, full
        vmovdqu8        %%ZT4, [%%PLAIN_CYPH_IN + %%DATA_OFFSET]
        vmovdqu8        %%ZT5, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 64]
%else
        lea             %%IA0, [rel byte64_len_to_mask_table]
        mov             %%IA1, %%LENGTH
        sub             %%IA1, 64
        kmovq           %%MASKREG, [%%IA0 + 8*%%IA1]
        vmovdqu8        %%ZT4, [%%PLAIN_CYPH_IN + %%DATA_OFFSET]
        vmovdqu8        %%ZT5{%%MASKREG}{z}, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 64]
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; the last AES round  (NROUNDS + 1) and XOR against plain/cipher text
        vbroadcastf64x2 %%ZT3, [%%GDATA + (aes_round * 16)]
        ZMM_AESENC_ROUND_BLOCKS_0_16 %%ZT1, %%ZT2, no_zmm, no_zmm, \
                        %%ZT3, aes_round, \
                        %%ZT4, %%ZT5, no_zmm, no_zmm, \
                        8, NROUNDS

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; store the cipher/plain text data
%ifidn %%FULL_PARTIAL, full
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET], %%ZT1
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 64], %%ZT2
%else
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET], %%ZT1
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 64]{%%MASKREG}, %%ZT2
%endif

        ;; =================================================
        ;; prep cipher text blocks for the next ghash round

%ifnidn %%FULL_PARTIAL, full
%ifidn %%INSTANCE_TYPE, multi_call
        ;; for partial block & multi_call we need encrypted counter block
        vpxorq          %%ZT3, %%ZT2, %%ZT5
        vextracti32x4   %%AES_PARTIAL_BLOCK, %%ZT3, 3
%endif
        ;; for GHASH computation purpose clear the top bytes of the partial block
%ifidn %%ENC_DEC, ENC
        vmovdqu8        %%ZT2{%%MASKREG}{z}, %%ZT2
%else
        vmovdqu8        %%ZT5{%%MASKREG}{z}, %%ZT5
%endif
%endif  ; %ifnidn %%FULL_PARTIAL, full

        ;; =================================================
        ;; shuffle cipher text blocks for GHASH computation
%ifidn %%ENC_DEC, ENC
        vpshufb         %%GHASHIN_AESOUT_B03, %%ZT1, %%SHFMSK
        vpshufb         %%GHASHIN_AESOUT_B47, %%ZT2, %%SHFMSK
%else
        vpshufb         %%GHASHIN_AESOUT_B03, %%ZT4, %%SHFMSK
        vpshufb         %%GHASHIN_AESOUT_B47, %%ZT5, %%SHFMSK
%endif

%ifidn %%DO_REDUCTION, do_reduction
        ;; =================================================
        ;; XOR current GHASH value (ZT13) into block 0
        vpxorq          %%GHASHIN_AESOUT_B03, %%ZT13
%endif
%ifidn %%DO_REDUCTION, final_reduction
        ;; =================================================
        ;; Return GHASH value (ZT13) in TO_REDUCE_L
        vmovdqa64       %%TO_REDUCE_L, %%ZT13
%endif

%endmacro                       ; GHASH_8_ENCRYPT_8_PARALLEL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Main GCM macro stitching cipher with GHASH
;;; - operates on single stream
;;; - encrypts 16 blocks at a time
;;; - ghash the 16 previously encrypted ciphertext blocks
;;; - no partial block or multi_call handling here
%macro  GHASH_16_ENCRYPT_16_PARALLEL 42
%define %%GDATA                 %1  ; [in] key pointer
%define %%CYPH_PLAIN_OUT        %2  ; [in] pointer to output buffer
%define %%PLAIN_CYPH_IN         %3  ; [in] pointer to input buffer
%define %%DATA_OFFSET           %4  ; [in] data offset
%define %%CTR_BE                %5  ; [in/out] ZMM counter blocks (last 4) in big-endian
%define %%CTR_CHECK             %6  ; [in/out] GP with 8-bit counter for overflow check
%define %%HASHKEY_OFFSET        %7  ; [in] numerical offset for the highest hash key
%define %%AESOUT_BLK_OFFSET     %8  ; [in] numerical offset for AES-CTR out
%define %%GHASHIN_BLK_OFFSET    %9  ; [in] numerical offset for GHASH blocks in
%define %%SHFMSK                %10 ; [in] ZMM with byte swap mask for pshufb
%define %%ZT1                   %11 ; [clobbered] temporary ZMM (cipher)
%define %%ZT2                   %12 ; [clobbered] temporary ZMM (cipher)
%define %%ZT3                   %13 ; [clobbered] temporary ZMM (cipher)
%define %%ZT4                   %14 ; [clobbered] temporary ZMM (cipher)
%define %%ZT5                   %15 ; [clobbered/out] temporary ZMM or GHASH OUT (final_reduction)
%define %%ZT6                   %16 ; [clobbered] temporary ZMM (cipher)
%define %%ZT7                   %17 ; [clobbered] temporary ZMM (cipher)
%define %%ZT8                   %18 ; [clobbered] temporary ZMM (cipher)
%define %%ZT9                   %19 ; [clobbered] temporary ZMM (cipher)
%define %%ZT10                  %20 ; [clobbered] temporary ZMM (ghash)
%define %%ZT11                  %21 ; [clobbered] temporary ZMM (ghash)
%define %%ZT12                  %22 ; [clobbered] temporary ZMM (ghash)
%define %%ZT13                  %23 ; [clobbered] temporary ZMM (ghash)
%define %%ZT14                  %24 ; [clobbered] temporary ZMM (ghash)
%define %%ZT15                  %25 ; [clobbered] temporary ZMM (ghash)
%define %%ZT16                  %26 ; [clobbered] temporary ZMM (ghash)
%define %%ZT17                  %27 ; [clobbered] temporary ZMM (ghash)
%define %%ZT18                  %28 ; [clobbered] temporary ZMM (ghash)
%define %%ZT19                  %29 ; [clobbered] temporary ZMM
%define %%ZT20                  %30 ; [clobbered] temporary ZMM
%define %%ZT21                  %31 ; [clobbered] temporary ZMM
%define %%ZT22                  %32 ; [clobbered] temporary ZMM
%define %%ZT23                  %33 ; [clobbered] temporary ZMM
%define %%ADDBE_4x4             %34 ; [in] ZMM with 4x128bits 4 in big-endian
%define %%ADDBE_1234            %35 ; [in] ZMM with 4x128bits 1, 2, 3 and 4 in big-endian
%define %%TO_REDUCE_L           %36 ; [in/out] ZMM for low 4x128-bit GHASH sum
%define %%TO_REDUCE_H           %37 ; [in/out] ZMM for hi 4x128-bit GHASH sum
%define %%TO_REDUCE_M           %38 ; [in/out] ZMM for medium 4x128-bit GHASH sum
%define %%DO_REDUCTION          %39 ; [in] "no_reduction", "final_reduction", "first_time"
%define %%ENC_DEC               %40 ; [in] cipher direction
%define %%DATA_DISPL            %41 ; [in] fixed numerical data displacement/offset
%define %%GHASH_IN              %42 ; [in] current GHASH value or "no_ghash_in"

%define %%B00_03 %%ZT1
%define %%B04_07 %%ZT2
%define %%B08_11 %%ZT3
%define %%B12_15 %%ZT4

%define %%GH1H  %%ZT5 ; @note: do not change this mapping
%define %%GH1L  %%ZT6
%define %%GH1M  %%ZT7
%define %%GH1T  %%ZT8

%define %%GH2H  %%ZT9
%define %%GH2L  %%ZT10
%define %%GH2M  %%ZT11
%define %%GH2T  %%ZT12

%define %%RED_POLY %%GH2T
%define %%RED_P1   %%GH2L
%define %%RED_T1   %%GH2H
%define %%RED_T2   %%GH2M

%define %%GH3H  %%ZT13
%define %%GH3L  %%ZT14
%define %%GH3M  %%ZT15
%define %%GH3T  %%ZT16

%define %%DATA1 %%ZT13
%define %%DATA2 %%ZT14
%define %%DATA3 %%ZT15
%define %%DATA4 %%ZT16

%define %%AESKEY1  %%ZT17
%define %%AESKEY2  %%ZT18

%define %%GHKEY1  %%ZT19
%define %%GHKEY2  %%ZT20
%define %%GHDAT1  %%ZT21
%define %%GHDAT2  %%ZT22

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; prepare counter blocks

        cmp             BYTE(%%CTR_CHECK), (256 - 16)
        jae             %%_16_blocks_overflow
        vpaddd          %%B00_03, %%CTR_BE, %%ADDBE_1234
        vpaddd          %%B04_07, %%B00_03, %%ADDBE_4x4
        vpaddd          %%B08_11, %%B04_07, %%ADDBE_4x4
        vpaddd          %%B12_15, %%B08_11, %%ADDBE_4x4
        jmp             %%_16_blocks_ok
%%_16_blocks_overflow:
        vpshufb         %%CTR_BE, %%CTR_BE, %%SHFMSK
        vmovdqa64       %%B12_15, [rel ddq_add_4444]
        vpaddd          %%B00_03, %%CTR_BE, [rel ddq_add_1234]
        vpaddd          %%B04_07, %%B00_03, %%B12_15
        vpaddd          %%B08_11, %%B04_07, %%B12_15
        vpaddd          %%B12_15, %%B08_11, %%B12_15
        vpshufb         %%B00_03, %%SHFMSK
        vpshufb         %%B04_07, %%SHFMSK
        vpshufb         %%B08_11, %%SHFMSK
        vpshufb         %%B12_15, %%SHFMSK
%%_16_blocks_ok:

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; pre-load constants
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 0)]
%ifnidn %%GHASH_IN, no_ghash_in
        vpxorq          %%GHDAT1, %%GHASH_IN, [rsp + %%GHASHIN_BLK_OFFSET + (0*64)]
%else
        vmovdqa64       %%GHDAT1, [rsp + %%GHASHIN_BLK_OFFSET + (0*64)]
%endif
        vmovdqu64       %%GHKEY1, [%%GDATA + %%HASHKEY_OFFSET + (0*64)]

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; save counter for the next round
        ;; increment counter overflow check register
        vshufi64x2      %%CTR_BE, %%B12_15, %%B12_15, 1111_1111b
        add             BYTE(%%CTR_CHECK), 16

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; pre-load constants
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 1)]
        vmovdqu64       %%GHKEY2, [%%GDATA + %%HASHKEY_OFFSET + (1*64)]
        vmovdqa64       %%GHDAT2, [rsp + %%GHASHIN_BLK_OFFSET + (1*64)]

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; stitch AES rounds with GHASH

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 0 - ARK

        vpxorq          %%B00_03, %%AESKEY1
        vpxorq          %%B04_07, %%AESKEY1
        vpxorq          %%B08_11, %%AESKEY1
        vpxorq          %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 2)]

        ;;==================================================
        ;; GHASH 4 blocks (15 to 12)
        vpclmulqdq      %%GH1H, %%GHDAT1, %%GHKEY1, 0x11     ; a1*b1
        vpclmulqdq      %%GH1L, %%GHDAT1, %%GHKEY1, 0x00     ; a0*b0
        vpclmulqdq      %%GH1M, %%GHDAT1, %%GHKEY1, 0x01     ; a1*b0
        vpclmulqdq      %%GH1T, %%GHDAT1, %%GHKEY1, 0x10     ; a0*b1

        vmovdqu64       %%GHKEY1, [%%GDATA + %%HASHKEY_OFFSET + (2*64)]
        vmovdqa64       %%GHDAT1, [rsp + %%GHASHIN_BLK_OFFSET + (2*64)]

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 1
        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 3)]

        ;; =================================================
        ;; GHASH 4 blocks (11 to 8)
        vpclmulqdq      %%GH2M, %%GHDAT2, %%GHKEY2, 0x10     ; a0*b1
        vpclmulqdq      %%GH2T, %%GHDAT2, %%GHKEY2, 0x01     ; a1*b0
        vpclmulqdq      %%GH2H, %%GHDAT2, %%GHKEY2, 0x11     ; a1*b1
        vpclmulqdq      %%GH2L, %%GHDAT2, %%GHKEY2, 0x00     ; a0*b0

        vmovdqu64       %%GHKEY2, [%%GDATA + %%HASHKEY_OFFSET + (3*64)]
        vmovdqa64       %%GHDAT2, [rsp + %%GHASHIN_BLK_OFFSET + (3*64)]

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 2
        vaesenc         %%B00_03, %%B00_03, %%AESKEY1
        vaesenc         %%B04_07, %%B04_07, %%AESKEY1
        vaesenc         %%B08_11, %%B08_11, %%AESKEY1
        vaesenc         %%B12_15, %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 4)]

        ;; =================================================
        ;; GHASH 4 blocks (7 to 4)
        vpclmulqdq      %%GH3M, %%GHDAT1, %%GHKEY1, 0x10     ; a0*b1
        vpclmulqdq      %%GH3T, %%GHDAT1, %%GHKEY1, 0x01     ; a1*b0
        vpclmulqdq      %%GH3H, %%GHDAT1, %%GHKEY1, 0x11     ; a1*b1
        vpclmulqdq      %%GH3L, %%GHDAT1, %%GHKEY1, 0x00     ; a0*b0

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES rounds 3
        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 5)]

        ;; =================================================
        ;; Gather (XOR) GHASH for 12 blocks
        vpternlogq      %%GH1H, %%GH2H, %%GH3H, 0x96
        vpternlogq      %%GH1L, %%GH2L, %%GH3L, 0x96
        vpternlogq      %%GH1T, %%GH2T, %%GH3T, 0x96
        vpternlogq      %%GH1M, %%GH2M, %%GH3M, 0x96

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES rounds 4
        vaesenc         %%B00_03, %%B00_03, %%AESKEY1
        vaesenc         %%B04_07, %%B04_07, %%AESKEY1
        vaesenc         %%B08_11, %%B08_11, %%AESKEY1
        vaesenc         %%B12_15, %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 6)]

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; load plain/cipher text (recycle GH3xx registers)
        VX512LDR        %%DATA1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + %%DATA_DISPL + (0 * 64)]
        VX512LDR        %%DATA2, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + %%DATA_DISPL + (1 * 64)]
        VX512LDR        %%DATA3, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + %%DATA_DISPL + (2 * 64)]
        VX512LDR        %%DATA4, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + %%DATA_DISPL + (3 * 64)]

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES rounds 5
        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 7)]

        ;; =================================================
        ;; GHASH 4 blocks (3 to 0)
        vpclmulqdq      %%GH2M, %%GHDAT2, %%GHKEY2, 0x10     ; a0*b1
        vpclmulqdq      %%GH2T, %%GHDAT2, %%GHKEY2, 0x01     ; a1*b0
        vpclmulqdq      %%GH2H, %%GHDAT2, %%GHKEY2, 0x11     ; a1*b1
        vpclmulqdq      %%GH2L, %%GHDAT2, %%GHKEY2, 0x00     ; a0*b0

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 6
        vaesenc         %%B00_03, %%B00_03, %%AESKEY1
        vaesenc         %%B04_07, %%B04_07, %%AESKEY1
        vaesenc         %%B08_11, %%B08_11, %%AESKEY1
        vaesenc         %%B12_15, %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 8)]

        ;; =================================================
        ;; gather GHASH in GH1L (low) and GH1H (high)
%ifidn %%DO_REDUCTION, first_time
        vpternlogq      %%GH1M, %%GH1T, %%GH2T, 0x96        ; TM
        vpxorq          %%TO_REDUCE_M, %%GH1M, %%GH2M       ; TM
        vpxorq          %%TO_REDUCE_H, %%GH1H, %%GH2H       ; TH
        vpxorq          %%TO_REDUCE_L, %%GH1L, %%GH2L       ; TL
%endif
%ifidn %%DO_REDUCTION, no_reduction
        vpternlogq      %%GH1M, %%GH1T, %%GH2T, 0x96        ; TM
        vpternlogq      %%TO_REDUCE_M, %%GH1M, %%GH2M, 0x96 ; TM
        vpternlogq      %%TO_REDUCE_H, %%GH1H, %%GH2H, 0x96 ; TH
        vpternlogq      %%TO_REDUCE_L, %%GH1L, %%GH2L, 0x96 ; TL
%endif
%ifidn %%DO_REDUCTION, final_reduction
        ;; phase 1: add mid products together
        ;; also load polynomial constant for reduction
        vpternlogq      %%GH1M, %%GH1T, %%GH2T, 0x96 ; TM
        vpternlogq      %%GH1M, %%TO_REDUCE_M, %%GH2M, 0x96

        vpsrldq         %%GH2M, %%GH1M, 8
        vpslldq         %%GH1M, %%GH1M, 8

        vmovdqa64       XWORD(%%RED_POLY), [rel POLY2]
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 7
        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 9)]

        ;; =================================================
        ;; Add mid product to high and low
%ifidn %%DO_REDUCTION, final_reduction
        vpternlogq      %%GH1H, %%GH2H, %%GH2M, 0x96    ; TH = TH1 + TH2 + TM>>64
        vpxorq          %%GH1H, %%TO_REDUCE_H
        vpternlogq      %%GH1L, %%GH2L, %%GH1M, 0x96    ; TL = TL1 + TL2 + TM<<64
        vpxorq          %%GH1L, %%TO_REDUCE_L
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 8
        vaesenc         %%B00_03, %%B00_03, %%AESKEY1
        vaesenc         %%B04_07, %%B04_07, %%AESKEY1
        vaesenc         %%B08_11, %%B08_11, %%AESKEY1
        vaesenc         %%B12_15, %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 10)]

        ;; =================================================
        ;; horizontal xor of low and high 4x128
%ifidn %%DO_REDUCTION, final_reduction
        VHPXORI4x128    %%GH1H, %%GH2H
        VHPXORI4x128    %%GH1L, %%GH2L
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES round 9
        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
%if (NROUNDS >= 11)
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 11)]
%endif
        ;; =================================================
        ;; first phase of reduction
%ifidn %%DO_REDUCTION, final_reduction
        vpclmulqdq      XWORD(%%RED_P1), XWORD(%%RED_POLY), XWORD(%%GH1L), 0x01
        vpslldq         XWORD(%%RED_P1), XWORD(%%RED_P1), 8             ; shift-L 2 DWs
        vpxorq          XWORD(%%RED_P1), XWORD(%%GH1L), XWORD(%%RED_P1) ; first phase of the reduct
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; AES rounds up to 11 (AES192) or 13 (AES256)
        ;; AES128 is done
%if (NROUNDS >= 11)
        vaesenc         %%B00_03, %%B00_03, %%AESKEY1
        vaesenc         %%B04_07, %%B04_07, %%AESKEY1
        vaesenc         %%B08_11, %%B08_11, %%AESKEY1
        vaesenc         %%B12_15, %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 12)]

        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
%if (NROUNDS == 13)
        vbroadcastf64x2 %%AESKEY2, [%%GDATA + (16 * 13)]

        vaesenc         %%B00_03, %%B00_03, %%AESKEY1
        vaesenc         %%B04_07, %%B04_07, %%AESKEY1
        vaesenc         %%B08_11, %%B08_11, %%AESKEY1
        vaesenc         %%B12_15, %%B12_15, %%AESKEY1
        vbroadcastf64x2 %%AESKEY1, [%%GDATA + (16 * 14)]

        vaesenc         %%B00_03, %%B00_03, %%AESKEY2
        vaesenc         %%B04_07, %%B04_07, %%AESKEY2
        vaesenc         %%B08_11, %%B08_11, %%AESKEY2
        vaesenc         %%B12_15, %%B12_15, %%AESKEY2
%endif ; GCM256 / NROUNDS = 13 (15 including the first and the last)
%endif ; GCM192 / NROUNDS = 11 (13 including the first and the last)

        ;; =================================================
        ;; second phase of the reduction
%ifidn %%DO_REDUCTION, final_reduction
        vpclmulqdq      XWORD(%%RED_T1), XWORD(%%RED_POLY), XWORD(%%RED_P1), 0x00
        vpsrldq         XWORD(%%RED_T1), XWORD(%%RED_T1), 4 ; shift-R 1-DW to obtain 2-DWs shift-R

        vpclmulqdq      XWORD(%%RED_T2), XWORD(%%RED_POLY), XWORD(%%RED_P1), 0x10
        vpslldq         XWORD(%%RED_T2), XWORD(%%RED_T2), 4 ; shift-L 1-DW for result without shifts
        ;; GH1H = GH1H x RED_T1 x RED_T2
        vpternlogq      XWORD(%%GH1H), XWORD(%%RED_T2), XWORD(%%RED_T1), 0x96
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; the last AES round
        vaesenclast     %%B00_03, %%B00_03, %%AESKEY1
        vaesenclast     %%B04_07, %%B04_07, %%AESKEY1
        vaesenclast     %%B08_11, %%B08_11, %%AESKEY1
        vaesenclast     %%B12_15, %%B12_15, %%AESKEY1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; XOR against plain/cipher text
        vpxorq          %%B00_03, %%B00_03, %%DATA1
        vpxorq          %%B04_07, %%B04_07, %%DATA2
        vpxorq          %%B08_11, %%B08_11, %%DATA3
        vpxorq          %%B12_15, %%B12_15, %%DATA4

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; store cipher/plain text
        VX512STR        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + %%DATA_DISPL + (0 * 64)], %%B00_03
        VX512STR        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + %%DATA_DISPL + (1 * 64)], %%B04_07
        VX512STR        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + %%DATA_DISPL + (2 * 64)], %%B08_11
        VX512STR        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + %%DATA_DISPL + (3 * 64)], %%B12_15

        ;; =================================================
        ;; shuffle cipher text blocks for GHASH computation
%ifidn %%ENC_DEC, ENC
        vpshufb         %%B00_03, %%B00_03, %%SHFMSK
        vpshufb         %%B04_07, %%B04_07, %%SHFMSK
        vpshufb         %%B08_11, %%B08_11, %%SHFMSK
        vpshufb         %%B12_15, %%B12_15, %%SHFMSK
%else
        vpshufb         %%B00_03, %%DATA1, %%SHFMSK
        vpshufb         %%B04_07, %%DATA2, %%SHFMSK
        vpshufb         %%B08_11, %%DATA3, %%SHFMSK
        vpshufb         %%B12_15, %%DATA4, %%SHFMSK
%endif

        ;; =================================================
        ;; store shuffled cipher text for ghashing
        vmovdqa64       [rsp + %%AESOUT_BLK_OFFSET + (0*64)], %%B00_03
        vmovdqa64       [rsp + %%AESOUT_BLK_OFFSET + (1*64)], %%B04_07
        vmovdqa64       [rsp + %%AESOUT_BLK_OFFSET + (2*64)], %%B08_11
        vmovdqa64       [rsp + %%AESOUT_BLK_OFFSET + (3*64)], %%B12_15

%ifidn %%DO_REDUCTION, final_reduction
        ;; =================================================
        ;; Return GHASH value  through %%GH1H
%endif

%endmacro                       ; GHASH_16_ENCRYPT_16_PARALLEL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GHASH the last 8 ciphertext blocks.
;;; - optionally accepts GHASH product sums as input
%macro  GHASH_LAST_8 10-13
%define %%GDATA         %1      ; [in] key pointer
%define %%BL47          %2      ; [in/clobbered] ZMM AES blocks 4 to 7
%define %%BL03          %3      ; [in/cloberred] ZMM AES blocks 0 to 3
%define %%ZTH           %4      ; [cloberred] ZMM temporary
%define %%ZTM           %5      ; [cloberred] ZMM temporary
%define %%ZTL           %6      ; [cloberred] ZMM temporary
%define %%ZT01          %7      ; [cloberred] ZMM temporary
%define %%ZT02          %8      ; [cloberred] ZMM temporary
%define %%ZT03          %9      ; [cloberred] ZMM temporary
%define %%AAD_HASH      %10     ; [out] XMM hash value
%define %%GH            %11     ; [in/optional] ZMM with GHASH high product sum
%define %%GL            %12     ; [in/optional] ZMM with GHASH low product sum
%define %%GM            %13     ; [in/optional] ZMM with GHASH mid product sum

        VCLMUL_STEP1    %%GDATA, %%BL47, %%ZT01, %%ZTH, %%ZTM, %%ZTL

%if %0 > 10
        ;; add optional sums before step2
        vpxorq          %%ZTH, %%ZTH, %%GH
        vpxorq          %%ZTL, %%ZTL, %%GL
        vpxorq          %%ZTM, %%ZTM, %%GM
%endif

        VCLMUL_STEP2    %%GDATA, %%BL47, %%BL03, %%ZT01, %%ZT02, %%ZT03, %%ZTH, %%ZTM, %%ZTL

        vmovdqa64       XWORD(%%ZT03), [rel POLY2]
        VCLMUL_REDUCE   %%AAD_HASH, XWORD(%%ZT03), XWORD(%%BL47), XWORD(%%BL03), \
                XWORD(%%ZT01), XWORD(%%ZT02)
%endmacro                       ; GHASH_LAST_8

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GHASH the last 7 cipher text blocks.
;;; - it uses same GHASH macros as GHASH_LAST_8 but with some twist
;;; - it loads GHASH keys for each of the data blocks, so that:
;;;     - blocks 4, 5 and 6 will use GHASH keys 3, 2, 1 respectively
;;;     - code ensures that unused block 7 and corresponding GHASH key are zeroed
;;;       (clmul product is zero this way and will not affect the result)
;;;     - blocks 0, 1, 2 and 3 will use USE GHASH keys 7, 6, 5 and 4 respectively
;;; - optionally accepts GHASH product sums as input
%macro  GHASH_LAST_7 13-16
%define %%GDATA         %1      ; [in] key pointer
%define %%BL47          %2      ; [in/clobbered] ZMM AES blocks 4 to 7
%define %%BL03          %3      ; [in/cloberred] ZMM AES blocks 0 to 3
%define %%ZTH           %4      ; [cloberred] ZMM temporary
%define %%ZTM           %5      ; [cloberred] ZMM temporary
%define %%ZTL           %6      ; [cloberred] ZMM temporary
%define %%ZT01          %7      ; [cloberred] ZMM temporary
%define %%ZT02          %8      ; [cloberred] ZMM temporary
%define %%ZT03          %9      ; [cloberred] ZMM temporary
%define %%ZT04          %10     ; [cloberred] ZMM temporary
%define %%AAD_HASH      %11     ; [out] XMM hash value
%define %%MASKREG       %12     ; [clobbered] mask register to use for loads
%define %%IA0           %13     ; [clobbered] GP temporary register
%define %%GH            %14     ; [in/optional] ZMM with GHASH high product sum
%define %%GL            %15     ; [in/optional] ZMM with GHASH low product sum
%define %%GM            %16     ; [in/optional] ZMM with GHASH mid product sum

        vmovdqa64       XWORD(%%ZT04), [rel POLY2]

        VCLMUL_1_TO_8_STEP1 %%GDATA, %%BL47, %%ZT01, %%ZT02, %%ZTH, %%ZTM, %%ZTL, 7

%if %0 > 13
        ;; add optional sums before step2
        vpxorq          %%ZTH, %%ZTH, %%GH
        vpxorq          %%ZTL, %%ZTL, %%GL
        vpxorq          %%ZTM, %%ZTM, %%GM
%endif

        VCLMUL_1_TO_8_STEP2 %%GDATA, %%BL47, %%BL03, \
                %%ZT01, %%ZT02, %%ZT03, \
                %%ZTH, %%ZTM, %%ZTL, 7

        VCLMUL_REDUCE   %%AAD_HASH, XWORD(%%ZT04), XWORD(%%BL47), XWORD(%%BL03), \
                XWORD(%%ZT01), XWORD(%%ZT02)
%endmacro                       ; GHASH_LAST_7


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Encryption of a single block
%macro  ENCRYPT_SINGLE_BLOCK 2
%define %%GDATA %1
%define %%XMM0  %2

                vpxorq          %%XMM0, %%XMM0, [%%GDATA+16*0]
%assign i 1
%rep NROUNDS
                vaesenc         %%XMM0, [%%GDATA+16*i]
%assign i (i+1)
%endrep
                vaesenclast     %%XMM0, [%%GDATA+16*i]
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Save register content for the caller
%macro FUNC_SAVE 0
        ;; Required for Update/GMC_ENC
        ;the number of pushes must equal STACK_OFFSET
        mov     rax, rsp

        sub     rsp, STACK_FRAME_SIZE
        and     rsp, ~63

        mov     [rsp + STACK_GP_OFFSET + 0*8], r12
        mov     [rsp + STACK_GP_OFFSET + 1*8], r13
        mov     [rsp + STACK_GP_OFFSET + 2*8], r14
        mov     [rsp + STACK_GP_OFFSET + 3*8], r15
        mov     [rsp + STACK_GP_OFFSET + 4*8], rax ; stack
        mov     r14, rax                               ; r14 is used to retrieve stack args
        mov     [rsp + STACK_GP_OFFSET + 5*8], rbp
        mov     [rsp + STACK_GP_OFFSET + 6*8], rbx
%ifidn __OUTPUT_FORMAT__, win64
        mov     [rsp + STACK_GP_OFFSET + 7*8], rdi
        mov     [rsp + STACK_GP_OFFSET + 8*8], rsi
%endif

%ifidn __OUTPUT_FORMAT__, win64
        ; xmm6:xmm15 need to be maintained for Windows
        vmovdqu [rsp + STACK_XMM_OFFSET + 0*16], xmm6
        vmovdqu [rsp + STACK_XMM_OFFSET + 1*16], xmm7
        vmovdqu [rsp + STACK_XMM_OFFSET + 2*16], xmm8
        vmovdqu [rsp + STACK_XMM_OFFSET + 3*16], xmm9
        vmovdqu [rsp + STACK_XMM_OFFSET + 4*16], xmm10
        vmovdqu [rsp + STACK_XMM_OFFSET + 5*16], xmm11
        vmovdqu [rsp + STACK_XMM_OFFSET + 6*16], xmm12
        vmovdqu [rsp + STACK_XMM_OFFSET + 7*16], xmm13
        vmovdqu [rsp + STACK_XMM_OFFSET + 8*16], xmm14
        vmovdqu [rsp + STACK_XMM_OFFSET + 9*16], xmm15
%endif
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Restore register content for the caller
%macro FUNC_RESTORE 0

%ifdef SAFE_DATA
        clear_scratch_gps_asm
        clear_scratch_zmms_asm
%else
        vzeroupper
%endif

%ifidn __OUTPUT_FORMAT__, win64
        vmovdqu xmm15, [rsp + STACK_XMM_OFFSET + 9*16]
        vmovdqu xmm14, [rsp + STACK_XMM_OFFSET + 8*16]
        vmovdqu xmm13, [rsp + STACK_XMM_OFFSET + 7*16]
        vmovdqu xmm12, [rsp + STACK_XMM_OFFSET + 6*16]
        vmovdqu xmm11, [rsp + STACK_XMM_OFFSET + 5*16]
        vmovdqu xmm10, [rsp + STACK_XMM_OFFSET + 4*16]
        vmovdqu xmm9, [rsp + STACK_XMM_OFFSET + 3*16]
        vmovdqu xmm8, [rsp + STACK_XMM_OFFSET + 2*16]
        vmovdqu xmm7, [rsp + STACK_XMM_OFFSET + 1*16]
        vmovdqu xmm6, [rsp + STACK_XMM_OFFSET + 0*16]
%endif

        ;; Required for Update/GMC_ENC
        mov     rbp, [rsp + STACK_GP_OFFSET + 5*8]
        mov     rbx, [rsp + STACK_GP_OFFSET + 6*8]
%ifidn __OUTPUT_FORMAT__, win64
        mov     rdi, [rsp + STACK_GP_OFFSET + 7*8]
        mov     rsi, [rsp + STACK_GP_OFFSET + 8*8]
%endif
        mov     r12, [rsp + STACK_GP_OFFSET + 0*8]
        mov     r13, [rsp + STACK_GP_OFFSET + 1*8]
        mov     r14, [rsp + STACK_GP_OFFSET + 2*8]
        mov     r15, [rsp + STACK_GP_OFFSET + 3*8]
        mov     rsp, [rsp + STACK_GP_OFFSET + 4*8] ; stack
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UPDATE_HASH: Calculates the hash of the data which will not be encrypted.
; Input: The input data (A_IN), that data's length (A_LEN), hash that being updated (HASH), and the hash key (HASH_KEY).
; Output: The hash of the data (HASH).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  UPDATE_HASH    18
%define %%A_IN          %1      ; [in] DATA text pointer
%define %%A_LEN         %2      ; [in] DATA length
%define %%HASH          %3      ; [out] xmm ghash value
%define %%GDATA_KEY     %4      ; [in] pointer to keys
%define %%ZT0           %5      ; [clobbered] ZMM register
%define %%ZT1           %6      ; [clobbered] ZMM register
%define %%ZT2           %7      ; [clobbered] ZMM register
%define %%ZT3           %8      ; [clobbered] ZMM register
%define %%ZT4           %9      ; [clobbered] ZMM register
%define %%ZT5           %10     ; [clobbered] ZMM register
%define %%ZT6           %11     ; [clobbered] ZMM register
%define %%ZT7           %12     ; [clobbered] ZMM register
%define %%ZT8           %13     ; [clobbered] ZMM register
%define %%ZT9           %14     ; [clobbered] ZMM register
%define %%T1            %15     ; [clobbered] GP register
%define %%T2            %16     ; [clobbered] GP register
%define %%T3            %17     ; [clobbered] GP register
%define %%MASKREG       %18     ; [clobbered] mask register

%define %%SHFMSK %%ZT9
%define %%POLY   %%ZT8
%define %%TH     %%ZT7
%define %%TM     %%ZT6
%define %%TL     %%ZT5

        mov             %%T1, %%A_IN            ; T1 = DATA
        mov             %%T2, %%A_LEN           ; T2 = DATALen

        vmovdqa64       %%SHFMSK, [rel SHUF_MASK]
        vmovdqa64       %%POLY, [rel POLY2]

%%_get_DATA_loop128:
        cmp             %%T2, 128
        jl              %%_exit_DATA_loop128

        vmovdqu64       %%ZT2, [%%T1 + 64*0]  ; LO blocks (0-3)
        vmovdqu64       %%ZT1, [%%T1 + 64*1]  ; HI blocks (4-7)
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         %%ZT1, %%SHFMSK

        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)

        VCLMUL_STEP1    %%GDATA_KEY, %%ZT1, %%ZT0, %%TH, %%TM, %%TL
        VCLMUL_STEP2    %%GDATA_KEY, %%ZT1, %%ZT2, %%ZT0, %%ZT3, %%ZT4, %%TH, %%TM, %%TL

        ;; result in %%ZT1(H):%%ZT2(L)
        ;; reduce and put the result in HASH
        VCLMUL_REDUCE   %%HASH, XWORD(%%POLY), XWORD(%%ZT1), XWORD(%%ZT2), \
                XWORD(%%ZT0), XWORD(%%ZT3)

        sub             %%T2, 128
        je              %%_CALC_DATA_HASH_done

        add             %%T1, 128
        jmp             %%_get_DATA_loop128

%%_exit_DATA_loop128:
        or              %%T2, %%T2
        jz              %%_CALC_DATA_HASH_done

        ;; prep mask source address
        lea             %%T3, [rel byte64_len_to_mask_table]
        lea             %%T3, [%%T3 + %%T2*8]

        ;; calculate number of blocks to ghash (including partial bytes)
        add             %%T2, 15
        and             %%T2, -16       ; 1 to 8 blocks possible here
        shr             %%T2, 4
        cmp             %%T2, 7
        je              %%_DATA_blocks_7
        cmp             %%T2, 6
        je              %%_DATA_blocks_6
        cmp             %%T2, 5
        je              %%_DATA_blocks_5
        cmp             %%T2, 4
        je              %%_DATA_blocks_4
        cmp             %%T2, 3
        je              %%_DATA_blocks_3
        cmp             %%T2, 2
        je              %%_DATA_blocks_2
        cmp             %%T2, 1
        je              %%_DATA_blocks_1
        ;; fall through for 8 blocks

        ;; The flow of each of these cases is identical:
        ;; - load blocks plain text
        ;; - shuffle loaded blocks
        ;; - xor in current hash value into block 0
        ;; - perform up multiplications with ghash keys
        ;; - jump to reduction code
%%_DATA_blocks_8:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        %%ZT1{%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         %%ZT1, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH) ; xor in current ghash
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 8
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 8
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_7:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        %%ZT1{%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         %%ZT1, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH) ; xor in current ghash
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 7
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 7
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_6:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        YWORD(%%ZT1){%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         YWORD(%%ZT1), YWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 6
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 6
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_5:
        sub             %%T3, (64 * 8)
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2, [%%T1 + 64*0]
        vmovdqu8        XWORD(%%ZT1){%%MASKREG}{z}, [%%T1 + 64*1]
        vpshufb         %%ZT2, %%SHFMSK
        vpshufb         XWORD(%%ZT1), XWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 5
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 5
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_4:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2{%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         %%ZT2, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 4
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 4
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_3:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        %%ZT2{%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         %%ZT2, %%SHFMSK
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 3
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 3
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_2:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        YWORD(%%ZT2){%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         YWORD(%%ZT2), YWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 2
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 2
        jmp             %%_DATA_blocks_done

%%_DATA_blocks_1:
        kmovq           %%MASKREG, [%%T3]
        vmovdqu8        XWORD(%%ZT2){%%MASKREG}{z}, [%%T1 + 64*0]
        vpshufb         XWORD(%%ZT2), XWORD(%%SHFMSK)
        vpxorq          %%ZT2, %%ZT2, ZWORD(%%HASH)
        VCLMUL_1_TO_8_STEP1 %%GDATA_KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 1
        VCLMUL_1_TO_8_STEP2 %%GDATA_KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 1

%%_DATA_blocks_done:
        ;; Multiplications have been done. Do the reduction now
        VCLMUL_REDUCE   %%HASH, XWORD(%%POLY), XWORD(%%ZT1), XWORD(%%ZT2), \
                        XWORD(%%ZT0), XWORD(%%ZT3)
%%_CALC_DATA_HASH_done:
        ;; result in HASH

%endmacro ; UPDATE_HASH


%macro CALC_J0 19
%define %%KEY           %1 ;; [in] Pointer to GCM KEY structure
%define %%IV            %2 ;; [in] Pointer to IV
%define %%IV_LEN        %3 ;; [in] IV length
%define %%J0            %4 ;; [out] XMM reg to contain J0
%define %%ZT0           %5 ;; [clobbered] ZMM register
%define %%ZT1           %6 ;; [clobbered] ZMM register
%define %%ZT2           %7 ;; [clobbered] ZMM register
%define %%ZT3           %8 ;; [clobbered] ZMM register
%define %%ZT4           %9 ;; [clobbered] ZMM register
%define %%ZT5           %10 ;; [clobbered] ZMM register
%define %%ZT6           %11 ;; [clobbered] ZMM register
%define %%ZT7           %12 ;; [clobbered] ZMM register
%define %%ZT8           %13 ;; [clobbered] ZMM register
%define %%ZT9           %14 ;; [clobbered] ZMM register
%define %%T1            %15 ;; [clobbered] GP register
%define %%T2            %16 ;; [clobbered] GP register
%define %%T3            %17 ;; [clobbered] GP register
%define %%MASKREG       %18 ;; [clobbered] mask register
%define %%IV_GEN_LEN    %19 ;; [in] IV general length 

%define %%POLY   %%ZT8
%define %%TH     %%ZT7
%define %%TM     %%ZT6
%define %%TL     %%ZT5

        ;; J0 = GHASH(IV || 0s+64 || len(IV)64)
        ;; s = 16 * RoundUp(len(IV)/16) -  len(IV) */

        ;; Calculate GHASH of (IV || 0s)
        UPDATE_HASH   %%IV, %%IV_LEN, %%J0, %%KEY, %%ZT0, %%ZT1, %%ZT2, %%ZT3, \
                      %%ZT4, %%ZT5, %%ZT6, %%ZT7, %%ZT8, %%ZT9, %%T1, %%T2,\
                      %%T3, %%MASKREG

        ;; Calculate GHASH of last 16-byte block (0 || len(IV)64)
        mov     %%T1, %%IV_GEN_LEN
        shl     %%T1, 3 ;; IV length in bits
        vmovq   XWORD(%%ZT2), %%T1
        ;; Might need shuffle of ZT2
        vpxorq  %%ZT2, ZWORD(%%J0)
        VCLMUL_1_TO_8_STEP1 %%KEY, %%ZT1, %%ZT0, %%ZT3, %%TH, %%TM, %%TL, 1
        VCLMUL_1_TO_8_STEP2 %%KEY, %%ZT1, %%ZT2, \
                %%ZT0, %%ZT3, %%ZT4, \
                %%TH, %%TM, %%TL, 1

        ;; Multiplications have been done. Do the reduction now
        VCLMUL_REDUCE   %%J0, XWORD(%%POLY), XWORD(%%ZT1), XWORD(%%ZT2), \
                        XWORD(%%ZT0), XWORD(%%ZT3)
        vpshufb %%J0, [rel SHUF_MASK] ; perform a 16Byte swap
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Cipher and ghash of payloads shorter than 256 bytes
;;; - number of blocks in the message comes as argument
;;; - depending on the number of blocks an optimized variant of
;;;   INITIAL_BLOCKS_PARTIAL is invoked
%macro  GCM_ENC_DEC_SMALL   42
%define %%GDATA_KEY         %1  ; [in] key pointer
%define %%GDATA_CTX         %2  ; [in] context pointer
%define %%CYPH_PLAIN_OUT    %3  ; [in] output buffer
%define %%PLAIN_CYPH_IN     %4  ; [in] input buffer
%define %%PLAIN_CYPH_LEN    %5  ; [in] buffer length
%define %%ENC_DEC           %6  ; [in] cipher direction
%define %%DATA_OFFSET       %7  ; [in] data offset
%define %%LENGTH            %8  ; [in] data length
%define %%NUM_BLOCKS        %9  ; [in] number of blocks to process 1 to 16
%define %%CTR               %10 ; [in/out] XMM counter block
%define %%HASH_IN_OUT       %11 ; [in/out] XMM GHASH value
%define %%INSTANCE_TYPE     %12 ; [in] single or multi call
%define %%ZTMP0             %13 ; [clobbered] ZMM register
%define %%ZTMP1             %14 ; [clobbered] ZMM register
%define %%ZTMP2             %15 ; [clobbered] ZMM register
%define %%ZTMP3             %16 ; [clobbered] ZMM register
%define %%ZTMP4             %17 ; [clobbered] ZMM register
%define %%ZTMP5             %18 ; [clobbered] ZMM register
%define %%ZTMP6             %19 ; [clobbered] ZMM register
%define %%ZTMP7             %20 ; [clobbered] ZMM register
%define %%ZTMP8             %21 ; [clobbered] ZMM register
%define %%ZTMP9             %22 ; [clobbered] ZMM register
%define %%ZTMP10            %23 ; [clobbered] ZMM register
%define %%ZTMP11            %24 ; [clobbered] ZMM register
%define %%ZTMP12            %25 ; [clobbered] ZMM register
%define %%ZTMP13            %26 ; [clobbered] ZMM register
%define %%ZTMP14            %27 ; [clobbered] ZMM register
%define %%ZTMP15            %28 ; [clobbered] ZMM register
%define %%ZTMP16            %29 ; [clobbered] ZMM register
%define %%ZTMP17            %30 ; [clobbered] ZMM register
%define %%ZTMP18            %31 ; [clobbered] ZMM register
%define %%ZTMP19            %32 ; [clobbered] ZMM register
%define %%ZTMP20            %33 ; [clobbered] ZMM register
%define %%ZTMP21            %34 ; [clobbered] ZMM register
%define %%ZTMP22            %35 ; [clobbered] ZMM register
%define %%GH                %36 ; [in] ZMM ghash sum (high)
%define %%GL                %37 ; [in] ZMM ghash sum (low)
%define %%GM                %38 ; [in] ZMM ghash sum (middle)
%define %%IA0               %39 ; [clobbered] GP register
%define %%IA1               %40 ; [clobbered] GP register
%define %%MASKREG           %41 ; [clobbered] mask register
%define %%SHUFMASK          %42 ; [in] ZMM with BE/LE shuffle mask

        cmp     %%NUM_BLOCKS, 8
        je      %%_small_initial_num_blocks_is_8
        jl      %%_small_initial_num_blocks_is_7_1


        cmp     %%NUM_BLOCKS, 12
        je      %%_small_initial_num_blocks_is_12
        jl      %%_small_initial_num_blocks_is_11_9

        ;; 16, 15, 14 or 13
        cmp     %%NUM_BLOCKS, 16
        je      %%_small_initial_num_blocks_is_16
        cmp     %%NUM_BLOCKS, 15
        je      %%_small_initial_num_blocks_is_15
        cmp     %%NUM_BLOCKS, 14
        je      %%_small_initial_num_blocks_is_14
        jmp     %%_small_initial_num_blocks_is_13

%%_small_initial_num_blocks_is_11_9:
        ;; 11, 10 or 9
        cmp     %%NUM_BLOCKS, 11
        je      %%_small_initial_num_blocks_is_11
        cmp     %%NUM_BLOCKS, 10
        je      %%_small_initial_num_blocks_is_10
        jmp     %%_small_initial_num_blocks_is_9

%%_small_initial_num_blocks_is_7_1:
        cmp     %%NUM_BLOCKS, 4
        je      %%_small_initial_num_blocks_is_4
        jl      %%_small_initial_num_blocks_is_3_1
        ;; 7, 6 or 5
        cmp     %%NUM_BLOCKS, 7
        je      %%_small_initial_num_blocks_is_7
        cmp     %%NUM_BLOCKS, 6
        je      %%_small_initial_num_blocks_is_6
        jmp     %%_small_initial_num_blocks_is_5

%%_small_initial_num_blocks_is_3_1:
        ;; 3, 2 or 1
        cmp     %%NUM_BLOCKS, 3
        je      %%_small_initial_num_blocks_is_3
        cmp     %%NUM_BLOCKS, 2
        je      %%_small_initial_num_blocks_is_2

        ;; for %%NUM_BLOCKS == 1, just fall through and no 'jmp' needed

        ;; Use rep to generate different block size variants
        ;; - one block size has to be the first one
%assign num_blocks 1
%rep 16
%%_small_initial_num_blocks_is_ %+ num_blocks :
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, num_blocks, \
                %%CTR, %%HASH_IN_OUT, %%ENC_DEC, %%INSTANCE_TYPE, \
                %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, \
                %%ZTMP5, %%ZTMP6, %%ZTMP7, %%ZTMP8, %%ZTMP9, \
                %%ZTMP10, %%ZTMP11, %%ZTMP12, %%ZTMP13, %%ZTMP14, \
                %%ZTMP15, %%ZTMP16, %%ZTMP17, %%ZTMP18, %%ZTMP19, \
                %%ZTMP20, %%ZTMP21, %%ZTMP22, \
                %%GH, %%GL, %%GM, \
                %%IA0, %%IA1, %%MASKREG, %%SHUFMASK
%if num_blocks != 16
        jmp     %%_small_initial_blocks_encrypted
%endif
%assign num_blocks (num_blocks + 1)
%endrep

%%_small_initial_blocks_encrypted:

%endmacro                       ; GCM_ENC_DEC_SMALL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GCM_ENC_DEC Encodes/Decodes given data. Assumes that the passed gcm_context_data struct
; has been initialized by GCM_INIT
; Requires the input data be at least 1 byte long because of READ_SMALL_INPUT_DATA.
; Input: gcm_key_data struct* (GDATA_KEY), gcm_context_data *(GDATA_CTX), input text (PLAIN_CYPH_IN),
; input text length (PLAIN_CYPH_LEN) and whether encoding or decoding (ENC_DEC).
; Output: A cypher of the given plain text (CYPH_PLAIN_OUT), and updated GDATA_CTX
; Clobbers rax, r10-r15, and zmm0-zmm31, k1
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_ENC_DEC         7
%define %%GDATA_KEY         %1  ; [in] key pointer
%define %%GDATA_CTX         %2  ; [in] context pointer
%define %%CYPH_PLAIN_OUT    %3  ; [in] output buffer pointer
%define %%PLAIN_CYPH_IN     %4  ; [in] input buffer pointer
%define %%PLAIN_CYPH_LEN    %5  ; [in] buffer length
%define %%ENC_DEC           %6  ; [in] cipher direction
%define %%INSTANCE_TYPE     %7  ; [in] 'single_call' or 'multi_call' selection

%define %%IA0               r10
%define %%IA1               r12
%define %%IA2               r13
%define %%IA3               r15
%define %%IA4               r11
%define %%IA5               rax

%define %%LENGTH            %%IA2
%define %%CTR_CHECK         %%IA3
%define %%DATA_OFFSET       %%IA4

%define %%HASHK_PTR         %%IA5

%define %%GCM_INIT_CTR_BLOCK    xmm2 ; hardcoded in GCM_INIT for now

%define %%AES_PARTIAL_BLOCK     xmm8
%define %%CTR_BLOCK2z           zmm18
%define %%CTR_BLOCKz            zmm9
%define %%CTR_BLOCKx            xmm9
%define %%AAD_HASHz             zmm14
%define %%AAD_HASHx             xmm14

;;; ZTMP0 - ZTMP12 - used in by8 code, by128/48 code and GCM_ENC_DEC_SMALL
%define %%ZTMP0                 zmm0
%define %%ZTMP1                 zmm3
%define %%ZTMP2                 zmm4
%define %%ZTMP3                 zmm5
%define %%ZTMP4                 zmm6
%define %%ZTMP5                 zmm7
%define %%ZTMP6                 zmm10
%define %%ZTMP7                 zmm11
%define %%ZTMP8                 zmm12
%define %%ZTMP9                 zmm13
%define %%ZTMP10                zmm15
%define %%ZTMP11                zmm16
%define %%ZTMP12                zmm17

;;; ZTMP13 - ZTMP22 - used in by128/48 code and GCM_ENC_DEC_SMALL
;;; - some used by8 code as well through TMPxy names
%define %%ZTMP13                zmm19
%define %%ZTMP14                zmm20
%define %%ZTMP15                zmm21
%define %%ZTMP16                zmm30   ; can be used in very/big_loop part
%define %%ZTMP17                zmm31   ; can be used in very/big_loop part
%define %%ZTMP18                zmm1
%define %%ZTMP19                zmm2
%define %%ZTMP20                zmm8
%define %%ZTMP21                zmm22
%define %%ZTMP22                zmm23

;;; Free to use: zmm24 - zmm29
;;; - used by by128/48 and by8
%define %%GH                    zmm24
%define %%GL                    zmm25
%define %%GM                    zmm26
%define %%SHUF_MASK             zmm29
%define %%CTR_BLOCK_SAVE        zmm28

;;; - used by by128/48 code only
%define %%ADDBE_4x4             zmm27
%define %%ADDBE_1234            zmm28       ; conflicts with CTR_BLOCK_SAVE

;; used by8 code only
%define %%GH4KEY                %%ZTMP17
%define %%GH8KEY                %%ZTMP16
%define %%BLK0                  %%ZTMP18
%define %%BLK1                  %%ZTMP19
%define %%ADD8BE                zmm27
%define %%ADD8LE                %%ZTMP13

%define %%MASKREG               k1

;; reduction every 48 blocks, depth 32 blocks
;; @note 48 blocks is the maximum capacity of the stack frame
%assign big_loop_nblocks        48
%assign big_loop_depth          32

;;; Macro flow:
;;; - for message size bigger than very_big_loop_nblocks process data
;;;   with "very_big_loop" parameters
;;; - for message size bigger than big_loop_nblocks process data
;;;   with "big_loop" parameters
;;; - calculate the number of 16byte blocks in the message
;;; - process (number of 16byte blocks) mod 8
;;;   '%%_initial_num_blocks_is_# .. %%_initial_blocks_encrypted'
;;; - process 8 16 byte blocks at a time until all are done in %%_encrypt_by_8_new

%ifidn __OUTPUT_FORMAT__, win64
        cmp             %%PLAIN_CYPH_LEN, 0
%else
        or              %%PLAIN_CYPH_LEN, %%PLAIN_CYPH_LEN
%endif
        je              %%_enc_dec_done

        xor             %%DATA_OFFSET, %%DATA_OFFSET

        ;; Update length of data processed
%ifidn __OUTPUT_FORMAT__, win64
        mov             %%IA0, %%PLAIN_CYPH_LEN
        add             [%%GDATA_CTX + InLen], %%IA0
%else
        add             [%%GDATA_CTX + InLen], %%PLAIN_CYPH_LEN
%endif
        vmovdqu64       %%AAD_HASHx, [%%GDATA_CTX + AadHash]

%ifidn %%INSTANCE_TYPE, multi_call
        ;; NOTE: partial block processing makes only sense for multi_call here.
        ;; Used for the update flow - if there was a previous partial
        ;; block fill the remaining bytes here.
        PARTIAL_BLOCK %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, \
                %%PLAIN_CYPH_LEN, %%DATA_OFFSET, %%AAD_HASHx, %%ENC_DEC, \
                %%IA0, %%IA1, %%IA2, %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, \
                %%ZTMP5, %%ZTMP6, %%ZTMP7, %%ZTMP8, %%ZTMP9, %%MASKREG
%endif

        ;;  lift counter block from GCM_INIT to here
%ifidn %%INSTANCE_TYPE, single_call
        vmovdqu64       %%CTR_BLOCKx, %%GCM_INIT_CTR_BLOCK
%else
        vmovdqu64       %%CTR_BLOCKx, [%%GDATA_CTX + CurCount]
%endif

        ;; Save the amount of data left to process in %%LENGTH
        mov             %%LENGTH, %%PLAIN_CYPH_LEN
%ifidn %%INSTANCE_TYPE, multi_call
        ;; NOTE: %%DATA_OFFSET is zero in single_call case.
        ;;      Consequently PLAIN_CYPH_LEN will never be zero after
        ;;      %%DATA_OFFSET subtraction below.
        ;; There may be no more data if it was consumed in the partial block.
        sub             %%LENGTH, %%DATA_OFFSET
        je              %%_enc_dec_done
%endif                          ; %%INSTANCE_TYPE, multi_call

        vmovdqa64       %%SHUF_MASK, [rel SHUF_MASK]
        vmovdqa64       %%ADDBE_4x4, [rel ddq_addbe_4444]

        cmp             %%LENGTH, (big_loop_nblocks * 16)
        jl              %%_message_below_big_nblocks

        ;; overwritten above by CTR_BLOCK_SAVE
        vmovdqa64        %%ADDBE_1234, [rel ddq_addbe_1234]

        INITIAL_BLOCKS_Nx16 %%PLAIN_CYPH_IN, %%CYPH_PLAIN_OUT, %%GDATA_KEY, %%DATA_OFFSET, \
                %%AAD_HASHz, %%CTR_BLOCKz, %%CTR_CHECK, \
                %%ZTMP0,  %%ZTMP1,  %%ZTMP2,  %%ZTMP3,  \
                %%ZTMP4,  %%ZTMP5,  %%ZTMP6,  %%ZTMP7,  \
                %%ZTMP8,  %%ZTMP9,  %%ZTMP10, %%ZTMP11, \
                %%ZTMP12, %%ZTMP13, %%ZTMP14, %%ZTMP15, \
                %%ZTMP16, %%ZTMP17, %%ZTMP18, %%ZTMP19, \
                %%ZTMP20, %%ZTMP21, %%ZTMP22, \
                %%GH, %%GL, %%GM, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%SHUF_MASK, %%ENC_DEC, big_loop_nblocks, big_loop_depth

        sub             %%LENGTH, (big_loop_nblocks * 16)
        cmp             %%LENGTH, (big_loop_nblocks * 16)
        jl              %%_no_more_big_nblocks

%%_encrypt_big_nblocks:
        GHASH_ENCRYPT_Nx16_PARALLEL \
                %%PLAIN_CYPH_IN, %%CYPH_PLAIN_OUT, %%GDATA_KEY, %%DATA_OFFSET, \
                %%CTR_BLOCKz, %%SHUF_MASK, \
                %%ZTMP0,  %%ZTMP1,  %%ZTMP2,  %%ZTMP3,  \
                %%ZTMP4,  %%ZTMP5,  %%ZTMP6,  %%ZTMP7,  \
                %%ZTMP8,  %%ZTMP9,  %%ZTMP10, %%ZTMP11, \
                %%ZTMP12, %%ZTMP13, %%ZTMP14, %%ZTMP15, \
                %%ZTMP16, %%ZTMP17, %%ZTMP18, %%ZTMP19, \
                %%ZTMP20, %%ZTMP21, %%ZTMP22, \
                %%GH, %%GL, %%GM, \
                %%ADDBE_4x4, %%ADDBE_1234, %%AAD_HASHz, \
                %%ENC_DEC, big_loop_nblocks, big_loop_depth, %%CTR_CHECK

        sub             %%LENGTH, (big_loop_nblocks * 16)
        cmp             %%LENGTH, (big_loop_nblocks * 16)
        jge             %%_encrypt_big_nblocks

%%_no_more_big_nblocks:
        vpshufb         %%CTR_BLOCKx, XWORD(%%SHUF_MASK)
        vmovdqa64       XWORD(%%CTR_BLOCK_SAVE), %%CTR_BLOCKx

        GHASH_LAST_Nx16 %%GDATA_KEY, %%AAD_HASHz, \
                %%ZTMP0,  %%ZTMP1,  %%ZTMP2,  %%ZTMP3,  \
                %%ZTMP4,  %%ZTMP5,  %%ZTMP6,  %%ZTMP7,  \
                %%ZTMP8,  %%ZTMP9,  %%ZTMP10, %%ZTMP11, \
                %%ZTMP12, %%ZTMP13, %%ZTMP14, %%ZTMP15, \
                %%GH, %%GL, %%GM, big_loop_nblocks, big_loop_depth

        or              %%LENGTH, %%LENGTH
        jz              %%_ghash_done

%%_message_below_big_nblocks:

        ;; Less than 256 bytes will be handled by the small message code, which
        ;; can process up to 16 x blocks (16 bytes each)
        cmp             %%LENGTH, (16 * 16)
        jge             %%_large_message_path

        ;; Determine how many blocks to process
        ;; - process one additional block if there is a partial block
        mov             %%IA1, %%LENGTH
        add             %%IA1, 15
        shr             %%IA1, 4
        ;; %%IA1 can be in the range from 0 to 16

        GCM_ENC_DEC_SMALL \
                %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, \
                %%PLAIN_CYPH_LEN, %%ENC_DEC, %%DATA_OFFSET, \
                %%LENGTH, %%IA1, %%CTR_BLOCKx, %%AAD_HASHx, %%INSTANCE_TYPE, \
                %%ZTMP0,  %%ZTMP1,  %%ZTMP2,  %%ZTMP3,  \
                %%ZTMP4,  %%ZTMP5,  %%ZTMP6,  %%ZTMP7,  \
                %%ZTMP8,  %%ZTMP9,  %%ZTMP10, %%ZTMP11, \
                %%ZTMP12, %%ZTMP13, %%ZTMP14, %%ZTMP15, \
                %%ZTMP16, %%ZTMP17, %%ZTMP18, %%ZTMP19, \
                %%ZTMP20, %%ZTMP21, %%ZTMP22, \
                no_zmm, no_zmm, no_zmm, \
                %%IA0, %%IA3, %%MASKREG, %%SHUF_MASK

        vmovdqa64       XWORD(%%CTR_BLOCK_SAVE), %%CTR_BLOCKx

        jmp     %%_ghash_done

%%_large_message_path:
        ;; Determine how many blocks to process in INITIAL
        ;; - process one additional block in INITIAL if there is a partial block
        mov             %%IA1, %%LENGTH
        and             %%IA1, 0xff
        add             %%IA1, 15
        shr             %%IA1, 4
        ;; Don't allow 8 INITIAL blocks since this will
        ;; be handled by the x8 partial loop.
        and             %%IA1, 7
        je              %%_initial_num_blocks_is_0
        cmp             %%IA1, 1
        je              %%_initial_num_blocks_is_1
        cmp             %%IA1, 2
        je              %%_initial_num_blocks_is_2
        cmp             %%IA1, 3
        je              %%_initial_num_blocks_is_3
        cmp             %%IA1, 4
        je              %%_initial_num_blocks_is_4
        cmp             %%IA1, 5
        je              %%_initial_num_blocks_is_5
        cmp             %%IA1, 6
        je              %%_initial_num_blocks_is_6

%assign number_of_blocks 7
%rep 8
%%_initial_num_blocks_is_ %+ number_of_blocks:
        INITIAL_BLOCKS  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, \
                %%LENGTH, %%DATA_OFFSET, number_of_blocks, %%CTR_BLOCKx, %%AAD_HASHz, \
                %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, \
                %%ZTMP5, %%ZTMP6, %%ZTMP7, %%ZTMP8, %%ZTMP9, %%ZTMP10, %%ZTMP11, \
                %%IA0, %%IA1, %%ENC_DEC, %%MASKREG, %%SHUF_MASK, no_partial_block
%if number_of_blocks != 0
        jmp             %%_initial_blocks_encrypted
%endif
%assign number_of_blocks (number_of_blocks - 1)
%endrep

%%_initial_blocks_encrypted:
        vmovdqa64       XWORD(%%CTR_BLOCK_SAVE), %%CTR_BLOCKx

        ;; move cipher blocks from intial blocks to input of by8 macro
        ;; and for GHASH_LAST_8/7
        ;; - ghash value already xor'ed into block 0
        vmovdqa64       %%BLK0, %%ZTMP0
        vmovdqa64       %%BLK1, %%ZTMP1

        ;; The entire message cannot get processed in INITIAL_BLOCKS
        ;; - GCM_ENC_DEC_SMALL handles up to 16 blocks
        ;; - INITIAL_BLOCKS processes up to 15 blocks
        ;; - no need to check for zero length at this stage

        ;; In order to have only one reduction at the end
        ;; start HASH KEY pointer needs to be determined based on length and
        ;; call type.
        ;; - note that 8 blocks are already ciphered in INITIAL_BLOCKS and
        ;;   subtracted from LENGTH
        lea             %%IA1, [%%LENGTH + (8 * 16)]
        add             %%IA1, 15
        and             %%IA1, 0x3f0
%ifidn %%INSTANCE_TYPE, multi_call
        ;; if partial block and multi_call then change hash key start by one
        mov             %%IA0, %%LENGTH
        and             %%IA0, 15
        add             %%IA0, 15
        and             %%IA0, 16
        sub             %%IA1, %%IA0
%endif
        lea             %%HASHK_PTR, [%%GDATA_KEY + HashKey + 16]
        sub             %%HASHK_PTR, %%IA1
        ;; HASHK_PTR
        ;; - points at the first hash key to start GHASH with
        ;; - needs to be updated as the message is processed (incremented)

        ;; pre-load constants
        vmovdqa64       %%ADD8BE, [rel ddq_addbe_8888]
        vmovdqa64       %%ADD8LE, [rel ddq_add_8888]
        vpxorq          %%GH, %%GH
        vpxorq          %%GL, %%GL
        vpxorq          %%GM, %%GM

        ;; prepare counter 8 blocks
        vshufi64x2      %%CTR_BLOCKz, %%CTR_BLOCKz, %%CTR_BLOCKz, 0
        vpaddd          %%CTR_BLOCK2z, %%CTR_BLOCKz, [rel ddq_add_5678]
        vpaddd          %%CTR_BLOCKz, %%CTR_BLOCKz, [rel ddq_add_1234]
        vpshufb         %%CTR_BLOCKz,  %%SHUF_MASK
        vpshufb         %%CTR_BLOCK2z, %%SHUF_MASK

        ;; Process 7 full blocks plus a partial block
        cmp             %%LENGTH, 128
        jl              %%_encrypt_by_8_partial

%%_encrypt_by_8_parallel:
        ;; in_order vs. out_order is an optimization to increment the counter
        ;; without shuffling it back into little endian.
        ;; %%CTR_CHECK keeps track of when we need to increment in order so
        ;; that the carry is handled correctly.

        vmovq           %%CTR_CHECK, XWORD(%%CTR_BLOCK_SAVE)

%%_encrypt_by_8_new:
        and             WORD(%%CTR_CHECK), 255
        add             WORD(%%CTR_CHECK), 8

        vmovdqu64       %%GH4KEY, [%%HASHK_PTR + (4 * 16)]
        vmovdqu64       %%GH8KEY, [%%HASHK_PTR + (0 * 16)]

        GHASH_8_ENCRYPT_8_PARALLEL  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, \
                %%DATA_OFFSET, %%CTR_BLOCKz, %%CTR_BLOCK2z,\
                %%BLK0, %%BLK1, %%AES_PARTIAL_BLOCK, \
                out_order, %%ENC_DEC, full, %%IA0, %%IA1, %%LENGTH, %%INSTANCE_TYPE, \
                %%GH4KEY, %%GH8KEY, %%SHUF_MASK, \
                %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, %%ZTMP5, %%ZTMP6, \
                %%ZTMP7, %%ZTMP8, %%ZTMP9, %%ZTMP10, %%ZTMP11, %%ZTMP12, \
                %%MASKREG, no_reduction, %%GL, %%GH, %%GM

        add             %%HASHK_PTR, (8 * 16)
        add             %%DATA_OFFSET, 128
        sub             %%LENGTH, 128
        jz              %%_encrypt_done

        cmp             WORD(%%CTR_CHECK), (256 - 8)
        jae             %%_encrypt_by_8

        vpaddd          %%CTR_BLOCKz, %%ADD8BE
        vpaddd          %%CTR_BLOCK2z, %%ADD8BE

        cmp             %%LENGTH, 128
        jl              %%_encrypt_by_8_partial

        jmp             %%_encrypt_by_8_new

%%_encrypt_by_8:
        vpshufb         %%CTR_BLOCKz,  %%SHUF_MASK
        vpshufb         %%CTR_BLOCK2z, %%SHUF_MASK
        vpaddd          %%CTR_BLOCKz,  %%ADD8LE
        vpaddd          %%CTR_BLOCK2z, %%ADD8LE
        vpshufb         %%CTR_BLOCKz,  %%SHUF_MASK
        vpshufb         %%CTR_BLOCK2z, %%SHUF_MASK

        cmp             %%LENGTH, 128
        jge             %%_encrypt_by_8_new

%%_encrypt_by_8_partial:
        ;; Test to see if we need a by 8 with partial block. At this point
        ;; bytes remaining should be either zero or between 113-127.
        ;; 'in_order' shuffle needed to align key for partial block xor.
        ;; 'out_order' is a little faster because it avoids extra shuffles.
        ;;  - counter blocks for the next 8 blocks are prepared and in BE format
        ;;  - we can go ahead with out_order scenario

        vmovdqu64       %%GH4KEY, [%%HASHK_PTR + (4 * 16)]
        vmovdqu64       %%GH8KEY, [%%HASHK_PTR + (0 * 16)]

        GHASH_8_ENCRYPT_8_PARALLEL  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, \
                %%DATA_OFFSET, %%CTR_BLOCKz, %%CTR_BLOCK2z, \
                %%BLK0, %%BLK1, %%AES_PARTIAL_BLOCK, \
                out_order, %%ENC_DEC, partial, %%IA0, %%IA1, %%LENGTH, %%INSTANCE_TYPE, \
                %%GH4KEY, %%GH8KEY, %%SHUF_MASK, \
                %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, %%ZTMP5, %%ZTMP6, \
                %%ZTMP7, %%ZTMP8, %%ZTMP9, %%ZTMP10, %%ZTMP11, %%ZTMP12, \
                %%MASKREG, no_reduction, %%GL, %%GH, %%GM

        add             %%HASHK_PTR, (8 * 16)
        add             %%DATA_OFFSET, (128 - 16)
        sub             %%LENGTH, (128 - 16)

%ifidn %%INSTANCE_TYPE, multi_call
        mov             [%%GDATA_CTX + PBlockLen], %%LENGTH
        vmovdqu64       [%%GDATA_CTX + PBlockEncKey], %%AES_PARTIAL_BLOCK
%endif

%%_encrypt_done:
        ;; Extract the last counter block in LE format
        vextracti32x4   XWORD(%%CTR_BLOCK_SAVE), %%CTR_BLOCK2z, 3
        vpshufb         XWORD(%%CTR_BLOCK_SAVE), XWORD(%%SHUF_MASK)

        ;; GHASH last cipher text blocks in xmm1-xmm8
        ;; - if block 8th is partial in a multi-call path then skip the block
%ifidn %%INSTANCE_TYPE, multi_call
        cmp             qword [%%GDATA_CTX + PBlockLen], 0
        jz              %%_hash_last_8

        ;; save the 8th partial block as GHASH_LAST_7 will clobber %%BLK1
        vextracti32x4   XWORD(%%ZTMP7), %%BLK1, 3

        GHASH_LAST_7 %%GDATA_KEY, %%BLK1, %%BLK0, \
                %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, %%ZTMP5, %%ZTMP6, \
                %%AAD_HASHx, %%MASKREG, %%IA0, %%GH, %%GL, %%GM

        ;; XOR the partial word into the hash
        vpxorq          %%AAD_HASHx, %%AAD_HASHx, XWORD(%%ZTMP7)
        jmp             %%_ghash_done
%%_hash_last_8:
%endif
        GHASH_LAST_8 %%GDATA_KEY, %%BLK1, %%BLK0, \
                %%ZTMP0, %%ZTMP1, %%ZTMP2, %%ZTMP3, %%ZTMP4, %%ZTMP5, %%AAD_HASHx, \
                %%GH, %%GL, %%GM
%%_ghash_done:
        vmovdqu64       [%%GDATA_CTX + CurCount], XWORD(%%CTR_BLOCK_SAVE)
        vmovdqu64       [%%GDATA_CTX + AadHash], %%AAD_HASHx
%%_enc_dec_done:

%endmacro                       ; GCM_ENC_DEC

;;; ===========================================================================
;;; ===========================================================================
;;; Encrypt/decrypt the initial 16 blocks
%macro INITIAL_BLOCKS_16 22
%define %%IN            %1      ; [in] input buffer
%define %%OUT           %2      ; [in] output buffer
%define %%KP            %3      ; [in] pointer to expanded keys
%define %%DATA_OFFSET   %4      ; [in] data offset
%define %%GHASH         %5      ; [in] ZMM with AAD (low 128 bits)
%define %%CTR           %6      ; [in] ZMM with CTR BE blocks 4x128 bits
%define %%CTR_CHECK     %7      ; [in/out] GPR with counter overflow check
%define %%ADDBE_4x4     %8      ; [in] ZMM 4x128bits with value 4 (big endian)
%define %%ADDBE_1234    %9      ; [in] ZMM 4x128bits with values 1, 2, 3 & 4 (big endian)
%define %%T0            %10     ; [clobered] temporary ZMM register
%define %%T1            %11     ; [clobered] temporary ZMM register
%define %%T2            %12     ; [clobered] temporary ZMM register
%define %%T3            %13     ; [clobered] temporary ZMM register
%define %%T4            %14     ; [clobered] temporary ZMM register
%define %%T5            %15     ; [clobered] temporary ZMM register
%define %%T6            %16     ; [clobered] temporary ZMM register
%define %%T7            %17     ; [clobered] temporary ZMM register
%define %%T8            %18     ; [clobered] temporary ZMM register
%define %%SHUF_MASK     %19     ; [in] ZMM with BE/LE shuffle mask
%define %%ENC_DEC       %20     ; [in] ENC (encrypt) or DEC (decrypt) selector
%define %%BLK_OFFSET    %21     ; [in] stack frame offset to ciphered blocks
%define %%DATA_DISPL    %22     ; [in] fixed numerical data displacement/offset

%define %%B00_03        %%T5
%define %%B04_07        %%T6
%define %%B08_11        %%T7
%define %%B12_15        %%T8

%assign stack_offset (%%BLK_OFFSET)

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; prepare counter blocks

        cmp             BYTE(%%CTR_CHECK), (256 - 16)
        jae             %%_next_16_overflow
        vpaddd          %%B00_03, %%CTR, %%ADDBE_1234
        vpaddd          %%B04_07, %%B00_03, %%ADDBE_4x4
        vpaddd          %%B08_11, %%B04_07, %%ADDBE_4x4
        vpaddd          %%B12_15, %%B08_11, %%ADDBE_4x4
        jmp             %%_next_16_ok
%%_next_16_overflow:
        vpshufb         %%CTR, %%CTR, %%SHUF_MASK
        vmovdqa64       %%B12_15, [rel ddq_add_4444]
        vpaddd          %%B00_03, %%CTR, [rel ddq_add_1234]
        vpaddd          %%B04_07, %%B00_03, %%B12_15
        vpaddd          %%B08_11, %%B04_07, %%B12_15
        vpaddd          %%B12_15, %%B08_11, %%B12_15
        vpshufb         %%B00_03, %%SHUF_MASK
        vpshufb         %%B04_07, %%SHUF_MASK
        vpshufb         %%B08_11, %%SHUF_MASK
        vpshufb         %%B12_15, %%SHUF_MASK
%%_next_16_ok:
        vshufi64x2      %%CTR, %%B12_15, %%B12_15, 1111_1111b
        add             BYTE(%%CTR_CHECK), 16

        ;; === load 16 blocks of data
        VX512LDR        %%T0, [%%IN + %%DATA_OFFSET + %%DATA_DISPL + (64*0)]
        VX512LDR        %%T1, [%%IN + %%DATA_OFFSET + %%DATA_DISPL + (64*1)]
        VX512LDR        %%T2, [%%IN + %%DATA_OFFSET + %%DATA_DISPL + (64*2)]
        VX512LDR        %%T3, [%%IN + %%DATA_OFFSET + %%DATA_DISPL + (64*3)]

        ;; move to AES encryption rounds
%assign i 0
        vbroadcastf64x2 %%T4, [%%KP + (16*i)]
        vpxorq          %%B00_03, %%B00_03, %%T4
        vpxorq          %%B04_07, %%B04_07, %%T4
        vpxorq          %%B08_11, %%B08_11, %%T4
        vpxorq          %%B12_15, %%B12_15, %%T4
%assign i (i + 1)

%rep NROUNDS
        vbroadcastf64x2 %%T4, [%%KP + (16*i)]
        vaesenc         %%B00_03, %%B00_03, %%T4
        vaesenc         %%B04_07, %%B04_07, %%T4
        vaesenc         %%B08_11, %%B08_11, %%T4
        vaesenc         %%B12_15, %%B12_15, %%T4
%assign i (i + 1)
%endrep

        vbroadcastf64x2 %%T4, [%%KP + (16*i)]
        vaesenclast     %%B00_03, %%B00_03, %%T4
        vaesenclast     %%B04_07, %%B04_07, %%T4
        vaesenclast     %%B08_11, %%B08_11, %%T4
        vaesenclast     %%B12_15, %%B12_15, %%T4

        ;;  xor against text
        vpxorq          %%B00_03, %%B00_03, %%T0
        vpxorq          %%B04_07, %%B04_07, %%T1
        vpxorq          %%B08_11, %%B08_11, %%T2
        vpxorq          %%B12_15, %%B12_15, %%T3

        ;; store
        VX512STR        [%%OUT + %%DATA_OFFSET + %%DATA_DISPL + (64*0)], %%B00_03
        VX512STR        [%%OUT + %%DATA_OFFSET + %%DATA_DISPL + (64*1)], %%B04_07
        VX512STR        [%%OUT + %%DATA_OFFSET + %%DATA_DISPL + (64*2)], %%B08_11
        VX512STR        [%%OUT + %%DATA_OFFSET + %%DATA_DISPL + (64*3)], %%B12_15

%ifidn  %%ENC_DEC, DEC
        ;; decryption - cipher text needs to go to GHASH phase
        vpshufb         %%B00_03, %%T0, %%SHUF_MASK
        vpshufb         %%B04_07, %%T1, %%SHUF_MASK
        vpshufb         %%B08_11, %%T2, %%SHUF_MASK
        vpshufb         %%B12_15, %%T3, %%SHUF_MASK
%else
        ;; encryption
        vpshufb         %%B00_03, %%B00_03, %%SHUF_MASK
        vpshufb         %%B04_07, %%B04_07, %%SHUF_MASK
        vpshufb         %%B08_11, %%B08_11, %%SHUF_MASK
        vpshufb         %%B12_15, %%B12_15, %%SHUF_MASK
%endif

%ifnidn %%GHASH, no_ghash
        ;; === xor cipher block 0 with GHASH for the next GHASH round
        vpxorq          %%B00_03, %%B00_03, %%GHASH
%endif

        vmovdqa64       [rsp + stack_offset + (0 * 64)], %%B00_03
        vmovdqa64       [rsp + stack_offset + (1 * 64)], %%B04_07
        vmovdqa64       [rsp + stack_offset + (2 * 64)], %%B08_11
        vmovdqa64       [rsp + stack_offset + (3 * 64)], %%B12_15
%endmacro                       ;INITIAL_BLOCKS_16

;;; ===========================================================================
;;; ===========================================================================
;;; Encrypt the initial N x 16 blocks
;;; - A x 16 blocks are encrypted/decrypted first (pipeline depth)
;;; - B x 16 blocks are encrypted/decrypted and previous A x 16 are ghashed
;;; - A + B = N
%macro INITIAL_BLOCKS_Nx16 39
%define %%IN            %1      ; [in] input buffer
%define %%OUT           %2      ; [in] output buffer
%define %%KP            %3      ; [in] pointer to expanded keys
%define %%DATA_OFFSET   %4      ; [in/out] data offset
%define %%GHASH         %5      ; [in] ZMM with AAD (low 128 bits)
%define %%CTR           %6      ; [in/out] ZMM with CTR: in - LE & 128b; out - BE & 4x128b
%define %%CTR_CHECK     %7      ; [in/out] GPR with counter overflow check
%define %%T0            %8      ; [clobered] temporary ZMM register
%define %%T1            %9      ; [clobered] temporary ZMM register
%define %%T2            %10     ; [clobered] temporary ZMM register
%define %%T3            %11     ; [clobered] temporary ZMM register
%define %%T4            %12     ; [clobered] temporary ZMM register
%define %%T5            %13     ; [clobered] temporary ZMM register
%define %%T6            %14     ; [clobered] temporary ZMM register
%define %%T7            %15     ; [clobered] temporary ZMM register
%define %%T8            %16     ; [clobered] temporary ZMM register
%define %%T9            %17     ; [clobered] temporary ZMM register
%define %%T10           %18     ; [clobered] temporary ZMM register
%define %%T11           %19     ; [clobered] temporary ZMM register
%define %%T12           %20     ; [clobered] temporary ZMM register
%define %%T13           %21     ; [clobered] temporary ZMM register
%define %%T14           %22     ; [clobered] temporary ZMM register
%define %%T15           %23     ; [clobered] temporary ZMM register
%define %%T16           %24     ; [clobered] temporary ZMM register
%define %%T17           %25     ; [clobered] temporary ZMM register
%define %%T18           %26     ; [clobered] temporary ZMM register
%define %%T19           %27     ; [clobered] temporary ZMM register
%define %%T20           %28     ; [clobered] temporary ZMM register
%define %%T21           %29     ; [clobered] temporary ZMM register
%define %%T22           %30     ; [clobered] temporary ZMM register
%define %%GH            %31     ; [out] ZMM ghash sum (high)
%define %%GL            %32     ; [out] ZMM ghash sum (low)
%define %%GM            %33     ; [out] ZMM ghash sum (middle)
%define %%ADDBE_4x4     %34     ; [in] ZMM 4x128bits with value 4 (big endian)
%define %%ADDBE_1234    %35     ; [in] ZMM 4x128bits with values 1, 2, 3 & 4 (big endian)
%define %%SHUF_MASK     %36     ; [in] ZMM with BE/LE shuffle mask
%define %%ENC_DEC       %37     ; [in] ENC (encrypt) or DEC (decrypt) selector
%define %%NBLOCKS       %38     ; [in] number of blocks: multiple of 16
%define %%DEPTH_BLK     %39     ; [in] pipline depth, number of blocks (mulitple of 16)

%assign aesout_offset (STACK_LOCAL_OFFSET + (0 * 16))
%assign ghashin_offset (STACK_LOCAL_OFFSET + (0 * 16))
%assign hkey_offset HashKey_ %+ %%NBLOCKS
%assign data_in_out_offset 0

        ;; set up CTR_CHECK
        vmovd           DWORD(%%CTR_CHECK), XWORD(%%CTR)
        and             DWORD(%%CTR_CHECK), 255

        ;; in LE format after init, convert to BE
        vshufi64x2      %%CTR, %%CTR, %%CTR, 0
        vpshufb         %%CTR, %%CTR, %%SHUF_MASK

        ;; ==== AES lead in

        ;; first 16 blocks - just cipher
        INITIAL_BLOCKS_16       %%IN, %%OUT, %%KP, %%DATA_OFFSET, \
                                %%GHASH, %%CTR, %%CTR_CHECK, %%ADDBE_4x4, %%ADDBE_1234, \
                                %%T0, %%T1, %%T2, %%T3, %%T4, \
                                %%T5, %%T6, %%T7, %%T8, \
                                %%SHUF_MASK, %%ENC_DEC, aesout_offset, data_in_out_offset

%assign aesout_offset (aesout_offset + (16 * 16))
%assign data_in_out_offset (data_in_out_offset + (16 * 16))

%if (%%DEPTH_BLK > 16)
%rep ((%%DEPTH_BLK - 16) / 16)
        INITIAL_BLOCKS_16       %%IN, %%OUT, %%KP, %%DATA_OFFSET, \
                                no_ghash, %%CTR, %%CTR_CHECK, %%ADDBE_4x4, %%ADDBE_1234, \
                                %%T0, %%T1, %%T2, %%T3, %%T4, \
                                %%T5, %%T6, %%T7, %%T8, \
                                %%SHUF_MASK, %%ENC_DEC, aesout_offset, data_in_out_offset
%assign aesout_offset (aesout_offset + (16 * 16))
%assign data_in_out_offset (data_in_out_offset + (16 * 16))
%endrep
%endif

        ;; ==== GHASH + AES follows

        ;; first 16 blocks stitched
        GHASH_16_ENCRYPT_16_PARALLEL  %%KP, %%OUT, %%IN, %%DATA_OFFSET, \
                %%CTR, %%CTR_CHECK, \
                hkey_offset, aesout_offset, ghashin_offset, %%SHUF_MASK, \
                %%T0,  %%T1,  %%T2,  %%T3, \
                %%T4,  %%T5,  %%T6,  %%T7, \
                %%T8,  %%T9,  %%T10, %%T11,\
                %%T12, %%T13, %%T14, %%T15,\
                %%T16, %%T17, %%T18, %%T19, \
                %%T20, %%T21, %%T22, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%GL, %%GH, %%GM, \
                first_time, %%ENC_DEC, data_in_out_offset, no_ghash_in

%if ((%%NBLOCKS - %%DEPTH_BLK) > 16)
%rep ((%%NBLOCKS - %%DEPTH_BLK - 16) / 16)
%assign ghashin_offset (ghashin_offset + (16 * 16))
%assign hkey_offset (hkey_offset + (16 * 16))
%assign aesout_offset (aesout_offset + (16 * 16))
%assign data_in_out_offset (data_in_out_offset + (16 * 16))

        ;; mid 16 blocks - stitched
        GHASH_16_ENCRYPT_16_PARALLEL  %%KP, %%OUT, %%IN, %%DATA_OFFSET, \
                %%CTR, %%CTR_CHECK, \
                hkey_offset, aesout_offset, ghashin_offset, %%SHUF_MASK, \
                %%T0,  %%T1,  %%T2,  %%T3, \
                %%T4,  %%T5,  %%T6,  %%T7, \
                %%T8,  %%T9,  %%T10, %%T11,\
                %%T12, %%T13, %%T14, %%T15,\
                %%T16, %%T17, %%T18, %%T19, \
                %%T20, %%T21, %%T22, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%GL, %%GH, %%GM, \
                no_reduction, %%ENC_DEC, data_in_out_offset, no_ghash_in
%endrep
%endif
        add             %%DATA_OFFSET, (%%NBLOCKS * 16)

%endmacro                       ;INITIAL_BLOCKS_Nx16

;;; ===========================================================================
;;; ===========================================================================
;;; GHASH the last 16 blocks of cipher text (last part of by 32/64/128 code)
%macro  GHASH_LAST_Nx16 23
%define %%KP            %1      ; [in] pointer to expanded keys
%define %%GHASH         %2      ; [out] ghash output
%define %%T1            %3      ; [clobbered] temporary ZMM
%define %%T2            %4      ; [clobbered] temporary ZMM
%define %%T3            %5      ; [clobbered] temporary ZMM
%define %%T4            %6      ; [clobbered] temporary ZMM
%define %%T5            %7      ; [clobbered] temporary ZMM
%define %%T6            %8      ; [clobbered] temporary ZMM
%define %%T7            %9      ; [clobbered] temporary ZMM
%define %%T8            %10     ; [clobbered] temporary ZMM
%define %%T9            %11     ; [clobbered] temporary ZMM
%define %%T10           %12     ; [clobbered] temporary ZMM
%define %%T11           %13     ; [clobbered] temporary ZMM
%define %%T12           %14     ; [clobbered] temporary ZMM
%define %%T13           %15     ; [clobbered] temporary ZMM
%define %%T14           %16     ; [clobbered] temporary ZMM
%define %%T15           %17     ; [clobbered] temporary ZMM
%define %%T16           %18     ; [clobbered] temporary ZMM
%define %%GH            %19     ; [in/cloberred] ghash sum (high)
%define %%GL            %20     ; [in/cloberred] ghash sum (low)
%define %%GM            %21     ; [in/cloberred] ghash sum (medium)
%define %%LOOP_BLK      %22     ; [in] numerical number of blocks handled by the loop
%define %%DEPTH_BLK     %23     ; [in] numerical number, pipeline depth (ghash vs aes)

%define %%T0H           %%T1
%define %%T0L           %%T2
%define %%T0M1          %%T3
%define %%T0M2          %%T4

%define %%T1H           %%T5
%define %%T1L           %%T6
%define %%T1M1          %%T7
%define %%T1M2          %%T8

%define %%T2H           %%T9
%define %%T2L           %%T10
%define %%T2M1          %%T11
%define %%T2M2          %%T12

%define %%BLK1          %%T13
%define %%BLK2          %%T14

%define %%HK1           %%T15
%define %%HK2           %%T16

%assign hashk      HashKey_ %+ %%DEPTH_BLK
%assign cipher_blk (STACK_LOCAL_OFFSET + ((%%LOOP_BLK - %%DEPTH_BLK) * 16))

        ;; load cipher blocks and ghash keys
        vmovdqa64       %%BLK1, [rsp + cipher_blk]
        vmovdqa64       %%BLK2, [rsp + cipher_blk + 64]
        vmovdqu64       %%HK1, [%%KP + hashk]
        vmovdqu64       %%HK2, [%%KP + hashk + 64]
        ;; ghash blocks 0-3
        vpclmulqdq      %%T0H, %%BLK1, %%HK1, 0x11      ; %%TH = a1*b1
        vpclmulqdq      %%T0L, %%BLK1, %%HK1, 0x00      ; %%TL = a0*b0
        vpclmulqdq      %%T0M1, %%BLK1, %%HK1, 0x01     ; %%TM1 = a1*b0
        vpclmulqdq      %%T0M2, %%BLK1, %%HK1, 0x10     ; %%TM2 = a0*b1
        ;; ghash blocks 4-7
        vpclmulqdq      %%T1H, %%BLK2, %%HK2, 0x11      ; %%TTH = a1*b1
        vpclmulqdq      %%T1L, %%BLK2, %%HK2, 0x00      ; %%TTL = a0*b0
        vpclmulqdq      %%T1M1, %%BLK2, %%HK2, 0x01     ; %%TTM1 = a1*b0
        vpclmulqdq      %%T1M2, %%BLK2, %%HK2, 0x10     ; %%TTM2 = a0*b1
        vpternlogq      %%T0H, %%T1H, %%GH, 0x96        ; T0H = T0H + T1H + GH
        vpternlogq      %%T0L, %%T1L, %%GL, 0x96        ; T0L = T0L + T1L + GL
        vpternlogq      %%T0M1, %%T1M1, %%GM, 0x96      ; T0M1 = T0M1 + T1M1 + GM
        vpxorq          %%T0M2, %%T0M2, %%T1M2          ; T0M2 = T0M2 + T1M2

%rep ((%%DEPTH_BLK - 8) / 8)
%assign hashk      (hashk + 128)
%assign cipher_blk (cipher_blk + 128)

        ;; remaining blocks
        ;; load next 8 cipher blocks and corresponding ghash keys
        vmovdqa64       %%BLK1, [rsp + cipher_blk]
        vmovdqa64       %%BLK2, [rsp + cipher_blk + 64]
        vmovdqu64       %%HK1, [%%KP + hashk]
        vmovdqu64       %%HK2, [%%KP + hashk + 64]
        ;; ghash blocks 0-3
        vpclmulqdq      %%T1H, %%BLK1, %%HK1, 0x11      ; %%TH = a1*b1
        vpclmulqdq      %%T1L, %%BLK1, %%HK1, 0x00      ; %%TL = a0*b0
        vpclmulqdq      %%T1M1, %%BLK1, %%HK1, 0x01     ; %%TM1 = a1*b0
        vpclmulqdq      %%T1M2, %%BLK1, %%HK1, 0x10     ; %%TM2 = a0*b1
        ;; ghash blocks 4-7
        vpclmulqdq      %%T2H, %%BLK2, %%HK2, 0x11      ; %%TTH = a1*b1
        vpclmulqdq      %%T2L, %%BLK2, %%HK2, 0x00      ; %%TTL = a0*b0
        vpclmulqdq      %%T2M1, %%BLK2, %%HK2, 0x01     ; %%TTM1 = a1*b0
        vpclmulqdq      %%T2M2, %%BLK2, %%HK2, 0x10     ; %%TTM2 = a0*b1
        ;; update sums
        vpternlogq      %%T0H, %%T1H, %%T2H, 0x96       ; TH = T0H + T1H + T2H
        vpternlogq      %%T0L, %%T1L, %%T2L, 0x96       ; TL = T0L + T1L + T2L
        vpternlogq      %%T0M1, %%T1M1, %%T2M1, 0x96    ; TM1 = T0M1 + T1M1 xor T2M1
        vpternlogq      %%T0M2, %%T1M2, %%T2M2, 0x96    ; TM2 = T0M2 + T1M1 xor T2M2
%endrep

        ;; integrate TM into TH and TL
        vpxorq          %%T0M1, %%T0M1, %%T0M2
        vpsrldq         %%T1M1, %%T0M1, 8
        vpslldq         %%T1M2, %%T0M1, 8
        vpxorq          %%T0H, %%T0H, %%T1M1
        vpxorq          %%T0L, %%T0L, %%T1M2

        ;; add TH and TL 128-bit words horizontally
        VHPXORI4x128    %%T0H, %%T2M1
        VHPXORI4x128    %%T0L, %%T2M2

        ;; reduction
        vmovdqa64       %%HK1, [rel POLY2]
        VCLMUL_REDUCE   %%GHASH, %%HK1, %%T0H, %%T0L, %%T0M1, %%T0M2
%endmacro

;;; ===========================================================================
;;; ===========================================================================
;;; Encrypt & ghash multiples of 16 blocks

%macro GHASH_ENCRYPT_Nx16_PARALLEL 39
%define %%IN                    %1      ; [in] input buffer
%define %%OUT                   %2      ; [in] output buffer
%define %%GDATA_KEY             %3      ; [in] pointer to expanded keys
%define %%DATA_OFFSET           %4      ; [in/out] data offset
%define %%CTR_BE                %5      ; [in/out] ZMM last counter block
%define %%SHFMSK                %6      ; [in] ZMM with byte swap mask for pshufb
%define %%ZT0                   %7      ; [clobered] temporary ZMM register
%define %%ZT1                   %8      ; [clobered] temporary ZMM register
%define %%ZT2                   %9      ; [clobered] temporary ZMM register
%define %%ZT3                   %10     ; [clobered] temporary ZMM register
%define %%ZT4                   %11     ; [clobered] temporary ZMM register
%define %%ZT5                   %12     ; [clobered] temporary ZMM register
%define %%ZT6                   %13     ; [clobered] temporary ZMM register
%define %%ZT7                   %14     ; [clobered] temporary ZMM register
%define %%ZT8                   %15     ; [clobered] temporary ZMM register
%define %%ZT9                   %16     ; [clobered] temporary ZMM register
%define %%ZT10                  %17     ; [clobered] temporary ZMM register
%define %%ZT11                  %18     ; [clobered] temporary ZMM register
%define %%ZT12                  %19     ; [clobered] temporary ZMM register
%define %%ZT13                  %20     ; [clobered] temporary ZMM register
%define %%ZT14                  %21     ; [clobered] temporary ZMM register
%define %%ZT15                  %22     ; [clobered] temporary ZMM register
%define %%ZT16                  %23     ; [clobered] temporary ZMM register
%define %%ZT17                  %24     ; [clobered] temporary ZMM register
%define %%ZT18                  %25     ; [clobered] temporary ZMM register
%define %%ZT19                  %26     ; [clobered] temporary ZMM register
%define %%ZT20                  %27     ; [clobered] temporary ZMM register
%define %%ZT21                  %28     ; [clobered] temporary ZMM register
%define %%ZT22                  %29     ; [clobered] temporary ZMM register
%define %%GTH                   %30     ; [in/out] ZMM GHASH sum (high)
%define %%GTL                   %31     ; [in/out] ZMM GHASH sum (low)
%define %%GTM                   %32     ; [in/out] ZMM GHASH sum (medium)
%define %%ADDBE_4x4             %33     ; [in] ZMM 4x128bits with value 4 (big endian)
%define %%ADDBE_1234            %34     ; [in] ZMM 4x128bits with values 1, 2, 3 & 4 (big endian)
%define %%GHASH                 %35     ; [clobbered] ZMM with intermidiate GHASH value
%define %%ENC_DEC               %36     ; [in] ENC (encrypt) or DEC (decrypt) selector
%define %%NUM_BLOCKS            %37     ; [in] number of blocks to process in the loop
%define %%DEPTH_BLK             %38     ; [in] pipeline depth in blocks
%define %%CTR_CHECK             %39     ; [in/out] counter to check byte overflow

%assign aesout_offset (STACK_LOCAL_OFFSET + (0 * 16))
%assign ghashin_offset (STACK_LOCAL_OFFSET + ((%%NUM_BLOCKS - %%DEPTH_BLK) * 16))
%assign hkey_offset  HashKey_ %+ %%DEPTH_BLK
%assign data_in_out_offset 0

        ;; mid 16 blocks
%if (%%DEPTH_BLK > 16)
%rep ((%%DEPTH_BLK - 16) / 16)
        GHASH_16_ENCRYPT_16_PARALLEL  %%GDATA_KEY, %%OUT, %%IN, %%DATA_OFFSET, \
                %%CTR_BE, %%CTR_CHECK, \
                hkey_offset, aesout_offset, ghashin_offset, %%SHFMSK, \
                %%ZT0,  %%ZT1,  %%ZT2,  %%ZT3, \
                %%ZT4,  %%ZT5,  %%ZT6,  %%ZT7, \
                %%ZT8,  %%ZT9,  %%ZT10, %%ZT11,\
                %%ZT12, %%ZT13, %%ZT14, %%ZT15,\
                %%ZT16, %%ZT17, %%ZT18, %%ZT19, \
                %%ZT20, %%ZT21, %%ZT22, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%GTL, %%GTH, %%GTM, \
                no_reduction, %%ENC_DEC, data_in_out_offset, no_ghash_in

%assign aesout_offset (aesout_offset + (16 * 16))
%assign ghashin_offset (ghashin_offset + (16 * 16))
%assign hkey_offset (hkey_offset + (16 * 16))
%assign data_in_out_offset (data_in_out_offset + (16 * 16))
%endrep
%endif

        ;; 16 blocks with reduction
        GHASH_16_ENCRYPT_16_PARALLEL  %%GDATA_KEY, %%OUT, %%IN, %%DATA_OFFSET, \
                %%CTR_BE, %%CTR_CHECK, \
                HashKey_16, aesout_offset, ghashin_offset, %%SHFMSK, \
                %%ZT0,  %%ZT1,  %%ZT2,  %%ZT3, \
                %%ZT4,  %%ZT5,  %%ZT6,  %%ZT7, \
                %%ZT8,  %%ZT9,  %%ZT10, %%ZT11,\
                %%ZT12, %%ZT13, %%ZT14, %%ZT15,\
                %%ZT16, %%ZT17, %%ZT18, %%ZT19, \
                %%ZT20, %%ZT21, %%ZT22, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%GTL, %%GTH, %%GTM, \
                final_reduction, %%ENC_DEC, data_in_out_offset, no_ghash_in

%assign aesout_offset (aesout_offset + (16 * 16))
%assign data_in_out_offset (data_in_out_offset + (16 * 16))
%assign ghashin_offset (STACK_LOCAL_OFFSET + (0 * 16))
%assign hkey_offset HashKey_ %+ %%NUM_BLOCKS

        ;; === xor cipher block 0 with GHASH (ZT4)
        vmovdqa64        %%GHASH, %%ZT4

        ;; start the pipeline again
        GHASH_16_ENCRYPT_16_PARALLEL  %%GDATA_KEY, %%OUT, %%IN, %%DATA_OFFSET, \
                %%CTR_BE, %%CTR_CHECK, \
                hkey_offset, aesout_offset, ghashin_offset, %%SHFMSK, \
                %%ZT0,  %%ZT1,  %%ZT2,  %%ZT3, \
                %%ZT4,  %%ZT5,  %%ZT6,  %%ZT7, \
                %%ZT8,  %%ZT9,  %%ZT10, %%ZT11,\
                %%ZT12, %%ZT13, %%ZT14, %%ZT15,\
                %%ZT16, %%ZT17, %%ZT18, %%ZT19, \
                %%ZT20, %%ZT21, %%ZT22, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%GTL, %%GTH, %%GTM, \
                first_time, %%ENC_DEC, data_in_out_offset, %%GHASH

%if ((%%NUM_BLOCKS - %%DEPTH_BLK) > 16)
%rep ((%%NUM_BLOCKS - %%DEPTH_BLK - 16 ) / 16)

%assign aesout_offset (aesout_offset + (16 * 16))
%assign data_in_out_offset (data_in_out_offset + (16 * 16))
%assign ghashin_offset (ghashin_offset + (16 * 16))
%assign hkey_offset (hkey_offset + (16 * 16))

        GHASH_16_ENCRYPT_16_PARALLEL  %%GDATA_KEY, %%OUT, %%IN, %%DATA_OFFSET, \
                %%CTR_BE, %%CTR_CHECK, \
                hkey_offset, aesout_offset, ghashin_offset, %%SHFMSK, \
                %%ZT0,  %%ZT1,  %%ZT2,  %%ZT3, \
                %%ZT4,  %%ZT5,  %%ZT6,  %%ZT7, \
                %%ZT8,  %%ZT9,  %%ZT10, %%ZT11,\
                %%ZT12, %%ZT13, %%ZT14, %%ZT15,\
                %%ZT16, %%ZT17, %%ZT18, %%ZT19, \
                %%ZT20, %%ZT21, %%ZT22, \
                %%ADDBE_4x4, %%ADDBE_1234, \
                %%GTL, %%GTH, %%GTM, \
                no_reduction, %%ENC_DEC, data_in_out_offset, no_ghash_in
%endrep
%endif

        add     %%DATA_OFFSET, (%%NUM_BLOCKS * 16)

%endmacro                       ;GHASH_ENCRYPT_Nx16_PARALLEL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_UPDATE_AAD_HASH update AadHash value in gcm_context_data struct to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Additional Authentication data (A_IN), Additional Data length (A_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (AadHash).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_UPDATE_AAD_HASH        19
%define %%GDATA_KEY                %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX                %2      ; [in] GCM context pointer
%define %%A_IN                     %3      ; [in] AAD pointer
%define %%A_LEN                    %4      ; [in] AAD processing length in bytes
%define %%GPR1                     %5      ; [clobbered] GP register
%define %%GPR2                     %6      ; [clobbered] GP register
%define %%GPR3                     %7      ; [clobbered] GP register
%define %%MASKREG                  %8      ; [clobbered] mask register
%define %%AAD_HASH                 %9      ; [out] XMM for AAD_HASH value (xmm14)
%define %%ZT0                      %10     ; [clobbered] ZMM register
%define %%ZT1                      %11     ; [clobbered] ZMM register
%define %%ZT2                      %12     ; [clobbered] ZMM register
%define %%ZT3                      %13     ; [clobbered] ZMM register
%define %%ZT4                      %14     ; [clobbered] ZMM register
%define %%ZT5                      %15     ; [clobbered] ZMM register
%define %%ZT6                      %16     ; [clobbered] ZMM register
%define %%ZT7                      %17     ; [clobbered] ZMM register
%define %%ZT8                      %18     ; [clobbered] ZMM register
%define %%ZT9                      %19     ; [clobbered] ZMM register

        vmovdqu64       %%AAD_HASH, [%%GDATA_CTX + AadHash]   ; load current hash

        UPDATE_HASH     %%A_IN, %%A_LEN, %%AAD_HASH, %%GDATA_KEY, \
                        %%ZT0, %%ZT1, %%ZT2, %%ZT3, %%ZT4, %%ZT5, %%ZT6, %%ZT7, %%ZT8, %%ZT9, \
                        %%GPR1, %%GPR2, %%GPR3, %%MASKREG

        vmovdqu64       [%%GDATA_CTX + AadHash], %%AAD_HASH   ; store updated hash

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_UPDATE_IV_HASH update orig_IV value in gcm_context_data struct to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Initialization Vector (IV), Initialization Vector length (IV_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (orig_IV).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_UPDATE_IV_HASH         19
%define %%GDATA_KEY                %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX                %2      ; [in] GCM context pointer
%define %%IV                       %3      ; [in] IV pointer
%define %%IV_LEN                   %4      ; [in] IV processing length in bytes
%define %%GPR1                     %5      ; [clobbered] GP register
%define %%GPR2                     %6      ; [clobbered] GP register
%define %%GPR3                     %7      ; [clobbered] GP register
%define %%MASKREG                  %8      ; [clobbered] mask register
%define %%IV_HASH                  %9      ; [out] XMM for IV_HASH value (xmm14)
%define %%ZT0                      %10     ; [clobbered] ZMM register
%define %%ZT1                      %11     ; [clobbered] ZMM register
%define %%ZT2                      %12     ; [clobbered] ZMM register
%define %%ZT3                      %13     ; [clobbered] ZMM register
%define %%ZT4                      %14     ; [clobbered] ZMM register
%define %%ZT5                      %15     ; [clobbered] ZMM register
%define %%ZT6                      %16     ; [clobbered] ZMM register
%define %%ZT7                      %17     ; [clobbered] ZMM register
%define %%ZT8                      %18     ; [clobbered] ZMM register
%define %%ZT9                      %19     ; [clobbered] ZMM register

        vmovdqu64       %%IV_HASH, [%%GDATA_CTX + OrigIV]   ; load current hash

        UPDATE_HASH     %%IV, %%IV_LEN, %%IV_HASH, %%GDATA_KEY, \
                        %%ZT0, %%ZT1, %%ZT2, %%ZT3, %%ZT4, %%ZT5, %%ZT6, %%ZT7, %%ZT8, %%ZT9, \
                        %%GPR1, %%GPR2, %%GPR3, %%MASKREG

        vmovdqu64       [%%GDATA_CTX + OrigIV], %%IV_HASH   ; store updated hash

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_FINALIZE_AAD_HASH finalize updating of AadHash value in gcm_context_data struct 
;;; to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Additional Authentication data (A_IN), Additional Data length (A_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (AadHash) and initialized other parts of GDATA_CTX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_FINALIZE_AAD_HASH      20
%define %%GDATA_KEY                %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX                %2      ; [in] GCM context pointer
%define %%A_IN                     %3      ; [in] AAD pointer
%define %%A_LEN                    %4      ; [in] AAD processing length in bytes
%define %%GPR1                     %5      ; [clobbered] GP register
%define %%GPR2                     %6      ; [clobbered] GP register
%define %%GPR3                     %7      ; [clobbered] GP register
%define %%MASKREG                  %8      ; [clobbered] mask register
%define %%AAD_HASH                 %9      ; [out] XMM for AAD_HASH value (xmm14)
%define %%ZT0                      %10     ; [clobbered] ZMM register
%define %%ZT1                      %11     ; [clobbered] ZMM register
%define %%ZT2                      %12     ; [clobbered] ZMM register
%define %%ZT3                      %13     ; [clobbered] ZMM register
%define %%ZT4                      %14     ; [clobbered] ZMM register
%define %%ZT5                      %15     ; [clobbered] ZMM register
%define %%ZT6                      %16     ; [clobbered] ZMM register
%define %%ZT7                      %17     ; [clobbered] ZMM register
%define %%ZT8                      %18     ; [clobbered] ZMM register
%define %%ZT9                      %19     ; [clobbered] ZMM register
%define %%A_GEN_LEN                %20     ; [in] AAD general length in bytes

        vmovdqu64       %%AAD_HASH, [%%GDATA_CTX + AadHash]   ; load current hash

        ;; Check AAD length in bytes != 0
        cmp             %%A_LEN, 0
        jz              %%zero_len

        UPDATE_HASH     %%A_IN, %%A_LEN, %%AAD_HASH, %%GDATA_KEY, \
                        %%ZT0, %%ZT1, %%ZT2, %%ZT3, %%ZT4, %%ZT5, %%ZT6, %%ZT7, %%ZT8, %%ZT9, \
                        %%GPR1, %%GPR2, %%GPR3, %%MASKREG

%%zero_len:

        mov             %%GPR1, %%A_GEN_LEN
        vmovdqu64       [%%GDATA_CTX + AadHash], %%AAD_HASH   ; ctx.aad hash = aad_hash
        mov             [%%GDATA_CTX + AadLen], %%GPR1        ; ctx.aad_length = aad_length

        xor             %%GPR1, %%GPR1
        mov             [%%GDATA_CTX + InLen], %%GPR1         ; ctx.in_length = 0
        mov             [%%GDATA_CTX + PBlockLen], %%GPR1     ; ctx.partial_block_length = 0

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_FINALIZE_IV_HASH finalize updating of AadHash value in gcm_context_data struct 
;;; to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Initialization Vector (IV), Initialization Vector length (IV_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (orig_IV). and initialized other parts of GDATA_CTX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_FINALIZE_IV_HASH       19-20
%define %%GDATA_KEY                %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX                %2      ; [in] GCM context pointer
%define %%IV                       %3      ; [in] IV pointer
%define %%IV_LEN                   %4      ; [in] IV processing length in bytes
%define %%GPR1                     %5      ; [clobbered] GP register
%define %%GPR2                     %6      ; [clobbered] GP register
%define %%GPR3                     %7      ; [clobbered] GP register
%define %%MASKREG                  %8      ; [clobbered] mask register
%define %%CUR_COUNT                %9      ; [out] XMM for IV value (xmm14)
%define %%ZT0                      %10     ; [clobbered] ZMM register
%define %%ZT1                      %11     ; [clobbered] ZMM register
%define %%ZT2                      %12     ; [clobbered] ZMM register
%define %%ZT3                      %13     ; [clobbered] ZMM register
%define %%ZT4                      %14     ; [clobbered] ZMM register
%define %%ZT5                      %15     ; [clobbered] ZMM register
%define %%ZT6                      %16     ; [clobbered] ZMM register
%define %%ZT7                      %17     ; [clobbered] ZMM register
%define %%ZT8                      %18     ; [clobbered] ZMM register
%define %%ZT9                      %19     ; [clobbered] ZMM register
%define %%IV_GEN_LEN               %20     ; [in] IV general length in bytes

%if %0 == 19 ;; IV is 12 bytes 

        ;; read 12 IV bytes and pad with 0x00000001
        vmovdqu8        %%CUR_COUNT, [rel ONEf]
        mov             %%GPR2, %%IV
        mov             %%GPR1, 0x0000_0000_0000_0fff
        kmovq           %%MASKREG, %%GPR1
        vmovdqu8        %%CUR_COUNT{%%MASKREG}, [%%GPR2]      ; ctr = IV | 0x1

%else ;; IV is different than 12 bytes
        
        vmovdqu         %%CUR_COUNT, [%%GDATA_CTX + OrigIV]

        CALC_J0 %%GDATA_KEY, %%IV, %%IV_LEN, %%CUR_COUNT, \
                %%ZT0, %%ZT1, %%ZT2, %%ZT3, %%ZT4, %%ZT5, %%ZT6, %%ZT7, %%ZT8, %%ZT9, \
                %%GPR1, %%GPR2, %%GPR3, %%MASKREG, %%IV_GEN_LEN

%endif

        vmovdqu64       [%%GDATA_CTX + OrigIV], %%CUR_COUNT   ; ctx.orig_IV = iv

        ;; store IV as counter in LE format
        vpshufb         %%CUR_COUNT, [rel SHUF_MASK]
        vmovdqu         [%%GDATA_CTX + CurCount], %%CUR_COUNT ; ctx.current_counter = iv

%endmacro


;;; ===========================================================================

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GCM_COMPLETE Finishes Encryption/Decryption of last partial block after GCM_UPDATE finishes.
; Input: A gcm_key_data * (GDATA_KEY), gcm_context_data (GDATA_CTX) and whether encoding or decoding (ENC_DEC).
; Output: Authorization Tag (AUTH_TAG) and Authorization Tag length (AUTH_TAG_LEN)
; Clobbers rax, r10-r12, and xmm0-xmm2, xmm5-xmm6, xmm9-xmm11, xmm13-xmm15
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_COMPLETE            6
%define %%GDATA_KEY             %1
%define %%GDATA_CTX             %2
%define %%AUTH_TAG              %3
%define %%AUTH_TAG_LEN          %4
%define %%ENC_DEC               %5
%define %%INSTANCE_TYPE         %6
%define %%PLAIN_CYPH_LEN        rax

        vmovdqu xmm13, [%%GDATA_KEY + HashKey]
        ;; Start AES as early as possible
        vmovdqu xmm9, [%%GDATA_CTX + OrigIV]    ; xmm9 = Y0
        ENCRYPT_SINGLE_BLOCK %%GDATA_KEY, xmm9  ; E(K, Y0)

%ifidn %%INSTANCE_TYPE, multi_call
        ;; If the GCM function is called as a single function call rather
        ;; than invoking the individual parts (init, update, finalize) we
        ;; can remove a write to read dependency on AadHash.
        vmovdqu xmm14, [%%GDATA_CTX + AadHash]

        ;; Encrypt the final partial block. If we did this as a single call then
        ;; the partial block was handled in the main GCM_ENC_DEC macro.
        mov     r12, [%%GDATA_CTX + PBlockLen]
        cmp     r12, 0

        je %%_partial_done

        GHASH_MUL xmm14, xmm13, xmm0, xmm10, xmm11, xmm5, xmm6 ;GHASH computation for the last <16 Byte block
        vmovdqu [%%GDATA_CTX + AadHash], xmm14

%%_partial_done:

%endif

        mov     r12, [%%GDATA_CTX + AadLen]     ; r12 = aadLen (number of bytes)
        mov     %%PLAIN_CYPH_LEN, [%%GDATA_CTX + InLen]

        shl     r12, 3                      ; convert into number of bits
        vmovd   xmm15, r12d                 ; len(A) in xmm15

        shl     %%PLAIN_CYPH_LEN, 3         ; len(C) in bits  (*128)
        vmovq   xmm1, %%PLAIN_CYPH_LEN
        vpslldq xmm15, xmm15, 8             ; xmm15 = len(A)|| 0x0000000000000000
        vpxor   xmm15, xmm15, xmm1          ; xmm15 = len(A)||len(C)

        vpxor   xmm14, xmm15
        GHASH_MUL       xmm14, xmm13, xmm0, xmm10, xmm11, xmm5, xmm6
        vpshufb  xmm14, [rel SHUF_MASK]         ; perform a 16Byte swap

        vpxor   xmm9, xmm9, xmm14


%%_return_T:
        mov     r10, %%AUTH_TAG             ; r10 = authTag
        mov     r11, %%AUTH_TAG_LEN         ; r11 = auth_tag_len

        cmp     r11, 16
        je      %%_T_16

        cmp     r11, 12
        je      %%_T_12

        cmp     r11, 8
        je      %%_T_8

        simd_store_avx_15 r10, xmm9, r11, r12, rax
        jmp     %%_return_T_done
%%_T_8:
        vmovq    rax, xmm9
        mov     [r10], rax
        jmp     %%_return_T_done
%%_T_12:
        vmovq    rax, xmm9
        mov     [r10], rax
        vpsrldq xmm9, xmm9, 8
        vmovd    eax, xmm9
        mov     [r10 + 8], eax
        jmp     %%_return_T_done
%%_T_16:
        vmovdqu  [r10], xmm9

%%_return_T_done:

%ifdef SAFE_DATA
        ;; Clear sensitive data from context structure
        vpxor   xmm0, xmm0
        vmovdqu [%%GDATA_CTX + AadHash], xmm0
        vmovdqu [%%GDATA_CTX + PBlockEncKey], xmm0
%endif
%endmacro ; GCM_COMPLETE


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_precomp_128_vaes_avx512 /
;       aes_gcm_precomp_192_vaes_avx512 /
;       aes_gcm_precomp_256_vaes_avx512
;       (struct gcm_key_data *key_data)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM FN_NAME(precomp,_), PUBLIC
;; Parameter is passed through register
%ifdef SAFE_PARAM
        ;; Check key_data != NULL
        cmp     arg1, 0
        jz      exit_precomp
%endif

        FUNC_SAVE

        vpxor   xmm6, xmm6
        ENCRYPT_SINGLE_BLOCK    arg1, xmm6              ; xmm6 = HashKey

        vpshufb  xmm6, [rel SHUF_MASK]
        ;;;;;;;;;;;;;;;  PRECOMPUTATION of HashKey<<1 mod poly from the HashKey;;;;;;;;;;;;;;;
        vmovdqa  xmm2, xmm6
        vpsllq   xmm6, xmm6, 1
        vpsrlq   xmm2, xmm2, 63
        vmovdqa  xmm1, xmm2
        vpslldq  xmm2, xmm2, 8
        vpsrldq  xmm1, xmm1, 8
        vpor     xmm6, xmm6, xmm2
        ;reduction
        vpshufd  xmm2, xmm1, 00100100b
        vpcmpeqd xmm2, [rel TWOONE]
        vpand    xmm2, xmm2, [rel POLY]
        vpxor    xmm6, xmm6, xmm2                       ; xmm6 holds the HashKey<<1 mod poly
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vmovdqu  [arg1 + HashKey], xmm6                 ; store HashKey<<1 mod poly


        PRECOMPUTE arg1, xmm6, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5

        FUNC_RESTORE
.exit_precomp:

        ret

ENDFUNC FN_NAME(precomp,_)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_enc_128_update_vaes_avx512 / aes_gcm_enc_192_update_vaes_avx512 /
;       aes_gcm_enc_256_update_vaes_avx512
;       (const struct gcm_key_data *key_data,
;        struct gcm_context_data *context_data,
;        u8       *out,
;        const u8 *in,
;        u64      plaintext_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM FN_NAME(enc,_update_), PUBLIC

        FUNC_SAVE

%ifdef SAFE_PARAM
        ;; Check key_data != NULL
        cmp     arg1, 0
        jz      exit_update_enc

        ;; Check context_data != NULL
        cmp     arg2, 0
        jz      exit_update_enc

        ;; Check if plaintext_len == 0
        cmp     arg5, 0
        jz      skip_in_out_check_update_enc

        ;; Check out != NULL (plaintext_len != 0)
        cmp     arg3, 0
        jz      exit_update_enc

        ;; Check in != NULL (plaintext_len != 0)
        cmp     arg4, 0
        jz      exit_update_enc

.skip_in_out_check_update_enc:
%endif
        GCM_ENC_DEC arg1, arg2, arg3, arg4, arg5, ENC, multi_call

.exit_update_enc:
        FUNC_RESTORE
        ret

ENDFUNC FN_NAME(enc,_update_)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_dec_128_update_vaes_avx512 / aes_gcm_dec_192_update_vaes_avx512 /
;       aes_gcm_dec_256_update_vaes_avx512
;       (const struct gcm_key_data *key_data,
;        struct gcm_context_data *context_data,
;        u8       *out,
;        const u8 *in,
;        u64      plaintext_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM FN_NAME(dec,_update_), PUBLIC
        FUNC_SAVE

%ifdef SAFE_PARAM
        ;; Check key_data != NULL
        cmp     arg1, 0
        jz      exit_update_dec

        ;; Check context_data != NULL
        cmp     arg2, 0
        jz      exit_update_dec

        ;; Check if plaintext_len == 0
        cmp     arg5, 0
        jz      skip_in_out_check_update_dec

        ;; Check out != NULL (plaintext_len != 0)
        cmp     arg3, 0
        jz      exit_update_dec

        ;; Check in != NULL (plaintext_len != 0)
        cmp     arg4, 0
        jz      exit_update_dec

.skip_in_out_check_update_dec:
%endif

        GCM_ENC_DEC arg1, arg2, arg3, arg4, arg5, DEC, multi_call

.exit_update_dec:
        FUNC_RESTORE
        ret

ENDFUNC FN_NAME(dec,_update_)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_gettag_128_vaes_avx512 / aes_gcm_gettag_192_vaes_avx512 /
;       aes_gcm_gettag_256_vaes_avx512
;       (const struct gcm_key_data *key_data,
;        struct gcm_context_data *context_data,
;        u8       *auth_tag,
;        u64      auth_tag_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM FN_NAME(gettag,_), PUBLIC

;; All parameters are passed through registers
%ifdef SAFE_PARAM
        ;; Check key_data != NULL
        cmp     arg1, 0
        jz      exit_enc_fin

        ;; Check context_data != NULL
        cmp     arg2, 0
        jz      exit_enc_fin

        ;; Check auth_tag != NULL
        cmp     arg3, 0
        jz      exit_enc_fin

        ;; Check auth_tag_len == 0 or > 16
        cmp     arg4, 0
        jz      exit_enc_fin

        cmp     arg4, 16
        ja      exit_enc_fin
%endif

        FUNC_SAVE
        GCM_COMPLETE    arg1, arg2, arg3, arg4, ENC, multi_call

        FUNC_RESTORE

.exit_enc_fin:
        ret

ENDFUNC FN_NAME(gettag,_)

%ifdef GCM128_MODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   ghash_vaes_avx512
;        const struct gcm_key_data *key_data,
;        const void   *in,
;        const u64    in_len,
;        void         *tag,
;        const u64    tag_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM ghash_vaes_avx512, PUBLIC

        FUNC_SAVE

%ifdef SAFE_PARAM
        ;; Check key_data != NULL
        cmp     arg1, 0
        jz      exit_ghash

        ;; Check in != NULL
        cmp     arg2, 0
        jz      exit_ghash

        ;; Check in_len != 0
        cmp     arg3, 0
        jz      exit_ghash

        ;; Check tag != NULL
        cmp     arg4, 0
        jz      exit_ghash

        ;; Check tag_len != 0
        cmp     arg5, 0
        jz      exit_ghash
%endif

        CALC_AAD_HASH arg2, arg3, xmm0, arg1, zmm1, zmm2, zmm3, zmm4, zmm5, \
                      zmm6, zmm7, zmm8, zmm9, zmm10, r10, r11, r12, k1

        vpshufb xmm0, [rel SHUF_MASK] ; perform a 16Byte swap

        simd_store_avx arg4, xmm0, arg5, r12, rax

.exit_ghash:
        FUNC_RESTORE

        ret
ENDFUNC ghash_vaes_avx512
%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; IV and AAD in disconnected buffers processing 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%ifdef GCM128_MODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_aad_hash_update_vaes512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *aad,
;        const u64    aad_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_aad_hash_update_vaes512, PUBLIC

        FUNC_SAVE

        GCM_UPDATE_AAD_HASH arg1, arg2, arg3, arg4, r10, r11, r12, k1, xmm14, zmm1, \
                            zmm2, zmm11, zmm3, zmm4, zmm5, zmm6, zmm7, zmm8, zmm9


        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_aad_hash_update_vaes512

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_aad_hash_finalize_vaes512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *aad,
;        const u64    aad_len,
;        const u64    aad_general_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_aad_hash_finalize_vaes512, PUBLIC

        FUNC_SAVE

        GCM_FINALIZE_AAD_HASH arg1, arg2, arg3, arg4, r10, r11, r12, k1, xmm14, zmm1, \
                              zmm2, zmm11, zmm3, zmm4, zmm5, zmm6, zmm7, zmm8, zmm9, arg5

        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_aad_hash_finalize_vaes512

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_iv_hash_update_vaes512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *iv,
;        const u64    iv_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_iv_hash_update_vaes512, PUBLIC

        FUNC_SAVE

        GCM_UPDATE_IV_HASH  arg1, arg2, arg3, arg4, r10, r11, r12, k1, xmm14, zmm1, \
                            zmm2, zmm11, zmm3, zmm4, zmm5, zmm6, zmm7, zmm8, zmm9


        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_iv_hash_update_vaes512

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_iv_hash_finalize_vaes512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *iv,
;        const u64    iv_len,
;        const u64    iv_general_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_iv_hash_finalize_vaes512, PUBLIC

        FUNC_SAVE

        cmp     arg5, 12
        je      .iv_eq_12

        GCM_FINALIZE_IV_HASH arg1, arg2, arg3, arg4, r10, r11, r12, k1, xmm14, zmm1, \
                              zmm2, zmm11, zmm3, zmm4, zmm5, zmm6, zmm7, zmm8, zmm9, arg5
        jmp .skip_iv_eq_12

.iv_eq_12:

        GCM_FINALIZE_IV_HASH arg1, arg2, arg3, arg4, r10, r11, r12, k1, xmm14, zmm1, \
                              zmm2, zmm11, zmm3, zmm4, zmm5, zmm6, zmm7, zmm8, zmm9
.skip_iv_eq_12:

        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_iv_hash_finalize_vaes512

%endif

%endif
