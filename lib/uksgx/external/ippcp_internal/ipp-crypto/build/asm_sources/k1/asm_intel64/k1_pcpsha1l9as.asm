;===============================================================================
; Copyright 2017-2021 Intel Corporation
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
%if (_IPP32E >= _IPP32E_L9 )


;;
;; assignments
;;
%xdefine hA  eax        ;; hash values into GPU registers
%xdefine F  ebp
%xdefine hB  ebx
%xdefine hC  ecx
%xdefine hD  edx
%xdefine hE  r8d

%xdefine T1  r10d       ;; SHA1 round computation (temporary)
%xdefine T2  r11d

%xdefine W00  ymm2      ;; W values into YMM registers
%xdefine W04  ymm3
%xdefine W08  ymm4
%xdefine W12  ymm5
%xdefine W16  ymm6
%xdefine W20  ymm7
%xdefine W24  ymm8
%xdefine W28  ymm9

%xdefine W16L  xmm6
%xdefine W20L  xmm7
%xdefine W24L  xmm8
%xdefine W28L  xmm9

%xdefine WTMP1  ymm0    ;; msg schedulling computation (temporary)
%xdefine WTMP2  ymm1
%xdefine WTMP3  ymm10

%xdefine YMM_SHUFB  ymm11  ;; byte swap constant
%xdefine YMM_K  ymm12  ;; sha1 round constant value

%xdefine F_PTR  r13        ;; frame ptr/data block ptr
;;W_PTR textequ r12      ;; frame ptr/data block ptr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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
;; msg schedulling for initial 00-15 sha1 rounds:
;;    - byte swap input
;;    - add sha1 round constant
%macro W_CALC_00_15 2.nolist
  %xdefine %%nr %1
  %xdefine %%Wchunk %2

   vpshufb  W, %%Wchunk, YMM_SHUFB
   vpaddd   %%Wchunk, W, YMM_K
   vmovdqa  ymmword [F_PTR + (%%nr)*sizeof(dword)*2], %%Wchunk

   ROTATE_W
%endmacro

;;
;; msg schedulling for other 16-79 sha1 rounds:
;;
%macro W_CALC 1.nolist
  %xdefine %%rndw %1

   %if (%%rndw < 32)
      %if ((%%rndw & 3) == 0)        ;; scheduling to interleave with ALUs
         vpalignr W, W_minus_12, W_minus_16, 8     ;; w[t-14]
         vpsrldq  WTMP1, W_minus_04, 4             ;; w[t-3]
         vpxor    W, W, W_minus_16
         vpxor    WTMP1, WTMP1, W_minus_08
      %elif ((%%rndw & 3) == 1)
         vpxor    W, W, WTMP1
         vpsrld   WTMP1, W, 31
         vpslldq  WTMP2, W, 12
         vpaddd   W, W, W
      %elif ((%%rndw & 3) == 2)
         vpsrld   WTMP3, WTMP2, 30
         vpxor    W, W, WTMP1
         vpslld   WTMP2, WTMP2, 2
         vpxor    W, W, WTMP3
      %elif ((%%rndw & 3) == 3)
         vpxor    W, W, WTMP2
         ;;vpaddd   WTMP1, W, ymmword [K_SHA1_PTR]
         vpaddd   WTMP1, W, YMM_K
         vmovdqa  ymmword [F_PTR+4*sizeof(ymmword)+((%%rndw & 15)/4)*sizeof(ymmword)],WTMP1
         ROTATE_W
      %endif

   %elif (%%rndw < 80)
      %if ((%%rndw & 3) == 0)        ;; scheduling to interleave with ALUs
         vpalignr WTMP1, W_minus_04, W_minus_08, 8
         vpxor    W, W, W_minus_28  ;; W == W_minus_32
      %elif ((%%rndw & 3) == 1)
         vpxor    W, W, W_minus_16
         vpxor    W, W, WTMP1
      %elif ((%%rndw & 3) == 2)
         vpslld   WTMP1, W, 2
         vpsrld   W, W, 30
      %elif ((%%rndw & 3) == 3)
         vpxor    W, WTMP1, W
         ;;vpaddd   WTMP1, W, ymmword [K_SHA1_PTR]
         vpaddd   WTMP1, W, YMM_K
         vmovdqa  ymmword [F_PTR+4*sizeof(ymmword)+((%%rndw & 15)/4)*sizeof(ymmword)],WTMP1
         ROTATE_W
      %endif

   %endif
