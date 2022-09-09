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
;               Message block processing according to SHA1
;
;     Content:
;        UpdateSHA1
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA1_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
;;%if (_IPP32E >= _IPP32E_E9 )
%if (_IPP32E == _IPP32E_E9 )

;;
;; SHA1 constants K[i]
%xdefine SHA1_K1  (05a827999h)
%xdefine SHA1_K2  (06ed9eba1h)
%xdefine SHA1_K3  (08f1bbcdch)
%xdefine SHA1_K4  (0ca62c1d6h)

;;
;; Magic functions defined in FIPS 180-1
;;
;; F1, F2, F3 and F4 assumes, that
;; - T1 returns function value
;; - T2 is the temporary
;;

%macro F1 3.nolist
  %xdefine %%B %1
  %xdefine %%C %2
  %xdefine %%D %3

   mov   T1,%%C
   xor   T1,%%D
   and   T1,%%B
   xor   T1,%%D
%endmacro

%macro F2 3.nolist
  %xdefine %%B %1
  %xdefine %%C %2
  %xdefine %%D %3

   mov   T1,%%D
   xor   T1,%%C
   xor   T1,%%B
%endmacro

%macro F3 3.nolist
  %xdefine %%B %1
  %xdefine %%C %2
  %xdefine %%D %3

   mov   T1,%%C
   mov   T2,%%B
   or    T1,%%B
   and   T2,%%C
   and   T1,%%D
   or    T1,T2
%endmacro

%macro F4 3.nolist
  %xdefine %%B %1
  %xdefine %%C %2
  %xdefine %%D %3

   F2    %%B,%%C,%%D
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; rotations
;;

%macro ROL_5 1.nolist
  %xdefine %%x %1

   shld  %%x,%%x, 5
%endmacro

%macro ROL_30 1.nolist
  %xdefine %%x %1

   shld  %%x,%%x, 30
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; textual rotation of W array
;;
%macro ROTATE_W 0.nolist
  %xdefine W_minus_32   W_minus_28
  %xdefine W_minus_28   W_minus_24
  %xdefine W_minus_24   W_minus_20
  %xdefine W_minus_20   W_minus_16
  %xdefine W_minus_16   W_minus_12
  %xdefine W_minus_12   W_minus_08
  %xdefine W_minus_08   W_minus_04
  %xdefine W_minus_04   W
  %xdefine W   W_minus_32
%endmacro

;;
;; SHA1 update round:
;;    - F1 magic is used (and imbedded into the macros directly)
;;    - 16 bytes of input are swapped
;;
%macro SHA1_UPDATE_RND_F1_BSWAP 7.nolist
  %xdefine %%A %1
  %xdefine %%B %2
  %xdefine %%C %3
  %xdefine %%D %4
  %xdefine %%E %5
  %xdefine %%nr %6
  %xdefine %%Wchunk %7

   vpshufb  W, %%Wchunk, XMM_SHUFB_BSWAP
   vpaddd   %%Wchunk, W, oword [K_XMM]
   vmovdqa  oword [rsp + (%%nr & 15)*4], %%Wchunk
   mov      T1,%%C   ; F1
   mov      T2,%%A
   xor      T1,%%D   ; F1
   and      T1,%%B   ; F1
   ROL_5    T2
   xor      T1,%%D   ; F1
   add      %%E, T2
   ROL_30   %%B
   add      T1, dword [rsp + (%%nr & 15)*4]
   add      %%E,T1

   ROTATE_W
%endmacro

;;
;; SHA1 update round:
;;    - F1 magic is used (and imbedded into the macros directly)
;;
%macro SHA1_UPDATE_RND_F1 6.nolist
  %xdefine %%A %1
  %xdefine %%B %2
  %xdefine %%C %3
  %xdefine %%D %4
  %xdefine %%E %5
  %xdefine %%nr %6

   mov      T1,%%C   ; F1
   mov      T2,%%A
   xor      T1,%%D   ; F1
   ROL_5    T2
   and      T1,%%B   ; F1
   xor      T1,%%D   ; F1
   add      %%E, T2
   ROL_30   %%B
   add      T1, dword [rsp + (%%nr & 15)*4]
   add      %%E,T1
