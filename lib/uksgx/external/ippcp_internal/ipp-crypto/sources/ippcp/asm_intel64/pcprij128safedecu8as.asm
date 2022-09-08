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
;               Rijndael-128 (AES) cipher functions.
;               (It's the special free from Sbox/tables implementation)
;
;     Content:
;        SafeDecrypt_RIJ128()
;
;     History:
;
;   Notes.
;   The implementation is based on
;   isomorphism between native GF(2^8) and composite GF((2^4)^2).
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "ia_32e_regs.inc"
%include "pcpvariant.inc"

%if (_AES_NI_ENABLING_ == _FEATURE_OFF_) || (_AES_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_U8)

%macro PTRANSFORM 6.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%memTransLO %3
  %xdefine %%memTransHI %4
  %xdefine %%tmp %5
  %xdefine %%srcLO %6

   movdqa   %%dst, oword [rel %%memTransLO]     ;; LO transformation
   movdqa   %%tmp, oword [rel %%memTransHI]     ;; HI transformation

   movdqa   %%srcLO, %%src     ;; split src:
   psrlw    %%src, 4         ;;
   pand     %%srcLO, xmm7    ;; low 4 bits -> srcLO
   pand     %%src, xmm7      ;; upper 4 bits -> src

   pshufb   %%dst, %%srcLO     ;; transformation
   pshufb   %%tmp, %%src
   pxor     %%dst, %%tmp
%endmacro

%macro PLOOKUP_MEM 3.nolist
  %xdefine %%dst %1
  %xdefine %%src %2
  %xdefine %%Table %3

   movdqa   %%dst, OWORD [rel %%Table]
   pshufb   %%dst,%%src
%endmacro

%macro PREDUCE_MOD15 2.nolist
  %xdefine %%dst %1
  %xdefine %%src %2

   movdqa   %%dst, %%src
   pcmpgtb  %%src, xmm7
   psubb    %%dst, %%src
%endmacro

%macro PINVERSE_GF16_INV 6.nolist
  %xdefine %%xmmB %1
  %xdefine %%xmmC %2
  %xdefine %%xmmP %3
  %xdefine %%xmmQ %4
  %xdefine %%xmmD %5
  %xdefine %%xmmT %6

   PLOOKUP_MEM    %%xmmT, %%xmmC, GF16_logTbl    ;; xmmT = index_of(c)
   pxor           %%xmmC, %%xmmB
   PLOOKUP_MEM    %%xmmQ, %%xmmC, GF16_logTbl    ;; xmmQ = index_of(b xor c)

   PLOOKUP_MEM    %%xmmD, %%xmmB, GF16_sqr1      ;; xmmD = sqr(b)*beta^14
   PLOOKUP_MEM    %%xmmP, %%xmmB, GF16_logTbl    ;; xmmP = index_of(b)

   paddb          %%xmmT, %%xmmQ                                     ;; xmmT = index_of(c) + index_of(b xor c)
   PREDUCE_MOD15  %%xmmC, %%xmmT                                     ;;
   PLOOKUP_MEM    %%xmmT, %%xmmC, GF16_expTbl    ;; c*(b xor c)

   pxor           %%xmmD, %%xmmT                                     ;; xmmD = delta = (c*(b xor c)) xor (sqr(b)*beta^14)
   PLOOKUP_MEM    %%xmmT, %%xmmD, GF16_invLog    ;; xmmT = index_of( inv(delta) )

   paddb          %%xmmQ, %%xmmT  ;; xmmQ = index_of((b xor c) * inv(delta))
   paddb          %%xmmP, %%xmmT  ;; xmmP = index_of(b * inv(delta))
   PREDUCE_MOD15  %%xmmT, %%xmmQ
   PLOOKUP_MEM    %%xmmC, %%xmmT, GF16_expTbl
   PREDUCE_MOD15  %%xmmT, %%xmmP
   PLOOKUP_MEM    %%xmmB, %%xmmT, GF16_expTbl
%endmacro

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR

DECODE_DATA:

;; (forward) native GF(2^8) to composite GF((2^4)^2) transformation : {0x01,0x2E,0x49,0x43,0x35,0xD0,0x3D,0xE9}
TransFwdLO \
      DB    000h  ;; 000h                             ;; 0
      DB    001h  ;; 001h                             ;; 1
      DB    02Eh  ;; 02Eh                             ;; 2
      DB    02Fh  ;; 02Eh XOR 001h                    ;; 3
      DB    049h  ;; 049h                             ;; 4
      DB    048h  ;; 049h XOR 001h                    ;; 5
      DB    067h  ;; 049h XOR 02Eh                    ;; 6
      DB    066h  ;; 049h XOR 02Eh XOR 001h           ;; 7
      DB    043h  ;; 043h                             ;; 8
      DB    042h  ;; 043h XOR 001h                    ;; 9
      DB    06Dh  ;; 043h XOR 02Eh                    ;; a
      DB    06Ch  ;; 043h XOR 02Eh XOR 001h           ;; b
      DB    00Ah  ;; 043h XOR 049h                    ;; c
      DB    00Bh  ;; 043h XOR 049h XOR 001h           ;; d
      DB    024h  ;; 043h XOR 049h XOR 02Eh           ;; e
      DB    025h  ;; 043h XOR 049h XOR 02Eh XOR 001h  ;; f
TransFwdHI \
      DB    000h  ;; 000h                             ;; 0
      DB    035h  ;; 035h                             ;; 1
      DB    0D0h  ;; 0D0h                             ;; 2
      DB    0E5h  ;; 0D0h XOR 035h                    ;; 3
      DB    03Dh  ;; 03Dh                             ;; 4
      DB    008h  ;; 03Dh XOR 035h                    ;; 5
      DB    0EDh  ;; 03Dh XOR 0D0h                    ;; 6
      DB    0D8h  ;; 03Dh XOR 0D0h XOR 035h           ;; 7
      DB    0E9h  ;; 0E9h                             ;; 8
      DB    0DCh  ;; 0E9h XOR 035h                    ;; 9
      DB    039h  ;; 0E9h XOR 0D0h                    ;; a
      DB    00Ch  ;; 0E9h XOR 0D0h XOR 035h           ;; b
      DB    0D4h  ;; 0E9h XOR 03Dh                    ;; c
      DB    0E1h  ;; 0E9h XOR 03Dh XOR 035h           ;; d
      DB    004h  ;; 0E9h XOR 03Dh XOR 0D0h           ;; e
      DB    031h  ;; 0E9h XOR 03Dh XOR 0D0h XOR 035h  ;; f

;; (inverse) composite GF((2^4)^2) to native GF(2^8) transformation : {0x01,0x5C,0xE0,0x50,0x1F,0xEE,0x55,0x6A}
TransInvLO \
      DB    000h  ;; 000h                             ;; 0
      DB    001h  ;; 001h                             ;; 1
      DB    05Ch  ;; 05Ch                             ;; 2
      DB    05Dh  ;; 05Ch XOR 001h                    ;; 3
      DB    0E0h  ;; 0E0h                             ;; 4
      DB    0E1h  ;; 0E0h XOR 001h                    ;; 5
      DB    0BCh  ;; 0E0h XOR 05Ch                    ;; 6
      DB    0BDh  ;; 0E0h XOR 05Ch XOR 001h           ;; 7
      DB    050h  ;; 050h                             ;; 8
      DB    051h  ;; 050h XOR 001h                    ;; 9
      DB    00Ch  ;; 050h XOR 05Ch                    ;; a
      DB    00Dh  ;; 050h XOR 05Ch XOR 001h           ;; b
      DB    0B0h  ;; 050h XOR 0E0h                    ;; c
      DB    0B1h  ;; 050h XOR 0E0h XOR 001h           ;; d
      DB    0ECh  ;; 050h XOR 0E0h XOR 05Ch           ;; e
      DB    0EDh  ;; 050h XOR 0E0h XOR 05Ch XOR 001h  ;; f
TransInvHI \
      DB    000h  ;; 000h                             ;; 0
      DB    01Fh  ;; 01Fh                             ;; 1
      DB    0EEh  ;; 0EEh                             ;; 2
      DB    0F1h  ;; 0EEh XOR 01Fh                    ;; 3
      DB    055h  ;; 055h                             ;; 4
      DB    04Ah  ;; 055h XOR 01Fh                    ;; 5
      DB    0BBh  ;; 055h XOR 0EEh                    ;; 6
      DB    0A4h  ;; 055h XOR 0EEh XOR 01Fh           ;; 7
      DB    06Ah  ;; 06Ah                             ;; 8
      DB    075h  ;; 06Ah XOR 01Fh                    ;; 9
      DB    084h  ;; 06Ah XOR 0EEh                    ;; a
      DB    09Bh  ;; 06Ah XOR 0EEh XOR 01Fh           ;; b
      DB    03Fh  ;; 06Ah XOR 055h                    ;; c
      DB    020h  ;; 06Ah XOR 055h XOR 01Fh           ;; d
      DB    0D1h  ;; 06Ah XOR 055h XOR 0EEh           ;; e
      DB    0CEh  ;; 06Ah XOR 055h XOR 0EEh XOR 01Fh  ;; f


GF16_csize  DB 00Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh,0Fh

;; GF16 elements:
;;         0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
GF16_logTbl \
      DB 0C0h,00h,01h,04h,02h,08h,05h,0Ah,03h,0Eh,09h,07h,06h,0Dh,0Bh,0Ch
GF16_expTbl \
      DB 001h,02h,04h,08h,03h,06h,0Ch,0Bh,05h,0Ah,07h,0Eh,0Fh,0Dh,09h,01h
GF16_sqr1   \
      DB 000h,09h,02h,0Bh,08h,01h,0Ah,03h,06h,0Fh,04h,0Dh,0Eh,07h,0Ch,05h ;; sqr(GF16_element) * beta^14
GF16_invLog \
      DB 0C0h,00h,0Eh,0Bh,0Dh,07h,0Ah,05h,0Ch,01h,06h,08h,09h,02h,04h,03h

;; affine transformation matrix (inverse cipher) : {0x50,0x36,0x15,0x82,0x01,0x34,0x40,0x3E}
InvAffineLO \
      DB    000h  ;; 000h                             ;; 0
      DB    050h  ;; 050h                             ;; 1
      DB    036h  ;; 036h                             ;; 2
      DB    066h  ;; 036h XOR 050h                    ;; 3
      DB    015h  ;; 015h                             ;; 4
      DB    045h  ;; 015h XOR 050h                    ;; 5
      DB    023h  ;; 015h XOR 036h                    ;; 6
      DB    073h  ;; 015h XOR 036h XOR 050h           ;; 7
      DB    082h  ;; 082h                             ;; 8
      DB    0D2h  ;; 082h XOR 050h                    ;; 9
      DB    0B4h  ;; 082h XOR 036h                    ;; a
      DB    0E4h  ;; 082h XOR 036h XOR 050h           ;; b
      DB    097h  ;; 082h XOR 015h                    ;; c
      DB    0C7h  ;; 082h XOR 015h XOR 050h           ;; d
      DB    0A1h  ;; 082h XOR 015h XOR 036h           ;; e
      DB    0F1h  ;; 082h XOR 015h XOR 036h XOR 050h  ;; f
InvAffineHI \
      DB    000h  ;; 000h                             ;; 0
      DB    001h  ;; 001h                             ;; 1
      DB    034h  ;; 034h                             ;; 2
      DB    035h  ;; 034h XOR 001h                    ;; 3
      DB    040h  ;; 040h                             ;; 4
      DB    041h  ;; 040h XOR 001h                    ;; 5
      DB    074h  ;; 040h XOR 034h                    ;; 6
      DB    075h  ;; 040h XOR 034h XOR 001h           ;; 7
      DB    03Eh  ;; 03Eh                             ;; 8
      DB    03Fh  ;; 03Eh XOR 001h                    ;; 9
      DB    00Ah  ;; 03Eh XOR 034h                    ;; a
      DB    00Bh  ;; 03Eh XOR 034h XOR 001h           ;; b
      DB    07Eh  ;; 03Eh XOR 040h                    ;; c
      DB    07Fh  ;; 03Eh XOR 040h XOR 001h           ;; d
      DB    04Ah  ;; 03Eh XOR 040h XOR 034h           ;; e
      DB    04Bh  ;; 03Eh XOR 040h XOR 034h XOR 001h  ;; f

;; affine transformation constant (inverse cipher)
InvAffineCnt \
      DQ    04848484848484848h,04848484848484848h

;; shift rows transformation (inverse cipher)
InvShiftRows \
      DB    0,13,10,7,4,1,14,11,8,5,2,15,12,9,6,3

;; mix columns transformation (inverse cipher)
GF16mul_4_2x \
   DB 000h,024h,048h,06Ch,083h,0A7h,0CBh,0EFh,036h,012h,07Eh,05Ah,0B5h,091h,0FDh,0D9h  ;; *(4+2x)
GF16mul_1_6x \
   DB 000h,061h,0C2h,0A3h,0B4h,0D5h,076h,017h,058h,039h,09Ah,0FBh,0ECh,08Dh,02Eh,04Fh  ;; *(1+6x)

GF16mul_C_6x \
   DB 000h,06Ch,0CBh,0A7h,0B5h,0D9h,07Eh,012h,05Ah,036h,091h,0FDh,0EFh,083h,024h,048h  ;; *(C+6x)
GF16mul_3_Ax \
   DB 000h,0A3h,076h,0D5h,0ECh,04Fh,09Ah,039h,0FBh,058h,08Dh,02Eh,017h,0B4h,061h,0C2h ;; *(3+Ax)

GF16mul_B_0x \
   DB 000h,00Bh,005h,00Eh,00Ah,001h,00Fh,004h,007h,00Ch,002h,009h,00Dh,006h,008h,003h  ;; *(B+0x)
GF16mul_0_Bx \
   DB 000h,0B0h,050h,0E0h,0A0h,010h,0F0h,040h,070h,0C0h,020h,090h,0D0h,060h,080h,030h  ;; *(0+Bx)

GF16mul_2_4x \
   DB 000h,042h,084h,0C6h,038h,07Ah,0BCh,0FEh,063h,021h,0E7h,0A5h,05Bh,019h,0DFh,09Dh  ;; *(2+4x)
GF16mul_2_6x \
   DB 000h,062h,0C4h,0A6h,0B8h,0DAh,07Ch,01Eh,053h,031h,097h,0F5h,0EBh,089h,02Fh,04Dh  ;; *(2+6x)

ColumnROR    \
   DB 1,2,3,0,5,6,7,4,9,10,11,8,13,14,15,12


align IPP_ALIGN_FACTOR
;*************************************************************
;* void SafeDecrypt_RIJ128(const Ipp8u* pInpBlk,
;*                               Ipp8u* pOutBlk,
;*                               int    nr,
;*                        const Ipp8u* pKeys,
;*                        const void*  Tables)
;*************************************************************

;;
;; Lib = U8
;;
IPPASM SafeDecrypt_RIJ128,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM xmm6,xmm7
        COMP_ABI 5
;; rdi:  pInpBlk:  DWORD,    ; input  block address
;; rsi:  pOutBlk:  DWORD,    ; output block address
;; rdx:  nr:           DWORD,    ; number of rounds
;; rcx:  pKey:     DWORD     ; key material address

%assign RSIZE  sizeof(dword)         ; size of row
%assign SC  4                        ; columns in STATE
%assign SSIZE  RSIZE*SC              ; size of state

   lea      rax,[rdx*4]
   lea      rcx,[rcx+rax*4]       ; AES-128-keys

   movdqu   xmm0, oword [rdi] ; input block

   movdqa   xmm7, oword [rel GF16_csize]

   ;; convert input into the composite GF((2^4)^2)
   PTRANSFORM  xmm2, xmm0, TransFwdLO,TransFwdHI, xmm1, xmm3

   ;; initial whitening
   pxor     xmm2, oword [rcx]
   sub      rcx, SSIZE

   ;; (nr-1) regular rounds
   sub      rdx,1

.decode_round:
   ;; InvSubByte() Transformation:

   ;; affine transformation
   PTRANSFORM  xmm0, xmm2, InvAffineLO,InvAffineHI, xmm1, xmm3
   pxor        xmm0, oword [rel InvAffineCnt]  ; H(c), c=0x05

   ;; split input by low and upper parts
   movdqa      xmm1, xmm0
   pand        xmm0, xmm7  ; low parts (4 bits)
   psrlw       xmm1, 4
   pand        xmm1, xmm7  ; upper parts (4 bits)

   ;; compute multiplicative inverse
   PINVERSE_GF16_INV xmm1,xmm0, xmm3,xmm2,xmm4,xmm5

   ;; InvShiftRows() Transformation:
   pshufb      xmm0, [rel InvShiftRows]
   pshufb      xmm1, [rel InvShiftRows]

   ;; InvMixColumn() Transformation:
   PLOOKUP_MEM xmm2, xmm0, GF16mul_4_2x   ; mul H(0xE) = 0x24
   pshufb      xmm0, [rel ColumnROR]
   PLOOKUP_MEM xmm3, xmm1, GF16mul_1_6x
   pshufb      xmm1, [rel ColumnROR]
   pxor        xmm2, xmm3

   PLOOKUP_MEM xmm3, xmm0, GF16mul_C_6x   ; mul H(0xB) = 0x6C
   pshufb      xmm0, [rel ColumnROR]
   pxor        xmm2, xmm3
   PLOOKUP_MEM xmm3, xmm1, GF16mul_3_Ax
   pshufb      xmm1, [rel ColumnROR]
   pxor        xmm2, xmm3

   PLOOKUP_MEM xmm3, xmm0, GF16mul_B_0x   ; mul H(0xD) = 0x0B
   pshufb      xmm0, [rel ColumnROR]
   pxor        xmm2, xmm3
   PLOOKUP_MEM xmm3, xmm1, GF16mul_0_Bx
   pshufb      xmm1, [rel ColumnROR]
   pxor        xmm2, xmm3

   PLOOKUP_MEM xmm3, xmm0, GF16mul_2_4x   ; mul H(0x9) = 0x42
   pxor        xmm2, xmm3
   PLOOKUP_MEM xmm3, xmm1, GF16mul_2_6x
   pxor        xmm2, xmm3

   ;; AddRoundKey() Transformation:
   pxor        xmm2, oword [rcx]
   sub         rcx, SSIZE

   sub         rdx,1
   jg          .decode_round


   ;;
   ;; the last one is irregular
   ;;

   ;; InvSubByte() Transformation

   ;; affine transformation
   PTRANSFORM  xmm0, xmm2, InvAffineLO,InvAffineHI, xmm1, xmm3
   pxor        xmm0, oword [rel InvAffineCnt]  ; H(c), c=0x05

   ;; split input by low and upper parts
   movdqa      xmm1, xmm0
   pand        xmm0, xmm7  ; low parts (4 bits)
   psrlw       xmm1, 4
   pand        xmm1, xmm7  ; upper parts (4 bits)

   ;; compute multiplicative inverse
   PINVERSE_GF16_INV xmm1,xmm0, xmm3,xmm2,xmm4,xmm5

   ;; InvShiftRows() Transformation:
   psllw       xmm1, 4
   por         xmm1, xmm0
   pshufb      xmm1, [rel InvShiftRows]

   ;; AddRoundKey() Transformation:
   pxor        xmm1, oword [rcx]

   ;; convert output into the native GF(2^8)
   PTRANSFORM  xmm0,xmm1, TransInvLO,TransInvHI, xmm2, xmm3

   movdqu      oword [rsi], xmm0

   REST_XMM
   REST_GPR
   ret
ENDFUNC SafeDecrypt_RIJ128

%endif

%endif ;; _AES_NI_ENABLING_

