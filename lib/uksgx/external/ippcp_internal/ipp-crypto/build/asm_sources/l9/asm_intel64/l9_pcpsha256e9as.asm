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
;               Message block processing according to SHA256
;
;     Content:
;        UpdateSHA256
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA256_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E == _IPP32E_E9 )

%xdefine W0              xmm0
%xdefine W4              xmm1
%xdefine W8              xmm2
%xdefine W12             xmm3
%xdefine SIG1            xmm4
%xdefine SIG0            xmm5
%xdefine X               xmm6
%xdefine W               xmm7
%xdefine XMM_SHUFB_BSWAP xmm6

;; assign hash values to GPU registers
%xdefine A  eax
%xdefine B  ebx
%xdefine C  ecx
%xdefine D  edx
%xdefine E  r8d
%xdefine F  r9d
%xdefine G  r10d
%xdefine H  r11d
%xdefine T1 r12d
%xdefine T2 r13d
%xdefine T3 r14d
%xdefine T4 r15d

;; we are considering x, y, z are polynomials over GF(2)
;;                    & - multiplication
;;                    ^ - additive
;;                    operations

;;
;; Chj(x,y,z) = (x&y) ^ (~x & z)
;;            = (x&y) ^ ((1^x) &z)
;;            = (x&y) ^ (z ^ x&z)
;;            = x&y ^ z ^ x&z
;;            = x&(y^z) ^z
;;
%macro Chj 4.nolist
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
;; Maj(x,y,z) = (x&y) ^ (x&z) ^ (y&z)
;;            = (x&y) ^ (x&z) ^ (y&z) ^ (z&z) ^z   // note: ((z&z) ^z) = 0
;;            = x&(y^z) ^ z&(y^z) ^z
;;            = (x^z)&(y^z) ^z
;;
%macro Maj 4.nolist
  %xdefine %%F %1
  %xdefine %%X %2
  %xdefine %%Y %3
  %xdefine %%Z %4

   mov   %%F, %%X
   xor   %%F, %%Z
   xor   %%Y, %%Z
   and   %%F, %%Y
   xor   %%Y, %%Z
   xor   %%F, %%Z
%endmacro

%macro ROTR 2.nolist
  %xdefine %%X %1
  %xdefine %%n %2

   shrd  %%X,%%X, %%n
%endmacro

;;
;; Summ0(x) = ROR(x,2) ^ ROR(x,13) ^ ROR(x,22)
;;
%macro Summ0 2.nolist
  %xdefine %%F %1
  %xdefine %%X %2

   mov   %%F, %%X
   ROTR  %%F, (22-13)
   xor   %%F, %%X
   ROTR  %%F, (13-2)
   xor   %%F, %%X
   ROTR  %%F, 2
%endmacro

;;
;; Summ1(x) = ROR(x,6) ^ ROR(x,11) ^ ROR(x,25)
;;
%macro Summ1 2.nolist
  %xdefine %%F %1
  %xdefine %%X %2

   mov   %%F, %%X
   ROTR  %%F, (25-11)
   xor   %%F, %%X
   ROTR  %%F, (11-6)
   xor   %%F, %%X
   ROTR  %%F, 6
%endmacro

;;
;; regular round (i):
;;
;; T1 = h + Sigma1(e) + Ch(e,f,g) + K[i] + W[i]
;; T2 = Sigma0(a) + Maj(a,b,c)
;; h = g
;; g = f
;; f = e
;; e = d + T1
;; d = c
;; c = b
;; b = a
;; a = T1+T2
;;
;;    or
;;
;; h += Sigma1(e) + Ch(e,f,g) + K[i] + W[i]  (==T1)
;; d += h
;; T2 = Sigma0(a) + Maj(a,b,c)
;; h += T2
;; and following textual shift {a,b,c,d,e,f,g,h} => {h,a,b,c,d,e,f,g}
;;
%macro ROUND 11.nolist
  %xdefine %%nr %1
  %xdefine %%A %2
  %xdefine %%B %3
  %xdefine %%C %4
  %xdefine %%D %5
  %xdefine %%E %6
  %xdefine %%F %7
  %xdefine %%G %8
  %xdefine %%H %9
  %xdefine %%T1 %10
  %xdefine %%T2 %11

   add      %%H, dword [rsp+(%%nr & 3)*sizeof(dword)]
   Summ1    %%T1, %%E
   Chj      %%T2, %%E,%%F,%%G
   add      %%H, %%T1
   add      %%H, %%T2

   add      %%D, %%H

   Summ0    %%T1, %%A
   Maj      %%T2, %%A,%%B,%%C
   add      %%H, %%T1
   add      %%H, %%T2
