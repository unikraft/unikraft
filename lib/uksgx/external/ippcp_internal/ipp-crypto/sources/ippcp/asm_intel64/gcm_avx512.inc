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

%ifndef _GCM_AVX512_INC_
%define _GCM_AVX512_INC_

%include "os.inc"
%include "reg_sizes.inc"
%include "clear_regs.inc"
%include "gcm_defines.inc"
%include "gcm_keys_avx512.inc"
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
%define FN_NAME(x,y) aes_gcm_ %+ x %+ _128 %+ y %+ avx512
%endif

%ifdef GCM192_MODE
%define NROUNDS 11
%define FN_NAME(x,y) aes_gcm_ %+ x %+ _192 %+ y %+ avx512
%endif

%ifdef GCM256_MODE
%define NROUNDS 13
%define FN_NAME(x,y) aes_gcm_ %+ x %+ _256 %+ y %+ avx512
%endif

section .text
default rel

; need to push 4 registers into stack to maintain
%define STACK_OFFSET   8*4

%ifidn __OUTPUT_FORMAT__, win64
        %define XMM_STORAGE     16*10
%else
        %define XMM_STORAGE     0
%endif

%define TMP2    16*0    ; Temporary storage for AES State 2 (State 1 is stored in an XMM register)
%define TMP3    16*1    ; Temporary storage for AES State 3
%define TMP4    16*2    ; Temporary storage for AES State 4
%define TMP5    16*3    ; Temporary storage for AES State 5
%define TMP6    16*4    ; Temporary storage for AES State 6
%define TMP7    16*5    ; Temporary storage for AES State 7
%define TMP8    16*6    ; Temporary storage for AES State 8
%define LOCAL_STORAGE   16*7
%define VARIABLE_OFFSET LOCAL_STORAGE + XMM_STORAGE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Utility Macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GHASH_MUL MACRO to implement: Data*HashKey mod (128,127,126,121,0)
; Input: A and B (128-bits each, bit-reflected)
; Output: C = A*B*x mod poly, (i.e. >>1 )
; To compute GH = GH*HashKey mod poly, give HK = HashKey<<1 mod poly as input
; GH = GH * HK * x mod poly which is equivalent to GH*HashKey mod poly.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GHASH_MUL  7
%define %%GH %1         ; 16 Bytes
%define %%HK %2         ; 16 Bytes
%define %%T1 %3
%define %%T2 %4
%define %%T3 %5
%define %%T4 %6
%define %%T5 %7
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        vpclmulqdq      %%T1, %%GH, %%HK, 0x11          ; %%T1 = a1*b1
        vpclmulqdq      %%T2, %%GH, %%HK, 0x00          ; %%T2 = a0*b0
        vpclmulqdq      %%T3, %%GH, %%HK, 0x01          ; %%T3 = a1*b0
        vpclmulqdq      %%GH, %%GH, %%HK, 0x10          ; %%GH = a0*b1
        vpxor           %%GH, %%GH, %%T3


        vpsrldq         %%T3, %%GH, 8                   ; shift-R %%GH 2 DWs
        vpslldq         %%GH, %%GH, 8                   ; shift-L %%GH 2 DWs

        vpxor           %%T1, %%T1, %%T3
        vpxor           %%GH, %%GH, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqu         %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%GH, 0x01
        vpslldq         %%T2, %%T2, 8                    ; shift-L %%T2 2 DWs

        vpxor           %%GH, %%GH, %%T2                 ; first phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%T2, %%T3, %%GH, 0x00
        vpsrldq         %%T2, %%T2, 4                    ; shift-R %%T2 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%GH, %%T3, %%GH, 0x10
        vpslldq         %%GH, %%GH, 4                    ; shift-L %%GH 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%GH, %%GH, %%T2                 ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%GH, %%GH, %%T1                 ; the result is in %%GH
%endmacro


; In PRECOMPUTE, the commands filling Hashkey_i_k are not required for avx512
; functions, but are kept to allow users to switch cpu architectures between calls
; of pre, init, update, and finalize.
%macro  PRECOMPUTE 8
%define %%GDATA %1
%define %%HK    %2
%define %%T1    %3
%define %%T2    %4
%define %%T3    %5
%define %%T4    %6
%define %%T5    %7
%define %%T6    %8

        ; Haskey_i_k holds XORed values of the low and high parts of the Haskey_i
        vmovdqa  %%T5, %%HK

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^2<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_2], %%T5                    ;  [HashKey_2] = HashKey^2<<1 mod poly

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^3<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_3], %%T5

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^4<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_4], %%T5

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^5<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_5], %%T5

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^6<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_6], %%T5

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^7<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_7], %%T5

        GHASH_MUL %%T5, %%HK, %%T1, %%T3, %%T4, %%T6, %%T2      ;  %%T5 = HashKey^8<<1 mod poly
        vmovdqu  [%%GDATA + HashKey_8], %%T5
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; READ_SMALL_DATA_INPUT: Packs xmm register with data when data input is less than 16 bytes.
; Returns 0 if data has length 0.
; Input: The input data (INPUT), that data's length (LENGTH).
; Output: The packed xmm register (OUTPUT).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro READ_SMALL_DATA_INPUT    4
%define %%OUTPUT                %1 ; %%OUTPUT is an xmm register
%define %%INPUT                 %2
%define %%LENGTH                %3
%define %%TMP1                  %4

        lea             %%TMP1, [rel byte_len_to_mask_table]
%ifidn __OUTPUT_FORMAT__, win64
        add             %%TMP1, %%LENGTH
        add             %%TMP1, %%LENGTH
        kmovw           k1, [%%TMP1]
%else
        kmovw           k1, [%%TMP1 + %%LENGTH*2]
%endif
        vmovdqu8        XWORD(%%OUTPUT){k1}{z}, [%%INPUT]

%endmacro ; READ_SMALL_DATA_INPUT


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CALC_AAD_HASH: Calculates the hash of the data which will not be encrypted.
; Input: The input data (A_IN), that data's length (A_LEN), and the hash key (HASH_KEY).
; Output: The hash of the data (AAD_HASH).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  CALC_AAD_HASH   13
%define %%A_IN          %1
%define %%A_LEN         %2
%define %%AAD_HASH      %3
%define %%GDATA_KEY     %4
%define %%XTMP0         %5      ; xmm temp reg 5
%define %%XTMP1         %6      ; xmm temp reg 5
%define %%XTMP2         %7
%define %%XTMP3         %8
%define %%XTMP4         %9
%define %%XTMP5         %10     ; xmm temp reg 5
%define %%T1            %11     ; temp reg 1
%define %%T2            %12
%define %%T3            %13


        mov     %%T1, %%A_IN            ; T1 = AAD
        mov     %%T2, %%A_LEN           ; T2 = aadLen
        vpxor   %%AAD_HASH, %%AAD_HASH

%%_get_AAD_loop128:
        cmp     %%T2, 128
        jl      %%_exit_AAD_loop128

        vmovdqu         %%XTMP0, [%%T1 + 16*0]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vpxor           %%XTMP0, %%AAD_HASH

        vmovdqu         %%XTMP5, [%%GDATA_KEY + HashKey_8]
        vpclmulqdq      %%XTMP1, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = a1*b1
        vpclmulqdq      %%XTMP2, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = a0*b0
        vpclmulqdq      %%XTMP3, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = a1*b0
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10                 ; %%T4 = a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4                       ; %%T3 = a1*b0 + a0*b1

%assign i 1
%assign j 7
%rep 7
        vmovdqu         %%XTMP0, [%%T1 + 16*i]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vmovdqu         %%XTMP5, [%%GDATA_KEY + HashKey_ %+ j]
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = T1 + a1*b1
        vpxor           %%XTMP1, %%XTMP1, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = T2 + a0*b0
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = T3 + a1*b0 + a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4
%assign i (i + 1)
%assign j (j - 1)
%endrep

        vpslldq         %%XTMP4, %%XTMP3, 8                             ; shift-L 2 DWs
        vpsrldq         %%XTMP3, %%XTMP3, 8                             ; shift-R 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4
        vpxor           %%XTMP1, %%XTMP1, %%XTMP3                       ; accumulate the results in %%T1(M):%%T2(L)

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqa         %%XTMP5, [rel POLY2]
        vpclmulqdq      %%XTMP0, %%XTMP5, %%XTMP2, 0x01
        vpslldq         %%XTMP0, %%XTMP0, 8                             ; shift-L xmm2 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP0                       ; first phase of the reduction complete

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%XTMP3, %%XTMP5, %%XTMP2, 0x00
        vpsrldq         %%XTMP3, %%XTMP3, 4                             ; shift-R 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%XTMP4, %%XTMP5, %%XTMP2, 0x10
        vpslldq         %%XTMP4, %%XTMP4, 4                             ; shift-L 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%XTMP4, %%XTMP4, %%XTMP3                       ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%AAD_HASH, %%XTMP1, %%XTMP4                    ; the result is in %%T1

        sub     %%T2, 128
        je      %%_CALC_AAD_done

        add     %%T1, 128
        jmp     %%_get_AAD_loop128

%%_exit_AAD_loop128:
        cmp     %%T2, 16
        jl      %%_get_small_AAD_block

        ;; calculate hash_key position to start with
        mov     %%T3, %%T2
        and     %%T3, -16       ; 1 to 7 blocks possible here
        neg     %%T3
        add     %%T3, HashKey_1 + 16
        lea     %%T3, [%%GDATA_KEY + %%T3]

        vmovdqu         %%XTMP0, [%%T1]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vpxor           %%XTMP0, %%AAD_HASH

        vmovdqu         %%XTMP5, [%%T3]
        vpclmulqdq      %%XTMP1, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = a1*b1
        vpclmulqdq      %%XTMP2, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = a0*b0
        vpclmulqdq      %%XTMP3, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = a1*b0
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10                 ; %%T4 = a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4                       ; %%T3 = a1*b0 + a0*b1

        add     %%T3, 16        ; move to next hashkey
        add     %%T1, 16        ; move to next data block
        sub     %%T2, 16
        cmp     %%T2, 16
        jl      %%_AAD_reduce

%%_AAD_blocks:
        vmovdqu         %%XTMP0, [%%T1]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vmovdqu         %%XTMP5, [%%T3]
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = T1 + a1*b1
        vpxor           %%XTMP1, %%XTMP1, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = T2 + a0*b0
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = T3 + a1*b0 + a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4

        add     %%T3, 16        ; move to next hashkey
        add     %%T1, 16
        sub     %%T2, 16
        cmp     %%T2, 16
        jl      %%_AAD_reduce
        jmp     %%_AAD_blocks

%%_AAD_reduce:
        vpslldq         %%XTMP4, %%XTMP3, 8                             ; shift-L 2 DWs
        vpsrldq         %%XTMP3, %%XTMP3, 8                             ; shift-R 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4
        vpxor           %%XTMP1, %%XTMP1, %%XTMP3                       ; accumulate the results in %%T1(M):%%T2(L)

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqa         %%XTMP5, [rel POLY2]
        vpclmulqdq      %%XTMP0, %%XTMP5, %%XTMP2, 0x01
        vpslldq         %%XTMP0, %%XTMP0, 8                             ; shift-L xmm2 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP0                       ; first phase of the reduction complete

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%XTMP3, %%XTMP5, %%XTMP2, 0x00
        vpsrldq         %%XTMP3, %%XTMP3, 4                             ; shift-R 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%XTMP4, %%XTMP5, %%XTMP2, 0x10
        vpslldq         %%XTMP4, %%XTMP4, 4                             ; shift-L 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%XTMP4, %%XTMP4, %%XTMP3                       ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%AAD_HASH, %%XTMP1, %%XTMP4                    ; the result is in %%T1

        or      %%T2, %%T2
        je      %%_CALC_AAD_done

%%_get_small_AAD_block:
        vmovdqu         %%XTMP0, [%%GDATA_KEY + HashKey]
        READ_SMALL_DATA_INPUT   %%XTMP1, %%T1, %%T2, %%T3
        ;byte-reflect the AAD data
        vpshufb         %%XTMP1, [rel SHUF_MASK]
        vpxor           %%AAD_HASH, %%XTMP1
        GHASH_MUL       %%AAD_HASH, %%XTMP0, %%XTMP1, %%XTMP2, %%XTMP3, %%XTMP4, %%XTMP5

%%_CALC_AAD_done:

%endmacro ; CALC_AAD_HASH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PARTIAL_BLOCK: Handles encryption/decryption and the tag partial blocks between update calls.
; Requires the input data be at least 1 byte long.
; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX), input text (PLAIN_CYPH_IN),
; input text length (PLAIN_CYPH_LEN), the current data offset (DATA_OFFSET),
; and whether encoding or decoding (ENC_DEC)
; Output: A cypher of the first partial block (CYPH_PLAIN_OUT), and updated GDATA_CTX
; Clobbers rax, r10, r12, r13, r15, xmm0, xmm1, xmm2, xmm3, xmm5, xmm6, xmm9, xmm10, xmm11, xmm13
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro PARTIAL_BLOCK    8
%define %%GDATA_KEY             %1
%define %%GDATA_CTX             %2
%define %%CYPH_PLAIN_OUT        %3
%define %%PLAIN_CYPH_IN         %4
%define %%PLAIN_CYPH_LEN        %5
%define %%DATA_OFFSET           %6
%define %%AAD_HASH              %7
%define %%ENC_DEC               %8

        mov     r13, [%%GDATA_CTX + PBlockLen]
        cmp     r13, 0
        je      %%_partial_block_done           ;Leave Macro if no partial blocks

        cmp     %%PLAIN_CYPH_LEN, 16            ;Read in input data without over reading
        jl      %%_fewer_than_16_bytes
        VXLDR   xmm1, [%%PLAIN_CYPH_IN]         ;If more than 16 bytes of data, just fill the xmm register
        jmp     %%_data_read

%%_fewer_than_16_bytes:
        lea     r10, [%%PLAIN_CYPH_IN]
        READ_SMALL_DATA_INPUT   xmm1, r10, %%PLAIN_CYPH_LEN, rax

%%_data_read:                           ;Finished reading in data

        vmovdqu xmm9, [%%GDATA_CTX + PBlockEncKey]  ;xmm9 = my_ctx_data.partial_block_enc_key
        vmovdqu xmm13, [%%GDATA_KEY + HashKey]

        lea     r12, [rel SHIFT_MASK]

        add     r12, r13                        ; adjust the shuffle mask pointer to be able to shift r13 bytes (16-r13 is the number of bytes in plaintext mod 16)
        vmovdqu xmm2, [r12]                     ; get the appropriate shuffle mask
        vpshufb xmm9, xmm2                      ;shift right r13 bytes

%ifidn  %%ENC_DEC, DEC
        vmovdqa xmm3, xmm1
%endif
        vpxor   xmm9, xmm1                      ; Cyphertext XOR E(K, Yn)

        mov     r15, %%PLAIN_CYPH_LEN
        add     r15, r13
        sub     r15, 16                         ;Set r15 to be the amount of data left in CYPH_PLAIN_IN after filling the block
        jge     %%_no_extra_mask                ;Determine if if partial block is not being filled and shift mask accordingly
        sub     r12, r15
%%_no_extra_mask:

        vmovdqu xmm1, [r12 + ALL_F - SHIFT_MASK]; get the appropriate mask to mask out bottom r13 bytes of xmm9
        vpand   xmm9, xmm1                      ; mask out bottom r13 bytes of xmm9

%ifidn  %%ENC_DEC, DEC
        vpand   xmm3, xmm1
        vpshufb xmm3, [rel SHUF_MASK]
        vpshufb xmm3, xmm2
        vpxor   %%AAD_HASH, xmm3
%else
        vpshufb xmm9, [rel SHUF_MASK]
        vpshufb xmm9, xmm2
        vpxor   %%AAD_HASH, xmm9
%endif
        cmp     r15,0
        jl      %%_partial_incomplete

        GHASH_MUL       %%AAD_HASH, xmm13, xmm0, xmm10, xmm11, xmm5, xmm6       ;GHASH computation for the last <16 Byte block
        xor     rax,rax
        mov     [%%GDATA_CTX + PBlockLen], rax
        jmp     %%_enc_dec_done
%%_partial_incomplete:
%ifidn __OUTPUT_FORMAT__, win64
        mov     rax, %%PLAIN_CYPH_LEN
       	add     [%%GDATA_CTX + PBlockLen], rax
%else
        add     [%%GDATA_CTX + PBlockLen], %%PLAIN_CYPH_LEN
%endif
%%_enc_dec_done:
        vmovdqu [%%GDATA_CTX + AadHash], %%AAD_HASH

%ifidn  %%ENC_DEC, ENC
        vpshufb xmm9, [rel SHUF_MASK]       ; shuffle xmm9 back to output as ciphertext
        vpshufb xmm9, xmm2
%endif

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ; output encrypted Bytes
        cmp     r15,0
        jl      %%_partial_fill
        mov     r12, r13
        mov     r13, 16
        sub     r13, r12                        ; Set r13 to be the number of bytes to write out
        jmp     %%_count_set
%%_partial_fill:
        mov     r13, %%PLAIN_CYPH_LEN
%%_count_set:
        lea             rax, [rel byte_len_to_mask_table]
        kmovw           k1, [rax + r13*2]
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET]{k1}, xmm9
        add             %%DATA_OFFSET, r13
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

; if a = number of total plaintext bytes
; b = floor(a/16)
; %%num_initial_blocks = b mod 8;
; encrypt the initial %%num_initial_blocks blocks and apply ghash on the ciphertext
; %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r14 are used as a pointer only, not modified.
; Updated AAD_HASH is returned in %%T3

%macro INITIAL_BLOCKS 23
%define %%GDATA_KEY             %1
%define %%CYPH_PLAIN_OUT        %2
%define %%PLAIN_CYPH_IN         %3
%define %%LENGTH                %4
%define %%DATA_OFFSET           %5
%define %%num_initial_blocks    %6      ; can be 0, 1, 2, 3, 4, 5, 6 or 7
%define %%T1                    %7
%define %%T2                    %8
%define %%T3                    %9
%define %%T4                    %10
%define %%T5                    %11
%define %%CTR                   %12
%define %%XMM1                  %13
%define %%XMM2                  %14
%define %%XMM3                  %15
%define %%XMM4                  %16
%define %%XMM5                  %17
%define %%XMM6                  %18
%define %%XMM7                  %19
%define %%XMM8                  %20
%define %%T6                    %21
%define %%T_key                 %22
%define %%ENC_DEC               %23

%assign i (8-%%num_initial_blocks)
                ;; Move AAD_HASH to temp reg
                vmovdqu  %%T2, %%XMM8
                ;; Start AES for %%num_initial_blocks blocks
                ;; vmovdqu  %%CTR, [%%GDATA_CTX + CurCount]   ; %%CTR = Y0

%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                vpaddd   %%CTR, %%CTR, [rel ONE]     ; INCR Y0
                vmovdqa  reg(i), %%CTR
                vpshufb  reg(i), [rel SHUF_MASK]     ; perform a 16Byte swap
%assign i (i+1)
%endrep

%if(%%num_initial_blocks>0)
vmovdqu  %%T_key, [%%GDATA_KEY+16*0]
%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                vpxor    reg(i),reg(i),%%T_key
%assign i (i+1)
%endrep

%assign j 1
%rep NROUNDS
vmovdqu  %%T_key, [%%GDATA_KEY+16*j]
%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                vaesenc  reg(i),%%T_key
%assign i (i+1)
%endrep

%assign j (j+1)
%endrep


vmovdqu  %%T_key, [%%GDATA_KEY+16*j]
%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                vaesenclast      reg(i),%%T_key
%assign i (i+1)
%endrep

%endif ; %if(%%num_initial_blocks>0)



%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET]
                vpxor    reg(i), reg(i), %%T1
                ;; Write back ciphertext for %%num_initial_blocks blocks
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET], reg(i)
                add     %%DATA_OFFSET, 16
                %ifidn  %%ENC_DEC, DEC
                    vmovdqa  reg(i), %%T1
                %endif
                ;; Prepare ciphertext for GHASH computations
                vpshufb  reg(i), [rel SHUF_MASK]
%assign i (i+1)
%endrep


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%assign i (9-%%num_initial_blocks)
%if(%%num_initial_blocks>0)
        vmovdqa %%T3, reg(i)
%assign i (i+1)
%endif
%if %%num_initial_blocks>1
%rep %%num_initial_blocks-1
        vmovdqu [rsp + TMP %+ i], reg(i)
%assign i (i+1)
%endrep
%endif

                ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                ;; Prepare 8 counter blocks and perform rounds of AES cipher on
                ;; them, load plain/cipher text and store cipher/plain text.
                ;; Stitch GHASH computation in between AES rounds.
                vpaddd   %%XMM1, %%CTR, [rel ONE]   ; INCR Y0
                vpaddd   %%XMM2, %%CTR, [rel TWO]   ; INCR Y0
                vpaddd   %%XMM3, %%XMM1, [rel TWO]  ; INCR Y0
                vpaddd   %%XMM4, %%XMM2, [rel TWO]  ; INCR Y0
                vpaddd   %%XMM5, %%XMM3, [rel TWO]  ; INCR Y0
                vpaddd   %%XMM6, %%XMM4, [rel TWO]  ; INCR Y0
                vpaddd   %%XMM7, %%XMM5, [rel TWO]  ; INCR Y0
                vpaddd   %%XMM8, %%XMM6, [rel TWO]  ; INCR Y0
                vmovdqa  %%CTR, %%XMM8

                vpshufb  %%XMM1, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM2, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM3, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM4, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM5, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM6, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM7, [rel SHUF_MASK]    ; perform a 16Byte swap
                vpshufb  %%XMM8, [rel SHUF_MASK]    ; perform a 16Byte swap

                vmovdqu  %%T_key, [%%GDATA_KEY+16*0]
                vpxor    %%XMM1, %%XMM1, %%T_key
                vpxor    %%XMM2, %%XMM2, %%T_key
                vpxor    %%XMM3, %%XMM3, %%T_key
                vpxor    %%XMM4, %%XMM4, %%T_key
                vpxor    %%XMM5, %%XMM5, %%T_key
                vpxor    %%XMM6, %%XMM6, %%T_key
                vpxor    %%XMM7, %%XMM7, %%T_key
                vpxor    %%XMM8, %%XMM8, %%T_key

%assign i (8-%%num_initial_blocks)
%assign j (9-%%num_initial_blocks)
%assign k (%%num_initial_blocks)

%define %%T4_2 %%T4
%if(%%num_initial_blocks>0)
        ;; Hash in AES state
        ;; T2 - incoming AAD hash
        vpxor %%T2, %%T3

        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, first
%endif

                vmovdqu  %%T_key, [%%GDATA_KEY+16*1]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu  %%T_key, [%%GDATA_KEY+16*2]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>1)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

                vmovdqu  %%T_key, [%%GDATA_KEY+16*3]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu  %%T_key, [%%GDATA_KEY+16*4]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>2)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>3)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

                vmovdqu  %%T_key, [%%GDATA_KEY+16*5]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu  %%T_key, [%%GDATA_KEY+16*6]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>4)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

                vmovdqu  %%T_key, [%%GDATA_KEY+16*7]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu  %%T_key, [%%GDATA_KEY+16*8]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>5)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

                vmovdqu  %%T_key, [%%GDATA_KEY+16*9]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

%ifndef GCM128_MODE
                vmovdqu  %%T_key, [%%GDATA_KEY+16*10]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key
%endif

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>6)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

%ifdef GCM128_MODE
                vmovdqu  %%T_key, [%%GDATA_KEY+16*10]
                vaesenclast  %%XMM1, %%T_key
                vaesenclast  %%XMM2, %%T_key
                vaesenclast  %%XMM3, %%T_key
                vaesenclast  %%XMM4, %%T_key
                vaesenclast  %%XMM5, %%T_key
                vaesenclast  %%XMM6, %%T_key
                vaesenclast  %%XMM7, %%T_key
                vaesenclast  %%XMM8, %%T_key
%endif

%ifdef GCM192_MODE
                vmovdqu  %%T_key, [%%GDATA_KEY+16*11]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu          %%T_key, [%%GDATA_KEY+16*12]
                vaesenclast      %%XMM1, %%T_key
                vaesenclast      %%XMM2, %%T_key
                vaesenclast      %%XMM3, %%T_key
                vaesenclast      %%XMM4, %%T_key
                vaesenclast      %%XMM5, %%T_key
                vaesenclast      %%XMM6, %%T_key
                vaesenclast      %%XMM7, %%T_key
                vaesenclast      %%XMM8, %%T_key
%endif
%ifdef GCM256_MODE
                vmovdqu  %%T_key, [%%GDATA_KEY+16*11]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu          %%T_key, [%%GDATA_KEY+16*12]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key
%endif

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%if(%%num_initial_blocks>7)
        ;;                 GDATA,       HASHKEY, CIPHER,
        ;;               STATE_11, STATE_00, STATE_MID, T1, T2
        vmovdqu         %%T2, [rsp + TMP %+ j]
        GHASH_SINGLE_MUL %%GDATA_KEY, HashKey_ %+ k, %%T2, \
                         %%T1,     %%T4,   %%T6,    %%T5, %%T3, not_first
%endif

