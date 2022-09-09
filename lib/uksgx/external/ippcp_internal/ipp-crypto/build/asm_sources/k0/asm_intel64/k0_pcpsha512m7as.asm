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
;        UpdateSHA512
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA512_)
%if (_IPP32E >= _IPP32E_M7) && (_IPP32E < _IPP32E_E9 )


;;
;; ENDIANNESS
;;
%macro ENDIANNESS 2.nolist
  %xdefine %%dst %1
  %xdefine %%src %2

  %ifnidn %%dst,%%src
    mov %%dst,%%src
  %endif
    bswap %%dst
%endmacro

%macro ROT_R 2.nolist
  %xdefine %%r %1
  %xdefine %%nbits %2

  %if _IPP32E >= _IPP32E_L9
   rorx  %%r,%%r,%%nbits
  ;; %elif _IPP32E >= _IPP32E_Y8
  ;;  shrd  %%r,%%r,%%nbits
  %else
   ror   %%r,%%nbits
  %endif
%endmacro

;;
;; SHA512 step
;;
;;    Ipp64u T1 = H + SUM1(E) +  CH(E,F,G) + K_SHA512[t] + W[t];
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
%macro SHA512_STEP_2 13.nolist
  %xdefine %%regA %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regE %5
  %xdefine %%regF %6
  %xdefine %%regG %7
  %xdefine %%regH %8
  %xdefine %%regT %9
  %xdefine %%regF1 %10
  %xdefine %%regF2 %11
  %xdefine %%memW %12
  %xdefine %%memCNT %13

        ;; update H (start)
        add     %%regH,[%%memCNT]   ;; H += W[t]+K_SHA512[t]
        add     %%regH,[%%memW]

        ;; compute T = SUM1(E) + CH(E,F,G)
        mov     %%regF1,%%regE      ;; SUM1() = E
        mov     %%regF2,%%regE      ;; CH() = E
        ROT_R   %%regF1,14        ;; ROR(E,14)
        mov     %%regT, %%regE
        push    %%regE
        not     %%regF2           ;; ~E
        ROT_R   %%regE, 18        ;; ROR(E,18)
        and     %%regT, %%regF      ;; E&F
        and     %%regF2,%%regG      ;;~E&G
        xor     %%regF1,%%regE      ;; ROR(E,14)^ROR(E,18)
        ROT_R   %%regE, 23        ;; ROR(E,41)
        xor     %%regF2,%%regT      ;; CH() = (E&F)&(~E&G)
        xor     %%regF1,%%regE      ;; SUM1() = ROR(E,14)^ROR(E,18)^ROR(E,41)
        pop     %%regE            ;; repair E
        lea     %%regT, [%%regF1+%%regF2]

        ;; update H (continue)
        add     %%regH, %%regT

        ;; update D
        add     %%regD, %%regH

        ;; compute T = SUM0(A) + MAJ(A,B,C)
        mov     %%regF1,%%regA      ;; SUM0() = A
        mov     %%regF2,%%regA      ;; MAJ() = A
        ROT_R   %%regF1,28        ;; ROR(A,28)
        mov     %%regT, %%regA
        push    %%regA
        xor     %%regF2,%%regB      ;; A^B
        ROT_R   %%regA, 34        ;; ROR(A,34)
        and     %%regT, %%regB      ;; A&B
        and     %%regF2,%%regC      ;; (A^B)&C
        xor     %%regF1,%%regA      ;; ROR(A,2)^ROR(A,13)
        ROT_R   %%regA, 5         ;; ROR(A,39)
        xor     %%regF2,%%regT      ;; MAJ() = (A^B)^((A^B)&C)
        xor     %%regF1,%%regA      ;; SUM0() = ROR(A,28)^ROR(A,34)^ROR(A,39)
        pop     %%regA            ;; repair A
        lea     %%regT, [%%regF1+%%regF2]

        ;; update H (finish)
        add     %%regH, %%regT
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
%macro UPDATE_2 5.nolist
  %xdefine %%nr %1
  %xdefine %%sig0 %2
  %xdefine %%sig1 %3
  %xdefine %%W15 %4
  %xdefine %%W2 %5

   mov      %%sig0, [rsp+((%%nr-15) & 0Fh)*8]  ;; W[j-15]
   mov      %%sig1, [rsp+((%%nr-2) & 0Fh)*8]   ;; W[j-2]
   shr      %%sig0, 7                          ;; LSR(W[j-15], 7)
   shr      %%sig1, 6                          ;; LSR(W[j-2],  6)

   mov      %%W15, [rsp+((%%nr-15) & 0Fh)*8]   ;; W[j-15]
   mov      %%W2,  [rsp+((%%nr-2) & 0Fh)*8]    ;; W[j-2]
   ROT_R    %%W15, 1                           ;; ROR(W[j-15], 1)
   ROT_R    %%W2, 19                           ;; ROR(W[j-2], 19)
   xor      %%sig0, %%W15                        ;; SIG0 ^= ROR(W[j-15], 1)
   xor      %%sig1, %%W2                         ;; SIG1 ^= ROR(W[j-2], 19)

   ROT_R    %%W15, 7                           ;; ROR(W[j-15], 8)
   ROT_R    %%W2, 42                           ;; ROR(W[j-2], 61)
   xor      %%sig0, %%W15                        ;; SIG0 ^= ROR(W[j-15], 8)
   xor      %%sig1, %%W2                         ;; SIG1 ^= ROR(W[j-2], 61)

   add      %%sig0, [rsp+((%%nr-16) & 0Fh)*8]  ;; SIG0 += W[j-16]
   add      %%sig1, [rsp+((%%nr-7) & 0Fh)*8]   ;; SIG1 += W[j-7]
   add      %%sig0, %%sig1
   mov      [rsp+((%%nr-16) & 0Fh)*8], %%sig0
