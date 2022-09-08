;===============================================================================
; Copyright 2014-2021 Intel Corporation
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
;






%include "asmdefs.inc"
%include "ia_emm.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA256_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP >= _IPP_G9)

%xdefine XMM_SHUFB_BSWAP xmm6
%xdefine W0              xmm0
%xdefine W4              xmm1
%xdefine W8              xmm2
%xdefine W12             xmm3
%xdefine SIG1            xmm4
%xdefine SIG0            xmm5
%xdefine X               xmm6
%xdefine W               xmm7

%xdefine regTbl          ebx


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
   xor   %%Z, %%Y
   and   %%F, %%Z
   xor   %%Z, %%Y
   xor   %%F, %%Z
%endmacro


%macro ROTR 2.nolist
  %xdefine %%X %1
  %xdefine %%n %2

   shrd  %%X,%%X, %%n
   ;;ror   X, n
%endmacro


;;
;; Summ0(x) = ROR(x,2) ^ ROR(x,13) ^ ROR(x,22)
;;
%macro Summ0 3.nolist
  %xdefine %%F %1
  %xdefine %%X %2
  %xdefine %%T %3

   mov   %%F, %%X
   ROTR  %%F, 2
   mov   %%T, %%X
   ROTR  %%T, 13
   xor   %%F, %%T
   ROTR  %%T, (22-13)
   xor   %%F, %%T
%endmacro


;;
;; Summ1(x) = ROR(x,6) ^ ROR(x,11) ^ ROR(x,25)
;;
%macro Summ1 3.nolist
  %xdefine %%F %1
  %xdefine %%X %2
  %xdefine %%T %3

   mov   %%F, %%X
   ROTR  %%F, 6
   mov   %%T, %%X
   ROTR  %%T, 11
   xor   %%F, %%T
   ROTR  %%T, (25-11)
   xor   %%F, %%T
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
%macro ROUND 6.nolist
  %xdefine %%nr %1
  %xdefine %%hashBuff %2
  %xdefine %%wBuff %3
  %xdefine %%F1 %4
  %xdefine %%F2 %5
  %xdefine %%T1 %6
  ; %xdefine T2 %7

   Summ1    %%F1, eax, %%T1
   Chj      %%F2, eax,{[%%hashBuff+((vF-%%nr)&7)*sizeof(dword)]},{[%%hashBuff+((vG-%%nr)&7)*sizeof(dword)]}
   mov      eax, [%%hashBuff+((vH-%%nr)&7)*sizeof(dword)]
   add      eax, %%F1
   add      eax, %%F2
   add      eax, dword [%%wBuff+(%%nr&3)*sizeof(dword)]

   mov      %%F1, dword [%%hashBuff+((vB-%%nr)&7)*sizeof(dword)]
   mov      %%T1, dword [%%hashBuff+((vC-%%nr)&7)*sizeof(dword)]
   Maj      %%F2, edx,%%F1, %%T1
   Summ0    %%F1, edx, %%T1
   lea      edx, [%%F1+%%F2]

   add      edx,eax                                      ; T2+T1
   add      eax,[%%hashBuff+((vD-%%nr)&7)*sizeof(dword)] ; T1+d

   mov      [%%hashBuff+((vH-%%nr)&7)*sizeof(dword)],edx
   mov      [%%hashBuff+((vD-%%nr)&7)*sizeof(dword)],eax
%endmacro


