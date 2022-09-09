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
;               Message block processing according to SHA512
;
;     Content:
;        UpdateSHA256
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA512_)
;;%if (_IPP32E >= _IPP32E_E9)
%if (_IPP32E == _IPP32E_E9 )

%xdefine W0    xmm0
%xdefine W1    xmm1
%xdefine W2    xmm2
%xdefine W3    xmm3
%xdefine W4    xmm4
%xdefine W5    xmm5
%xdefine W6    xmm6
%xdefine W7    xmm7
%xdefine xT0   xmm8
%xdefine xT1   xmm9
%xdefine xT2   xmm10
%xdefine SIGMA xmm11

;; assign hash values to GPU registers
%xdefine A     r8
%xdefine B     r9
%xdefine C     r10
%xdefine D     r11
%xdefine E     r12
%xdefine F     r13
%xdefine G     r14
%xdefine H     r15
%xdefine T1    rax
%xdefine T2    rbx
%xdefine T0    rbp
%xdefine KK_SHA512  rcx

%macro ROTATE_H 0.nolist
  %xdefine %%_TMP   H
  %xdefine H   G
  %xdefine G   F
  %xdefine F   E
  %xdefine E   D
  %xdefine D   C
  %xdefine C   B
  %xdefine B   A
  %xdefine A   %%_TMP
%endmacro

%macro ROTATE_W 0.nolist
  %xdefine %%DUMMY   W0
  %xdefine W0   W1
  %xdefine W1   W2
  %xdefine W2   W3
  %xdefine W3   W4
  %xdefine W4   W5
  %xdefine W5   W6
  %xdefine W6   W7
  %xdefine W7   %%DUMMY
%endmacro

%macro ROR64 2.nolist
  %xdefine %%r %1
  %xdefine %%nbits %2

   %if _IPP32E >= _IPP32E_L9
   rorx  %%r,%%r,%%nbits
   %elif _IPP32E >= _IPP32E_Y8
   shld  %%r,%%r,(64-%%nbits)
   %else
   ror   %%r,%%nbits
   %endif
%endmacro

;;
;; CHJ(x,y,z) = (x & y) ^ (~x & z)
;;
;; CHJ(x,y,z) = (x & y) ^ (~x & z)
;;            = (x&y) ^ ((1^x) &z)
;;            = (x&y) ^ (z ^ x&z)
;;            = x&y ^ z ^ x&z
;;            = x&(y^z) ^z
;;
;; => CHJ(E,F,G) = ((F^G) & E) ^G
;;
%macro CHJ 4.nolist
  %xdefine %%F %1
  %xdefine %%X %2
  %xdefine %%Y %3
  %xdefine %%Z %4

   mov   %%F, %%Y
   xor   %%F, %%Z
   and   %%F, %%X
   xor   %%F, %%Z
%endmacro

;;
;; MAJ(x,y,z) = (x & y) ^ (x & z) ^ (y & z)
;;
;; MAJ(x,y,z) = (x&y) ^ (x&z) ^ (y&z)
;;            = (x&y) ^ (x&z) ^ (y&z) ^ (z&z) ^z   // note: ((z&z) ^z) = 0
;;            = x&(y^z) ^ z&(y^z) ^z
;;            = (x^z)&(y^z) ^z
;;
;; => MAJ(A,B,C) = ((A^C) & B) ^ (A&C)
;;
%macro MAJ 4.nolist
  %xdefine %%F %1
  %xdefine %%X %2
  %xdefine %%Y %3
  %xdefine %%Z %4

   mov   %%F, %%X
   xor   %%F, %%Z     ;; maj = x^z
   xor   %%Y, %%Z     ;; y ^= z
   and   %%F, %%Y     ;; maj = (x^z)&(y^z)
   xor   %%F, %%Z     ;; maj = (x^z)&(y^z) ^z
   xor   %%Y, %%Z     ;; restore y
%endmacro