%endmacro

segment .text align=IPP_ALIGN_FACTOR



;******************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA512(DigestSHA512 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;******************************************************************************************

;;
;; Lib = M7
;;
;; Caller = ippsSHA512Update
;; Caller = ippsSHA512Final
;; Caller = ippsSHA512MessageDigest
;;
;; Caller = ippsSHA384Update
;; Caller = ippsSHA384Final
;; Caller = ippsSHA384MessageDigest
;;
;; Caller = ippsHMACSHA512Update
;; Caller = ippsHMACSHA512Final
;; Caller = ippsHMACSHA512MessageDigest
;;
;; Caller = ippsHMACSHA384Update
;; Caller = ippsHMACSHA384Final
;; Caller = ippsHMACSHA384MessageDigest
;;

%xdefine KK_SHA512  rbp

%if _IPP32E >= _IPP32E_U8
align IPP_ALIGN_FACTOR
pByteSwp DB    7,6,5,4,3,2,1,0,  15,14,13,12,11,10,9,8
%endif

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA512,PUBLIC
%assign LOCAL_FRAME (16*sizeof(qword)+sizeof(qword))
        USES_GPR rbx,rsi,rdi,r12,r13,r14,r15,rbp
        USES_XMM
        COMP_ABI 4

;; rdi = hash
;; rsi = data buffer
;; rdx = buffer length
;; rcx = address of SHA512 constants

%xdefine MBS_SHA512    (128)

   movsxd   rdx, edx
   mov      qword [rsp+16*sizeof(qword)], rdx   ; save length of buffer

   mov  KK_SHA512, rcx

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
%if (_IPP32E >= _IPP32E_U8)
   movdqu   xmm4, oword [rel pByteSwp]

   movdqu   xmm0, oword [rsi+0*16]
   pshufb   xmm0, xmm4
   movdqa   oword [rsp+0*16], xmm0
   movdqu   xmm1, oword [rsi+1*16]
   pshufb   xmm1, xmm4
   movdqa   oword [rsp+1*16], xmm1
   movdqu   xmm2, oword [rsi+2*16]
   pshufb   xmm2, xmm4
   movdqa   oword [rsp+2*16], xmm2
   movdqu   xmm3, oword [rsi+3*16]
   pshufb   xmm3, xmm4
   movdqa   oword [rsp+3*16], xmm3

   movdqu   xmm0, oword [rsi+4*16]
   pshufb   xmm0, xmm4
   movdqa   oword [rsp+4*16], xmm0
   movdqu   xmm1, oword [rsi+5*16]
   pshufb   xmm1, xmm4
   movdqa   oword [rsp+5*16], xmm1
   movdqu   xmm2, oword [rsi+6*16]
   pshufb   xmm2, xmm4
   movdqa   oword [rsp+6*16], xmm2
   movdqu   xmm3, oword [rsi+7*16]
   pshufb   xmm3, xmm4
   movdqa   oword [rsp+7*16], xmm3
%else
   xor      rcx,rcx

align IPP_ALIGN_FACTOR
.loop1:
   mov      r8,[rsi+rcx*8+0*8]
   ENDIANNESS r8,r8
   mov      [rsp+rcx*8+0*8],r8

   mov      r9,[rsi+rcx*8+1*8]
   ENDIANNESS r9,r9
   mov      [rsp+rcx*8+1*8],r9

   add      rcx,2
   cmp      rcx,16
   jl       .loop1
%endif

;;
;; init A,B,C,D,E,F,G,H by the internal digest
;;
;;
;; init A, B, C, D, E, F, G, H by the internal digest
;;
   mov      r8, [rdi+0*8]       ; r8 = digest[0] (A)
   mov      r9, [rdi+1*8]       ; r9 = digest[1] (B)
   mov      r10,[rdi+2*8]       ; r10= digest[2] (C)
   mov      r11,[rdi+3*8]       ; r11= digest[3] (D)
   mov      r12,[rdi+4*8]       ; r12= digest[4] (E)
   mov      r13,[rdi+5*8]       ; r13= digest[5] (F)
   mov      r14,[rdi+6*8]       ; r14= digest[6] (G)
   mov      r15,[rdi+7*8]       ; r15= digest[7] (H)

;;
;; perform 0-79 steps
;;
;;                A,  B,  C,  D,  E,  F,  G,  H                  W[],        K[]
;;                -------------------------------------------------------------------------
   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*8}, {KK_SHA512+ 0*8}
   UPDATE_2       16, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*8}, {KK_SHA512+ 1*8}
   UPDATE_2       17, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*8}, {KK_SHA512+ 2*8}
   UPDATE_2       18, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*8}, {KK_SHA512+ 3*8}
   UPDATE_2       19, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*8}, {KK_SHA512+ 4*8}
   UPDATE_2       20, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*8}, {KK_SHA512+ 5*8}
   UPDATE_2       21, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*8}, {KK_SHA512+ 6*8}
   UPDATE_2       22, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*8}, {KK_SHA512+ 7*8}
   UPDATE_2       23, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*8}, {KK_SHA512+ 8*8}
   UPDATE_2       24, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*8}, {KK_SHA512+ 9*8}
   UPDATE_2       25, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*8}, {KK_SHA512+10*8}
   UPDATE_2       26, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*8}, {KK_SHA512+11*8}
   UPDATE_2       27, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*8}, {KK_SHA512+12*8}
   UPDATE_2       28, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*8}, {KK_SHA512+13*8}
   UPDATE_2       29, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*8}, {KK_SHA512+14*8}
   UPDATE_2       30, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*8}, {KK_SHA512+15*8}
   UPDATE_2       31, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*8}, {KK_SHA512+16*8}
   UPDATE_2       32, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*8}, {KK_SHA512+17*8}
   UPDATE_2       33, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*8}, {KK_SHA512+18*8}
   UPDATE_2       34, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*8}, {KK_SHA512+19*8}
   UPDATE_2       35, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*8}, {KK_SHA512+20*8}
   UPDATE_2       36, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*8}, {KK_SHA512+21*8}
   UPDATE_2       37, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*8}, {KK_SHA512+22*8}
   UPDATE_2       38, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*8}, {KK_SHA512+23*8}
   UPDATE_2       39, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*8}, {KK_SHA512+24*8}
   UPDATE_2       40, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*8}, {KK_SHA512+25*8}
   UPDATE_2       41, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*8}, {KK_SHA512+26*8}
   UPDATE_2       42, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*8}, {KK_SHA512+27*8}
   UPDATE_2       43, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*8}, {KK_SHA512+28*8}
   UPDATE_2       44, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*8}, {KK_SHA512+29*8}
   UPDATE_2       45, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*8}, {KK_SHA512+30*8}
   UPDATE_2       46, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*8}, {KK_SHA512+31*8}
   UPDATE_2       47, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*8}, {KK_SHA512+32*8}
   UPDATE_2       48, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*8}, {KK_SHA512+33*8}
   UPDATE_2       49, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*8}, {KK_SHA512+34*8}
   UPDATE_2       50, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*8}, {KK_SHA512+35*8}
   UPDATE_2       51, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*8}, {KK_SHA512+36*8}
   UPDATE_2       52, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*8}, {KK_SHA512+37*8}
   UPDATE_2       53, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*8}, {KK_SHA512+38*8}
   UPDATE_2       54, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*8}, {KK_SHA512+39*8}
   UPDATE_2       55, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*8}, {KK_SHA512+40*8}
   UPDATE_2       56, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*8}, {KK_SHA512+41*8}
   UPDATE_2       57, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*8}, {KK_SHA512+42*8}
   UPDATE_2       58, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*8}, {KK_SHA512+43*8}
   UPDATE_2       59, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*8}, {KK_SHA512+44*8}
   UPDATE_2       60, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*8}, {KK_SHA512+45*8}
   UPDATE_2       61, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*8}, {KK_SHA512+46*8}
   UPDATE_2       62, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*8}, {KK_SHA512+47*8}
   UPDATE_2       63, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*8}, {KK_SHA512+48*8}
   UPDATE_2       64, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*8}, {KK_SHA512+49*8}
   UPDATE_2       65, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*8}, {KK_SHA512+50*8}
   UPDATE_2       66, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*8}, {KK_SHA512+51*8}
   UPDATE_2       67, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*8}, {KK_SHA512+52*8}
   UPDATE_2       68, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*8}, {KK_SHA512+53*8}
   UPDATE_2       69, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*8}, {KK_SHA512+54*8}
   UPDATE_2       70, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*8}, {KK_SHA512+55*8}
   UPDATE_2       71, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*8}, {KK_SHA512+56*8}
   UPDATE_2       72, rax,rbx, rcx,rdx
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*8}, {KK_SHA512+57*8}
   UPDATE_2       73, rax,rbx, rcx,rdx
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*8}, {KK_SHA512+58*8}
   UPDATE_2       74, rax,rbx, rcx,rdx
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*8}, {KK_SHA512+59*8}
   UPDATE_2       75, rax,rbx, rcx,rdx
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*8}, {KK_SHA512+60*8}
   UPDATE_2       76, rax,rbx, rcx,rdx
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*8}, {KK_SHA512+61*8}
   UPDATE_2       77, rax,rbx, rcx,rdx
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*8}, {KK_SHA512+62*8}
   UPDATE_2       78, rax,rbx, rcx,rdx
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*8}, {KK_SHA512+63*8}
   UPDATE_2       79, rax,rbx, rcx,rdx

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*8}, {KK_SHA512+64*8}
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*8}, {KK_SHA512+65*8}
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*8}, {KK_SHA512+66*8}
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*8}, {KK_SHA512+67*8}
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*8}, {KK_SHA512+68*8}
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*8}, {KK_SHA512+69*8}
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*8}, {KK_SHA512+70*8}
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*8}, {KK_SHA512+71*8}

   SHA512_STEP_2  r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*8}, {KK_SHA512+72*8}
   SHA512_STEP_2  r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*8}, {KK_SHA512+73*8}
   SHA512_STEP_2  r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*8}, {KK_SHA512+74*8}
   SHA512_STEP_2  r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*8}, {KK_SHA512+75*8}
   SHA512_STEP_2  r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*8}, {KK_SHA512+76*8}
   SHA512_STEP_2  r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*8}, {KK_SHA512+77*8}
   SHA512_STEP_2  r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*8}, {KK_SHA512+78*8}
   SHA512_STEP_2  r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*8}, {KK_SHA512+79*8}

;;
;; update digest
;;
   add      [rdi+0*8],r8
   add      [rdi+1*8],r9
   add      [rdi+2*8],r10
   add      [rdi+3*8],r11
   add      [rdi+4*8],r12
   add      [rdi+5*8],r13
   add      [rdi+6*8],r14
   add      [rdi+7*8],r15

   add      rsi, MBS_SHA512
   sub      qword [rsp+16*sizeof(qword)], MBS_SHA512
   jg       .sha512_block_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA512

%endif    ;; (_IPP32E >= _IPP32E_M7) AND (_IPP32E < _IPP32E_E9 )
%endif    ;; _ENABLE_ALG_SHA512_

