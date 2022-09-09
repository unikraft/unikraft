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
%include "ia_32e_regs.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA256_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_M7) && (_IPP32E < _IPP32E_U8 )


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

;;
;; single SHA256 step
;;
;;    Ipp32u T1 = H + SUM1(E) +  CH(E,F,G) + K_SHA256[t] + W[t];
;;    Ipp32u T2 =     SUM0(A) + MAJ(A,B,C);
;;    D+= T1;
;;    H = T1 + T2;
;;
;; where
;;    SUM1(x) = ROR(x,6) ^ ROR(x,11) ^ ROR(x,25)
;;    SUM0(x) = ROR(x,2) ^ ROR(x,13) ^ ROR(x,22)
;;
;;     CH(x,y,z) = (x & y) ^ (~x & z)
;;    MAJ(x,y,z) = (x & y) ^ (x & z) ^ (y & z) = (x&y)^((x^y)&z)
;;
%macro SHA256_STEP_2 13.nolist
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
  %xdefine %%immCNT %13

        ;; update H (start)
        add     %%regH%+d,%%immCNT    ;; H += W[t]+K_SHA256[t]
        add     %%regH%+d,[%%memW]

        ;; compute T = SUM1(E) + CH(E,F,G)
        mov     %%regF1%+d,%%regE%+d   ;; SUM1() = E
        mov     %%regF2%+d,%%regE%+d   ;; CH() = E
        ror     %%regF1%+d,6        ;; ROR(E,6)
        mov     %%regT%+d, %%regE%+d
        push    %%regE
        not     %%regF2%+d          ;; ~E
        ror     %%regE%+d, 11       ;; ROR(E,11)
        and     %%regT%+d, %%regF%+d   ;; E&F
        and     %%regF2%+d,%%regG%+d   ;;~E&G
        xor     %%regF1%+d,%%regE%+d   ;; ROR(E,6)^ROR(E,11)
        ror     %%regE%+d, 14       ;; ROR(E,25)
        xor     %%regF2%+d,%%regT%+d   ;; CH() = (E&F)&(~E&G)
        xor     %%regF1%+d,%%regE%+d   ;; SUM1() = ROR(E,6)^ROR(E,11)^ROR(E,25)
        pop     %%regE             ;; repair E
        lea     %%regT, [%%regF1+%%regF2]

        ;; update H (continue)
        add     %%regH%+d, %%regT%+d

        ;; update D
        add     %%regD%+d, %%regH%+d

        ;; compute T = SUM0(A) + MAJ(A,B,C)
        mov     %%regF1%+d,%%regA%+d   ;; SUM0() = A
        mov     %%regF2%+d,%%regA%+d   ;; MAJ() = A
        ror     %%regF1%+d,2        ;; ROR(A,2)
        mov     %%regT%+d, %%regA%+d
        push    %%regA
        xor     %%regF2%+d,%%regB%+d   ;; A^B
        ror     %%regA%+d, 13       ;; ROR(A,13)
        and     %%regT%+d, %%regB%+d   ;; A&B
        and     %%regF2%+d,%%regC%+d   ;; (A^B)&C
        xor     %%regF1%+d,%%regA%+d   ;; ROR(A,2)^ROR(A,13)
        ror     %%regA%+d, 9        ;; ROR(A,22)
        xor     %%regF2%+d,%%regT%+d   ;; MAJ() = (A^B)^((A^B)&C)
        xor     %%regF1%+d,%%regA%+d   ;; SUM0() = ROR(A,2)^ROR(A,13)^ROR(A,22)
        pop     %%regA             ;; repair A
        lea     %%regT, [%%regF1+%%regF2]

        ;; update H (finish)
        add     %%regH%+d, %%regT%+d
%endmacro