%ifdef GCM256_MODE             ; GCM256
                vmovdqu  %%T_key, [%%GDATA_KEY+16*13]
                vaesenc  %%XMM1, %%T_key
                vaesenc  %%XMM2, %%T_key
                vaesenc  %%XMM3, %%T_key
                vaesenc  %%XMM4, %%T_key
                vaesenc  %%XMM5, %%T_key
                vaesenc  %%XMM6, %%T_key
                vaesenc  %%XMM7, %%T_key
                vaesenc  %%XMM8, %%T_key

                vmovdqu          %%T_key, [%%GDATA_KEY+16*14]
                vaesenclast      %%XMM1, %%T_key
                vaesenclast      %%XMM2, %%T_key
                vaesenclast      %%XMM3, %%T_key
                vaesenclast      %%XMM4, %%T_key
                vaesenclast      %%XMM5, %%T_key
                vaesenclast      %%XMM6, %%T_key
                vaesenclast      %%XMM7, %%T_key
                vaesenclast      %%XMM8, %%T_key
%endif                          ;  GCM256 mode

%if(%%num_initial_blocks>0)
        vpsrldq %%T3, %%T6, 8            ; shift-R %%T2 2 DWs
        vpslldq %%T6, %%T6, 8            ; shift-L %%T3 2 DWs
        vpxor   %%T1, %%T1, %%T3         ; accumulate the results in %%T1:%%T4
        vpxor   %%T4, %%T6, %%T4

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ; First phase of the reduction
        vmovdqu         %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%T4, 0x01
        vpslldq         %%T2, %%T2, 8             ; shift-L xmm2 2 DWs

        ;; First phase of the reduction complete
        vpxor           %%T4, %%T4, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ; Second phase of the reduction
        vpclmulqdq      %%T2, %%T3, %%T4, 0x00
        ;; Shift-R xmm2 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)
        vpsrldq         %%T2, %%T2, 4

        vpclmulqdq      %%T4, %%T3, %%T4, 0x10
        ;; Shift-L xmm0 1 DW (Shift-L 1-DW to obtain result with no shifts)
        vpslldq         %%T4, %%T4, 4
        ;; Second phase of the reduction complete
        vpxor           %%T4, %%T4, %%T2
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ; The result is in %%T3
        vpxor           %%T3, %%T1, %%T4
%else
        ;; The hash should end up in T3
        vmovdqa  %%T3, %%T2
%endif

        ;; Final hash is now in T3
%if %%num_initial_blocks > 0
        ;; NOTE: obsolete in case %%num_initial_blocks = 0
        sub     %%LENGTH, 16*%%num_initial_blocks
%endif

                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*0]
                vpxor    %%XMM1, %%XMM1, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*0], %%XMM1
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM1, %%T1
                %endif

                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*1]
                vpxor    %%XMM2, %%XMM2, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*1], %%XMM2
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM2, %%T1
                %endif

                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*2]
                vpxor    %%XMM3, %%XMM3, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*2], %%XMM3
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM3, %%T1
                %endif

                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*3]
                vpxor    %%XMM4, %%XMM4, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*3], %%XMM4
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM4, %%T1
                %endif

                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*4]
                vpxor    %%XMM5, %%XMM5, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*4], %%XMM5
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM5, %%T1
                %endif

                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*5]
                vpxor    %%XMM6, %%XMM6, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*5], %%XMM6
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM6, %%T1
                %endif

               VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*6]
                vpxor    %%XMM7, %%XMM7, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*6], %%XMM7
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM7, %%T1
                %endif

%if %%num_initial_blocks > 0
                ;; NOTE: 'jl' is never taken for %%num_initial_blocks = 0
                ;;      This macro is executed for lenght 128 and up,
                ;;      zero length is checked in GCM_ENC_DEC.
                ;; If the last block is partial then the xor will be done later
                ;; in ENCRYPT_FINAL_PARTIAL_BLOCK.
                ;; We know it's partial if LENGTH - 16*num_initial_blocks < 128
                cmp %%LENGTH, 128
                jl %%_initial_skip_last_word_write
%endif
                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET + 16*7]
                vpxor    %%XMM8, %%XMM8, %%T1
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET + 16*7], %%XMM8
                %ifidn  %%ENC_DEC, DEC
                vmovdqa  %%XMM8, %%T1
                %endif

                ;; Update %%LENGTH with the number of blocks processed
                sub     %%LENGTH, 16
                add     %%DATA_OFFSET, 16
%%_initial_skip_last_word_write:
                sub     %%LENGTH, 128-16
                add     %%DATA_OFFSET, 128-16

                vpshufb  %%XMM1, [rel SHUF_MASK]             ; perform a 16Byte swap
                ;; Combine GHASHed value with the corresponding ciphertext
                vpxor    %%XMM1, %%XMM1, %%T3
                vpshufb  %%XMM2, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb  %%XMM3, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb  %%XMM4, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb  %%XMM5, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb  %%XMM6, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb  %%XMM7, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb  %%XMM8, [rel SHUF_MASK]             ; perform a 16Byte swap

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%%_initial_blocks_done:


%endmacro

;;; INITIAL_BLOCKS macro with support for a partial final block.
;;; num_initial_blocks is expected to include the partial final block
;;;     in the count.
%macro INITIAL_BLOCKS_PARTIAL 25
%define %%GDATA_KEY             %1
%define %%GDATA_CTX             %2
%define %%CYPH_PLAIN_OUT        %3
%define %%PLAIN_CYPH_IN         %4
%define %%LENGTH                %5
%define %%DATA_OFFSET           %6
%define %%num_initial_blocks    %7  ; can be 1, 2, 3, 4, 5, 6 or 7 (not 0)
%define %%T1                    %8
%define %%T2                    %9
%define %%T3                    %10 ; [out] hash value
%define %%T4                    %11
%define %%T5                    %12
%define %%CTR                   %13
%define %%XMM1                  %14
%define %%XMM2                  %15
%define %%XMM3                  %16
%define %%XMM4                  %17
%define %%XMM5                  %18
%define %%XMM6                  %19
%define %%XMM7                  %20
%define %%XMM8                  %21 ; [in] hash value
%define %%T6                    %22
%define %%T_key                 %23
%define %%ENC_DEC               %24
%define %%INSTANCE_TYPE         %25

                ;; Move AAD_HASH to temp reg
                vmovdqu  %%T2, %%XMM8

%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                ;; Compute AES counters
                vpaddd   %%CTR, %%CTR, [rel ONE]     ; INCR Y0
                vmovdqa  reg(i), %%CTR
                vpshufb  reg(i), [rel SHUF_MASK]     ; perform a 16Byte swap
%assign i (i+1)
%endrep

vmovdqu  %%T_key, [%%GDATA_KEY+16*0]
%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                ; Start AES for %%num_initial_blocks blocks
                vpxor    reg(i),reg(i),%%T_key
%assign i (i+1)
%endrep

%assign j 1
%rep NROUNDS
vmovdqu  %%T_key, [%%GDATA_KEY+16*j]
%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                vaesenc  reg(i),%%T_key
%assign i (i+1)
%endrep

%assign j (j+1)
%endrep


vmovdqu  %%T_key, [%%GDATA_KEY+16*j]
%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks
                vaesenclast      reg(i),%%T_key
%assign i (i+1)
%endrep

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Hash all but the last block of data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%assign i (9-%%num_initial_blocks)
%rep %%num_initial_blocks-1
                ;; Encrypt the message for all but the last block
                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET]
                vpxor    reg(i), reg(i), %%T1
                ;; write back ciphertext for %%num_initial_blocks blocks
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET], reg(i)
                add     %%DATA_OFFSET, 16
%ifidn  %%ENC_DEC, DEC
                vmovdqa  reg(i), %%T1
%endif
                ;; Prepare ciphertext for GHASH computations
                vpshufb  reg(i), [rel SHUF_MASK]
%assign i (i+1)
%endrep

%if %%num_initial_blocks > 1
                ;; The final block of data may be <16B
                sub      %%LENGTH, 16*(%%num_initial_blocks-1)
%endif

%if %%num_initial_blocks < 8
                ;; NOTE: the 'jl' is always taken for num_initial_blocks = 8.
                ;;      This is run in the context of GCM_ENC_DEC_SMALL for length < 128.
                cmp      %%LENGTH, 16
                jl       %%_small_initial_partial_block

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Handle a full length final block - encrypt and hash all blocks
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                sub      %%LENGTH, 16
	        mov	[%%GDATA_CTX + PBlockLen], %%LENGTH

                ;; Encrypt the message
                VXLDR  %%T1, [%%PLAIN_CYPH_IN + %%DATA_OFFSET]
                vpxor    reg(i), reg(i), %%T1
                ;; write back ciphertext for %%num_initial_blocks blocks
                VXSTR  [%%CYPH_PLAIN_OUT + %%DATA_OFFSET], reg(i)
                add     %%DATA_OFFSET, 16
%ifidn  %%ENC_DEC, DEC
                vmovdqa  reg(i), %%T1
%endif
                ;; Prepare ciphertext for GHASH computations
                vpshufb  reg(i), [rel SHUF_MASK]

        ;; Hash all of the data
%assign i (8-%%num_initial_blocks)
%assign j (9-%%num_initial_blocks)
%assign k (%%num_initial_blocks)
%assign last_block_to_hash 0

%if(%%num_initial_blocks>last_block_to_hash)
        ;; Hash in AES state
        vpxor %%T2, reg(j)

        ;; T2 - incoming AAD hash
        ;; reg(i) holds ciphertext
        ;; T5 - hash key
        ;; T6 - updated xor
        ;; reg(1)/xmm1 should now be available for tmp use
        vmovdqu         %%T5, [%%GDATA_KEY + HashKey_ %+ k]
        vpclmulqdq      %%T1, %%T2, %%T5, 0x11             ; %%T4 = a1*b1
        vpclmulqdq      %%T4, %%T2, %%T5, 0x00             ; %%T4 = a0*b0
        vpclmulqdq      %%T6, %%T2, %%T5, 0x01             ; %%T6 = a1*b0
        vpclmulqdq      %%T5, %%T2, %%T5, 0x10             ; %%T5 = a0*b1
        vpxor           %%T6, %%T6, %%T5
%endif

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%assign rep_count (%%num_initial_blocks-1)
%rep rep_count

        vmovdqu         %%T5, [%%GDATA_KEY + HashKey_ %+ k]
        vpclmulqdq      %%T3, reg(j), %%T5, 0x11
        vpxor           %%T1, %%T1, %%T3

        vpclmulqdq      %%T3, reg(j), %%T5, 0x00
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, reg(j), %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, reg(j), %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%endrep

        ;; Record that a reduction is needed
        mov      r12, 1

        jmp      %%_small_initial_compute_hash


%endif                          ; %if %%num_initial_blocks < 8

%%_small_initial_partial_block:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Handle ghash for a <16B final block
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;; In this case if it's a single call to encrypt we can
        ;; hash all of the data but if it's an init / update / finalize
        ;; series of call we need to leave the last block if it's
        ;; less than a full block of data.

	mov	[%%GDATA_CTX + PBlockLen], %%LENGTH
        vmovdqu [%%GDATA_CTX + PBlockEncKey], reg(i)
        ;; Handle a partial final block
        ;;                            GDATA,    KEY,   T1,   T2
        ;; r13 - length
        ;; LT16 - indicates type of read and that the buffer is less than 16 bytes long
        ;;      NOTE: could be replaced with %%LENGTH but at this point
        ;;      %%LENGTH is always less than 16.
        ;;      No PLAIN_CYPH_LEN argument available in this macro.
        ENCRYPT_FINAL_PARTIAL_BLOCK reg(i), %%T1, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, LT16, %%ENC_DEC, %%DATA_OFFSET
        vpshufb  reg(i), [rel SHUF_MASK]

%ifidn %%INSTANCE_TYPE, multi_call
%assign i (8-%%num_initial_blocks)
%assign j (9-%%num_initial_blocks)
%assign k (%%num_initial_blocks-1)
%assign last_block_to_hash 1
%else
%assign i (8-%%num_initial_blocks)
%assign j (9-%%num_initial_blocks)
%assign k (%%num_initial_blocks)
%assign last_block_to_hash 0
%endif

%if(%%num_initial_blocks>last_block_to_hash)
        ;; Record that a reduction is needed
        mov            r12, 1
        ;; Hash in AES state
        vpxor          %%T2, reg(j)

        ;; T2 - incoming AAD hash
        ;; reg(i) holds ciphertext
        ;; T5 - hash key
        ;; T6 - updated xor
        ;; reg(1)/xmm1 should now be available for tmp use
        vmovdqu         %%T5, [%%GDATA_KEY + HashKey_ %+ k]
        vpclmulqdq      %%T1, %%T2, %%T5, 0x11             ; %%T4 = a1*b1
        vpclmulqdq      %%T4, %%T2, %%T5, 0x00             ; %%T4 = a0*b0
        vpclmulqdq      %%T6, %%T2, %%T5, 0x01             ; %%T6 = a1*b0
        vpclmulqdq      %%T5, %%T2, %%T5, 0x10             ; %%T5 = a0*b1
        vpxor           %%T6, %%T6, %%T5