;;
;; W[i] = Sigma1(W[i-2]) + W[i-7] + Sigma0(W[i-15]) + W[i-16], i=16,..,63
;;
;;for next rounds 16,17,18 and 19:
;; W[0] <= W[16] = Sigma1(W[14]) + W[ 9] + Sigma0(W[1]) + W[0]
;; W[1] <= W[17] = Sigma1(W[15]) + W[10] + Sigma0(W[2]) + W[1]
;; W[2] <= W[18] = Sigma1(W[ 0]) + W[11] + Sigma0(W[3]) + W[1]
;; W[3] <= W[19] = Sigma1(W[ 1]) + W[12] + Sigma0(W[4]) + W[2]
;;
;; the process is repeated exactly because texual round of W[]
;;
;; Sigma1() and Sigma0() functions are defined as following:
;; Sigma1(X) = ROR(X,17)^ROR(X,19)^SHR(X,10)
;; Sigma0(X) = ROR(X, 7)^ROR(X,18)^SHR(X, 3)
;;
%macro UPDATE_W 8.nolist
  %xdefine %%xS %1
  %xdefine %%xS0 %2
  %xdefine %%xS4 %3
  %xdefine %%xS8 %4
  %xdefine %%xS12 %5
  %xdefine %%SIGMA1 %6
  %xdefine %%SIGMA0 %7
  %xdefine %%X %8

   vpshufd  %%X, %%xS12, 11111010b   ;; SIGMA1 = {W[15],W[15],W[14],W[14]}
   vpsrld   %%SIGMA1, %%X, 10
   vpsrlq   %%X, %%X, 17
   vpxor    %%SIGMA1, %%SIGMA1, %%X
   vpsrlq   %%X, %%X, (19-17)
   vpxor    %%SIGMA1, %%SIGMA1, %%X

   vpshufd  %%X, %%xS0, 10100101b   ;; SIGMA0 = {W[2],W[2],W[1],W[1]}
   vpsrld   %%SIGMA0, %%X, 3
   vpsrlq   %%X, %%X, 7
   vpxor    %%SIGMA0, %%SIGMA0, %%X
   vpsrlq   %%X, %%X, (18-7)
   vpxor    %%SIGMA0, %%SIGMA0, %%X

   vpshufd  %%xS, %%xS0, 01010000b   ;; {W[ 1],W[ 1],W[ 0],W[ 0]}
   vpaddd   %%SIGMA1, %%SIGMA1, %%SIGMA0
   vpshufd  %%X, %%xS8, 10100101b    ;; {W[10],W[10],W[ 9],W[ 9]}
   vpaddd   %%xS, %%xS, %%SIGMA1
   vpaddd   %%xS, %%xS, %%X


   vpshufd  %%X, %%xS, 10100000b   ;; SIGMA1 = {W[1],W[1],W[0],W[0]}
   vpsrld   %%SIGMA1, %%X, 10
   vpsrlq   %%X, %%X, 17
   vpxor    %%SIGMA1, %%SIGMA1, %%X
   vpsrlq   %%X, %%X, (19-17)
   vpxor    %%SIGMA1, %%SIGMA1, %%X

   vpalignr %%X, %%xS4, %%xS0, (3*sizeof(dword)) ;; SIGMA0 = {W[4],W[4],W[3],W[3]}
   vpshufd  %%X, %%X, 01010000b
   vpsrld   %%SIGMA0, %%X, 3
   vpsrlq   %%X, %%X, 7
   vpxor    %%SIGMA0, %%SIGMA0, %%X
   vpsrlq   %%X, %%X, (18-7)
   vpxor    %%SIGMA0, %%SIGMA0, %%X

   vpalignr %%X, %%xS12, %%xS8, (3*sizeof(dword))  ;; {W[14],W[13],W[12],W[11]}
   vpshufd  %%xS0, %%xS0, 11111010b              ;; {W[ 3],W[ 3],W[ 2],W[ 2]}
   vpaddd   %%SIGMA1, %%SIGMA1, %%SIGMA0
   vpshufd  %%X, %%X, 01010000b                  ;; {W[12],W[12],W[11],W[11]}
   vpaddd   %%xS0, %%xS0, %%SIGMA1
   vpaddd   %%xS0, %%xS0, %%X

   vpshufd  %%xS, %%xS, 10001000b          ;; {W[1],W[0],W[1],W[0]}
   vpshufd  %%xS0, %%xS0, 10001000b        ;; {W[3],W[2],W[3],W[2]}
   vpalignr %%xS0, %%xS0, %%xS, (2*sizeof(dword)) ;; {W[3],W[2],W[1],W[0]}
%endmacro



segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR
SWP_BYTE:
pByteSwp DB    3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; UpdateSHA256(Ipp32u digest[], Ipp8u dataBlock[], int datalen, Ipp32u K_256[])
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM UpdateSHA256,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pHash   [ebp + ARG_1 + 0*sizeof(dword)] ; pointer to hash
%xdefine pData   [ebp + ARG_1 + 1*sizeof(dword)] ; pointer to data block
%xdefine dataLen [ebp + ARG_1 + 2*sizeof(dword)] ; data length
%xdefine pTbl    [ebp + ARG_1 + 3*sizeof(dword)] ; pointer to the SHA256 const table

%xdefine MBS_SHA256 (64)

%assign  hSize      sizeof(dword)*8 ; size of hash
%assign  wSize      sizeof(oword)   ; W values queue (dwords)
%assign  cntSize    sizeof(dword)   ; local counter

%assign  hashOff    0               ; hash address
%assign  wOff       hashOff+hSize   ; W values offset
%assign  cntOff     wOff+wSize

%assign stackSize  (hSize+wSize+cntSize)   ; stack size
   sub            esp, stackSize

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha256_block_loop:

   mov            eax, pHash  ; pointer to the hash
   vmovdqu        W0, oword [eax]              ; load initial hash value
   vmovdqu        W4, oword [eax+sizeof(oword)]
   vmovdqu        oword [esp+hashOff], W0
   vmovdqu        oword [esp+hashOff+sizeof(oword)*1], W4

   mov            eax, pData     ; pointer to the data block
   mov            regTbl, pTbl   ; pointer to SHA256 table (points K_256[] constants)
  ;vmovdqa        XMM_SHUFB_BSWAP, oword pByteSwp ; load shuffle mask
   LD_ADDR        ecx, SWP_BYTE
   movdqa         XMM_SHUFB_BSWAP, oword [ecx+(pByteSwp-SWP_BYTE)]

   vmovdqu        W0, oword [eax]       ; load buffer content
   vmovdqu        W4, oword [eax+sizeof(oword)]
   vmovdqu        W8, oword [eax+sizeof(oword)*2]
   vmovdqu        W12,oword [eax+sizeof(oword)*3]