;;
;; SUM1(x) = ROR64(x,14) ^ ROR64(x,18) ^ ROR64(x,41)
;;
;;=> SUM1(x) = ROR((ROR((ROR(x, 23) ^x), 4) ^x), 14)
;;
%macro SUM1 2.nolist
  %xdefine %%F %1
  %xdefine %%X %2

   mov   %%F, %%X
   ROR64 %%F, 23
   xor   %%F, %%X
   ROR64 %%F, 4
   xor   %%F, %%X
   ROR64 %%F, 14
%endmacro

;;
;;    SUM0(x) = ROR64(x,28) ^ ROR64(x,34) ^ ROR64(x,39)
;;
;; => SUM0(x) = ROR((ROR((ROR(x,  5) ^x), 6) ^x), 28)
;;
%macro SUM0 2.nolist
  %xdefine %%F %1
  %xdefine %%X %2

   mov   %%F, %%X
   ROR64 %%F, 5
   xor   %%F, %%X
   ROR64 %%F, 6
   xor   %%F, %%X
   ROR64 %%F, 28
%endmacro

;; regular SHA512 step
;;
;;    Ipp64u T1 = H + SUM1(E) + CHJ(E,F,G) + K_SHA512[t] + W[t];
;;    Ipp64u T2 =     SUM0(A) + MAJ(A,B,C);
;;    D+= T1;
;;    H = T1 + T2;
;;
;; where
;;    SUM1(x) = ROR64(x,14) ^ ROR64(x,18) ^ ROR64(x,41)
;;    SUM0(x) = ROR64(x,28) ^ ROR64(x,34) ^ ROR64(x,39)
;;
;;    CHJ(x,y,z) = (x & y) ^ (~x & z)
;;    MAJ(x,y,z) = (x & y) ^ (x & z) ^ (y & z) = (x&y)^((x^y)&z)
;;
%macro SHA512_STEP_v0 1.nolist
  %xdefine %%nr %1

   add   H, qword [rsp+(%%nr & 1)*sizeof(qword)]
   SUM1  T0, E
   CHJ   T1, E,F,G
   add   H, T1
   add   H, T0

   add   D, H

   SUM0  T0, A
   MAJ   T1, A,B,C
   add   H, T1
   add   H, T0

   ROTATE_H
%endmacro

