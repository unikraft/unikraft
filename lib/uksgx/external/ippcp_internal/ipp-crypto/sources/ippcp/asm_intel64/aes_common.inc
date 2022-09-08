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

%ifndef _AES_COMMON_ASM_
%define _AES_COMMON_ASM_

%include "reg_sizes.inc"

;; =============================================================================
;; Generic macro to produce code that executes %%OPCODE instruction
;; on selected number of AES blocks (16 bytes long ) between 0 and 16.
;; All three operands of the instruction come from registers.
;; Note: if 3 blocks are left at the end instruction is produced to operate all
;;       4 blocks (full width of ZMM)

%macro ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 14
%define %%NUM_BLOCKS    %1      ; [in] numerical value, number of AES blocks (0 to 16)
%define %%OPCODE        %2      ; [in] instruction name
%define %%DST0          %3      ; [out] destination ZMM register
%define %%DST1          %4      ; [out] destination ZMM register
%define %%DST2          %5      ; [out] destination ZMM register
%define %%DST3          %6      ; [out] destination ZMM register
%define %%SRC1_0        %7      ; [in] source 1 ZMM register
%define %%SRC1_1        %8      ; [in] source 1 ZMM register
%define %%SRC1_2        %9      ; [in] source 1 ZMM register
%define %%SRC1_3        %10     ; [in] source 1 ZMM register
%define %%SRC2_0        %11     ; [in] source 2 ZMM register
%define %%SRC2_1        %12     ; [in] source 2 ZMM register
%define %%SRC2_2        %13     ; [in] source 2 ZMM register
%define %%SRC2_3        %14     ; [in] source 2 ZMM register

%assign reg_idx     0
%assign blocks_left %%NUM_BLOCKS

%rep (%%NUM_BLOCKS / 4)
%xdefine %%DSTREG  %%DST %+ reg_idx
%xdefine %%SRC1REG %%SRC1_ %+ reg_idx
%xdefine %%SRC2REG %%SRC2_ %+ reg_idx
        %%OPCODE        %%DSTREG, %%SRC1REG, %%SRC2REG
%undef %%DSTREG
%undef %%SRC1REG
%undef %%SRC2REG
%assign reg_idx     (reg_idx + 1)
%assign blocks_left (blocks_left - 4)
%endrep

%xdefine %%DSTREG  %%DST %+ reg_idx
%xdefine %%SRC1REG %%SRC1_ %+ reg_idx
%xdefine %%SRC2REG %%SRC2_ %+ reg_idx

%if blocks_left == 1
        %%OPCODE        XWORD(%%DSTREG), XWORD(%%SRC1REG), XWORD(%%SRC2REG)
%elif blocks_left == 2
        %%OPCODE        YWORD(%%DSTREG), YWORD(%%SRC1REG), YWORD(%%SRC2REG)
%elif blocks_left == 3
        %%OPCODE        %%DSTREG, %%SRC1REG, %%SRC2REG
%endif

%endmacro

;; =============================================================================
;; Loads specified number of AES blocks into ZMM registers
;; %%FLAGS are optional and only affect behavior when 3 trailing blocks are left
;; - if %%FlAGS not provided then exactly 3 blocks are loaded (move and insert)
;; - if "load_4_instead_of_3" option is passed then 4 blocks are loaded
%macro ZMM_LOAD_BLOCKS_0_16 7-8
%define %%NUM_BLOCKS    %1 ; [in] numerical value, number of AES blocks (0 to 16)
%define %%INP           %2 ; [in] input data pointer to read from
%define %%DATA_OFFSET   %3 ; [in] offset to the output pointer (GP or numerical)
%define %%DST0          %4 ; [out] ZMM register with loaded data
%define %%DST1          %5 ; [out] ZMM register with loaded data
%define %%DST2          %6 ; [out] ZMM register with loaded data
%define %%DST3          %7 ; [out] ZMM register with loaded data
%define %%FLAGS         %8 ; [in] optional "load_4_instead_of_3"

%assign src_offset  0
%assign dst_idx     0

%rep (%%NUM_BLOCKS / 4)
%xdefine %%DSTREG %%DST %+ dst_idx
        vmovdqu8        %%DSTREG, [%%INP + %%DATA_OFFSET + src_offset]