%else
        ;; Record that a reduction is not needed -
        ;; In this case no hashes are computed because there
        ;; is only one initial block and it is < 16B in length.
        xor             r12, r12
%endif

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%ifidn %%INSTANCE_TYPE, multi_call
%assign rep_count (%%num_initial_blocks-2)
%%_multi_call_hash:
%else
%assign rep_count (%%num_initial_blocks-1)
%endif

%if rep_count < 0
        ;; fix for negative rep_count
%assign rep_count 0
%endif

%rep rep_count

        vmovdqu         %%T5, [%%GDATA_KEY + HashKey_ %+ k]
        vpclmulqdq      %%T3, reg(j), %%T5, 0x11
        vpxor           %%T1, %%T1, %%T3

        vpclmulqdq      %%T3, reg(j), %%T5, 0x00
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, reg(j), %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, reg(j), %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

%assign i (i+1)
%assign j (j+1)
%assign k (k-1)
%endrep

%%_small_initial_compute_hash:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Ghash reduction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%if(%%num_initial_blocks=1)
%ifidn %%INSTANCE_TYPE, multi_call
        ;; We only need to check if a reduction is needed if
        ;; initial_blocks == 1 and init/update/final is being used.
        ;; In this case we may just have a partial block, and that
        ;; gets hashed in finalize.
        ;; cmp     r12, 0
        or      r12, r12
        je      %%_no_reduction_needed
%endif
%endif

        vpsrldq %%T3, %%T6, 8          ; shift-R %%T2 2 DWs
        vpslldq %%T6, %%T6, 8          ; shift-L %%T3 2 DWs
        vpxor   %%T1, %%T1, %%T3       ; accumulate the results in %%T1:%%T4
        vpxor   %%T4, %%T6, %%T4

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; First phase of the reduction
        vmovdqu         %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%T4, 0x01
        ;; shift-L xmm2 2 DWs
        vpslldq         %%T2, %%T2, 8
        vpxor           %%T4, %%T4, %%T2

        ;; First phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; Second phase of the reduction

        vpclmulqdq      %%T2, %%T3, %%T4, 0x00
        ;; Shift-R xmm2 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)
        vpsrldq         %%T2, %%T2, 4

        vpclmulqdq      %%T4, %%T3, %%T4, 0x10
        ;; Shift-L xmm0 1 DW (Shift-L 1-DW to obtain result with no shifts)
        vpslldq         %%T4, %%T4, 4

        vpxor           %%T4, %%T4, %%T2
        ;; Second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%T3, %%T1, %%T4

%ifidn %%INSTANCE_TYPE, multi_call
        ;; If using init/update/finalize, we need to xor any partial block data
        ;; into the hash.
%if %%num_initial_blocks > 1
        ;; NOTE: for %%num_initial_blocks = 0 the xor never takes place
%if %%num_initial_blocks != 8
        ;; NOTE: for %%num_initial_blocks = 8, %%LENGTH, stored in [PBlockLen] is never zero
        cmp             qword [%%GDATA_CTX + PBlockLen], 0
        je              %%_no_partial_block_xor
%endif                          ; %%num_initial_blocks != 8
        vpxor           %%T3, %%T3, reg(8)
%%_no_partial_block_xor:
%endif                          ; %%num_initial_blocks > 1
%endif                          ; %%INSTANCE_TYPE, multi_call

%if(%%num_initial_blocks=1)
%ifidn %%INSTANCE_TYPE, multi_call
        ;; NOTE: %%_no_reduction_needed case only valid for
        ;;      multi_call with initial_blocks = 1.
        ;; Look for comment above around '_no_reduction_needed'
        ;; The jmp below is obsolete as the code will fall through.

        ;; The result is in %%T3
        jmp             %%_after_reduction

%%_no_reduction_needed:
        ;; The hash should end up in T3. The only way we should get here is if
        ;; there is a partial block of data, so xor that into the hash.
        vpxor            %%T3, %%T2, reg(8)
%endif                          ; %%INSTANCE_TYPE = multi_call
%endif                          ; %%num_initial_blocks=1

%%_after_reduction:
        ;; Final hash is now in T3

%endmacro                       ; INITIAL_BLOCKS_PARTIAL



; encrypt 8 blocks at a time
; ghash the 8 previously encrypted ciphertext blocks
; %%GDATA (KEY), %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN are used as pointers only, not modified
; %%DATA_OFFSET is the data offset value
%macro  GHASH_8_ENCRYPT_8_PARALLEL 23
%define %%GDATA                 %1
%define %%CYPH_PLAIN_OUT        %2
%define %%PLAIN_CYPH_IN         %3
%define %%DATA_OFFSET           %4
%define %%T1    %5
%define %%T2    %6
%define %%T3    %7
%define %%T4    %8
%define %%T5    %9
%define %%T6    %10
%define %%CTR   %11
%define %%XMM1  %12
%define %%XMM2  %13
%define %%XMM3  %14
%define %%XMM4  %15
%define %%XMM5  %16
%define %%XMM6  %17
%define %%XMM7  %18
%define %%XMM8  %19
%define %%T7    %20
%define %%loop_idx      %21
%define %%ENC_DEC       %22
%define %%FULL_PARTIAL  %23

        vmovdqa %%T2, %%XMM1
        vmovdqu [rsp + TMP2], %%XMM2
        vmovdqu [rsp + TMP3], %%XMM3
        vmovdqu [rsp + TMP4], %%XMM4
        vmovdqu [rsp + TMP5], %%XMM5
        vmovdqu [rsp + TMP6], %%XMM6
        vmovdqu [rsp + TMP7], %%XMM7
        vmovdqu [rsp + TMP8], %%XMM8

%ifidn %%loop_idx, in_order
                vpaddd  %%XMM1, %%CTR,  [rel ONE]           ; INCR CNT
                vmovdqu %%T5, [rel TWO]
                vpaddd  %%XMM2, %%CTR, %%T5
                vpaddd  %%XMM3, %%XMM1, %%T5
                vpaddd  %%XMM4, %%XMM2, %%T5
                vpaddd  %%XMM5, %%XMM3, %%T5
                vpaddd  %%XMM6, %%XMM4, %%T5
                vpaddd  %%XMM7, %%XMM5, %%T5
                vpaddd  %%XMM8, %%XMM6, %%T5
                vmovdqa %%CTR, %%XMM8

                vmovdqu %%T5, [rel SHUF_MASK]
                vpshufb %%XMM1, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM2, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM3, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM4, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM5, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM6, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM7, %%T5             ; perform a 16Byte swap
                vpshufb %%XMM8, %%T5             ; perform a 16Byte swap
%else
                vpaddd  %%XMM1, %%CTR,  [rel ONEf]          ; INCR CNT
                vmovdqu %%T5, [rel TWOf]
                vpaddd  %%XMM2, %%CTR,  %%T5
                vpaddd  %%XMM3, %%XMM1, %%T5
                vpaddd  %%XMM4, %%XMM2, %%T5
                vpaddd  %%XMM5, %%XMM3, %%T5
                vpaddd  %%XMM6, %%XMM4, %%T5
                vpaddd  %%XMM7, %%XMM5, %%T5
                vpaddd  %%XMM8, %%XMM6, %%T5
                vmovdqa %%CTR, %%XMM8
%endif



        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                vmovdqu %%T1, [%%GDATA + 16*0]
                vpxor   %%XMM1, %%XMM1, %%T1
                vpxor   %%XMM2, %%XMM2, %%T1
                vpxor   %%XMM3, %%XMM3, %%T1
                vpxor   %%XMM4, %%XMM4, %%T1
                vpxor   %%XMM5, %%XMM5, %%T1
                vpxor   %%XMM6, %%XMM6, %%T1
                vpxor   %%XMM7, %%XMM7, %%T1
                vpxor   %%XMM8, %%XMM8, %%T1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                vmovdqu %%T1, [%%GDATA + 16*1]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1


                vmovdqu %%T1, [%%GDATA + 16*2]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_8]
        vpclmulqdq      %%T4, %%T2, %%T5, 0x11                  ; %%T4 = a1*b1
        vpclmulqdq      %%T7, %%T2, %%T5, 0x00                  ; %%T7 = a0*b0
        vpclmulqdq      %%T6, %%T2, %%T5, 0x01                  ; %%T6 = a1*b0
        vpclmulqdq      %%T5, %%T2, %%T5, 0x10                  ; %%T5 = a0*b1
        vpxor           %%T6, %%T6, %%T5

                vmovdqu %%T1, [%%GDATA + 16*3]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1

        vmovdqu         %%T1, [rsp + TMP2]
        vmovdqu         %%T5, [%%GDATA + HashKey_7]
        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

                vmovdqu %%T1, [%%GDATA + 16*4]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vmovdqu         %%T1, [rsp + TMP3]
        vmovdqu         %%T5, [%%GDATA + HashKey_6]
        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

                vmovdqu %%T1, [%%GDATA + 16*5]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1


        vmovdqu         %%T1, [rsp + TMP4]
        vmovdqu         %%T5, [%%GDATA + HashKey_5]
        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

                vmovdqu %%T1, [%%GDATA + 16*6]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1

        vmovdqu         %%T1, [rsp + TMP5]
        vmovdqu         %%T5, [%%GDATA + HashKey_4]
        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

                vmovdqu %%T1, [%%GDATA + 16*7]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1

        vmovdqu         %%T1, [rsp + TMP6]
        vmovdqu         %%T5, [%%GDATA + HashKey_3]
        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

                vmovdqu %%T1, [%%GDATA + 16*8]
                vaesenc %%XMM1, %%T1
                vaesenc %%XMM2, %%T1
                vaesenc %%XMM3, %%T1
                vaesenc %%XMM4, %%T1
                vaesenc %%XMM5, %%T1
                vaesenc %%XMM6, %%T1
                vaesenc %%XMM7, %%T1
                vaesenc %%XMM8, %%T1

        vmovdqu         %%T1, [rsp + TMP7]
        vmovdqu         %%T5, [%%GDATA + HashKey_2]
        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T4, %%T4, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                vmovdqu %%T5, [%%GDATA + 16*9]
                vaesenc %%XMM1, %%T5
                vaesenc %%XMM2, %%T5
                vaesenc %%XMM3, %%T5
                vaesenc %%XMM4, %%T5
                vaesenc %%XMM5, %%T5
                vaesenc %%XMM6, %%T5
                vaesenc %%XMM7, %%T5
                vaesenc %%XMM8, %%T5

        vmovdqu         %%T1, [rsp + TMP8]
        vmovdqu         %%T5, [%%GDATA + HashKey]


        vpclmulqdq      %%T3, %%T1, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x01
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x10
        vpxor           %%T6, %%T6, %%T3

        vpclmulqdq      %%T3, %%T1, %%T5, 0x11
        vpxor           %%T1, %%T4, %%T3


                vmovdqu %%T5, [%%GDATA + 16*10]
 %ifndef GCM128_MODE            ; GCM192 or GCM256
                vaesenc %%XMM1, %%T5
                vaesenc %%XMM2, %%T5
                vaesenc %%XMM3, %%T5
                vaesenc %%XMM4, %%T5
                vaesenc %%XMM5, %%T5
                vaesenc %%XMM6, %%T5
                vaesenc %%XMM7, %%T5
                vaesenc %%XMM8, %%T5

                vmovdqu %%T5, [%%GDATA + 16*11]
                vaesenc %%XMM1, %%T5
                vaesenc %%XMM2, %%T5
                vaesenc %%XMM3, %%T5
                vaesenc %%XMM4, %%T5
                vaesenc %%XMM5, %%T5
                vaesenc %%XMM6, %%T5
                vaesenc %%XMM7, %%T5
                vaesenc %%XMM8, %%T5

                vmovdqu %%T5, [%%GDATA + 16*12]
%endif
%ifdef GCM256_MODE
                vaesenc %%XMM1, %%T5
                vaesenc %%XMM2, %%T5
                vaesenc %%XMM3, %%T5
                vaesenc %%XMM4, %%T5
                vaesenc %%XMM5, %%T5
                vaesenc %%XMM6, %%T5
                vaesenc %%XMM7, %%T5
                vaesenc %%XMM8, %%T5

                vmovdqu %%T5, [%%GDATA + 16*13]
                vaesenc %%XMM1, %%T5
                vaesenc %%XMM2, %%T5
                vaesenc %%XMM3, %%T5
                vaesenc %%XMM4, %%T5
                vaesenc %%XMM5, %%T5
                vaesenc %%XMM6, %%T5
                vaesenc %%XMM7, %%T5
                vaesenc %%XMM8, %%T5

                vmovdqu %%T5, [%%GDATA + 16*14]