%endmacro

%macro ROUND_v1 12.nolist
  %xdefine %%nr %1
  %xdefine %%A %2
  %xdefine %%B %3
  %xdefine %%C %4
  %xdefine %%D %5
  %xdefine %%E %6
  %xdefine %%F %7
  %xdefine %%G %8
  %xdefine %%H %9
  %xdefine %%T1 %10
  %xdefine %%T2 %11
  %xdefine %%T3 %12

   Summ1    %%T1, %%E             ;; S1
   Chj      %%T2, %%E,%%F,%%G         ;; CH
   add      %%H, dword [rsp+(%%nr & 3)*sizeof(dword)]
   add      %%H, %%T1
   add      %%H, %%T2

   Maj      %%T1, %%A,%%B,%%C         ;; MAJ
   Summ0    %%T2, %%A             ;; S0

   add      %%D, %%H

   add      %%H, %%T1
   add      %%H, %%T2
%endmacro

;; W[i] = Sigma1(W[i-2]) + W[i-7] + Sigma0(W[i-15]) + W[i-16], i=16,..,63
;;
;;for next rounds 16,17,18 and 19:
;; W[0] <= W[16] = Sigma1(W[14]) + W[ 9] + Sigma0(W[1]) + W[0]
;; W[1] <= W[17] = Sigma1(W[15]) + W[10] + Sigma0(W[2]) + W[1]
;; W[2] <= W[18] = Sigma1(W[ 0]) + W[11] + Sigma0(W[3]) + W[2]
;; W[3] <= W[19] = Sigma1(W[ 1]) + W[12] + Sigma0(W[4]) + W[3]
;;
;; the process is repeated exactly because texual round of W[]
;;
;; Sigma1() and Sigma0() functions are defined as following:
;; Sigma1(X) = ROR(X,17)^ROR(X,19)^SHR(X,10)
;; Sigma0(X) = ROR(X, 7)^ROR(X,18)^SHR(X, 3)
;;
%macro UPDATE_W 8.nolist
  %xdefine %%xS0 %1
  %xdefine %%xS4 %2
  %xdefine %%xS8 %3
  %xdefine %%xS12 %4
  %xdefine %%SIGMA %5
  %xdefine %%xS %6
  %xdefine %%xT %7
  %xdefine %%phase %8

%if %%phase == 0
   ;; SIGMA0
   vpalignr %%xS, %%xS4, %%xS0, 4      ;; xS = {W04,W03,W02,W01}
   vpsrld   %%SIGMA, %%xS, 3         ;; SIGMA0 = SHR({W04,W03,W02,W01},3)
   vpsrld   %%xT, %%xS, 7            ;; SIGMA0 ^= SHR({W04,W03,W02,W01},7)
   vpxor    %%SIGMA, %%SIGMA, %%xT
   vpsrld   %%xT, %%xS, 18           ;; SIGMA0 ^= SHR({W04,W03,W02,W01},18)
   vpxor    %%SIGMA, %%SIGMA, %%xT
   vpslld   %%xT, %%xS, (32-18)      ;; SIGMA0 ^= SHL({W04,W03,W02,W01},32-18)
   vpxor    %%SIGMA, %%SIGMA, %%xT
   vpslld   %%xT, %%xS, (32-7)       ;; SIGMA0 ^= SHL({W04,W03,W02,W01},32-7)
   vpxor    %%SIGMA, %%SIGMA, %%xT
%endif
%if %%phase == 1
   vpalignr %%xS, %%xS12,%%xS8, 4      ;; xS = {W12,W11,W10,W09}
   vpaddd   %%xS0, %%xS0, %%xS         ;; xS0 = xS0 + {W12,W11,W10,W09} + SIGMA0
   vpaddd   %%xS0, %%xS0, %%SIGMA