%endmacro

;;
;; update W
;;
%macro W_CALC 1.nolist
  %xdefine %%nr %1

  %assign %%W_CALC_ahead  8

  %assign %%i  (%%nr + %%W_CALC_ahead)

   %if (%%i < 20)
  %xdefine K_XMM  K_BASE
   %elif (%%i < 40)
  %xdefine K_XMM  K_BASE+16
   %elif (%%i < 60)
  %xdefine K_XMM  K_BASE+32
   %else
  %xdefine K_XMM  K_BASE+48
   %endif

   %if (%%i < 32)
      %if ((%%i & 3) == 0)        ;; just scheduling to interleave with ALUs
         vpalignr W, W_minus_12, W_minus_16, 8       ; w[i-14]
         vpsrldq  W_TMP, W_minus_04, 4               ; w[i-3]
         vpxor    W, W, W_minus_08
      %elif ((%%i & 3) == 1)
         vpxor    W_TMP, W_TMP, W_minus_16
         vpxor    W, W, W_TMP
         vpslldq  W_TMP2, W, 12
      %elif ((%%i & 3) == 2)
         vpslld   W_TMP, W, 1
         vpsrld   W, W, 31
         vpor     W_TMP, W_TMP, W
         vpslld   W, W_TMP2, 2
         vpsrld   W_TMP2, W_TMP2, 30
      %elif ((%%i & 3) == 3)
         vpxor    W_TMP, W_TMP, W
         vpxor    W, W_TMP, W_TMP2
         vpaddd   W_TMP, W, oword [K_XMM]
         vmovdqa  oword [rsp + ((%%i & (~3)) & 15)*4],W_TMP

         ROTATE_W
      %endif

;; %elif (i < 83)
   %elif (%%i < 80)
      %if ((%%i & 3) == 0)  ;; scheduling to interleave with ALUs
         vpalignr W_TMP, W_minus_04, W_minus_08, 8
         vpxor    W, W, W_minus_28      ;; W == W_minus_32
      %elif ((%%i & 3) == 1)
         vpxor    W_TMP, W_TMP, W_minus_16
         vpxor    W, W, W_TMP
      %elif ((%%i & 3) == 2)
         vpslld   W_TMP, W, 2
         vpsrld   W, W, 30
         vpor     W, W_TMP, W
      %elif ((%%i & 3) == 3)
         vpaddd   W_TMP, W, oword [K_XMM]
         vmovdqa  oword [rsp + ((%%i & (~3)) & 15)*4],W_TMP

         ROTATE_W
      %endif

   %endif
%endmacro

;;
;; Regular hash update
;;
%macro SHA1_UPDATE_REGULAR 7.nolist
  %xdefine %%A %1
  %xdefine %%B %2
  %xdefine %%C %3
  %xdefine %%D %4
  %xdefine %%E %5
  %xdefine %%nr %6
  %xdefine %%MagiF %7

   W_CALC   %%nr

   add      %%E, dword [rsp + (%%nr & 15)*4]
   %%MagiF    %%B,%%C,%%D
   add      %%D, dword [rsp +((%%nr+1) & 15)*4]
   ROL_30   %%B
   mov      T3,%%A
   add      %%E, T1
   ROL_5    T3
   add      T3, %%E
   mov      %%E, T3

   W_CALC   %%nr+1

   ROL_5    T3
   add      %%D,T3
   %%MagiF    %%A,%%B,%%C
   add      %%D, T1
   ROL_30   %%A

; write: %1, %2
; rotate: %1<=%4, %2<=%5, %3<=%1, %4<=%2, %5<=%3
%endmacro

;; update hash macro
%macro UPDATE_HASH 2.nolist
  %xdefine %%hash0 %1
  %xdefine %%hashAdd %2

     add    %%hashAdd, %%hash0
     mov    %%hash0, %%hashAdd
%endmacro

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR

K_XMM_AR       dd SHA1_K1, SHA1_K1, SHA1_K1, SHA1_K1
               dd SHA1_K2, SHA1_K2, SHA1_K2, SHA1_K2
               dd SHA1_K3, SHA1_K3, SHA1_K3, SHA1_K3
               dd SHA1_K4, SHA1_K4, SHA1_K4, SHA1_K4

shuffle_mask   DD 00010203h
               DD 04050607h
               DD 08090a0bh
               DD 0c0d0e0fh

;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA1(DigestSHA1 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;*****************************************************************************************

;;
;; Lib = Y8
;;
;; Caller = ippsSHA1Update
;; Caller = ippsSHA1Final
;; Caller = ippsSHA1MessageDigest
;;
;; Caller = ippsHMACSHA1Update
;; Caller = ippsHMACSHA1Final
;; Caller = ippsHMACSHA1MessageDigest
;;

;; assign hash values to GPU registers
%xdefine A      ecx
%xdefine B      eax
%xdefine C      edx
%xdefine D      r8d
%xdefine E      r9d

;; temporary
%xdefine T1     r10d
%xdefine T2     r11d
%xdefine T3     r13d
%xdefine T4     r13d

%xdefine W_TMP  xmm0
%xdefine W_TMP2 xmm1

%xdefine W0     xmm2
%xdefine W4     xmm3
%xdefine W8     xmm4
%xdefine W12    xmm5
%xdefine W16    xmm6
%xdefine W20    xmm7
%xdefine W24    xmm8
%xdefine W28    xmm9

;; endianness swap constant
%xdefine XMM_SHUFB_BSWAP  xmm10

;; K_BASE contains [K_XMM_AR] address
%xdefine K_BASE  r12


align IPP_ALIGN_FACTOR
IPPASM UpdateSHA1,PUBLIC
%assign LOCAL_FRAME (16*4)
        USES_GPR rdi,rsi,r12,r13,r14
        USES_XMM_AVX xmm6,xmm7,xmm8,xmm9,xmm10
        COMP_ABI 4

;;
;; rdi = digest ptr
;; rsi = data block ptr
;; rdx = data length
;; rcx = dummy

%xdefine MBS_SHA1    (64)

   movsxd         r14, edx

   movdqa         XMM_SHUFB_BSWAP, oword [rel shuffle_mask]   ; load shuffle mask
   lea            K_BASE, [rel K_XMM_AR]                          ; SHA1 const array address

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha1_block_loop:
   mov            A, dword [rdi]         ; load initial hash value
   mov            B, dword [rdi+4]
   mov            C, dword [rdi+8]
   mov            D, dword [rdi+12]
   mov            E, dword [rdi+16]

   movdqu         W28, oword [rsi]       ; load buffer content
   movdqu         W24, oword [rsi+16]
   movdqu         W20, oword [rsi+32]
   movdqu         W16, oword [rsi+48]

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;;SHA1_MAIN_BODY

%xdefine W   W0
%xdefine W_minus_04   W4
%xdefine W_minus_08   W8
%xdefine W_minus_12   W12
%xdefine W_minus_16   W16
%xdefine W_minus_20   W20
%xdefine W_minus_24   W24
%xdefine W_minus_28   W28
%xdefine W_minus_32   W

   ;; assignment
%xdefine K_XMM  K_BASE

;;F textequ <F1>
   SHA1_UPDATE_RND_F1_BSWAP   A,B,C,D,E, 0, W28
   SHA1_UPDATE_RND_F1         E,A,B,C,D, 1
   SHA1_UPDATE_RND_F1         D,E,A,B,C, 2
   SHA1_UPDATE_RND_F1         C,D,E,A,B, 3

   SHA1_UPDATE_RND_F1_BSWAP   B,C,D,E,A, 4, W24
   SHA1_UPDATE_RND_F1         A,B,C,D,E, 5
   SHA1_UPDATE_RND_F1         E,A,B,C,D, 6
   SHA1_UPDATE_RND_F1         D,E,A,B,C, 7

   SHA1_UPDATE_RND_F1_BSWAP   C,D,E,A,B, 8, W20
   SHA1_UPDATE_RND_F1         B,C,D,E,A, 9
   SHA1_UPDATE_RND_F1         A,B,C,D,E, 10
   SHA1_UPDATE_RND_F1         E,A,B,C,D, 11

   SHA1_UPDATE_RND_F1_BSWAP   D,E,A,B,C, 12, W16

   W_CALC 8
   W_CALC 9
   W_CALC 10

   SHA1_UPDATE_RND_F1         C,D,E,A,B, 13

   W_CALC 11
   W_CALC 12

   SHA1_UPDATE_RND_F1         B,C,D,E,A, 14

   W_CALC 13
   W_CALC 14
   W_CALC 15

   SHA1_UPDATE_RND_F1         A,B,C,D,E, 15

   SHA1_UPDATE_REGULAR        E,A,B,C,D,16, F1
   SHA1_UPDATE_REGULAR        C,D,E,A,B,18, F1

;;F textequ <F2>
   SHA1_UPDATE_REGULAR        A,B,C,D,E,20, F2
   SHA1_UPDATE_REGULAR        D,E,A,B,C,22, F2
   SHA1_UPDATE_REGULAR        B,C,D,E,A,24, F2
   SHA1_UPDATE_REGULAR        E,A,B,C,D,26, F2
   SHA1_UPDATE_REGULAR        C,D,E,A,B,28, F2

   SHA1_UPDATE_REGULAR        A,B,C,D,E,30, F2
   SHA1_UPDATE_REGULAR        D,E,A,B,C,32, F2
   SHA1_UPDATE_REGULAR        B,C,D,E,A,34, F2
   SHA1_UPDATE_REGULAR        E,A,B,C,D,36, F2
   SHA1_UPDATE_REGULAR        C,D,E,A,B,38, F2

;;F textequ <F3>
   SHA1_UPDATE_REGULAR        A,B,C,D,E,40, F3
   SHA1_UPDATE_REGULAR        D,E,A,B,C,42, F3
   SHA1_UPDATE_REGULAR        B,C,D,E,A,44, F3
   SHA1_UPDATE_REGULAR        E,A,B,C,D,46, F3
   SHA1_UPDATE_REGULAR        C,D,E,A,B,48, F3

   SHA1_UPDATE_REGULAR        A,B,C,D,E,50, F3
   SHA1_UPDATE_REGULAR        D,E,A,B,C,52, F3
   SHA1_UPDATE_REGULAR        B,C,D,E,A,54, F3
   SHA1_UPDATE_REGULAR        E,A,B,C,D,56, F3
   SHA1_UPDATE_REGULAR        C,D,E,A,B,58, F3

;;F textequ <F4>
   SHA1_UPDATE_REGULAR        A,B,C,D,E,60, F4
   SHA1_UPDATE_REGULAR        D,E,A,B,C,62, F4
   SHA1_UPDATE_REGULAR        B,C,D,E,A,64, F4
   SHA1_UPDATE_REGULAR        E,A,B,C,D,66, F4
   SHA1_UPDATE_REGULAR        C,D,E,A,B,68, F4

   SHA1_UPDATE_REGULAR        A,B,C,D,E,70, F4
   SHA1_UPDATE_REGULAR        D,E,A,B,C,72, F4
   SHA1_UPDATE_REGULAR        B,C,D,E,A,74, F4
   SHA1_UPDATE_REGULAR        E,A,B,C,D,76, F4
   SHA1_UPDATE_REGULAR        C,D,E,A,B,78, F4
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   UPDATE_HASH    dword [rdi],   A
   UPDATE_HASH    dword [rdi+4], B
   UPDATE_HASH    dword [rdi+8], C
   UPDATE_HASH    dword [rdi+12],D
   UPDATE_HASH    dword [rdi+16],E

   add            rsi, MBS_SHA1
   sub            r14, MBS_SHA1
   jg             .sha1_block_loop

   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC UpdateSHA1

%endif    ;; _IPP32E >= _IPP32E_E9
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA1_