%assign vA  0
%assign vB  1
%assign vC  2
%assign vD  3
%assign vE  4
%assign vF  5
%assign vG  6
%assign vH  7

   mov      eax, [esp+hashOff+vE*sizeof(dword)]
   mov      edx, [esp+hashOff+vA*sizeof(dword)]

   ;; perform 0-3 regular rounds
   vpshufb  W0, W0, XMM_SHUFB_BSWAP                      ; swap input
   vpaddd   W, W0, oword [regTbl+sizeof(oword)*0]    ; T += K_SHA256[0-3]
   vmovdqu  oword [esp+wOff], W
   ROUND    0, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    1, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    2, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    3, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   ;; perform next 4-7 regular rounds
   vpshufb  W4, W4, XMM_SHUFB_BSWAP                      ; swap input
   vpaddd   W, W4, oword [regTbl+sizeof(oword)*1]    ; T += K_SHA256[4-7]
   vmovdqu  oword [esp+wOff], W
   ROUND    4, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    5, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    6, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    7, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   ;; perform next 8-11 regular rounds
   vpshufb  W8, W8, XMM_SHUFB_BSWAP                      ; swap input
   vpaddd   W, W8, oword [regTbl+sizeof(oword)*2]    ; T += K_SHA256[8-11]
   vmovdqu  oword [esp+wOff], W
   ROUND    8, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND    9, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   10, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   11, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   ;; perform next 12-15 regular rounds
   vpshufb  W12, W12, XMM_SHUFB_BSWAP                    ; swap input
   vpaddd   W, W12, oword [regTbl+sizeof(oword)*3]   ; T += K_SHA256[12-15]
   vmovdqu  oword [esp+wOff], W
   ROUND   12, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   13, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   14, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   15, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   mov      dword [esp+cntOff], (64-16)        ; init counter
.loop_16_63:
   add      regTbl, sizeof(oword)*4                ; update SHA_256 pointer

   UPDATE_W    W, W0, W4, W8, W12, SIG1,SIG0,X           ; round: 16*i - 16*i+3
   vpaddd      W, W0, oword [regTbl+sizeof(oword)*0] ; T += K_SHA256[16-19]
   vmovdqu     oword [esp+wOff], W
   ROUND   16, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   17, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   18, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   19, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   UPDATE_W    W, W4, W8, W12,W0,  SIG1,SIG0,X           ; round: 20*i 20*i+3
   vpaddd      W, W4, oword [regTbl+sizeof(oword)*1] ; T += K_SHA256[20-23]
   vmovdqu     oword [esp+wOff], W
   ROUND   20, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   21, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   22, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   23, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   UPDATE_W    W, W8, W12,W0, W4,  SIG1,SIG0,X           ; round: 24*i - 24*i+3
   vpaddd      W, W8, oword [regTbl+sizeof(oword)*2] ; T += K_SHA256[24-27]
   vmovdqu     oword [esp+wOff], W
   ROUND   24, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   25, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   26, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   27, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   UPDATE_W    W, W12,W0, W4, W8,  SIG1,SIG0,X           ; round: 28*i - 28*i+3
   vpaddd      W, W12, oword [regTbl+sizeof(oword)*3]; T += K_SHA256[28-31]
   vmovdqu     oword [esp+wOff], W
   ROUND   28, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   29, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   30, {esp+hashOff},{esp+wOff}, esi,edi,ecx
   ROUND   31, {esp+hashOff},{esp+wOff}, esi,edi,ecx

   sub         dword [esp+cntOff], 16
   jg          .loop_16_63

   mov            eax, pHash  ; pointer to the hash
   vmovdqu        W0, oword [esp+hashOff]
   vmovdqu        W4, oword [esp+hashOff+sizeof(oword)*1]

   ; update hash
   vmovdqu        W, oword [eax]
   vpaddd         W, W, W0
   vmovdqu        oword [eax], W
   vmovdqu        W, oword [eax+sizeof(oword)]
   vpaddd         W, W, W4
   vmovdqu        oword [eax+sizeof(oword)], W

   add            dword pData, MBS_SHA256
   sub            dword dataLen, MBS_SHA256
   jg             .sha256_block_loop

   add            esp, stackSize
   REST_GPR
   ret
ENDFUNC UpdateSHA256

%endif    ;; _IPP32E_G9 and above
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA256_