%endif
%if %%phase == 2
   ;; SIGMA1 (low)
   vpshufd  %%xS, %%xS12, 11111010b  ;; xS = {W[15],W[15],W[14],W[14]}
   vpsrld   %%SIGMA, %%xS, 10        ;; SIGMA1 = SHR({W[15],W[15],W[14],W[14]},10)
   vpsrlq   %%xT, %%xS, 17           ;; xT = SHR({W[15],W[15],W[14],W[14]},17)
   vpsrlq   %%xS, %%xS, 19           ;; xS = SHR({W[15],W[15],W[14],W[14]},19)
   vpxor    %%SIGMA, %%SIGMA, %%xT
   vpxor    %%SIGMA, %%SIGMA, %%xS     ;; SIGMA1 ^= xT ^ xS
   vpshufb  %%SIGMA, %%SIGMA, [rel SHUFD_ZZ10]  ;; SIGMA1 = {zzz,zzz,SIGMA1.1,SIGMA1.0}

   vpaddd   %%xS0, %%xS0, %%SIGMA      ;; {???,???,new W01,new W00} = {??,??,W17,W16}
%endif
%if %%phase == 3
   ;; SIGMA1 (hight)
   vpshufd  %%xS, %%xS0, 01010000b   ;; xS = {W17,W17,W16,W16}
   vpsrld   %%SIGMA, %%xS, 10        ;; SIGMA1 = SHR({W[17],W[17],W[16],W[16]},10)
   vpsrlq   %%xT, %%xS, 17           ;; xT = ROTR({W[17],W[17],W[16],W[16]},17)- low 32 rotation in fact
   vpsrlq   %%xS, %%xS, 19           ;; xS = RORR({W[17],W[17],W[16],W[16]},19) - low 32 rotation in fact
   vpxor    %%SIGMA, %%SIGMA, %%xT
   vpxor    %%SIGMA, %%SIGMA, %%xS     ;; SIGMA1 ^= xT ^ xS
   vpshufb  %%SIGMA, %%SIGMA, [rel SHUFD_32ZZ]  ;; SIGMA1 = {SIGMA1.3,SIGMA1.2,zzz,zzz}

   vpaddd   %%xS0, %%xS0, %%SIGMA      ;; {new W03, new W02,new W01,new W00} = {W19,W18,W17,W16}
%endif
%endmacro

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR
SHUFB_BSWAP DB    3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12
SHUFD_ZZ10  DB    0,1,2,3, 8,9,10,11, 0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
SHUFD_32ZZ  DB    0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh, 0,1,2,3, 8,9,10,11

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; UpdateSHA256(Ipp32u digest[], Ipp8u dataBlock[], int datalen, Ipp32u K_256[])
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA256,PUBLIC
%assign LOCAL_FRAME sizeof(oword)+sizeof(qword)
        USES_GPR rbx,rsi,rdi,rbp,r12,r13,r14,r15
        USES_XMM xmm6,xmm7
        COMP_ABI 4
;;
;; rdi = pointer to the updated hash
;; rsi = pointer to the data block
;; rdx = data block length
;; rcx = pointer to the SHA_256 constant
;;