%undef %%DSTREG
%assign src_offset  (src_offset + 64)
%assign dst_idx     (dst_idx + 1)
%endrep

%assign blocks_left (%%NUM_BLOCKS % 4)
%xdefine %%DSTREG %%DST %+ dst_idx

%if blocks_left == 1
        vmovdqu8        XWORD(%%DSTREG), [%%INP + %%DATA_OFFSET + src_offset]
%elif blocks_left == 2
        vmovdqu8        YWORD(%%DSTREG), [%%INP + %%DATA_OFFSET + src_offset]
%elif blocks_left == 3
%ifidn %%FLAGS, load_4_instead_of_3
        vmovdqu8        %%DSTREG, [%%INP + %%DATA_OFFSET + src_offset]
%else
        vmovdqu8        YWORD(%%DSTREG), [%%INP + %%DATA_OFFSET + src_offset]
        vinserti64x2    %%DSTREG, [%%INP + %%DATA_OFFSET + src_offset + 32], 2
%endif
%endif

%endmacro

;; =============================================================================
;; Loads specified number of AES blocks into ZMM registers using mask register
;; for the last loaded register (xmm, ymm or zmm).
;; Loads take place at 1 byte granularity.
%macro ZMM_LOAD_MASKED_BLOCKS_0_16 8
%define %%NUM_BLOCKS    %1 ; [in] numerical value, number of AES blocks (0 to 16)
%define %%INP           %2 ; [in] input data pointer to read from
%define %%DATA_OFFSET   %3 ; [in] offset to the output pointer (GP or numerical)
%define %%DST0          %4 ; [out] ZMM register with loaded data
%define %%DST1          %5 ; [out] ZMM register with loaded data
%define %%DST2          %6 ; [out] ZMM register with loaded data
%define %%DST3          %7 ; [out] ZMM register with loaded data
%define %%MASK          %8 ; [in] mask register

%assign src_offset  0
%assign dst_idx     0
%assign blocks_left %%NUM_BLOCKS

%if %%NUM_BLOCKS > 0
%rep (((%%NUM_BLOCKS + 3) / 4) - 1)
%xdefine %%DSTREG %%DST %+ dst_idx
        vmovdqu8        %%DSTREG, [%%INP + %%DATA_OFFSET + src_offset]
%undef %%DSTREG
%assign src_offset  (src_offset + 64)
%assign dst_idx     (dst_idx + 1)
%assign blocks_left (blocks_left - 4)
%endrep
%endif  ; %if %%NUM_BLOCKS > 0

%xdefine %%DSTREG %%DST %+ dst_idx

%if blocks_left == 1
        vmovdqu8        XWORD(%%DSTREG){%%MASK}{z}, [%%INP + %%DATA_OFFSET + src_offset]
%elif blocks_left == 2
        vmovdqu8        YWORD(%%DSTREG){%%MASK}{z}, [%%INP + %%DATA_OFFSET + src_offset]
%elif (blocks_left == 3 || blocks_left == 4)
        vmovdqu8        %%DSTREG{%%MASK}{z}, [%%INP + %%DATA_OFFSET + src_offset]
%endif

%endmacro

;; =============================================================================
;; Stores specified number of AES blocks from ZMM registers
%macro ZMM_STORE_BLOCKS_0_16 7
%define %%NUM_BLOCKS    %1 ; [in] numerical value, number of AES blocks (0 to 16)
%define %%OUTP          %2 ; [in] output data pointer to write to
%define %%DATA_OFFSET   %3 ; [in] offset to the output pointer (GP or numerical)
%define %%SRC0          %4 ; [in] ZMM register with data to store
%define %%SRC1          %5 ; [in] ZMM register with data to store
%define %%SRC2          %6 ; [in] ZMM register with data to store
%define %%SRC3          %7 ; [in] ZMM register with data to store

%assign dst_offset  0
%assign src_idx     0

%rep (%%NUM_BLOCKS / 4)
%xdefine %%SRCREG %%SRC %+ src_idx
        vmovdqu8         [%%OUTP + %%DATA_OFFSET + dst_offset], %%SRCREG