%endif                          ; GCM256

%assign i 0
%assign j 1
%rep 8

        ;; SNP TBD: This is pretty ugly - consider whether just XORing the
        ;; data in after vaesenclast is simpler and performant. Would
        ;; also have to ripple it through partial block and ghash_mul_8.
%ifidn %%FULL_PARTIAL, full
    %ifdef  NT_LD
        VXLDR   %%T2, [%%PLAIN_CYPH_IN+%%DATA_OFFSET+16*i]
        vpxor   %%T2, %%T2, %%T5
    %else
        vpxor   %%T2, %%T5, [%%PLAIN_CYPH_IN+%%DATA_OFFSET+16*i]
    %endif

    %ifidn %%ENC_DEC, ENC
        vaesenclast     reg(j), reg(j), %%T2
    %else
        vaesenclast     %%T3, reg(j), %%T2
        vpxor   reg(j), %%T2, %%T5
        VXSTR [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*i], %%T3
    %endif

%else
    ; Don't read the final data during partial block processing
    %ifdef  NT_LD
        %if (i<7)
            VXLDR   %%T2, [%%PLAIN_CYPH_IN+%%DATA_OFFSET+16*i]
            vpxor   %%T2, %%T2, %%T5
        %else
            ;; Stage the key directly in T2 rather than hash it with plaintext
            vmovdqu %%T2, %%T5
        %endif
    %else
        %if (i<7)
            vpxor   %%T2, %%T5, [%%PLAIN_CYPH_IN+%%DATA_OFFSET+16*i]
        %else
            ;; Stage the key directly in T2 rather than hash it with plaintext
            vmovdqu %%T2, %%T5
        %endif
    %endif

    %ifidn %%ENC_DEC, ENC
        vaesenclast     reg(j), reg(j), %%T2
    %else
        %if (i<7)
            vaesenclast     %%T3, reg(j), %%T2
            vpxor   reg(j), %%T2, %%T5
            ;; Do not read the data since it could fault
            VXSTR [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*i], %%T3
        %else
            vaesenclast     reg(j), reg(j), %%T2
        %endif
    %endif
%endif

%assign i (i+1)
%assign j (j+1)
%endrep


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


        vpslldq %%T3, %%T6, 8                                   ; shift-L %%T3 2 DWs
        vpsrldq %%T6, %%T6, 8                                   ; shift-R %%T2 2 DWs
        vpxor   %%T7, %%T7, %%T3
        vpxor   %%T1, %%T1, %%T6                                ; accumulate the results in %%T1:%%T7



        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqu         %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%T7, 0x01
        vpslldq         %%T2, %%T2, 8                           ; shift-L xmm2 2 DWs

        vpxor           %%T7, %%T7, %%T2                        ; first phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    %ifidn %%ENC_DEC, ENC
        ; Write to the Ciphertext buffer
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*0], %%XMM1
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*1], %%XMM2
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*2], %%XMM3
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*3], %%XMM4
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*4], %%XMM5
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*5], %%XMM6
        VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*6], %%XMM7
        %ifidn %%FULL_PARTIAL, full
            ;; Avoid writing past the buffer if handling a partial block
            VXSTR   [%%CYPH_PLAIN_OUT+%%DATA_OFFSET+16*7], %%XMM8
        %endif
    %endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%T2, %%T3, %%T7, 0x00
        vpsrldq         %%T2, %%T2, 4                                   ; shift-R xmm2 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%T4, %%T3, %%T7, 0x10
        vpslldq         %%T4, %%T4, 4                                   ; shift-L xmm0 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%T4, %%T4, %%T2                                ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%T1, %%T1, %%T4                                ; the result is in %%T1

                vpshufb %%XMM1, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb %%XMM2, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb %%XMM3, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb %%XMM4, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb %%XMM5, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb %%XMM6, [rel SHUF_MASK]             ; perform a 16Byte swap
                vpshufb %%XMM7, [rel SHUF_MASK]             ; perform a 16Byte swap
        vpshufb %%XMM8, [rel SHUF_MASK]             ; perform a 16Byte swap


        vpxor   %%XMM1, %%T1


%endmacro                       ; GHASH_8_ENCRYPT_8_PARALLEL


; GHASH the last 4 ciphertext blocks.
%macro  GHASH_LAST_8 16
%define %%GDATA %1
%define %%T1    %2
%define %%T2    %3
%define %%T3    %4
%define %%T4    %5
%define %%T5    %6
%define %%T6    %7
%define %%T7    %8
%define %%XMM1  %9
%define %%XMM2  %10
%define %%XMM3  %11
%define %%XMM4  %12
%define %%XMM5  %13
%define %%XMM6  %14
%define %%XMM7  %15
%define %%XMM8  %16

        ;; Karatsuba Method

        vmovdqu         %%T5, [%%GDATA + HashKey_8]

        vpshufd         %%T2, %%XMM1, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM1
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T6, %%XMM1, %%T5, 0x11
        vpclmulqdq      %%T7, %%XMM1, %%T5, 0x00

        vpclmulqdq      %%XMM1, %%T2, %%T3, 0x00

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_7]
        vpshufd         %%T2, %%XMM2, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM2
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM2, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM2, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_6]
        vpshufd         %%T2, %%XMM3, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM3
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM3, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM3, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_5]
        vpshufd         %%T2, %%XMM4, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM4
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM4, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM4, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_4]
        vpshufd         %%T2, %%XMM5, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM5
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM5, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM5, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_3]
        vpshufd         %%T2, %%XMM6, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM6
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM6, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM6, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_2]
        vpshufd         %%T2, %%XMM7, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM7
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM7, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM7, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey]
        vpshufd         %%T2, %%XMM8, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM8
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM8, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM8, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2
        vpxor           %%XMM1, %%XMM1, %%T6
        vpxor           %%T2, %%XMM1, %%T7




        vpslldq %%T4, %%T2, 8
        vpsrldq %%T2, %%T2, 8

        vpxor   %%T7, %%T7, %%T4
        vpxor   %%T6, %%T6, %%T2                               ; <%%T6:%%T7> holds the result of the accumulated carry-less multiplications

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqu         %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%T7, 0x01
        vpslldq         %%T2, %%T2, 8                           ; shift-L xmm2 2 DWs

        vpxor           %%T7, %%T7, %%T2                        ; first phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


        ;second phase of the reduction
        vpclmulqdq      %%T2, %%T3, %%T7, 0x00
        vpsrldq         %%T2, %%T2, 4                           ; shift-R %%T2 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%T4, %%T3, %%T7, 0x10
        vpslldq         %%T4, %%T4, 4                           ; shift-L %%T4 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%T4, %%T4, %%T2                        ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%T6, %%T6, %%T4                        ; the result is in %%T6
%endmacro


; GHASH the last 4 ciphertext blocks.
%macro  GHASH_LAST_7 15
%define %%GDATA %1
%define %%T1    %2
%define %%T2    %3
%define %%T3    %4
%define %%T4    %5
%define %%T5    %6
%define %%T6    %7
%define %%T7    %8
%define %%XMM1  %9
%define %%XMM2  %10
%define %%XMM3  %11
%define %%XMM4  %12
%define %%XMM5  %13
%define %%XMM6  %14
%define %%XMM7  %15

        ;; Karatsuba Method

        vmovdqu         %%T5, [%%GDATA + HashKey_7]

        vpshufd         %%T2, %%XMM1, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM1
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T6, %%XMM1, %%T5, 0x11
        vpclmulqdq      %%T7, %%XMM1, %%T5, 0x00

        vpclmulqdq      %%XMM1, %%T2, %%T3, 0x00

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_6]
        vpshufd         %%T2, %%XMM2, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM2
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM2, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM2, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_5]
        vpshufd         %%T2, %%XMM3, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM3
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM3, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM3, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_4]
        vpshufd         %%T2, %%XMM4, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM4
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM4, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM4, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_3]
        vpshufd         %%T2, %%XMM5, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM5
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM5, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM5, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_2]
        vpshufd         %%T2, %%XMM6, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM6
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM6, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM6, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vmovdqu         %%T5, [%%GDATA + HashKey_1]
        vpshufd         %%T2, %%XMM7, 01001110b
        vpshufd         %%T3, %%T5, 01001110b
        vpxor           %%T2, %%T2, %%XMM7
        vpxor           %%T3, %%T3, %%T5

        vpclmulqdq      %%T4, %%XMM7, %%T5, 0x11
        vpxor           %%T6, %%T6, %%T4

        vpclmulqdq      %%T4, %%XMM7, %%T5, 0x00
        vpxor           %%T7, %%T7, %%T4

        vpclmulqdq      %%T2, %%T2, %%T3, 0x00

        vpxor           %%XMM1, %%XMM1, %%T2

        ;;;;;;;;;;;;;;;;;;;;;;

        vpxor           %%XMM1, %%XMM1, %%T6
        vpxor           %%T2, %%XMM1, %%T7




        vpslldq %%T4, %%T2, 8
        vpsrldq %%T2, %%T2, 8

        vpxor   %%T7, %%T7, %%T4
        vpxor   %%T6, %%T6, %%T2                               ; <%%T6:%%T7> holds the result of the accumulated carry-less multiplications

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqu         %%T3, [rel POLY2]

        vpclmulqdq      %%T2, %%T3, %%T7, 0x01
        vpslldq         %%T2, %%T2, 8                           ; shift-L xmm2 2 DWs

        vpxor           %%T7, %%T7, %%T2                        ; first phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


        ;second phase of the reduction
        vpclmulqdq      %%T2, %%T3, %%T7, 0x00
        vpsrldq         %%T2, %%T2, 4                           ; shift-R %%T2 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%T4, %%T3, %%T7, 0x10
        vpslldq         %%T4, %%T4, 4                           ; shift-L %%T4 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%T4, %%T4, %%T2                        ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%T6, %%T6, %%T4                        ; the result is in %%T6
%endmacro



;;; Handle encryption of the final partial block
;;; IN:
;;;   r13  - Number of bytes to read
;;; MODIFIES:
;;;   KEY  - Key for encrypting the partial block
;;; SMASHES:
;;;   rax, T1
;;; Note:
;;;   PLAIN_CYPH_LEN is unused at this stage. Previously:
;;;     it was used  to determine if buffer is big enough to do
;;;     a 16 byte read & shift.
;;;     'LT16' is passed here only if buffer is known to be smaller
;;;     than 16 bytes.
;;;     Any other value passed here will result in 16 byte read
;;;     code path.
%macro  ENCRYPT_FINAL_PARTIAL_BLOCK 7
%define %%KEY             %1
%define %%T1              %2
%define %%CYPH_PLAIN_OUT  %3
%define %%PLAIN_CYPH_IN   %4
%define %%PLAIN_CYPH_LEN  %5
%define %%ENC_DEC         %6
%define %%DATA_OFFSET     %7

        ;; %%PLAIN_CYPH_IN + %%DATA_OFFSET
        ;;               - input data address
        ;; r13           - input data length
        ;; rax           - temp registers
        ;; out:
        ;; T1            - packed output
        ;; k1            - valid byte mask
        READ_SMALL_DATA_INPUT   %%T1, %%PLAIN_CYPH_IN+%%DATA_OFFSET, r13, rax

        ;; At this point T1 contains the partial block data
        ;; Plaintext XOR E(K, Yn)
        vpxorq          %%KEY, %%KEY, %%T1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; Output r13 Bytes
        vmovdqu8        [%%CYPH_PLAIN_OUT + %%DATA_OFFSET]{k1}, %%KEY

%ifidn  %%ENC_DEC, DEC
        ;; If decrypt, restore the ciphertext into %%KEY
        vmovdqa64       %%KEY, %%T1
%else
        vmovdqu8        %%KEY{k1}{z}, %%KEY
%endif
%endmacro                       ; ENCRYPT_FINAL_PARTIAL_BLOCK



; Encryption of a single block
%macro  ENCRYPT_SINGLE_BLOCK 2
%define %%GDATA %1
%define %%XMM0  %2

                vpxor    %%XMM0, %%XMM0, [%%GDATA+16*0]
%assign i 1
%rep NROUNDS
                vaesenc  %%XMM0, [%%GDATA+16*i]
%assign i (i+1)
%endrep
                vaesenclast      %%XMM0, [%%GDATA+16*i]
%endmacro


;; Start of Stack Setup

%macro FUNC_SAVE 0
        ;; Required for Update/GMC_ENC
        ;the number of pushes must equal STACK_OFFSET
        push    r12
        push    r13
        push    r14
        push    r15
        mov     r14, rsp

        sub     rsp, VARIABLE_OFFSET
        and     rsp, ~63