%xdefine MBS_SHA256    (64)

   movsxd   rdx, edx
   mov      qword [rsp+sizeof(oword)], rdx   ; save length of buffer

   mov      rbp, rcx ; rbp points K_256[] constants

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
.sha256_block_loop:
   vmovdqu  W0, oword [rsi]       ; load buffer content and swap
   vpshufb  W0, W0, [rel SHUFB_BSWAP]
   vmovdqu  W4, oword [rsi+sizeof(oword)]
   vpshufb  W4, W4, [rel SHUFB_BSWAP]
   vmovdqu  W8, oword [rsi+sizeof(oword)*2]
   vpshufb  W8, W8, [rel SHUFB_BSWAP]
   vmovdqu  W12,oword [rsi+sizeof(oword)*3]
   vpshufb  W12,W12, [rel SHUFB_BSWAP]

   mov      A, dword [rdi]         ; load initial hash value
   mov      B, dword [rdi+sizeof(dword)]
   mov      C, dword [rdi+sizeof(dword)*2]
   mov      D, dword [rdi+sizeof(dword)*3]
   mov      E, dword [rdi+sizeof(dword)*4]
   mov      F, dword [rdi+sizeof(dword)*5]
   mov      G, dword [rdi+sizeof(dword)*6]
   mov      H, dword [rdi+sizeof(dword)*7]

   ;; perform 0-3 regular rounds
   vpaddd   W, W0, oword [rbp+sizeof(oword)*0] ; T += K_SHA256[0-3]
   vmovdqa  oword [rsp], W
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 0              ; update for round: 16-19
   ROUND    0, A,B,C,D,E,F,G,H, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 1
   ROUND    1, H,A,B,C,D,E,F,G, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 2
   ROUND    2, G,H,A,B,C,D,E,F, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 3
   ROUND    3, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 4-7 regular rounds
   vpaddd   W, W4, oword [rbp+sizeof(oword)*1] ; T += K_SHA256[4-7]
   vmovdqa  oword [rsp], W
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 0              ; update for round: 20-23
   ROUND    4, E,F,G,H,A,B,C,D, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 1
   ROUND    5, D,E,F,G,H,A,B,C, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 2
   ROUND    6, C,D,E,F,G,H,A,B, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 3
   ROUND    7, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 8-11 regular rounds
   vpaddd   W, W8, oword [rbp+sizeof(oword)*2] ; T += K_SHA256[8-11]
   vmovdqa  oword [rsp], W
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 0              ; update for round: 24-27
   ROUND    8, A,B,C,D,E,F,G,H, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 1
   ROUND    9, H,A,B,C,D,E,F,G, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 2
   ROUND   10, G,H,A,B,C,D,E,F, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 3
   ROUND   11, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 12-15 regular rounds
   vpaddd   W, W12, oword [rbp+sizeof(oword)*3] ; T += K_SHA256[12-15]
   vmovdqa  oword [rsp], W
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 0              ; update for round: 28-31
   ROUND   12, E,F,G,H,A,B,C,D, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 1
   ROUND   13, D,E,F,G,H,A,B,C, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 2
   ROUND   14, C,D,E,F,G,H,A,B, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 3
   ROUND   15, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 16-19 regular rounds
   vpaddd      W, W0, oword [rbp+sizeof(oword)*4] ; T += K_SHA256[16-19]
   vmovdqa     oword [rsp], W
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 0              ; update for round: 32-35
   ROUND   16, A,B,C,D,E,F,G,H, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 1
   ROUND   17, H,A,B,C,D,E,F,G, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 2
   ROUND   18, G,H,A,B,C,D,E,F, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 3
   ROUND   19, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 20-23 regular rounds
   vpaddd      W, W4, oword [rbp+sizeof(oword)*5] ; T += K_SHA256[20-23]
   vmovdqa     oword [rsp], W
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 0              ; update for round: 36-39
   ROUND   20, E,F,G,H,A,B,C,D, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 1
   ROUND   21, D,E,F,G,H,A,B,C, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 2
   ROUND   22, C,D,E,F,G,H,A,B, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 3
   ROUND   23, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 24-27 regular rounds
   vpaddd      W, W8, oword [rbp+sizeof(oword)*6] ; T += K_SHA256[24-27]
   vmovdqa     oword [rsp], W
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 0              ; update for round: 40-43
   ROUND   24, A,B,C,D,E,F,G,H, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 1
   ROUND   25, H,A,B,C,D,E,F,G, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 2
   ROUND   26, G,H,A,B,C,D,E,F, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 3
   ROUND   27, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 28-31 regular rounds
   vpaddd      W, W12, oword [rbp+sizeof(oword)*7]   ; T += K_SHA256[28-31]
   vmovdqa     oword [rsp], W
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 0              ; update for round: 44-47
   ROUND   28, E,F,G,H,A,B,C,D, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 1
   ROUND   29, D,E,F,G,H,A,B,C, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 2
   ROUND   30, C,D,E,F,G,H,A,B, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 3
   ROUND   31, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 32-35 regular rounds
   vpaddd      W, W0, oword [rbp+sizeof(oword)*8]    ; T += K_SHA256[32-35]
   vmovdqa     oword [rsp], W
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 0              ; update for round: 48-51
   ROUND   32, A,B,C,D,E,F,G,H, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 1
   ROUND   33, H,A,B,C,D,E,F,G, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 2
   ROUND   34, G,H,A,B,C,D,E,F, T1,T2
   UPDATE_W W0,W4,W8,W12, SIG1,W,X, 3
   ROUND   35, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 36-39 regular rounds
   vpaddd      W, W4, oword [rbp+sizeof(oword)*9]    ; T += K_SHA256[36-39]
   vmovdqa     oword [rsp], W
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 0              ; update for round: 52-55
   ROUND   36, E,F,G,H,A,B,C,D, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 1
   ROUND   37, D,E,F,G,H,A,B,C, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 2
   ROUND   38, C,D,E,F,G,H,A,B, T1,T2
   UPDATE_W W4,W8,W12,W0, SIG1,W,X, 3
   ROUND   39, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 40-43 regular rounds
   vpaddd      W, W8, oword [rbp+sizeof(oword)*10]   ; T += K_SHA256[40-43]
   vmovdqa     oword [rsp], W
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 0              ; update for round: 56-59
   ROUND   40, A,B,C,D,E,F,G,H, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 1
   ROUND   41, H,A,B,C,D,E,F,G, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 2
   ROUND   42, G,H,A,B,C,D,E,F, T1,T2
   UPDATE_W W8,W12,W0,W4, SIG1,W,X, 3
   ROUND   43, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 44-47 regular rounds
   vpaddd      W, W12, oword [rbp+sizeof(oword)*11]  ; T += K_SHA256[44-47]
   vmovdqa     oword [rsp], W
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 0              ; update for round: 60-63
   ROUND   44, E,F,G,H,A,B,C,D, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 1
   ROUND   45, D,E,F,G,H,A,B,C, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 2
   ROUND   46, C,D,E,F,G,H,A,B, T1,T2
   UPDATE_W W12,W0,W4,W8, SIG1,W,X, 3
   ROUND   47, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 48-51 regular rounds
   vpaddd      W, W0, oword [rbp+sizeof(oword)*12]   ; T += K_SHA256[48-51]
   vmovdqa     oword [rsp], W
   ROUND   48, A,B,C,D,E,F,G,H, T1,T2
   ROUND   49, H,A,B,C,D,E,F,G, T1,T2
   ROUND   50, G,H,A,B,C,D,E,F, T1,T2
   ROUND   51, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 52-55 regular rounds
   vpaddd      W, W4, oword [rbp+sizeof(oword)*13]   ; T += K_SHA256[52-55]
   vmovdqa     oword [rsp], W
   ROUND   52, E,F,G,H,A,B,C,D, T1,T2
   ROUND   53, D,E,F,G,H,A,B,C, T1,T2
   ROUND   54, C,D,E,F,G,H,A,B, T1,T2
   ROUND   55, B,C,D,E,F,G,H,A, T1,T2

   ;; perform next 56-59 regular rounds
   vpaddd      W, W8, oword [rbp+sizeof(oword)*14]   ; T += K_SHA256[56-59]
   vmovdqa     oword [rsp], W
   ROUND   56, A,B,C,D,E,F,G,H, T1,T2
   ROUND   57, H,A,B,C,D,E,F,G, T1,T2
   ROUND   58, G,H,A,B,C,D,E,F, T1,T2
   ROUND   59, F,G,H,A,B,C,D,E, T1,T2

   ;; perform next 60-63 regular rounds
   vpaddd      W, W12, oword [rbp+sizeof(oword)*15]  ; T += K_SHA256[60-63]
   vmovdqa     oword [rsp], W
   ROUND   60, E,F,G,H,A,B,C,D, T1,T2
   ROUND   61, D,E,F,G,H,A,B,C, T1,T2
   ROUND   62, C,D,E,F,G,H,A,B, T1,T2
   ROUND   63, B,C,D,E,F,G,H,A, T1,T2

   add            dword [rdi], A         ; update shash
   add            dword [rdi+sizeof(dword)*1], B
   add            dword [rdi+sizeof(dword)*2], C
   add            dword [rdi+sizeof(dword)*3], D
   add            dword [rdi+sizeof(dword)*4], E
   add            dword [rdi+sizeof(dword)*5], F
   add            dword [rdi+sizeof(dword)*6], G
   add            dword [rdi+sizeof(dword)*7], H

   add      rsi, MBS_SHA256
   sub      qword [rsp+sizeof(oword)], MBS_SHA256
   jg       .sha256_block_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA256

%endif    ;; _IPP32E_E9 and above
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA256_