%endmacro

;;
;; update hash macro
;;
%macro UPDATE_HASH 2.nolist
  %xdefine %%hashMem %1
  %xdefine %%hash %2

     add    %%hash, %%hashMem
     mov    %%hashMem, %%hash
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; textual rotation of HASH arguments
;;
%macro ROTATE_H 0.nolist
  %xdefine %%_X   hE
  %xdefine hE   hD
  %xdefine hD   hC
  %xdefine hC   hB
  %xdefine hB   F
  %xdefine F   hA
  %xdefine hA  %%_X
%endmacro

;;
;; SHA1 rounds 0 - 19
;; on entry:
;;    - a, f(), b', c, d, e values
;;    where: f() = F(b,c,d) = (b&c) & (~b&d) already pre-computed for this round
;;            b' = rotl(b,30)                already pre-computed for the next round
;; note:
;;    %if nr==19 the f(b,c,d)=b^c^d precomputed
;;
%macro SHA1_ROUND_00_19 1.nolist
  %xdefine %%nr %1

   ;; hE+=W[nr]+K
   add   hE, dword [F_PTR+((%%nr & 15)/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]

%if (((%%nr+1) & 15) == 0)
   add   F_PTR, (16/4)*sizeof(ymmword)
%endif

%if (%%nr < 19)
   andn  T1,hA,hC           ;; ~hB&hC (next round)
%endif
   add   hE, F              ;; hE += F()
   rorx  T2,hA, 27          ;; hA<<<5
   rorx  F, hA, 2           ;; hB<<<30 (next round)
%if (%%nr < 19)
   and   hA, hB             ;; hB&hC (next round)
%endif
   add   hE, T2             ;; hE += (hA<<<5)
%if (%%nr < 19)
   xor   hA, T1             ;; F() = (hB&hC)^(~hB&hC) (next round)
%else
   xor   hA, hB             ;; F() = hB^hC^hD next round
   xor   hA, hC
%endif

   ROTATE_H
%endmacro

;;
;; SHA1 rounds 20 - 39
;; on entry:
;;    - a, f(), b', c, d, e values
;;    where: f() = F(b,c,d) = b^c^d already pre-computed for this round
;;            b' = rotl(b,30)       already pre-computed for the next round
;;
;; note:
;;    %if nr==39 the f(b,c,d)=(b^c)&(c^d) precomputed
;;
%macro SHA1_ROUND_20_39 1.nolist
  %xdefine %%nr %1

   ;; hE+=W[nr]+K
   add   hE, dword [F_PTR+((%%nr & 15)/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]

%if (((%%nr+1) & 15) == 0)
   add   F_PTR, (16/4)*sizeof(ymmword)
%endif

   add   hE, F              ;; hE += F()
   rorx  T2,hA, 27          ;; hA<<<5
   rorx  F, hA, 2           ;; hB<<<30 (next round)
   xor   hA, hB             ;; hB^hC (next round)
   add   hE, T2             ;; hE += (hA<<<5)
%if (%%nr < 39)
   xor   hA, hC             ;; F() = hB^hC^hD  (next round)
%else
   mov   T1, hB             ;; hC^hD (next round)
   xor   T1, hC             ;;
   and   hA, T1             ;; (hB^hC)&(hC^hD)
%endif

   ROTATE_H
%endmacro

;;
;; SHA1 rounds 40 - 59
;; on entry:
;;    - a, f(), b', c, d, e values
;;    where: f() = (b&c)^(c&d) already pre-computed (part of F()) for this round
;;            b' = rotl(b,30)       already pre-computed for the next round
;;
;; F(b,c,d) = (b&c)^(b&d)^(c&d)
;;
;; note, using GF(2): arithmetic
;; F(b,c,d) = (b&c)^(b&d)^(c&d) ~ bc+bd+cd
;;                              =(b+c)(c+d) +c^2 = (b+c)(c+d) +c
;;                              ~ ((b^c)&(c^d)) ^c
;; direct substitution:
;;    (b+c)(c+d) = bc + bd + c^2 + cd, but c^2 = c
;;
%macro SHA1_ROUND_40_59 1.nolist
  %xdefine %%nr %1

   ;; hE+=W[nr]+K
   add   hE, dword [F_PTR+((%%nr & 15)/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]

%if (((%%nr+1) & 15) == 0)
   add   F_PTR, (16/4)*sizeof(ymmword)
%endif

   xor   F, hC              ;; F() = ((b^c)&(c^d)) ^c
%if(%%nr < 59)
   mov   T1, hB             ;; hC^hD (next round)
   xor   T1, hC             ;;
%endif
   add   hE, F              ;; hE += F()
   rorx  T2,hA, 27          ;; hA<<<5
   rorx  F, hA, 2           ;; hB<<<30 (next round)
   xor   hA, hB             ;; hB^hC (next round)
   add   hE, T2             ;; hE += (hA<<<5)
%if(%%nr < 59)
   and   hA, T1             ;; (hB^hC)&(hC^hD) (next round)
%else
   xor   hA, hC             ;; (hB^hC^hD) (next round)
%endif

   ROTATE_H
%endmacro

;;
;; SHA1 rounds 60 - 79
;; on entry:
;;    - a, f(), b', c, d, e values
;;    where: f() = F(b,c,d) = b^c^d already pre-computed for this round
;;            b' = rotl(b,30)       already pre-computed for the next round
;;
%macro SHA1_ROUND_60_79 1.nolist
  %xdefine %%nr %1

   ;; hE+=W[nr]+K
   add   hE, dword [F_PTR+((%%nr & 15)/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]

%if (((%%nr+1) & 15) == 0)
   add   F_PTR, (16/4)*sizeof(ymmword)
%endif

   add   hE, F              ;; hE += F()
   rorx  T2,hA, 27          ;; hA<<<5
%if (%%nr < 79)
   rorx  F, hA, 2           ;; hB<<<30 (next round)
   xor   hA, hB             ;; hB^hC (next round)
%endif
   add   hE, T2             ;; hE += (hA<<<5)
%if (%%nr < 79)
   xor   hA, hC             ;; F() = hB^hC^hD  (next round)
%endif

   ROTATE_H
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR

SHA1_YMM_K     dd 05a827999h, 05a827999h, 05a827999h, 05a827999h, 05a827999h, 05a827999h, 05a827999h, 05a827999h
               dd 06ed9eba1h, 06ed9eba1h, 06ed9eba1h, 06ed9eba1h, 06ed9eba1h, 06ed9eba1h, 06ed9eba1h, 06ed9eba1h
               dd 08f1bbcdch, 08f1bbcdch, 08f1bbcdch, 08f1bbcdch, 08f1bbcdch, 08f1bbcdch, 08f1bbcdch, 08f1bbcdch
               dd 0ca62c1d6h, 0ca62c1d6h, 0ca62c1d6h, 0ca62c1d6h, 0ca62c1d6h, 0ca62c1d6h, 0ca62c1d6h, 0ca62c1d6h

SHA1_YMM_BF   dd 00010203h,04050607h,08090a0bh,0c0d0e0fh
              dd 00010203h,04050607h,08090a0bh,0c0d0e0fh


;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA1(DigestSHA1 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;*****************************************************************************************
align IPP_ALIGN_FACTOR
IPPASM UpdateSHA1,PUBLIC
%assign LOCAL_FRAME (sizeof(dword)*80*2)
        USES_GPR rdi,rsi,rbp,rbx,r12,r13,r14,r15
        USES_XMM_AVX xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12
        COMP_ABI 4

;; rdi = hash ptr
;; rsi = data block ptr
;; rdx = data length in bytes
;; rcx = dummy

%xdefine MBS_SHA1    (64)

   mov            r15, rsp                ; store orifinal rsp
   and            rsp, -IPP_ALIGN_FACTOR  ; 32-byte aligned stack

   movsxd         r14, edx                ; input length in bytes

   vmovdqa        YMM_SHUFB, [rel SHA1_YMM_BF] ; load byte shuffler

   mov      hA, dword [rdi]         ; load initial hash value
   mov      F,  dword [rdi+sizeof(dword)]
   mov      hC, dword [rdi+2*sizeof(dword)]
   mov      hD, dword [rdi+3*sizeof(dword)]
   mov      hE, dword [rdi+4*sizeof(dword)]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data 2 block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; assignment:
;; - W00,...,W28                 are fixed
;; - W_minus_04,...,W_minus_32   are rorarted
;; - W  corresponds to W[t]
;;
%xdefine W   W00
%xdefine W_minus_04   W04
%xdefine W_minus_08   W08
%xdefine W_minus_12   W12
%xdefine W_minus_16   W16
%xdefine W_minus_20   W20
%xdefine W_minus_24   W24
%xdefine W_minus_28   W28
%xdefine W_minus_32   W

align IPP_ALIGN_FACTOR
.sha1_block2_loop:
   lea      F_PTR, [rsi+MBS_SHA1]   ; next block

   cmp      r14, MBS_SHA1           ; %if single block processed
   cmovbe   F_PTR, rsi              ; use the same data block address

   ;;
   ;; load data block and merge next data block
   ;;
   vmovdqa  YMM_K, [rel SHA1_YMM_K]     ; pre-load sha1 constant

   vmovdqu  W28L, xmmword [rsi]                ; load data block
   vmovdqu  W24L, xmmword [rsi+1*sizeof(xmmword)]
   vmovdqu  W20L, xmmword [rsi+2*sizeof(xmmword)]
   vmovdqu  W16L, xmmword [rsi+3*sizeof(xmmword)]

   vinserti128 W28, W28, xmmword [F_PTR], 1     ; merge next data block
   vinserti128 W24, W24, xmmword [F_PTR+1*sizeof(xmmword)], 1
   vinserti128 W20, W20, xmmword [F_PTR+2*sizeof(xmmword)], 1
   vinserti128 W16, W16, xmmword [F_PTR+3*sizeof(xmmword)], 1

   mov      F_PTR, rsp     ;; set local data pointer

   W_CALC_00_15   0, W28   ;; msg scheduling for rounds 00 .. 15
   W_CALC_00_15   4, W24   ;;
   W_CALC_00_15   8, W20   ;; msg scheduling for rounds 08 .. 15
   W_CALC_00_15  12, W16   ;;

   rorx     hB, F, 2        ;; pre-compute (b<<<30) next round
   andn     T1,F, hD        ;; pre-compute F1(F,hC,hD) = hB&hC ^(~hB&hD)
   and      F, hC
   xor      F, T1

   W_CALC               16 ;; msg schedilling ahead 16 rounds
   SHA1_ROUND_00_19      0 ;; sha1 round

   W_CALC               17
   SHA1_ROUND_00_19      1

   W_CALC               18
   SHA1_ROUND_00_19      2

   W_CALC               19
   SHA1_ROUND_00_19      3

   ; pre-load sha1 constant
   vmovdqa              YMM_K, [rel SHA1_YMM_K+sizeof(ymmword)]
   W_CALC               20
   SHA1_ROUND_00_19      4

   W_CALC               21
   SHA1_ROUND_00_19      5

   W_CALC               22
   SHA1_ROUND_00_19      6

   W_CALC               23
   SHA1_ROUND_00_19      7

   W_CALC               24
   SHA1_ROUND_00_19      8

   W_CALC               25
   SHA1_ROUND_00_19      9

   W_CALC               26
   SHA1_ROUND_00_19     10

   W_CALC               27
   SHA1_ROUND_00_19     11

   W_CALC               28
   SHA1_ROUND_00_19     12

   W_CALC               29
   SHA1_ROUND_00_19     13

   W_CALC               30
   SHA1_ROUND_00_19     14

   W_CALC               31
   SHA1_ROUND_00_19     15

   W_CALC               32
   SHA1_ROUND_00_19     16

   W_CALC               33
   SHA1_ROUND_00_19     17

   W_CALC               34
   SHA1_ROUND_00_19     18

   W_CALC               35
   SHA1_ROUND_00_19     19

   W_CALC               36
   SHA1_ROUND_20_39     20

   W_CALC               37
   SHA1_ROUND_20_39     21

   W_CALC               38
   SHA1_ROUND_20_39     22

   W_CALC               39
   SHA1_ROUND_20_39     23

   ; pre-load sha1 constant
   vmovdqa              YMM_K, [rel SHA1_YMM_K+2*sizeof(ymmword)]
   W_CALC               40
   SHA1_ROUND_20_39     24

   W_CALC               41
   SHA1_ROUND_20_39     25

   W_CALC               42
   SHA1_ROUND_20_39     26

   W_CALC               43
   SHA1_ROUND_20_39     27

   W_CALC               44
   SHA1_ROUND_20_39     28

   W_CALC               45
   SHA1_ROUND_20_39     29

   W_CALC               46
   SHA1_ROUND_20_39     30

   W_CALC               47
   SHA1_ROUND_20_39     31

   W_CALC               48
   SHA1_ROUND_20_39     32

   W_CALC               49
   SHA1_ROUND_20_39     33

   W_CALC               50
   SHA1_ROUND_20_39     34

   W_CALC               51
   SHA1_ROUND_20_39     35

   W_CALC               52
   SHA1_ROUND_20_39     36

   W_CALC               53
   SHA1_ROUND_20_39     37

   W_CALC               54
   SHA1_ROUND_20_39     38

   W_CALC               55
   SHA1_ROUND_20_39     39

   W_CALC               56
   SHA1_ROUND_40_59     40

   W_CALC               57
   SHA1_ROUND_40_59     41

   W_CALC               58
   SHA1_ROUND_40_59     42

   W_CALC               59
   SHA1_ROUND_40_59     43

   ; pre-load sha1 constant
   vmovdqa              YMM_K, [rel SHA1_YMM_K+3*sizeof(ymmword)]
   W_CALC               60
   SHA1_ROUND_40_59     44

   W_CALC               61
   SHA1_ROUND_40_59     45

   W_CALC               62
   SHA1_ROUND_40_59     46

   W_CALC               63
   SHA1_ROUND_40_59     47

   W_CALC               64
   SHA1_ROUND_40_59     48

   W_CALC               65
   SHA1_ROUND_40_59     49

   W_CALC               66
   SHA1_ROUND_40_59     50

   W_CALC               67
   SHA1_ROUND_40_59     51

   W_CALC               68
   SHA1_ROUND_40_59     52

   W_CALC               69
   SHA1_ROUND_40_59     53

   W_CALC               70
   SHA1_ROUND_40_59     54

   W_CALC               71
   SHA1_ROUND_40_59     55

   W_CALC               72
   SHA1_ROUND_40_59     56

   W_CALC               73
   SHA1_ROUND_40_59     57

   W_CALC               74
   SHA1_ROUND_40_59     58

   W_CALC               75
   SHA1_ROUND_40_59     59

   W_CALC               76
   SHA1_ROUND_60_79     60

   W_CALC               77
   SHA1_ROUND_60_79     61

   W_CALC               78
   SHA1_ROUND_60_79     62

   W_CALC               79
   SHA1_ROUND_60_79     63

   SHA1_ROUND_60_79     64
   SHA1_ROUND_60_79     65
   SHA1_ROUND_60_79     66
   SHA1_ROUND_60_79     67
   SHA1_ROUND_60_79     68
   SHA1_ROUND_60_79     69
   SHA1_ROUND_60_79     70
   SHA1_ROUND_60_79     71
   SHA1_ROUND_60_79     72
   SHA1_ROUND_60_79     73
   SHA1_ROUND_60_79     74
   SHA1_ROUND_60_79     75
   SHA1_ROUND_60_79     76
   SHA1_ROUND_60_79     77
   SHA1_ROUND_60_79     78
   SHA1_ROUND_60_79     79

   lea      F_PTR, [rsp+sizeof(xmmword)]  ;; set local data pointer

   ;; update hash values by 1-st data block
   UPDATE_HASH    dword [rdi],   hA
   UPDATE_HASH    dword [rdi+4], F
   UPDATE_HASH    dword [rdi+8], hC
   UPDATE_HASH    dword [rdi+12],hD
   UPDATE_HASH    dword [rdi+16],hE

   cmp      r14, MBS_SHA1*2
   jl       .done

   rorx     hB, F, 2              ;; pre-compute (b<<<30) next round
   andn     T1,F, hD              ;; pre-compute F1(F,hC,hD) = hB&hC ^(~hB&hD)
   and      F, hC
   xor      F, T1

   SHA1_ROUND_00_19      0
   SHA1_ROUND_00_19      1
   SHA1_ROUND_00_19      2
   SHA1_ROUND_00_19      3
   SHA1_ROUND_00_19      4
   SHA1_ROUND_00_19      5
   SHA1_ROUND_00_19      6
   SHA1_ROUND_00_19      7
   SHA1_ROUND_00_19      8
   SHA1_ROUND_00_19      9
   SHA1_ROUND_00_19     10
   SHA1_ROUND_00_19     11
   SHA1_ROUND_00_19     12
   SHA1_ROUND_00_19     13
   SHA1_ROUND_00_19     14
   SHA1_ROUND_00_19     15
   SHA1_ROUND_00_19     16
   SHA1_ROUND_00_19     17
   SHA1_ROUND_00_19     18
   SHA1_ROUND_00_19     19

   SHA1_ROUND_20_39     20
   SHA1_ROUND_20_39     21
   SHA1_ROUND_20_39     22
   SHA1_ROUND_20_39     23
   SHA1_ROUND_20_39     24
   SHA1_ROUND_20_39     25
   SHA1_ROUND_20_39     26
   SHA1_ROUND_20_39     27
   SHA1_ROUND_20_39     28
   SHA1_ROUND_20_39     29
   SHA1_ROUND_20_39     30
   SHA1_ROUND_20_39     31
   SHA1_ROUND_20_39     32
   SHA1_ROUND_20_39     33
   SHA1_ROUND_20_39     34
   SHA1_ROUND_20_39     35
   SHA1_ROUND_20_39     36
   SHA1_ROUND_20_39     37
   SHA1_ROUND_20_39     38
   SHA1_ROUND_20_39     39

   SHA1_ROUND_40_59     40
   SHA1_ROUND_40_59     41
   SHA1_ROUND_40_59     42
   SHA1_ROUND_40_59     43
   SHA1_ROUND_40_59     44
   SHA1_ROUND_40_59     45
   SHA1_ROUND_40_59     46
   SHA1_ROUND_40_59     47
   SHA1_ROUND_40_59     48
   SHA1_ROUND_40_59     49
   SHA1_ROUND_40_59     50
   SHA1_ROUND_40_59     51
   SHA1_ROUND_40_59     52
   SHA1_ROUND_40_59     53
   SHA1_ROUND_40_59     54
   SHA1_ROUND_40_59     55
   SHA1_ROUND_40_59     56
   SHA1_ROUND_40_59     57
   SHA1_ROUND_40_59     58
   SHA1_ROUND_40_59     59

   SHA1_ROUND_60_79     60
   SHA1_ROUND_60_79     61
   SHA1_ROUND_60_79     62
   SHA1_ROUND_60_79     63
   SHA1_ROUND_60_79     64
   SHA1_ROUND_60_79     65
   SHA1_ROUND_60_79     66
   SHA1_ROUND_60_79     67
   SHA1_ROUND_60_79     68
   SHA1_ROUND_60_79     69
   SHA1_ROUND_60_79     70
   SHA1_ROUND_60_79     71
   SHA1_ROUND_60_79     72
   SHA1_ROUND_60_79     73
   SHA1_ROUND_60_79     74
   SHA1_ROUND_60_79     75
   SHA1_ROUND_60_79     76
   SHA1_ROUND_60_79     77
   SHA1_ROUND_60_79     78
   SHA1_ROUND_60_79     79

   ;; update hash values by 2-nd data block
   UPDATE_HASH    dword [rdi],   hA
   UPDATE_HASH    dword [rdi+4], F
   UPDATE_HASH    dword [rdi+8], hC
   UPDATE_HASH    dword [rdi+12],hD
   UPDATE_HASH    dword [rdi+16],hE

   ;; unfortunately 2*80%6 != 0
   ;; and so need to re-order hA,F,hB,hC,hD,hE values
   ;; to match the code generated for 1-st block processing
   mov      hB,  hD    ; re-order data physically
   mov      hD,  hA
   mov      T1, hE
   mov      hE,  F
   mov      F,  hC
   mov      hC, T1

   ROTATE_H          ; re-order data logically
   ROTATE_H          ; twice, because 6 -(2*80%6) = 2

   add      rsi, MBS_SHA1*2
   sub      r14, MBS_SHA1*2
   jg       .sha1_block2_loop

.done:
   mov   rsp, r15
   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC UpdateSHA1

%endif    ;; _IPP32E >= _IPP32E_L9
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA1_