%ifidn __OUTPUT_FORMAT__, win64
        ; xmm6:xmm15 need to be maintained for Windows
        vmovdqu [rsp + LOCAL_STORAGE + 0*16],xmm6
        vmovdqu [rsp + LOCAL_STORAGE + 1*16],xmm7
        vmovdqu [rsp + LOCAL_STORAGE + 2*16],xmm8
        vmovdqu [rsp + LOCAL_STORAGE + 3*16],xmm9
        vmovdqu [rsp + LOCAL_STORAGE + 4*16],xmm10
        vmovdqu [rsp + LOCAL_STORAGE + 5*16],xmm11
        vmovdqu [rsp + LOCAL_STORAGE + 6*16],xmm12
        vmovdqu [rsp + LOCAL_STORAGE + 7*16],xmm13
        vmovdqu [rsp + LOCAL_STORAGE + 8*16],xmm14
        vmovdqu [rsp + LOCAL_STORAGE + 9*16],xmm15
%endif
%endmacro


%macro FUNC_RESTORE 0

%ifdef SAFE_DATA
        clear_scratch_gps_asm
        clear_scratch_zmms_asm
%endif
%ifidn __OUTPUT_FORMAT__, win64
        vmovdqu xmm15, [rsp + LOCAL_STORAGE + 9*16]
        vmovdqu xmm14, [rsp + LOCAL_STORAGE + 8*16]
        vmovdqu xmm13, [rsp + LOCAL_STORAGE + 7*16]
        vmovdqu xmm12, [rsp + LOCAL_STORAGE + 6*16]
        vmovdqu xmm11, [rsp + LOCAL_STORAGE + 5*16]
        vmovdqu xmm10, [rsp + LOCAL_STORAGE + 4*16]
        vmovdqu xmm9, [rsp + LOCAL_STORAGE + 3*16]
        vmovdqu xmm8, [rsp + LOCAL_STORAGE + 2*16]
        vmovdqu xmm7, [rsp + LOCAL_STORAGE + 1*16]
        vmovdqu xmm6, [rsp + LOCAL_STORAGE + 0*16]
%endif
;; Required for Update/GMC_ENC
        mov     rsp, r14
        pop     r15
        pop     r14
        pop     r13
        pop     r12
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UPDATE_HASH: Calculates the hash of the data which will not be encrypted.
; Input: The input data (A_IN), that data's length (A_LEN), and the hash key (HASH_KEY).
; Output: The hash of the data (AAD_HASH).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  UPDATE_HASH   13
%define %%A_IN          %1
%define %%A_LEN         %2
%define %%AAD_HASH      %3
%define %%GDATA_KEY     %4
%define %%XTMP0         %5      ; xmm temp reg 5
%define %%XTMP1         %6      ; xmm temp reg 5
%define %%XTMP2         %7
%define %%XTMP3         %8
%define %%XTMP4         %9
%define %%XTMP5         %10     ; xmm temp reg 5
%define %%T1            %11     ; temp reg 1
%define %%T2            %12
%define %%T3            %13


        mov     %%T1, %%A_IN            ; T1 = AAD
        mov     %%T2, %%A_LEN           ; T2 = aadLen

%%_get_AAD_loop128:
        cmp     %%T2, 128
        jl      %%_exit_AAD_loop128

        vmovdqu         %%XTMP0, [%%T1 + 16*0]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vpxor           %%XTMP0, %%AAD_HASH

        vmovdqu         %%XTMP5, [%%GDATA_KEY + HashKey_8]
        vpclmulqdq      %%XTMP1, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = a1*b1
        vpclmulqdq      %%XTMP2, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = a0*b0
        vpclmulqdq      %%XTMP3, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = a1*b0
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10                 ; %%T4 = a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4                       ; %%T3 = a1*b0 + a0*b1

%assign i 1
%assign j 7
%rep 7
        vmovdqu         %%XTMP0, [%%T1 + 16*i]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vmovdqu         %%XTMP5, [%%GDATA_KEY + HashKey_ %+ j]
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = T1 + a1*b1
        vpxor           %%XTMP1, %%XTMP1, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = T2 + a0*b0
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = T3 + a1*b0 + a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4
%assign i (i + 1)
%assign j (j - 1)
%endrep

        vpslldq         %%XTMP4, %%XTMP3, 8                             ; shift-L 2 DWs
        vpsrldq         %%XTMP3, %%XTMP3, 8                             ; shift-R 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4
        vpxor           %%XTMP1, %%XTMP1, %%XTMP3                       ; accumulate the results in %%T1(M):%%T2(L)

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqa         %%XTMP5, [rel POLY2]
        vpclmulqdq      %%XTMP0, %%XTMP5, %%XTMP2, 0x01
        vpslldq         %%XTMP0, %%XTMP0, 8                             ; shift-L xmm2 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP0                       ; first phase of the reduction complete

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%XTMP3, %%XTMP5, %%XTMP2, 0x00
        vpsrldq         %%XTMP3, %%XTMP3, 4                             ; shift-R 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%XTMP4, %%XTMP5, %%XTMP2, 0x10
        vpslldq         %%XTMP4, %%XTMP4, 4                             ; shift-L 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%XTMP4, %%XTMP4, %%XTMP3                       ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%AAD_HASH, %%XTMP1, %%XTMP4                    ; the result is in %%T1

        sub     %%T2, 128
        je      %%_CALC_AAD_done

        add     %%T1, 128
        jmp     %%_get_AAD_loop128

%%_exit_AAD_loop128:
        cmp     %%T2, 16
        jl      %%_get_small_AAD_block

        ;; calculate hash_key position to start with
        mov     %%T3, %%T2
        and     %%T3, -16       ; 1 to 7 blocks possible here
        neg     %%T3
        add     %%T3, HashKey_1 + 16
        lea     %%T3, [%%GDATA_KEY + %%T3]

        vmovdqu         %%XTMP0, [%%T1]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vpxor           %%XTMP0, %%AAD_HASH

        vmovdqu         %%XTMP5, [%%T3]
        vpclmulqdq      %%XTMP1, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = a1*b1
        vpclmulqdq      %%XTMP2, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = a0*b0
        vpclmulqdq      %%XTMP3, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = a1*b0
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10                 ; %%T4 = a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4                       ; %%T3 = a1*b0 + a0*b1

        add     %%T3, 16        ; move to next hashkey
        add     %%T1, 16        ; move to next data block
        sub     %%T2, 16
        cmp     %%T2, 16
        jl      %%_AAD_reduce

%%_AAD_blocks:
        vmovdqu         %%XTMP0, [%%T1]
        vpshufb         %%XTMP0, [rel SHUF_MASK]

        vmovdqu         %%XTMP5, [%%T3]
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x11                 ; %%T1 = T1 + a1*b1
        vpxor           %%XTMP1, %%XTMP1, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x00                 ; %%T2 = T2 + a0*b0
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4

        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x01                 ; %%T3 = T3 + a1*b0 + a0*b1
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4
        vpclmulqdq      %%XTMP4, %%XTMP0, %%XTMP5, 0x10
        vpxor           %%XTMP3, %%XTMP3, %%XTMP4

        add     %%T3, 16        ; move to next hashkey
        add     %%T1, 16
        sub     %%T2, 16
        cmp     %%T2, 16
        jl      %%_AAD_reduce
        jmp     %%_AAD_blocks

%%_AAD_reduce:
        vpslldq         %%XTMP4, %%XTMP3, 8                             ; shift-L 2 DWs
        vpsrldq         %%XTMP3, %%XTMP3, 8                             ; shift-R 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP4
        vpxor           %%XTMP1, %%XTMP1, %%XTMP3                       ; accumulate the results in %%T1(M):%%T2(L)

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;first phase of the reduction
        vmovdqa         %%XTMP5, [rel POLY2]
        vpclmulqdq      %%XTMP0, %%XTMP5, %%XTMP2, 0x01
        vpslldq         %%XTMP0, %%XTMP0, 8                             ; shift-L xmm2 2 DWs
        vpxor           %%XTMP2, %%XTMP2, %%XTMP0                       ; first phase of the reduction complete

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;second phase of the reduction
        vpclmulqdq      %%XTMP3, %%XTMP5, %%XTMP2, 0x00
        vpsrldq         %%XTMP3, %%XTMP3, 4                             ; shift-R 1 DW (Shift-R only 1-DW to obtain 2-DWs shift-R)

        vpclmulqdq      %%XTMP4, %%XTMP5, %%XTMP2, 0x10
        vpslldq         %%XTMP4, %%XTMP4, 4                             ; shift-L 1 DW (Shift-L 1-DW to obtain result with no shifts)

        vpxor           %%XTMP4, %%XTMP4, %%XTMP3                       ; second phase of the reduction complete
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        vpxor           %%AAD_HASH, %%XTMP1, %%XTMP4                    ; the result is in %%T1

        or      %%T2, %%T2
        je      %%_CALC_AAD_done

%%_get_small_AAD_block:
        vmovdqu         %%XTMP0, [%%GDATA_KEY + HashKey]
        READ_SMALL_DATA_INPUT   %%XTMP1, %%T1, %%T2, %%T3
        ;byte-reflect the AAD data
        vpshufb         %%XTMP1, [rel SHUF_MASK]
        vpxor           %%AAD_HASH, %%XTMP1
        GHASH_MUL       %%AAD_HASH, %%XTMP0, %%XTMP1, %%XTMP2, %%XTMP3, %%XTMP4, %%XTMP5

%%_CALC_AAD_done:

%endmacro ; UPDATE_HASH

%macro CALC_J0 14
%define %%KEY           %1 ;; [in] Pointer to GCM KEY structure
%define %%IV            %2 ;; [in] Pointer to IV
%define %%IV_LEN        %3 ;; [in] IV length
%define %%J0            %4 ;; [out] XMM reg to contain J0
%define %%TMP0          %5 ;; [clobbered] Temporary GP reg
%define %%TMP1          %6 ;; [clobbered] Temporary GP reg
%define %%TMP2          %7 ;; [clobbered] Temporary GP reg
%define %%XTMP0         %8 ;; [clobbered] Temporary XMM reg
%define %%XTMP1         %9 ;; [clobbered] Temporary XMM reg
%define %%XTMP2         %10 ;; [clobbered] Temporary XMM reg
%define %%XTMP3         %11 ;; [clobbered] Temporary XMM reg
%define %%XTMP4         %12 ;; [clobbered] Temporary XMM reg
%define %%XTMP5         %13 ;; [clobbered] Temporary XMM reg
%define %%IV_GEN_LEN    %14 ;; [in] IV general length 

        ;; J0 = GHASH(IV || 0s+64 || len(IV)64)
        ;; s = 16 * RoundUp(len(IV)/16) -  len(IV) */

        ;; Calculate GHASH of (IV || 0s)
        UPDATE_HASH   %%IV, %%IV_LEN, %%J0, %%KEY, %%XTMP0, %%XTMP1, %%XTMP2, \
                      %%XTMP3, %%XTMP4, %%XTMP5, %%TMP0, %%TMP1, %%TMP2

        ;; Calculate GHASH of last 16-byte block (0 || len(IV)64)
        vmovdqu %%XTMP0, [%%KEY + HashKey]
        mov     %%TMP2, %%IV_GEN_LEN
        shl     %%TMP2, 3 ;; IV length in bits
        vmovq   %%XTMP1, %%TMP2
        vpxor   %%J0, %%XTMP1
        GHASH_MUL %%J0, %%XTMP0, %%XTMP1, %%XTMP2, %%XTMP3, %%XTMP4, %%XTMP5

        vpshufb %%J0, [rel SHUF_MASK] ; perform a 16Byte swap
%endmacro

%macro  GCM_ENC_DEC_SMALL   12
%define %%GDATA_KEY         %1
%define %%GDATA_CTX         %2
%define %%CYPH_PLAIN_OUT    %3
%define %%PLAIN_CYPH_IN     %4
%define %%PLAIN_CYPH_LEN    %5
%define %%ENC_DEC           %6
%define %%DATA_OFFSET       %7
%define %%LENGTH            %8  ; assumed r13
%define %%NUM_BLOCKS        %9
%define %%CTR               %10 ; assumed xmm9
%define %%HASH_OUT          %11 ; assumed xmm14
%define %%INSTANCE_TYPE     %12

        ;; NOTE: the check below is obsolete in current implementation. The check is already done in GCM_ENC_DEC.
        ;; cmp     %%NUM_BLOCKS, 0
        ;; je      %%_small_initial_blocks_encrypted
        cmp     %%NUM_BLOCKS, 8
        je      %%_small_initial_num_blocks_is_8
        cmp     %%NUM_BLOCKS, 7
        je      %%_small_initial_num_blocks_is_7
        cmp     %%NUM_BLOCKS, 6
        je      %%_small_initial_num_blocks_is_6
        cmp     %%NUM_BLOCKS, 5
        je      %%_small_initial_num_blocks_is_5
        cmp     %%NUM_BLOCKS, 4
        je      %%_small_initial_num_blocks_is_4
        cmp     %%NUM_BLOCKS, 3
        je      %%_small_initial_num_blocks_is_3
        cmp     %%NUM_BLOCKS, 2
        je      %%_small_initial_num_blocks_is_2

        jmp     %%_small_initial_num_blocks_is_1