%undef %%SRCREG
%assign dst_offset  (dst_offset + 64)
%assign src_idx     (src_idx + 1)
%endrep

%assign blocks_left (%%NUM_BLOCKS % 4)
%xdefine %%SRCREG %%SRC %+ src_idx

%if blocks_left == 1
        vmovdqu8        [%%OUTP + %%DATA_OFFSET + dst_offset], XWORD(%%SRCREG)
%elif blocks_left == 2
        vmovdqu8        [%%OUTP + %%DATA_OFFSET + dst_offset], YWORD(%%SRCREG)
%elif blocks_left == 3
        vmovdqu8        [%%OUTP + %%DATA_OFFSET + dst_offset], YWORD(%%SRCREG)
        vextracti32x4   [%%OUTP + %%DATA_OFFSET + dst_offset + 32], %%SRCREG, 2
%endif

%endmacro

;; =============================================================================
;; Stores specified number of AES blocks from ZMM registers with mask register
;; for the last loaded register (xmm, ymm or zmm).
;; Stores take place at 1 byte granularity.
%macro ZMM_STORE_MASKED_BLOCKS_0_16 8
%define %%NUM_BLOCKS    %1 ; [in] numerical value, number of AES blocks (0 to 16)
%define %%OUTP          %2 ; [in] output data pointer to write to
%define %%DATA_OFFSET   %3 ; [in] offset to the output pointer (GP or numerical)
%define %%SRC0          %4 ; [in] ZMM register with data to store
%define %%SRC1          %5 ; [in] ZMM register with data to store
%define %%SRC2          %6 ; [in] ZMM register with data to store
%define %%SRC3          %7 ; [in] ZMM register with data to store
%define %%MASK          %8 ; [in] mask register

%assign dst_offset  0
%assign src_idx     0
%assign blocks_left %%NUM_BLOCKS

%if %%NUM_BLOCKS > 0
%rep (((%%NUM_BLOCKS + 3) / 4) - 1)
%xdefine %%SRCREG %%SRC %+ src_idx
        vmovdqu8         [%%OUTP + %%DATA_OFFSET + dst_offset], %%SRCREG
%undef %%SRCREG
%assign dst_offset  (dst_offset + 64)
%assign src_idx     (src_idx + 1)
%assign blocks_left (blocks_left - 4)
%endrep
%endif  ; %if %%NUM_BLOCKS > 0

%xdefine %%SRCREG %%SRC %+ src_idx

%if blocks_left == 1
        vmovdqu8        [%%OUTP + %%DATA_OFFSET + dst_offset]{%%MASK}, XWORD(%%SRCREG)
%elif blocks_left == 2
        vmovdqu8        [%%OUTP + %%DATA_OFFSET + dst_offset]{%%MASK}, YWORD(%%SRCREG)
%elif (blocks_left == 3 || blocks_left == 4)
        vmovdqu8        [%%OUTP + %%DATA_OFFSET + dst_offset]{%%MASK}, %%SRCREG
%endif

%endmacro

;;; ===========================================================================
;;; Handles AES encryption rounds
;;; It handles special cases: the last and first rounds
;;; Optionally, it performs XOR with data after the last AES round.
;;; Uses NROUNDS parameterto check what needs to be done for the current round.
;;; If 3 blocks are trailing then operation on whole ZMM is performed (4 blocks).
%macro ZMM_AESENC_ROUND_BLOCKS_0_16 12
%define %%L0B0_3   %1      ; [in/out] zmm; blocks 0 to 3
%define %%L0B4_7   %2      ; [in/out] zmm; blocks 4 to 7
%define %%L0B8_11  %3      ; [in/out] zmm; blocks 8 to 11
%define %%L0B12_15 %4      ; [in/out] zmm; blocks 12 to 15
%define %%KEY      %5      ; [in] zmm containing round key
%define %%ROUND    %6      ; [in] round number
%define %%D0_3     %7      ; [in] zmm or no_data; plain/cipher text blocks 0-3
%define %%D4_7     %8      ; [in] zmm or no_data; plain/cipher text blocks 4-7
%define %%D8_11    %9      ; [in] zmm or no_data; plain/cipher text blocks 8-11
%define %%D12_15   %10     ; [in] zmm or no_data; plain/cipher text blocks 12-15
%define %%NUMBL    %11     ; [in] number of blocks; numerical value
%define %%NROUNDS  %12     ; [in] number of rounds; numerical value