;;
;; update W[]
;;
;; W[j] = SIG1(W[j- 2]) + W[j- 7]
;;       +SIG0(W[j-15]) + W[j-16]
;;
;; SIG0(x) = ROR(x,7) ^ROR(x,18) ^LSR(x,3)
;; SIG1(x) = ROR(x,17)^ROR(x,19) ^LSR(x,10)
;;
%macro UPDATE_2 5.nolist
  %xdefine %%nr %1
  %xdefine %%sig0 %2
  %xdefine %%sig1 %3
  %xdefine %%W15 %4
  %xdefine %%W2 %5

   mov      %%sig0, [rsp+((%%nr-15) & 0Fh)*4]    ;; W[j-15]
   mov      %%sig1, [rsp+((%%nr-2) & 0Fh)*4]     ;; W[j-2]
   shr      %%sig0, 3
   shr      %%sig1, 10
   mov      %%W15,  [rsp+((%%nr-15) & 0Fh)*4]    ;; W[j-15]
   mov       %%W2,  [rsp+((%%nr-2) & 0Fh)*4]     ;; W[j-2]
   ror      %%W15, 7
   ror      %%W2, 17
   xor      %%sig0, %%W15                        ;; SIG0 = LSR(W[j-15], 3)
   xor      %%sig1, %%W2                         ;; SIG1 = LSR(W[j-2], 10)
   ror      %%W15, 11                          ;; ROR(W[j-15], 18)
   ror      %%W2, 2                            ;; ROR(W[j-2, 19)
   xor      %%sig0, %%W15
   xor      %%sig1, %%W2

   add      %%sig0, [rsp+((%%nr-16) & 0Fh)*4]  ;;SIG0 += W[j-16]
   add      %%sig1, [rsp+((%%nr-7) & 0Fh)*4]   ;;SIG1 += W[j-7]
   add      %%sig0, %%sig1
   mov      [rsp+((%%nr-16) & 0Fh)*4], %%sig0
%endmacro

segment .text align=IPP_ALIGN_FACTOR


;******************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA256(DigestSHA256 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;******************************************************************************************

;;
;; Lib = M7
;;
;; Caller = ippsSHA256Update
;; Caller = ippsSHA256Final
;; Caller = ippsSHA256MessageDigest
;;
;; Caller = ippsSHA224Update
;; Caller = ippsSHA224Final
;; Caller = ippsSHA224MessageDigest
;;
;; Caller = ippsHMACSHA256Update
;; Caller = ippsHMACSHA256Final
;; Caller = ippsHMACSHA256MessageDigest
;;
;; Caller = ippsHMACSHA224Update
;; Caller = ippsHMACSHA224Final
;; Caller = ippsHMACSHA224MessageDigest
;;


%if (_IPP32E >= _IPP32E_U8)
align IPP_ALIGN_FACTOR
pByteSwp DB    3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12
%endif

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA256,PUBLIC
%assign LOCAL_FRAME (16*sizeof(dword) + sizeof(qword))
        USES_GPR rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 4

;; rdi = digest
;; rsi = data buffer
;; rdx = buffer len
;; rcx = dummy parameter

%xdefine MBS_SHA256    (64)

   movsxd   rdx, edx
   mov      qword [rsp+16*sizeof(dword)], rdx   ; save length of buffer

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha256_block_loop:

;;
;; initialize the first 16 words in the array W (remember about endian)
;;
%if (_IPP32E >= _IPP32E_U8)
   movdqa   xmm4, oword [rel pByteSwp]
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
%else
   xor      rcx,rcx
.loop1:
   mov      r8d,[rsi+rcx*4+0*4]
   ENDIANNESS r8d,r8d
   mov      [rsp+rcx*4+0*4],r8d

   mov      r9d,[rsi+rcx*4+1*4]
   ENDIANNESS r9d,r9d
   mov      [rsp+rcx*4+1*4],r9d

   add      rcx,2
   cmp      rcx,16
   jl       .loop1
%endif

;;
;; init A, B, C, D, E, F, G, H by the internal digest
;;
   mov      r8d, [rdi+0*4]       ; r8d = digest[0] (A)
   mov      r9d, [rdi+1*4]       ; r9d = digest[1] (B)
   mov      r10d,[rdi+2*4]       ; r10d= digest[2] (C)
   mov      r11d,[rdi+3*4]       ; r11d= digest[3] (D)
   mov      r12d,[rdi+4*4]       ; r12d= digest[4] (E)
   mov      r13d,[rdi+5*4]       ; r13d= digest[5] (F)
   mov      r14d,[rdi+6*4]       ; r14d= digest[6] (G)
   mov      r15d,[rdi+7*4]       ; r15d= digest[7] (H)

;;
;; perform 0-64 steps
;;
;;             A,   B,   C,   D,   E,   F,   G,   H     T,  F1,  F2,  W[],       K256
;;             ----------------------------------------------------------------------------
   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*4}, 0428A2F98h
   UPDATE_2    16, eax,ebx, ecx,edx
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*4}, 071374491h
   UPDATE_2    17, eax,ebx, ecx,edx
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*4}, 0B5C0FBCFh
   UPDATE_2    18, eax,ebx, ecx,edx
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*4}, 0E9B5DBA5h
   UPDATE_2    19, eax,ebx, ecx,edx
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*4}, 03956C25Bh
   UPDATE_2    20, eax,ebx, ecx,edx
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*4}, 059F111F1h
   UPDATE_2    21, eax,ebx, ecx,edx
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*4}, 0923F82A4h
   UPDATE_2    22, eax,ebx, ecx,edx
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*4}, 0AB1C5ED5h
   UPDATE_2    23, eax,ebx, ecx,edx
   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*4}, 0D807AA98h
   UPDATE_2    24, eax,ebx, ecx,edx
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*4}, 012835B01h
   UPDATE_2    25, eax,ebx, ecx,edx
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*4}, 0243185BEh
   UPDATE_2    26, eax,ebx, ecx,edx
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*4}, 0550C7DC3h
   UPDATE_2    27, eax,ebx, ecx,edx
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*4}, 072BE5D74h
   UPDATE_2    28, eax,ebx, ecx,edx
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*4}, 080DEB1FEh
   UPDATE_2    29, eax,ebx, ecx,edx
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*4}, 09BDC06A7h
   UPDATE_2    30, eax,ebx, ecx,edx
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*4}, 0C19BF174h
   UPDATE_2    31, eax,ebx, ecx,edx

   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*4}, 0E49B69C1h
   UPDATE_2      32, eax,ebx, ecx,edx
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*4}, 0EFBE4786h
   UPDATE_2      33, eax,ebx, ecx,edx
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*4}, 00FC19DC6h
   UPDATE_2      34, eax,ebx, ecx,edx
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*4}, 0240CA1CCh
   UPDATE_2      35, eax,ebx, ecx,edx
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*4}, 02DE92C6Fh
   UPDATE_2      36, eax,ebx, ecx,edx
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*4}, 04A7484AAh
   UPDATE_2      37, eax,ebx, ecx,edx
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*4}, 05CB0A9DCh
   UPDATE_2    38, eax,ebx, ecx,edx
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*4}, 076F988DAh
   UPDATE_2    39, eax,ebx, ecx,edx
   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*4}, 0983E5152h
   UPDATE_2    40, eax,ebx, ecx,edx
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*4}, 0A831C66Dh
   UPDATE_2      41, eax,ebx, ecx,edx
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*4}, 0B00327C8h
   UPDATE_2      42, eax,ebx, ecx,edx
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*4}, 0BF597FC7h
   UPDATE_2      43, eax,ebx, ecx,edx
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*4}, 0C6E00BF3h
   UPDATE_2      44, eax,ebx, ecx,edx
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*4}, 0D5A79147h
   UPDATE_2      45, eax,ebx, ecx,edx
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*4}, 006CA6351h
   UPDATE_2      46, eax,ebx, ecx,edx
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*4}, 014292967h
   UPDATE_2      47, eax,ebx, ecx,edx

   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*4}, 027B70A85h
   UPDATE_2      48, eax,ebx, ecx,edx
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*4}, 02E1B2138h
   UPDATE_2      49, eax,ebx, ecx,edx
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*4}, 04D2C6DFCh
   UPDATE_2      50, eax,ebx, ecx,edx
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*4}, 053380D13h
   UPDATE_2      51, eax,ebx, ecx,edx
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*4}, 0650A7354h
   UPDATE_2      52, eax,ebx, ecx,edx
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*4}, 0766A0ABBh
   UPDATE_2      53, eax,ebx, ecx,edx
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*4}, 081C2C92Eh
   UPDATE_2      54, eax,ebx, ecx,edx
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*4}, 092722C85h
   UPDATE_2      55, eax,ebx, ecx,edx
   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*4}, 0A2BFE8A1h
   UPDATE_2      56, eax,ebx, ecx,edx
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*4}, 0A81A664Bh
   UPDATE_2      57, eax,ebx, ecx,edx
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*4}, 0C24B8B70h
   UPDATE_2      58, eax,ebx, ecx,edx
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*4}, 0C76C51A3h
   UPDATE_2      59, eax,ebx, ecx,edx
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*4}, 0D192E819h
   UPDATE_2      60, eax,ebx, ecx,edx
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*4}, 0D6990624h
   UPDATE_2      61, eax,ebx, ecx,edx
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*4}, 0F40E3585h
   UPDATE_2      62, eax,ebx, ecx,edx
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*4}, 0106AA070h
   UPDATE_2      63, eax,ebx, ecx,edx

   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 0*4}, 019A4C116h
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 1*4}, 01E376C08h
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+ 2*4}, 02748774Ch
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+ 3*4}, 034B0BCB5h
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+ 4*4}, 0391C0CB3h
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+ 5*4}, 04ED8AA4Ah
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+ 6*4}, 05B9CCA4Fh
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+ 7*4}, 0682E6FF3h
   SHA256_STEP_2 r8, r9, r10,r11,r12,r13,r14,r15, rbx,rcx,rdx, {rsp+ 8*4}, 0748F82EEh
   SHA256_STEP_2 r15,r8, r9, r10,r11,r12,r13,r14, rbx,rcx,rdx, {rsp+ 9*4}, 078A5636Fh
   SHA256_STEP_2 r14,r15,r8, r9, r10,r11,r12,r13, rbx,rcx,rdx, {rsp+10*4}, 084C87814h
   SHA256_STEP_2 r13,r14,r15,r8, r9, r10,r11,r12, rbx,rcx,rdx, {rsp+11*4}, 08CC70208h
   SHA256_STEP_2 r12,r13,r14,r15,r8, r9, r10,r11, rbx,rcx,rdx, {rsp+12*4}, 090BEFFFAh
   SHA256_STEP_2 r11,r12,r13,r14,r15,r8, r9, r10, rbx,rcx,rdx, {rsp+13*4}, 0A4506CEBh
   SHA256_STEP_2 r10,r11,r12,r13,r14,r15,r8, r9,  rbx,rcx,rdx, {rsp+14*4}, 0BEF9A3F7h
   SHA256_STEP_2 r9, r10,r11,r12,r13,r14,r15,r8,  rbx,rcx,rdx, {rsp+15*4}, 0C67178F2h

;;
;; update digest
;;
   add      [rdi+0*4],r8d
   add      [rdi+1*4],r9d
   add      [rdi+2*4],r10d
   add      [rdi+3*4],r11d
   add      [rdi+4*4],r12d
   add      [rdi+5*4],r13d
   add      [rdi+6*4],r14d
   add      [rdi+7*4],r15d

   add      rsi, MBS_SHA256
   sub      qword [rsp+16*sizeof(dword)], MBS_SHA256
   jg       .sha256_block_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA256

%endif    ;; (_IPP32E >= _IPP32E_M7) AND (_IPP32E < _IPP32E_U8 )
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA256_