%macro SHA512_2step 9.nolist
  %xdefine %%nr %1
  %xdefine %%A %2
  %xdefine %%B %3
  %xdefine %%C %4
  %xdefine %%D %5
  %xdefine %%E %6
  %xdefine %%F %7
  %xdefine %%G %8
  %xdefine %%H %9

   mov      T0, %%E       ;; T0: SUM1(E)
   ROR64    T0, 23      ;; T0: SUM1(E)
   add      %%H, qword [rsp+(%%nr & 1)*sizeof(qword)]
   mov      T1, %%F       ;; T1: CHJ(E,F,G)
   xor      T0, %%E       ;; T0: SUM1(E)
   ROR64    T0, 4       ;; T0: SUM1(E)
   xor      T1, %%G       ;; T1: CHJ(E,G,G)
   and      T1, %%E       ;; T1: CHJ(E,G,G)
   xor      T0, %%E       ;; T0: SUM1(E)
   ROR64    T0,14       ;; T0: SUM1(E)
   xor      T1, %%G       ;; T1:  CHJ(E,G,G)
   add      %%H, T1
   add      %%H, T0       ;; H += SUM1(E) + CHJ(E,F,G) + K_SHA512[t] + W[t]

   add      %%D, %%H

   mov      T0, %%A       ;; T0: SUM0(A)
   ROR64    T0, 5       ;; T0: SUM0(A)
   mov      T1, %%A       ;; T1: MAJ(A,B,C)
   xor      T1, %%C       ;; T1: MAJ(A,B,C)
   xor      T0, %%A       ;; T0: SUM0(A)
   ROR64    T0, 6       ;; T0: SUM0(A)
   xor      %%B, %%C       ;; T1: MAJ(A,B,C)
   and      T1, %%B       ;; T1: MAJ(A,B,C)
   xor      T0, %%A       ;; T0: SUM0(A)
   ROR64    T0,28       ;; T0: SUM0(A)
   xor      %%B, %%C       ;; T1: MAJ(A,B,C)
   xor      T1, %%C       ;; T1: MAJ(A,B,C)

   add      %%H, T0
   add      %%H, T1

  ;ROTATE_H

   mov      T0, %%D       ;; T0: SUM1(E)
   ROR64    T0, 23      ;; T0: SUM1(E)
   add      %%G, qword [rsp+((%%nr+1) & 1)*sizeof(qword)]
   mov      T1, %%E       ;; T1: CHJ(E,F,G)
   xor      T0, %%D       ;; T0: SUM1(E)
   ROR64    T0, 4       ;; T0: SUM1(E)
   xor      T1, %%F       ;; T1: CHJ(E,G,G)
   and      T1, %%D       ;; T1: CHJ(E,G,G)
   xor      T0, %%D       ;; T0: SUM1(E)
   ROR64    T0,14       ;; T0: SUM1(E)
   xor      T1, %%F       ;; T1:  CHJ(E,G,G)
   add      %%G, T1
   add      %%G, T0       ;; H += SUM1(E) + CHJ(E,F,G) + K_SHA512[t] + W[t]

   add      %%C, %%G

   mov      T0, %%H       ;; T0: SUM0(A)
   ROR64    T0, 5       ;; T0: SUM0(A)
   mov      T1, %%H       ;; T1: MAJ(A,B,C)
   xor      T1, %%B       ;; T1: MAJ(A,B,C)
   xor      T0, %%H       ;; T0: SUM0(A)
   ROR64    T0, 6       ;; T0: SUM0(A)
   xor      %%A, %%B       ;; T1: MAJ(A,B,C)
   and      T1, %%A       ;; T1: MAJ(A,B,C)
   xor      T0, %%H       ;; T0: SUM0(A)
   ROR64    T0,28       ;; T0: SUM0(A)
   xor      %%A, %%B       ;; T1: MAJ(A,B,C)
   xor      T1, %%B       ;; T1: MAJ(A,B,C)

   add      %%G, T0
   add      %%G, T1

  ;ROTATE_H
%endmacro

;;
;; update W[]
;;
;; W[j] = SIG1(W[j- 2]) + W[j- 7]
;;       +SIG0(W[j-15]) + W[j-16]
;;
;; SIG0(x) = ROR(x,1) ^ROR(x,8) ^LSR(x,7)
;; SIG1(x) = ROR(x,19)^ROR(x,61) ^LSR(x,6)
;;
%macro SHA512_2Wupdate 0.nolist
   vpalignr xT1, W5, W4, 8    ;; xT1 = W[t-7]
   vpalignr xT0, W1, W0, 8    ;; xT0 = W[t-15]
   vpaddq   W0, W0, xT1       ;; W0  = W0 + W[t-7]

   vpsrlq   SIGMA, W7, 6      ;; SIG1: W[t-2]>>6
   vpsrlq   xT1,   W7,61      ;; SIG1: W[t-2]>>61
   vpsllq   xT2,   W7,(64-61) ;; SIG1: W[t-2]<<(64-61)
   vpxor    SIGMA, SIGMA, xT1
   vpxor    SIGMA, SIGMA, xT2
   vpsrlq   xT1,   W7,19      ;; SIG1: W[t-2]>>19
   vpsllq   xT2,   W7,(64-19) ;; SIG1: W[t-2]<<(64-19)
   vpxor    SIGMA, SIGMA, xT1
   vpxor    SIGMA, SIGMA, xT2
   vpaddq   W0, W0, SIGMA     ;; W0 = W0 + W[t-7] + SIG1(W[t-2])

   vpsrlq   SIGMA, xT0, 7     ;; SIG0: W[t-15]>>7
   vpsrlq   xT1,   xT0, 1     ;; SIG0: W[t-15]>>1
   vpsllq   xT2,   xT0,(64-1) ;; SIG0: W[t-15]<<(64-1)
   vpxor    SIGMA, SIGMA, xT1
   vpxor    SIGMA, SIGMA, xT2
   vpsrlq   xT1,   xT0, 8     ;; SIG0: W[t-15]>>8
   vpsllq   xT2,   xT0,(64-8) ;; SIG0: W[t-15]<<(64-8)
   vpxor    SIGMA, SIGMA, xT1
   vpxor    SIGMA, SIGMA, xT2
   vpaddq   W0, W0, SIGMA     ;; W0 = W0 + W[t-7] + SIG1(W[t-2]) +SIG0(W[t-15])

   ROTATE_W