%%_small_initial_num_blocks_is_8:
        ;; r13   - %%LENGTH
        ;; xmm12 - T1
        ;; xmm13 - T2
        ;; xmm14 - T3   - AAD HASH OUT when not producing 8 AES keys
        ;; xmm15 - T4
        ;; xmm11 - T5
        ;; xmm9  - CTR
        ;; xmm1  - XMM1 - Cipher + Hash when producing 8 AES keys
        ;; xmm2  - XMM2
        ;; xmm3  - XMM3
        ;; xmm4  - XMM4
        ;; xmm5  - XMM5
        ;; xmm6  - XMM6
        ;; xmm7  - XMM7
        ;; xmm8  - XMM8 - AAD HASH IN
        ;; xmm10 - T6
        ;; xmm0  - T_key
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 8, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_7:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 7, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_6:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 6, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_5:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 5, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_4:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 4, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_3:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 3, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_2:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 2, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
        jmp     %%_small_initial_blocks_encrypted

%%_small_initial_num_blocks_is_1:
        INITIAL_BLOCKS_PARTIAL  %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, \
                %%PLAIN_CYPH_IN, %%LENGTH, %%DATA_OFFSET, 1, \
                xmm12, xmm13, %%HASH_OUT, xmm15, xmm11, %%CTR, \
                xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, \
                xmm10, xmm0, %%ENC_DEC, %%INSTANCE_TYPE
%%_small_initial_blocks_encrypted:

%endmacro                       ; GCM_ENC_DEC_SMALL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GCM_ENC_DEC Encodes/Decodes given data. Assumes that the passed gcm_context_data struct
; has been initialized by GCM_INIT
; Requires the input data be at least 1 byte long because of READ_SMALL_INPUT_DATA.
; Input: gcm_key_data struct* (GDATA_KEY), gcm_context_data *(GDATA_CTX), input text (PLAIN_CYPH_IN),
; input text length (PLAIN_CYPH_LEN) and whether encoding or decoding (ENC_DEC).
; Output: A cypher of the given plain text (CYPH_PLAIN_OUT), and updated GDATA_CTX
; Clobbers rax, r10-r15, and xmm0-xmm15
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_ENC_DEC         7
%define %%GDATA_KEY         %1
%define %%GDATA_CTX         %2
%define %%CYPH_PLAIN_OUT    %3
%define %%PLAIN_CYPH_IN     %4
%define %%PLAIN_CYPH_LEN    %5
%define %%ENC_DEC           %6
%define %%INSTANCE_TYPE     %7
%define %%DATA_OFFSET       r11

; Macro flow:
; calculate the number of 16byte blocks in the message
; process (number of 16byte blocks) mod 8 '%%_initial_num_blocks_is_# .. %%_initial_blocks_encrypted'
; process 8 16 byte blocks at a time until all are done '%%_encrypt_by_8_new .. %%_eight_cipher_left'
; if there is a block of less tahn 16 bytes process it '%%_zero_cipher_left .. %%_multiple_of_16_bytes'

%ifidn __OUTPUT_FORMAT__, win64
        cmp     %%PLAIN_CYPH_LEN, 0
%else
        or      %%PLAIN_CYPH_LEN, %%PLAIN_CYPH_LEN
%endif
        je      %%_enc_dec_done

        xor     %%DATA_OFFSET, %%DATA_OFFSET
        ;; Update length of data processed
%ifidn __OUTPUT_FORMAT__, win64
        mov     rax, %%PLAIN_CYPH_LEN
       	add     [%%GDATA_CTX + InLen], rax
%else
        add    [%%GDATA_CTX + InLen], %%PLAIN_CYPH_LEN
%endif
        vmovdqu xmm13, [%%GDATA_KEY + HashKey]
        vmovdqu xmm8, [%%GDATA_CTX + AadHash]

%ifidn %%INSTANCE_TYPE, multi_call
        ;; NOTE: partial block processing makes only sense for multi_call here.
        ;; Used for the update flow - if there was a previous partial
        ;; block fill the remaining bytes here.
        PARTIAL_BLOCK %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, %%PLAIN_CYPH_LEN, %%DATA_OFFSET, xmm8, %%ENC_DEC
%endif

        ;;  lift CTR set from initial_blocks to here
%ifidn %%INSTANCE_TYPE, single_call
        vmovdqu xmm9, xmm2
%else
        vmovdqu xmm9, [%%GDATA_CTX + CurCount]
%endif

        ;; Save the amount of data left to process in r10
        mov     r13, %%PLAIN_CYPH_LEN
%ifidn %%INSTANCE_TYPE, multi_call
        ;; NOTE: %%DATA_OFFSET is zero in single_call case.
        ;;      Consequently PLAIN_CYPH_LEN will never be zero after
        ;;      %%DATA_OFFSET subtraction below.
        sub     r13, %%DATA_OFFSET

        ;; There may be no more data if it was consumed in the partial block.
        cmp     r13, 0
        je      %%_enc_dec_done
%endif                          ; %%INSTANCE_TYPE, multi_call
        mov     r10, r13

        ;; Determine how many blocks to process in INITIAL
        mov     r12, r13
        shr     r12, 4
        and     r12, 7

        ;; Process one additional block in INITIAL if there is a partial block
        and     r10, 0xf
        blsmsk  r10, r10    ; Set CF if zero
        cmc                 ; Flip CF
        adc     r12, 0x0    ; Process an additional INITIAL block if CF set

        ;;      Less than 127B will be handled by the small message code, which
        ;;      can process up to 7 16B blocks.
        cmp     r13, 128
        jge     %%_large_message_path

        GCM_ENC_DEC_SMALL %%GDATA_KEY, %%GDATA_CTX, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, %%PLAIN_CYPH_LEN, %%ENC_DEC, %%DATA_OFFSET, r13, r12, xmm9, xmm14, %%INSTANCE_TYPE
        jmp     %%_ghash_done

%%_large_message_path:
        and     r12, 0x7    ; Still, don't allow 8 INITIAL blocks since this will
                            ; can be handled by the x8 partial loop.

        cmp     r12, 0
        je      %%_initial_num_blocks_is_0
        cmp     r12, 7
        je      %%_initial_num_blocks_is_7
        cmp     r12, 6
        je      %%_initial_num_blocks_is_6
        cmp     r12, 5
        je      %%_initial_num_blocks_is_5
        cmp     r12, 4
        je      %%_initial_num_blocks_is_4
        cmp     r12, 3
        je      %%_initial_num_blocks_is_3
        cmp     r12, 2
        je      %%_initial_num_blocks_is_2

        jmp     %%_initial_num_blocks_is_1

%%_initial_num_blocks_is_7:
        ;; r13   - %%LENGTH
        ;; xmm12 - T1
        ;; xmm13 - T2
        ;; xmm14 - T3   - AAD HASH OUT when not producing 8 AES keys
        ;; xmm15 - T4
        ;; xmm11 - T5
        ;; xmm9  - CTR
        ;; xmm1  - XMM1 - Cipher + Hash when producing 8 AES keys
        ;; xmm2  - XMM2
        ;; xmm3  - XMM3
        ;; xmm4  - XMM4
        ;; xmm5  - XMM5
        ;; xmm6  - XMM6
        ;; xmm7  - XMM7
        ;; xmm8  - XMM8 - AAD HASH IN
        ;; xmm10 - T6
        ;; xmm0  - T_key
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 7, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_6:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 6, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_5:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 5, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_4:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 4, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_3:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 3, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_2:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 2, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_1:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 1, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC
        jmp     %%_initial_blocks_encrypted

%%_initial_num_blocks_is_0:
        INITIAL_BLOCKS  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, r13, %%DATA_OFFSET, 0, xmm12, xmm13, xmm14, xmm15, xmm11, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm10, xmm0, %%ENC_DEC


%%_initial_blocks_encrypted:
        ;; The entire message was encrypted processed in initial and now need to be hashed
        cmp     r13, 0
        je      %%_encrypt_done

        ;; Encrypt the final <16 byte (partial) block, then hash
        cmp     r13, 16
        jl      %%_encrypt_final_partial

        ;; Process 7 full blocks plus a partial block
        cmp     r13, 128
        jl      %%_encrypt_by_8_partial


%%_encrypt_by_8_parallel:
        ;; in_order vs. out_order is an optimization to increment the counter without shuffling
        ;; it back into little endian. r15d keeps track of when we need to increent in order so
        ;; that the carry is handled correctly.
        vmovd   r15d, xmm9
        and     r15d, 255
        vpshufb xmm9, [rel SHUF_MASK]


%%_encrypt_by_8_new:
        cmp     r15d, 255-8
        jg      %%_encrypt_by_8



        ;; xmm0  - T1
        ;; xmm10 - T2
        ;; xmm11 - T3
        ;; xmm12 - T4
        ;; xmm13 - T5
        ;; xmm14 - T6
        ;; xmm9  - CTR
        ;; xmm1  - XMM1
        ;; xmm2  - XMM2
        ;; xmm3  - XMM3
        ;; xmm4  - XMM4
        ;; xmm5  - XMM5
        ;; xmm6  - XMM6
        ;; xmm7  - XMM7
        ;; xmm8  - XMM8
        ;; xmm15 - T7
        add     r15b, 8
        GHASH_8_ENCRYPT_8_PARALLEL  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, %%DATA_OFFSET, xmm0, xmm10, xmm11, xmm12, xmm13, xmm14, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm15, out_order, %%ENC_DEC, full
        add     %%DATA_OFFSET, 128
        sub     r13, 128
        cmp     r13, 128
        jge     %%_encrypt_by_8_new

        vpshufb xmm9, [rel SHUF_MASK]
        jmp     %%_encrypt_by_8_parallel_done

%%_encrypt_by_8:
        vpshufb xmm9, [rel SHUF_MASK]
        add     r15b, 8
        GHASH_8_ENCRYPT_8_PARALLEL  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, %%DATA_OFFSET, xmm0, xmm10, xmm11, xmm12, xmm13, xmm14, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm15, in_order, %%ENC_DEC, full
        vpshufb  xmm9, [rel SHUF_MASK]
        add     %%DATA_OFFSET, 128
        sub     r13, 128
        cmp     r13, 128
        jge     %%_encrypt_by_8_new
        vpshufb  xmm9, [rel SHUF_MASK]


%%_encrypt_by_8_parallel_done:
        ;; Test to see if we need a by 8 with partial block. At this point
        ;; bytes remaining should be either zero or between 113-127.
        cmp     r13, 0
        je      %%_encrypt_done

%%_encrypt_by_8_partial:
        ;; Shuffle needed to align key for partial block xor. out_order
        ;; is a little faster because it avoids extra shuffles.
        ;; TBD: Might need to account for when we don't have room to increment the counter.


        ;; Process parallel buffers with a final partial block.
        GHASH_8_ENCRYPT_8_PARALLEL  %%GDATA_KEY, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, %%DATA_OFFSET, xmm0, xmm10, xmm11, xmm12, xmm13, xmm14, xmm9, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm15, in_order, %%ENC_DEC, partial


        add     %%DATA_OFFSET, 128-16
        sub     r13, 128-16

%%_encrypt_final_partial:

        vpshufb  xmm8, [rel SHUF_MASK]
        mov     [%%GDATA_CTX + PBlockLen], r13
        vmovdqu [%%GDATA_CTX + PBlockEncKey], xmm8

        ;; xmm8  - Final encrypted counter - need to hash with partial or full block ciphertext
        ;;                            GDATA,  KEY,   T1,    T2
        ENCRYPT_FINAL_PARTIAL_BLOCK xmm8, xmm0, %%CYPH_PLAIN_OUT, %%PLAIN_CYPH_IN, %%PLAIN_CYPH_LEN, %%ENC_DEC, %%DATA_OFFSET

        vpshufb  xmm8, [rel SHUF_MASK]


%%_encrypt_done:

        ;; Mapping to macro parameters
        ;; IN:
        ;;   xmm9 contains the counter
        ;;   xmm1-xmm8 contain the xor'd ciphertext
        ;; OUT:
        ;;   xmm14 contains the final hash
        ;;             GDATA,   T1,    T2,    T3,    T4,    T5,    T6,    T7, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8