;;; === first AES round
%if (%%ROUND < 1)
        ;;  round 0
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vpxorq, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%KEY, %%KEY, %%KEY, %%KEY
%endif                  ; ROUND 0

;;; === middle AES rounds
%if (%%ROUND >= 1 && %%ROUND <= %%NROUNDS)
        ;; rounds 1 to 9/11/13
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vaesenc, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%KEY, %%KEY, %%KEY, %%KEY
%endif                  ; rounds 1 to 9/11/13

;;; === last AES round
%if (%%ROUND > %%NROUNDS)
        ;; the last round - mix enclast with text xor's
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vaesenclast, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%KEY, %%KEY, %%KEY, %%KEY

;;; === XOR with data
%ifnidn %%D0_3, no_data
%ifnidn %%D4_7, no_data
%ifnidn %%D8_11, no_data
%ifnidn %%D12_15, no_data
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vpxorq, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%D0_3, %%D4_7, %%D8_11, %%D12_15
%endif                          ; !no_data
%endif                          ; !no_data
%endif                          ; !no_data
%endif                          ; !no_data

%endif                  ; The last round

%endmacro

;;; ===========================================================================
;;; Handles AES decryption rounds
;;; It handles special cases: the last and first rounds
;;; Optionally, it performs XOR with data after the last AES round.
;;; Uses NROUNDS parameter to check what needs to be done for the current round.
;;; If 3 blocks are trailing then operation on whole ZMM is performed (4 blocks).
%macro ZMM_AESDEC_ROUND_BLOCKS_0_16 12
%define %%L0B0_3   %1      ; [in/out] zmm; blocks 0 to 3
%define %%L0B4_7   %2      ; [in/out] zmm; blocks 4 to 7
%define %%L0B8_11  %3      ; [in/out] zmm; blocks 8 to 11
%define %%L0B12_15 %4      ; [in/out] zmm; blocks 12 to 15
%define %%KEY      %5      ; [in] zmm containing round key
%define %%ROUND    %6      ; [in] round number
%define %%D0_3     %7      ; [in] zmm or no_data; cipher text blocks 0-3
%define %%D4_7     %8      ; [in] zmm or no_data; cipher text blocks 4-7
%define %%D8_11    %9      ; [in] zmm or no_data; cipher text blocks 8-11
%define %%D12_15   %10     ; [in] zmm or no_data; cipher text blocks 12-15
%define %%NUMBL    %11     ; [in] number of blocks; numerical value
%define %%NROUNDS  %12     ; [in] number of rounds; numerical value

;;; === first AES round
%if (%%ROUND < 1)
        ;;  round 0
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vpxorq, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%KEY, %%KEY, %%KEY, %%KEY
%endif                  ; ROUND 0

;;; === middle AES rounds
%if (%%ROUND >= 1 && %%ROUND <= %%NROUNDS)
        ;; rounds 1 to 9/11/13
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vaesdec, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%KEY, %%KEY, %%KEY, %%KEY
%endif                  ; rounds 1 to 9/11/13

;;; === last AES round
%if (%%ROUND > %%NROUNDS)
        ;; the last round - mix enclast with text xor's
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vaesdeclast, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%KEY, %%KEY, %%KEY, %%KEY

;;; === XOR with data
%ifnidn %%D0_3, no_data
%ifnidn %%D4_7, no_data
%ifnidn %%D8_11, no_data
%ifnidn %%D12_15, no_data
        ZMM_OPCODE3_DSTR_SRC1R_SRC2R_BLOCKS_0_16 %%NUMBL, vpxorq, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%L0B0_3, %%L0B4_7, %%L0B8_11, %%L0B12_15, \
                        %%D0_3, %%D4_7, %%D8_11, %%D12_15
%endif                          ; !no_data
%endif                          ; !no_data
%endif                          ; !no_data
%endif                          ; !no_data

%endif                  ; The last round

%endmacro

%endif ;; _AES_COMMON_ASM