%endmacro

%macro SHA512_2step_2Wupdate 9.nolist
  %xdefine %%nr %1
  %xdefine %%A %2
  %xdefine %%B %3
  %xdefine %%C %4
  %xdefine %%D %5
  %xdefine %%E %6
  %xdefine %%F %7
  %xdefine %%G %8
  %xdefine %%H %9

   add      %%H, qword [rsp+(%%nr & 1)*sizeof(qword)]

   vpalignr xT1, W5, W4, 8    ;; xT1 = W[t-7]

   mov      T0, %%E       ;; T0: SUM1(E)
   ROR64    T0, 23      ;; T0: SUM1(E)

   vpalignr xT0, W1, W0, 8    ;; xT0 = W[t-15]

   mov      T1, %%F       ;; T1: CHJ(E,F,G)
   xor      T0, %%E       ;; T0: SUM1(E)

   vpaddq   W0, W0, xT1       ;; W0  = W0 + W[t-7]

   ROR64    T0, 4       ;; T0: SUM1(E)
   xor      T1, %%G       ;; T1: CHJ(E,G,G)

   vpsrlq   SIGMA, W7, 6      ;; SIG1: W[t-2]>>6

   and      T1, %%E       ;; T1: CHJ(E,G,G)
   xor      T0, %%E       ;; T0: SUM1(E)
   ROR64    T0,14       ;; T0: SUM1(E)

   vpsrlq   xT1,   W7,61      ;; SIG1: W[t-2]>>61

   xor      T1, %%G       ;; T1:  CHJ(E,G,G)
   add      %%H, T1
   add      %%H, T0       ;; H += SUM1(E) + CHJ(E,F,G) + K_SHA512[t] + W[t]

   vpsllq   xT2,   W7,(64-61) ;; SIG1: W[t-2]<<(64-61)

   add      %%D, %%H
   mov      T0, %%A       ;; T0: SUM0(A)
   ROR64    T0, 5       ;; T0: SUM0(A)

   vpxor    SIGMA, SIGMA, xT1

   mov      T1, %%A       ;; T1: MAJ(A,B,C)
   xor      T1, %%C       ;; T1: MAJ(A,B,C)

   vpxor    SIGMA, SIGMA, xT2

   xor      T0, %%A       ;; T0: SUM0(A)
   ROR64    T0, 6       ;; T0: SUM0(A)

   vpsrlq   xT1,   W7,19      ;; SIG1: W[t-2]>>19

   xor      %%B,  %%C       ;; T1: MAJ(A,B,C)
   and      T1, %%B       ;; T1: MAJ(A,B,C)
   xor      T0, %%A       ;; T0: SUM0(A)

   vpsllq   xT2,   W7,(64-19) ;; SIG1: W[t-2]<<(64-19)

   ROR64    T0,28       ;; T0: SUM0(A)
   xor      %%B,  %%C       ;; T1: MAJ(A,B,C)
   xor      T1, %%C       ;; T1: MAJ(A,B,C)

   vpxor    SIGMA, SIGMA, xT1
   add      %%H, T0
   add      %%H, T1
  ;ROTATE_H

   vpxor    SIGMA, SIGMA, xT2

   mov      T0, %%D       ;; T0: SUM1(E)
   ROR64    T0, 23      ;; T0: SUM1(E)

   vpaddq   W0, W0, SIGMA     ;; W0 = W0 + W[t-7] + SIG1(W[t-2])

   add      %%G, qword [rsp+((%%nr+1) & 1)*sizeof(qword)]
   mov      T1, %%E       ;; T1: CHJ(E,F,G)

   vpsrlq   SIGMA, xT0, 7     ;; SIG0: W[t-15]>>7

   xor      T0, %%D       ;; T0: SUM1(E)
   ROR64    T0, 4       ;; T0: SUM1(E)
   xor      T1, %%F       ;; T1: CHJ(E,G,G)

   vpsrlq   xT1,   xT0, 1     ;; SIG0: W[t-15]>>1

   and      T1, %%D       ;; T1: CHJ(E,G,G)
   xor      T0, %%D       ;; T0: SUM1(E)
   ROR64    T0,14       ;; T0: SUM1(E)

   vpsllq   xT2,   xT0,(64-1) ;; SIG0: W[t-15]<<(64-1)

   xor      T1, %%F       ;; T1:  CHJ(E,G,G)
   add      %%G, T1
   add      %%G, T0       ;; H += SUM1(E) + CHJ(E,F,G) + K_SHA512[t] + W[t]

   vpxor    SIGMA, SIGMA, xT1

   add      %%C, %%G
   mov      T0, %%H       ;; T0: SUM0(A)

   vpxor    SIGMA, SIGMA, xT2

   ROR64    T0, 5       ;; T0: SUM0(A)
   mov      T1, %%H       ;; T1: MAJ(A,B,C)

   vpsrlq   xT1,   xT0, 8     ;; SIG0: W[t-15]>>8

   xor      T1, %%B       ;; T1: MAJ(A,B,C)
   xor      T0, %%H       ;; T0: SUM0(A)
   ROR64    T0, 6       ;; T0: SUM0(A)

   vpsllq   xT2,   xT0,(64-8) ;; SIG0: W[t-15]<<(64-8)

   xor      %%A,  %%B       ;; T1: MAJ(A,B,C)
   and      T1, %%A       ;; T1: MAJ(A,B,C)
   xor      T0, %%H       ;; T0: SUM0(A)

   vpxor    SIGMA, SIGMA, xT1

   ROR64    T0,28       ;; T0: SUM0(A)
   xor      %%A,  %%B       ;; T1: MAJ(A,B,C)

   vpxor    SIGMA, SIGMA, xT2
   xor      T1, %%B       ;; T1: MAJ(A,B,C)
   add      %%G, T0

   vpaddq   W0, W0, SIGMA     ;; W0 = W0 + W[t-7] + SIG1(W[t-2]) +SIG0(W[t-15])
   add      %%G, T1

  ;ROTATE_H
   ROTATE_W