%ifidn %%INSTANCE_TYPE, multi_call
        mov     r13, [%%GDATA_CTX + PBlockLen]
        cmp     r13, 0
        jz      %%_hash_last_8
        GHASH_LAST_7 %%GDATA_KEY, xmm0, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
        ;; XOR the partial word into the hash
        vpxor   xmm14, xmm14, xmm8
        jmp     %%_ghash_done
%endif
%%_hash_last_8:
        GHASH_LAST_8 %%GDATA_KEY, xmm0, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8

%%_ghash_done:
        vmovdqu [%%GDATA_CTX + CurCount], xmm9  ; my_ctx_data.current_counter = xmm9
        vmovdqu [%%GDATA_CTX + AadHash], xmm14      ; my_ctx_data.aad hash = xmm14

%%_enc_dec_done:


%endmacro                       ; GCM_ENC_DEC

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
	mov	r12, [%%GDATA_CTX + PBlockLen]
	cmp	r12, 0

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

        simd_store_avx r10, xmm9, r11, r12, rax
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_UPDATE_AAD_HASH update AadHash value in gcm_context_data struct to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Additional Authentication data (A_IN), Additional Data length (A_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (AadHash).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_UPDATE_AAD_HASH   7
%define %%GDATA_KEY           %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX           %2      ; [in] GCM context pointer
%define %%A_IN                %3      ; [in] AAD pointer
%define %%A_LEN               %4      ; [in] AAD length in bytes
%define %%GPR1                %5      ; temp GPR
%define %%GPR2                %6      ; temp GPR
%define %%GPR3                %7      ; temp GPR

%define %%AAD_HASH      xmm14
        
        vmovdqu64       %%AAD_HASH, [%%GDATA_CTX + AadHash]   ; load current hash

        UPDATE_HASH     %%A_IN, %%A_LEN, %%AAD_HASH, %%GDATA_KEY, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, %%GPR1, %%GPR2, %%GPR3

        vmovdqu64       [%%GDATA_CTX + AadHash], %%AAD_HASH   ; store updated hash

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_FINALIZE_AAD_HASH finalize updating of AadHash value in gcm_context_data struct 
;;; to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Additional Authentication data (A_IN), Additional Data length (A_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (AadHash) and initialized other parts of GDATA_CTX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_FINALIZE_AAD_HASH   8
%define %%GDATA_KEY           %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX           %2      ; [in] GCM context pointer
%define %%A_IN                %3      ; [in] AAD pointer
%define %%A_LEN               %4      ; [in] AAD length in bytes
%define %%GPR1                %5      ; temp GPR
%define %%GPR2                %6      ; temp GPR
%define %%GPR3                %7      ; temp GPR
%define %%A_GEN_LEN           %8     ; [in] AAD general length in bytes

%define %%AAD_HASH      xmm14

        vmovdqu64       %%AAD_HASH, [%%GDATA_CTX + AadHash]   ; load current hash

        ;; Check AAD length in bytes != 0
        cmp             %%A_LEN, 0
        jz              %%zero_len

        UPDATE_HASH     %%A_IN, %%A_LEN, %%AAD_HASH, %%GDATA_KEY, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, %%GPR1, %%GPR2, %%GPR3

%%zero_len:

        mov             %%GPR1, %%A_GEN_LEN
        vmovdqu64       [%%GDATA_CTX + AadHash], %%AAD_HASH   ; ctx.aad hash = aad_hash
        mov             [%%GDATA_CTX + AadLen], %%GPR1        ; ctx.aad_length = aad_length

        xor             %%GPR1, %%GPR1
        mov             [%%GDATA_CTX + InLen], %%GPR1         ; ctx.in_length = 0
        mov             [%%GDATA_CTX + PBlockLen], %%GPR1     ; ctx.partial_block_length = 0

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_UPDATE_IV_HASH update orig_IV value in gcm_context_data struct to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Initialization Vector (IV), Initialization Vector length (IV_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (orig_IV).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_UPDATE_IV_HASH    7
%define %%GDATA_KEY           %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX           %2      ; [in] GCM context pointer
%define %%IV                  %3      ; [in] AAD pointer
%define %%IV_LEN              %4      ; [in] AAD length in bytes
%define %%GPR1                %5      ; temp GPR
%define %%GPR2                %6      ; temp GPR
%define %%GPR3                %7      ; temp GPR

%define %%IV_HASH       xmm14
        
        vmovdqu64       %%IV_HASH, [%%GDATA_CTX + OrigIV]   ; load current hash

        UPDATE_HASH     %%IV, %%IV_LEN, %%IV_HASH, %%GDATA_KEY, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, %%GPR1, %%GPR2, %%GPR3

        vmovdqu64       [%%GDATA_CTX + OrigIV], %%IV_HASH   ; store updated hash

%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; GCM_FINALIZE_IV_HASH finalize updating of AadHash value in gcm_context_data struct 
;;; to prepare for encoding/decoding.
;;; Input: gcm_key_data * (GDATA_KEY), gcm_context_data *(GDATA_CTX),
;;; Initialization Vector (IV), Initialization Vector length (IV_LEN).
;;; Output: Updated GDATA_CTX with the hash of A_IN (orig_IV). and initialized other parts of GDATA_CTX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro  GCM_FINALIZE_IV_HASH  7-8
%define %%GDATA_KEY           %1      ; [in] GCM expanded keys pointer
%define %%GDATA_CTX           %2      ; [in] GCM context pointer
%define %%IV                  %3      ; [in] IV pointer
%define %%IV_LEN              %4      ; [in] IV length in bytes
%define %%GPR1                %5      ; temp GPR
%define %%GPR2                %6      ; temp GPR
%define %%GPR3                %7      ; temp GPR
%define %%IV_GEN_LEN          %8    ; [in] IV general length in bytes

        
%if %0 == 7 ;; IV is 12 bytes 

        ;; read 12 IV bytes and pad with 0x00000001
        mov     %%GPR2, %%IV
        vmovd   xmm3, [%%GPR2 + 8]
        vpslldq xmm3, 8
        vmovq   xmm2, [%%GPR2]
        vmovdqa xmm4, [rel ONEf]
        vpternlogq xmm2, xmm3, xmm4, 0xfe     ; xmm2 = xmm2 or xmm3 or xmm4
                
%else ;; IV is 12 bytes

        vmovdqu xmm2, [%%GDATA_CTX + OrigIV]

        CALC_J0 %%GDATA_KEY, %%IV, %%IV_LEN, xmm2, r10, r11, r12, xmm0, xmm1, \
                xmm3, xmm4, xmm5, xmm6, %%IV_GEN_LEN

%endif
        vmovdqu [%%GDATA_CTX + OrigIV], xmm2                ; ctx_data.orig_IV = iv

        ;; store IV as counter in LE format
        vpshufb xmm2, [rel SHUF_MASK]
        vmovdqu [%%GDATA_CTX + CurCount], xmm2              ; ctx_data.current_counter = iv


%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_precomp_128_avx512 /
;       aes_gcm_precomp_192_avx512 /
;       aes_gcm_precomp_256_avx512
;       (struct gcm_key_data *key_data)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM FN_NAME(precomp,_), PUBLIC
;; Parameter is passed through register
%ifdef SAFE_PARAM
        ;; Check key_data != NULL
        cmp     arg1, 0
        jz      exit_precomp
%endif

        push    r12
        push    r13
        push    r14
        push    r15

        mov     r14, rsp



        sub     rsp, VARIABLE_OFFSET
        and     rsp, ~63                                 ; align rsp to 64 bytes

%ifidn __OUTPUT_FORMAT__, win64
        ; only xmm6 needs to be maintained
        vmovdqu [rsp + LOCAL_STORAGE + 0*16],xmm6
%endif

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

%ifdef SAFE_DATA
        clear_scratch_gps_asm
        clear_scratch_zmms_asm
%endif
%ifidn __OUTPUT_FORMAT__, win64
        vmovdqu xmm6, [rsp + LOCAL_STORAGE + 0*16]
%endif
        mov     rsp, r14

        pop     r15
        pop     r14
        pop     r13
        pop     r12

.exit_precomp:
        ret

ENDFUNC FN_NAME(precomp,_)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_enc_128_update_avx512 / aes_gcm_enc_192_update_avx512 /
;       aes_gcm_enc_256_update_avx512
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
;void   aes_gcm_dec_128_update_avx512 / aes_gcm_dec_192_update_avx512 /
;       aes_gcm_dec_256_update_avx512
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

skip_in_out_check_update_dec:
%endif

        GCM_ENC_DEC arg1, arg2, arg3, arg4, arg5, DEC, multi_call

.exit_update_dec:
        FUNC_RESTORE
        ret

ENDFUNC FN_NAME(dec,_update_)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_gettag_128_avx512 / aes_gcm_gettag_192_avx512 /
;	aes_gcm_gettag_256_avx512
;       (const struct gcm_key_data *key_data,
;        struct gcm_context_data *context_data,
;        u8       *auth_tag,
;        u64      auth_tag_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM FN_NAME(gettag,_), PUBLIC

        FUNC_SAVE
        
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

        push r12

%ifidn __OUTPUT_FORMAT__, win64
        ; xmm6:xmm15 need to be maintained for Windows
	sub	rsp, 7*16
        vmovdqu	[rsp + 0*16], xmm6
        vmovdqu	[rsp + 1*16], xmm9
        vmovdqu	[rsp + 2*16], xmm10
        vmovdqu	[rsp + 3*16], xmm11
        vmovdqu	[rsp + 4*16], xmm13
        vmovdqu	[rsp + 5*16], xmm14
        vmovdqu	[rsp + 6*16], xmm15
%endif
        GCM_COMPLETE    arg1, arg2, arg3, arg4, ENC, multi_call

%ifdef SAFE_DATA
        clear_scratch_gps_asm
        clear_scratch_zmms_asm
%endif
%ifidn __OUTPUT_FORMAT__, win64
        vmovdqu	xmm15, [rsp + 6*16]
        vmovdqu	xmm14, [rsp + 5*16]
        vmovdqu	xmm13, [rsp + 4*16]
        vmovdqu	xmm11, [rsp + 3*16]
        vmovdqu	xmm10, [rsp + 2*16]
        vmovdqu	xmm9,  [rsp + 1*16]
        vmovdqu	xmm6,  [rsp + 0*16]
        add     rsp, 7*16
%endif

        pop r12

.exit_enc_fin:
        FUNC_RESTORE
	ret

ENDFUNC FN_NAME(gettag,_)

%ifdef GCM128_MODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   ghash_avx512
;        const struct gcm_key_data *key_data,
;        const void   *in,
;        const u64    in_len,
;        void         *tag,
;        const u64    tag_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM ghash_avx512, PUBLIC

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

        CALC_AAD_HASH arg2, arg3, xmm0, arg1, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, \
                      r10, r11, r12

        vpshufb xmm0, [rel SHUF_MASK] ; perform a 16Byte swap

        simd_store_avx arg4, xmm0, arg5, r12, rax

.exit_ghash:
        FUNC_RESTORE

        ret

ENDFUNC ghash_avx512

%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; IV and AAD in disconnected buffers processing 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%ifdef GCM128_MODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_aad_hash_update_avx512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *aad,
;        const u64    aad_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_aad_hash_update_avx512, PUBLIC

        FUNC_SAVE

        GCM_UPDATE_AAD_HASH arg1, arg2, arg3, arg4, r10, r11, r12

        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_aad_hash_update_avx512

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_aad_hash_finalize_avx512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *aad,
;        const u64    aad_len,
;        const u64    aad_general_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_aad_hash_finalize_avx512, PUBLIC

        FUNC_SAVE

        GCM_FINALIZE_AAD_HASH arg1, arg2, arg3, arg4, r10, r11, r12, arg5

        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_aad_hash_finalize_avx512

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_iv_hash_update_avx512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *iv,
;        const u64    iv_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_iv_hash_update_avx512, PUBLIC

        FUNC_SAVE

        GCM_UPDATE_IV_HASH arg1, arg2, arg3, arg4, r10, r11, r12

        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_iv_hash_update_avx512

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;void   aes_gcm_iv_hash_finalize_avx512(
;        const struct gcm_key_data *key_data,
;        struct       gcm_context_data *context_data,
;        const u8    *iv,
;        const u64    iv_len,
;        const u64    iv_general_len);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IPPASM aes_gcm_iv_hash_finalize_avx512, PUBLIC

        FUNC_SAVE

        cmp     arg5, 12
        je      .iv_eq_12

        GCM_FINALIZE_IV_HASH arg1, arg2, arg3, arg4, r10, r11, r12, arg5
        jmp .skip_iv_eq_12

.iv_eq_12:

        GCM_FINALIZE_IV_HASH arg1, arg2, arg3, arg4, r10, r11, r12
.skip_iv_eq_12:

        FUNC_RESTORE

        ret
ENDFUNC aes_gcm_iv_hash_finalize_avx512

%endif

%endif