%endmacro

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
SHUFB_BSWAP DB    7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; UpdateSHA512(Ipp64u digest[], Ipp8u dataBlock[], int datalen, Ipp64u K_512[])
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA512,PUBLIC
%assign LOCAL_FRAME sizeof(oword)+sizeof(qword)
        USES_GPR rbx,rsi,rdi,rbp,r12,r13,r14,r15
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11
        COMP_ABI 4
;;
;; rdi = pointer to the updated hash
;; rsi = pointer to the data block
;; rdx = data block length
;; rcx = pointer to the SHA_512 constant
;;

%xdefine MBS_SHA512    (128)

   vmovdqa  xT1, oword [rel SHUFB_BSWAP]
   movsxd   rdx, edx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
.sha512_block_loop:
;;
;; initialize the first 16 qwords in the array W (remember about endian)
;;
   vmovdqu  W0, oword [rsi]       ; load buffer content and swap
   vpshufb  W0, W0, xT1
   vmovdqu  W1, oword [rsi+sizeof(oword)*1];
   vpshufb  W1, W1, xT1
   vmovdqu  W2, oword [rsi+sizeof(oword)*2];
   vpshufb  W2, W2, xT1
   vmovdqu  W3, oword [rsi+sizeof(oword)*3];
   vpshufb  W3, W3, xT1
   vmovdqu  W4, oword [rsi+sizeof(oword)*4];
   vpshufb  W4, W4, xT1
   vmovdqu  W5, oword [rsi+sizeof(oword)*5];
   vpshufb  W5, W5, xT1
   vmovdqu  W6, oword [rsi+sizeof(oword)*6];
   vpshufb  W6, W6, xT1
   vmovdqu  W7, oword [rsi+sizeof(oword)*7];
   vpshufb  W7, W7, xT1

   mov      A, qword [rdi]         ; load initial hash value
   mov      B, qword [rdi+sizeof(qword)]
   mov      C, qword [rdi+sizeof(qword)*2]
   mov      D, qword [rdi+sizeof(qword)*3]
   mov      E, qword [rdi+sizeof(qword)*4]
   mov      F, qword [rdi+sizeof(qword)*5]
   mov      G, qword [rdi+sizeof(qword)*6]
   mov      H, qword [rdi+sizeof(qword)*7]

   ;; perform  0- 9 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+ 0*sizeof(qword)] ; T += K_SHA512[0-1]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 0, A,B,C,D,E,F,G,H

   vpaddq   xT1, W0, oword [KK_SHA512+ 2*sizeof(qword)] ; T += K_SHA512[2-3]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 2, G,H,A,B,C,D,E,F

   vpaddq   xT1, W0, oword [KK_SHA512+ 4*sizeof(qword)] ; T += K_SHA512[4-5]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 4, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+ 6*sizeof(qword)] ; T += K_SHA512[6-7]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 6, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+ 8*sizeof(qword)] ; T += K_SHA512[8-9]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 8, A,B,C,D,E,F,G,H

   ;; perform 10-19 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+10*sizeof(qword)] ; T += K_SHA512[10-11]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 10, G,H,A,B,C,D,E,F

   vpaddq   xT1, W0, oword [KK_SHA512+12*sizeof(qword)] ; T += K_SHA512[12-13]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 12, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+14*sizeof(qword)] ; T += K_SHA512[14-15]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 14, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+16*sizeof(qword)] ; T += K_SHA512[16-17]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 16, A,B,C,D,E,F,G,H

   vpaddq   xT1, W0, oword [KK_SHA512+18*sizeof(qword)] ; T += K_SHA512[18-19]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 18, G,H,A,B,C,D,E,F

   ;; perform 20-29 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+20*sizeof(qword)] ; T += K_SHA512[20-21]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 20, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+22*sizeof(qword)] ; T += K_SHA512[22-23]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 22, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+24*sizeof(qword)] ; T += K_SHA512[24-25]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 24, A,B,C,D,E,F,G,H

   vpaddq   xT1, W0, oword [KK_SHA512+26*sizeof(qword)] ; T += K_SHA512[26-27]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 26, G,H,A,B,C,D,E,F

   vpaddq   xT1, W0, oword [KK_SHA512+28*sizeof(qword)] ; T += K_SHA512[28-29]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 28, E,F,G,H,A,B,C,D

   ;; perform 30-39 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+30*sizeof(qword)] ; T += K_SHA512[30-31]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 30, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+32*sizeof(qword)] ; T += K_SHA512[32-33]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 32, A,B,C,D,E,F,G,H

   vpaddq   xT1, W0, oword [KK_SHA512+34*sizeof(qword)] ; T += K_SHA512[34-35]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 34, G,H,A,B,C,D,E,F

   vpaddq   xT1, W0, oword [KK_SHA512+36*sizeof(qword)] ; T += K_SHA512[36-37]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 36, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+38*sizeof(qword)] ; T += K_SHA512[38-39]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 38, C,D,E,F,G,H,A,B

   ;; perform 40-49 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+40*sizeof(qword)] ; T += K_SHA512[40-41]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 40, A,B,C,D,E,F,G,H

   vpaddq   xT1, W0, oword [KK_SHA512+42*sizeof(qword)] ; T += K_SHA512[42-43]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 42, G,H,A,B,C,D,E,F

   vpaddq   xT1, W0, oword [KK_SHA512+44*sizeof(qword)] ; T += K_SHA512[44-45]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 44, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+46*sizeof(qword)] ; T += K_SHA512[46-47]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 46, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+48*sizeof(qword)] ; T += K_SHA512[48-49]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 48, A,B,C,D,E,F,G,H

   ;; perform 50-59 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+50*sizeof(qword)] ; T += K_SHA512[50-51]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 50, G,H,A,B,C,D,E,F

   vpaddq   xT1, W0, oword [KK_SHA512+52*sizeof(qword)] ; T += K_SHA512[52-53]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 52, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+54*sizeof(qword)] ; T += K_SHA512[54-55]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 54, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+56*sizeof(qword)] ; T += K_SHA512[56-57]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 56, A,B,C,D,E,F,G,H

   vpaddq   xT1, W0, oword [KK_SHA512+58*sizeof(qword)] ; T += K_SHA512[58-59]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 58, G,H,A,B,C,D,E,F

   ;; perform 60-69 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+60*sizeof(qword)] ; T += K_SHA512[60-61]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 60, E,F,G,H,A,B,C,D

   vpaddq   xT1, W0, oword [KK_SHA512+62*sizeof(qword)] ; T += K_SHA512[62-63]
   vmovdqa  oword [rsp], xT1
   SHA512_2step_2Wupdate 62, C,D,E,F,G,H,A,B

   vpaddq   xT1, W0, oword [KK_SHA512+64*sizeof(qword)] ; T += K_SHA512[64-65]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 64, A,B,C,D,E,F,G,H
   ROTATE_W

   vpaddq   xT1, W0, oword [KK_SHA512+66*sizeof(qword)] ; T += K_SHA512[66-67]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 66, G,H,A,B,C,D,E,F
   ROTATE_W

   vpaddq   xT1, W0, oword [KK_SHA512+68*sizeof(qword)] ; T += K_SHA512[68-69]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 68, E,F,G,H,A,B,C,D
   ROTATE_W

   ;; perform 70-79 rounds
   vpaddq   xT1, W0, oword [KK_SHA512+70*sizeof(qword)] ; T += K_SHA512[70-71]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 70, C,D,E,F,G,H,A,B
   ROTATE_W

   vpaddq   xT1, W0, oword [KK_SHA512+72*sizeof(qword)] ; T += K_SHA512[72-73]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 72, A,B,C,D,E,F,G,H
   ROTATE_W

   vpaddq   xT1, W0, oword [KK_SHA512+74*sizeof(qword)] ; T += K_SHA512[74-75]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 74, G,H,A,B,C,D,E,F
   ROTATE_W

   vpaddq   xT1, W0, oword [KK_SHA512+76*sizeof(qword)] ; T += K_SHA512[76-77]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 76, E,F,G,H,A,B,C,D
   ROTATE_W

   vpaddq   xT1, W0, oword [KK_SHA512+78*sizeof(qword)] ; T += K_SHA512[78-79]
   vmovdqa  oword [rsp], xT1
   SHA512_2step 78, C,D,E,F,G,H,A,B
   ROTATE_W

   add      qword [rdi], A         ; update shash
   add      qword [rdi+sizeof(qword)*1], B
   add      qword [rdi+sizeof(qword)*2], C
   add      qword [rdi+sizeof(qword)*3], D
   add      qword [rdi+sizeof(qword)*4], E
   add      qword [rdi+sizeof(qword)*5], F
   add      qword [rdi+sizeof(qword)*6], G
   add      qword [rdi+sizeof(qword)*7], H

   vmovdqa  xT1, oword [rel SHUFB_BSWAP]
   add      rsi, MBS_SHA512
   sub      rdx, MBS_SHA512
   jg       .sha512_block_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA512

%endif    ;; (_IPP32E >= _IPP32E_E9 )
%endif    ;; _ENABLE_ALG_SHA512_

