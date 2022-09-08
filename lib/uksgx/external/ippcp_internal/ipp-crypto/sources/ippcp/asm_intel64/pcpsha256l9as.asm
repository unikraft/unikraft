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
%if (_IPP32E >= _IPP32E_L9 )

;;
;; assignments
;;
%xdefine hA  eax  ;; hash values into GPU registers
%xdefine hB  ebx
%xdefine hC  ecx
%xdefine hD  edx
%xdefine hE  r8d
%xdefine hF  r9d
%xdefine hG  r10d
%xdefine hH  r11d

%xdefine T1  r12d ;; scratch
%xdefine T2  r13d
%xdefine T3  r14d
%xdefine T4  r15d
%xdefine T5  edi

%xdefine W0  ymm0 ;; W values into YMM registers
%xdefine W1  ymm1
%xdefine W2  ymm2
%xdefine W3  ymm3

%xdefine yT1 ymm4 ;; scratch
%xdefine yT2 ymm5
%xdefine yT3 ymm6
%xdefine yT4 ymm7

%xdefine W0L xmm0
%xdefine W1L xmm1
%xdefine W2L xmm2
%xdefine W3L xmm3

%xdefine YMM_zzBA        ymm8  ;; byte swap constant
%xdefine YMM_DCzz        ymm9  ;; byte swap constant
%xdefine YMM_SHUFB_BSWAP ymm10 ;; byte swap constant

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; textual rotation of W args
;;
%macro ROTATE_W 0.nolist
  %xdefine %%_X   W0
  %xdefine W0   W1
  %xdefine W1   W2
  %xdefine W2   W3
  %xdefine W3   %%_X
%endmacro

;;
;; textual rotation of HASH arguments
;;
%macro ROTATE_H 0.nolist
  %xdefine %%_X   hH
  %xdefine hH   hG
  %xdefine hG   hF
  %xdefine hF   hE
  %xdefine hE   hD
  %xdefine hD   hC
  %xdefine hC   hB
  %xdefine hB   hA
  %xdefine hA   %%_X
%endmacro

%macro ROTATE_T4_T5 0.nolist
  %xdefine %%T T4
  %xdefine T4 T5
  %xdefine T5 %%T
%endmacro

;;
;; compute next 4 W[t], W[t+1], W[t+2] and W[t+3], t=16,...63
;; (see pcpsha256e9as.asm for details)
%macro UPDATE_W 5.nolist
  %xdefine %%nr %1
  %xdefine %%W0 %2
  %xdefine %%W1 %3
  %xdefine %%W2 %4
  %xdefine %%W3 %5

  %assign %%W_AHEAD  16

   vpalignr yT3,%%W1,%%W0,4
   vpalignr yT2,%%W3,%%W2,4
   vpsrld   yT1,yT3,7
   vpaddd   %%W0,%%W0,yT2
   vpsrld   yT2,yT3,3
   vpslld   yT4,yT3,14
   vpxor    yT3,yT2,yT1
   vpshufd  yT2,%%W3,250
   vpsrld   yT1,yT1,11
   vpxor    yT3,yT3,yT4
   vpslld   yT4,yT4,11
   vpxor    yT3,yT3,yT1
   vpsrld   yT1,yT2,10
   vpxor    yT3,yT3,yT4
   vpsrlq   yT2,yT2,17
   vpaddd   %%W0,%%W0,yT3
   vpxor    yT1,yT1,yT2
   vpsrlq   yT2,yT2,2
   vpxor    yT1,yT1,yT2
   vpshufb  yT1,yT1,YMM_zzBA
   vpaddd   %%W0,%%W0,yT1
   vpshufd  yT2,%%W0,80
   vpsrld   yT1,yT2,10
   vpsrlq   yT2,yT2,17
   vpxor    yT1,yT1,yT2
   vpsrlq   yT2,yT2,2
   vpxor    yT1,yT1,yT2
   vpshufb  yT1,yT1,YMM_DCzz
   vpaddd   %%W0,%%W0,yT1
   vpaddd   yT1,%%W0,YMMWORD [rbp+(%%nr/4)*sizeof(ymmword)]
   vmovdqa  YMMWORD [rsi+(%%W_AHEAD/4)*sizeof(ymmword)+(%%nr/4)*sizeof(ymmword)],yT1
%endmacro

;;
;; regular round (i):
;;
;; T1 = h + Sum1(e) + Ch(e,f,g) + K[i] + W[i]
;; T2 = Sum0(a) + Maj(a,b,c)
;; h = g
;; g = f
;; f = e
;; e = d + T1
;; d = c
;; c = b
;; b = a
;; a = T1+T2
;;
;; sum1(e) = (e>>>25)^(e>>>11)^(e>>>6)
;; sum0(a) = (a>>>13)^(a>>>22)^(a>>>2)
;; ch(e,f,g) = (e&f)^(~e^g)
;; maj(a,b,m)= (a&b)^(a&c)^(b&c)
;;
;; note:
;; 1) U + ch(e,f,g) = U + (e&f) & (~e&g)
;; 2) maj(a,b,c)= (a&b)^(a&c)^(b&c) = (a^b)&(b^c) ^b
;; to make sure both are correct - use GF(2) arith instead of logic
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;    or
;; X = sum0(a[i-1]) computed on prev round
;; a[i] += X
;; h[i] += (K[i]+W[i]) + sum1(e[i]) + ch(e[i],f[i],g[i]) or
;; h[i] += (K[i]+W[i]) + sum1(e[i]) + (e[i]&f[i]) + (~e[i]&g[i])  -- helps break dependencies
;; d[i] += h[i]
;; h[i] += maj(a[i],b[i],c[i])
;; and following textual shift
;;   {a[i+1],b[i+1],c[i+1],d[i+1],e[i+1],f[i+1],g[i+1],h[i+1]} <= {h[i],a[i],b[i],c[i],d[i],e[i],f[i],g[i]}
;;
;; on entry:
;; - T1 = f
;; - T3 = sum0{a[i-1])
;; - T5 = b&c
%macro SHA256_ROUND 9.nolist
  %xdefine %%nr %1
  %xdefine %%hA %2
  %xdefine %%hB %3
  %xdefine %%hC %4
  %xdefine %%hD %5
  %xdefine %%hE %6
  %xdefine %%hF %7
  %xdefine %%hG %8
  %xdefine %%hH %9

   add   %%hH, dword [rsi+(%%nr/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)] ;; h += (k[t]+w[t])
   and   T1, %%hE       ;; ch(e,f,g): (f&e)
   rorx  T2, %%hE, 25   ;; sum1(e): e>>>25
   rorx  T4, %%hE, 11   ;; sum1(e): e>>>11
   add   %%hA, T3       ;; complete computation a += sum0(a[t-1])
   add   %%hH, T1       ;; h += (k[t]+w[t]) + (f&e)
   andn  T1, %%hE, %%hG   ;; ch(e,f,g): (~e&g)
   xor   T2, T4       ;; sum1(e): (e>>>25)^(e>>>11)
   rorx  T3, %%hE, 6    ;; sum1(e): e>>>6
   add   %%hH, T1       ;; h += (k[t]+w[t]) + (f&e) + (~e&g)
   xor   T2, T3       ;; sum1(e) = (e>>>25)^(e>>>11)^(e>>>6)
   mov   T4, %%hA       ;; maj(a,b,c): a
   rorx  T1, %%hA, 22   ;; sum0(a): a>>>22
   add   %%hH, T2       ;; h += (k[t]+w[t]) +(f&e) +(~e&g) +sig1(e)
   xor   T4, %%hB       ;; maj(a,b,c): (a^b)
   rorx  T3, %%hA, 13   ;; sum0(a): a>>>13
   rorx  T2, %%hA, 2    ;; sum0(a): a>>>2
   add   %%hD, %%hH       ;; d += h
   and   T5, T4       ;; maj(a,b,c): (b^c)&(a^b)
   xor   T3, T1       ;; sum0(a): (a>>>13)^(a>>>22)
   xor   T5, %%hB       ;; maj(a,b,c) = (b^c)&(a^b)^b = (a&b)^(a&c)^(b&c)
   xor   T3, T2       ;;  sum0(a): = (a>>>13)^(a>>>22)^(a>>>2)
   add   %%hH, T5       ;; h += (k[t]+w[t]) +(f&e) +(~e&g) +sig1(e) +maj(a,b,c)
   mov   T1, %%hE       ;; T1 = f     (next round)
   ROTATE_T4_T5     ;; T5 = (b^c) (next round)
%endmacro

;;
;; does 4 regular rounds and computes next 4 W values
;; (just 4 instances of SHA256_ROUND merged together woth UPDATE_W)
;;
%macro SHA256_4ROUND_SHED 1.nolist
  %xdefine %%round %1

  %assign %%W_AHEAD  16
vpalignr yT3,W1,W0,4
  %assign %%nr  %%round
   add   hH, dword [rsi+(%%nr/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]
   and   T1, hE
   rorx  T2, hE, 25
vpalignr yT2,W3,W2,4
   rorx  T4, hE, 11
   add   hA, T3
   add   hH, T1
vpsrld   yT1,yT3,7
   andn  T1, hE, hG
   xor   T2, T4
   rorx  T3, hE, 6
vpaddd   W0,W0,yT2
   add   hH, T1
   xor   T2, T3
   mov   T4, hA
vpsrld   yT2,yT3,3
   rorx  T1, hA, 22
   add   hH, T2
   xor   T4, hB
vpslld   yT4,yT3,14
   rorx  T3, hA, 13
   rorx  T2, hA, 2
   add   hD, hH
vpxor    yT3,yT2,yT1
   and   T5, T4
   xor   T3, T1
   xor   T5, hB
vpshufd  yT2,W3,250
   xor   T3, T2
   add   hH, T5
   mov   T1, hE
   ROTATE_T4_T5
   ROTATE_H

vpsrld   yT1,yT1,11
  %assign %%nr  %%nr+1
   add   hH, dword [rsi+(%%nr/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]
   and   T1, hE
   rorx  T2, hE, 25
vpxor    yT3,yT3,yT4
   rorx  T4, hE, 11
   add   hA, T3
   add   hH, T1
vpslld   yT4,yT4,11
   andn  T1, hE, hG
   xor   T2, T4
   rorx  T3, hE, 6
vpxor    yT3,yT3,yT1
   add   hH, T1
   xor   T2, T3
   mov   T4, hA
vpsrld   yT1,yT2,10
   rorx  T1, hA, 22
   add   hH, T2
   xor   T4, hB
vpxor    yT3,yT3,yT4
   rorx  T3, hA, 13
   rorx  T2, hA, 2
   add   hD, hH
vpsrlq   yT2,yT2,17
   and   T5, T4
   xor   T3, T1
   xor   T5, hB
vpaddd   W0,W0,yT3
   xor   T3, T2
   add   hH, T5
   mov   T1, hE
   ROTATE_T4_T5
   ROTATE_H

vpxor    yT1,yT1,yT2
  %assign %%nr  %%nr+1
   add   hH, dword [rsi+(%%nr/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]
   and   T1, hE
   rorx  T2, hE, 25
vpsrlq   yT2,yT2,2
   rorx  T4, hE, 11
   add   hA, T3
   add   hH, T1
vpxor    yT1,yT1,yT2
   andn  T1, hE, hG
   xor   T2, T4
   rorx  T3, hE, 6
vpshufb  yT1,yT1,YMM_zzBA
   add   hH, T1
   xor   T2, T3
   mov   T4, hA
vpaddd   W0,W0,yT1
   rorx  T1, hA, 22
   add   hH, T2
   xor   T4, hB
vpshufd  yT2,W0,80
   rorx  T3, hA, 13
   rorx  T2, hA, 2
   add   hD, hH
vpsrld   yT1,yT2,10
   and   T5, T4
   xor   T3, T1
   xor   T5, hB
vpsrlq   yT2,yT2,17
   xor   T3, T2
   add   hH, T5
   mov   T1, hE
   ROTATE_T4_T5
   ROTATE_H

vpxor    yT1,yT1,yT2
  %assign %%nr  %%nr+1
   add   hH, dword [rsi+(%%nr/4)*sizeof(ymmword)+(%%nr & 3)*sizeof(dword)]
   and   T1, hE
   rorx  T2, hE, 25
vpsrlq   yT2,yT2,2
   rorx  T4, hE, 11
   add   hA, T3
   add   hH, T1
vpxor    yT1,yT1,yT2
   andn  T1, hE, hG
   xor   T2, T4
   rorx  T3, hE, 6
vpshufb  yT1,yT1,YMM_DCzz
   add   hH, T1
   xor   T2, T3
   mov   T4, hA
vpaddd   W0,W0,yT1
   rorx  T1, hA, 22
   add   hH, T2
   xor   T4, hB
vpaddd   yT1,W0,YMMWORD [rbp+(%%nr/4)*sizeof(ymmword)]
   rorx  T3, hA, 13
   rorx  T2, hA, 2
   add   hD, hH
   and   T5, T4
   xor   T3, T1
   xor   T5, hB
vmovdqa  YMMWORD [rsi+(%%W_AHEAD/4)*sizeof(ymmword)+(%%round/4)*sizeof(ymmword)],yT1
   xor   T3, T2
   add   hH, T5
   mov   T1, hE
   ROTATE_T4_T5
   ROTATE_H

   ROTATE_W
%endmacro

;;
;; update hash
;;
%macro UPDATE_HASH 2.nolist
  %xdefine %%hashMem %1
  %xdefine %%hash %2

     add    %%hash, %%hashMem
     mov    %%hashMem, %%hash
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR

SHA256_YMM_K   dd 0428a2f98h, 071374491h, 0b5c0fbcfh, 0e9b5dba5h,  0428a2f98h, 071374491h, 0b5c0fbcfh, 0e9b5dba5h
               dd 03956c25bh, 059f111f1h, 0923f82a4h, 0ab1c5ed5h,  03956c25bh, 059f111f1h, 0923f82a4h, 0ab1c5ed5h
               dd 0d807aa98h, 012835b01h, 0243185beh, 0550c7dc3h,  0d807aa98h, 012835b01h, 0243185beh, 0550c7dc3h
               dd 072be5d74h, 080deb1feh, 09bdc06a7h, 0c19bf174h,  072be5d74h, 080deb1feh, 09bdc06a7h, 0c19bf174h
               dd 0e49b69c1h, 0efbe4786h, 00fc19dc6h, 0240ca1cch,  0e49b69c1h, 0efbe4786h, 00fc19dc6h, 0240ca1cch
               dd 02de92c6fh, 04a7484aah, 05cb0a9dch, 076f988dah,  02de92c6fh, 04a7484aah, 05cb0a9dch, 076f988dah
               dd 0983e5152h, 0a831c66dh, 0b00327c8h, 0bf597fc7h,  0983e5152h, 0a831c66dh, 0b00327c8h, 0bf597fc7h
               dd 0c6e00bf3h, 0d5a79147h, 006ca6351h, 014292967h,  0c6e00bf3h, 0d5a79147h, 006ca6351h, 014292967h
               dd 027b70a85h, 02e1b2138h, 04d2c6dfch, 053380d13h,  027b70a85h, 02e1b2138h, 04d2c6dfch, 053380d13h
               dd 0650a7354h, 0766a0abbh, 081c2c92eh, 092722c85h,  0650a7354h, 0766a0abbh, 081c2c92eh, 092722c85h
               dd 0a2bfe8a1h, 0a81a664bh, 0c24b8b70h, 0c76c51a3h,  0a2bfe8a1h, 0a81a664bh, 0c24b8b70h, 0c76c51a3h
               dd 0d192e819h, 0d6990624h, 0f40e3585h, 0106aa070h,  0d192e819h, 0d6990624h, 0f40e3585h, 0106aa070h
               dd 019a4c116h, 01e376c08h, 02748774ch, 034b0bcb5h,  019a4c116h, 01e376c08h, 02748774ch, 034b0bcb5h
               dd 0391c0cb3h, 04ed8aa4ah, 05b9cca4fh, 0682e6ff3h,  0391c0cb3h, 04ed8aa4ah, 05b9cca4fh, 0682e6ff3h
               dd 0748f82eeh, 078a5636fh, 084c87814h, 08cc70208h,  0748f82eeh, 078a5636fh, 084c87814h, 08cc70208h
               dd 090befffah, 0a4506cebh, 0bef9a3f7h, 0c67178f2h,  090befffah, 0a4506cebh, 0bef9a3f7h, 0c67178f2h

SHA256_YMM_BF  dd 000010203h, 004050607h, 008090a0bh, 00c0d0e0fh,  000010203h, 004050607h, 008090a0bh, 00c0d0e0fh

SHA256_DCzz db 0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh, 0,1,2,3, 8,9,10,11
            db 0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh, 0,1,2,3, 8,9,10,11
SHA256_zzBA db 0,1,2,3, 8,9,10,11, 0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh
            db 0,1,2,3, 8,9,10,11, 0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh,0ffh


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; UpdateSHA256(Ipp32u digest[], Ipp8u dataBlock[], int datalen, Ipp32u K_256[])
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA256,PUBLIC
%assign LOCAL_FRAME (sizeof(qword)*4 + sizeof(dword)*64*2)
        USES_GPR rbx,rsi,rdi,rbp,rbx,r12,r13,r14,r15
        USES_XMM_AVX xmm6,xmm7,xmm8,xmm9,xmm10
        COMP_ABI 4
;;
;; rdi = pointer to the updated hash
;; rsi = pointer to the data block
;; rdx = data block length
;; rcx = pointer to the SHA_256 constant (ignored)
;;

%xdefine MBS_SHA256    (64)

;;
;; stack structure:
;;
%assign _block    0                          ;; block address
%assign _hash    _block+sizeof(qword)        ;; hash address
%assign _len     _hash+sizeof(qword)         ;; rest of processed data
%assign _frame   _len+sizeof(qword)          ;; rsp before alignment
%assign _dataW   _frame+sizeof(qword)        ;; W[t] values


   mov      r15, rsp                ; store orifinal rsp
   and      rsp, -IPP_ALIGN_FACTOR  ; 32-byte aligned stack

   movsxd   r14, edx                ; input length in bytes

   mov      qword [rsp+_hash], rdi  ; store hash address
   mov      qword [rsp+_len], r14   ; store length
   mov      qword [rsp+_frame], r15 ; store rsp

   mov      hA, dword [rdi]       ; load initial hash value
   mov      hB, dword [rdi+1*sizeof(dword)]
   mov      hC, dword [rdi+2*sizeof(dword)]
   mov      hD, dword [rdi+3*sizeof(dword)]
   mov      hE, dword [rdi+4*sizeof(dword)]
   mov      hF, dword [rdi+5*sizeof(dword)]
   mov      hG, dword [rdi+6*sizeof(dword)]
   mov      hH, dword [rdi+7*sizeof(dword)]

   vmovdqa  YMM_SHUFB_BSWAP, ymmword [rel SHA256_YMM_BF]   ; load byte shuffler

   vmovdqa  YMM_zzBA, ymmword [rel SHA256_zzBA]   ; load byte shuffler (zzBA)
   vmovdqa  YMM_DCzz, ymmword [rel SHA256_DCzz]   ; load byte shuffler (DCzz)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data 2 block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
.sha256_block2_loop:
   lea      r12, [rsi+MBS_SHA256]   ; next block

   cmp      r14, MBS_SHA256         ; %if single block processed
   cmovbe   r12, rsi                ; use the same data block address
   lea      rbp, [rel SHA256_YMM_K] ; to SHA256 consts

   vmovdqu  W0L, xmmword [rsi]            ; load data block
   vmovdqu  W1L, xmmword [rsi+1*sizeof(xmmword)]
   vmovdqu  W2L, xmmword [rsi+2*sizeof(xmmword)]
   vmovdqu  W3L, xmmword [rsi+3*sizeof(xmmword)]

   vinserti128 W0, W0, xmmword [r12], 1   ; merge next data block
   vinserti128 W1, W1, xmmword [r12+1*sizeof(xmmword)], 1
   vinserti128 W2, W2, xmmword [r12+2*sizeof(xmmword)], 1
   vinserti128 W3, W3, xmmword [r12+3*sizeof(xmmword)], 1

   vpshufb  W0, W0, YMM_SHUFB_BSWAP
   vpshufb  W1, W1, YMM_SHUFB_BSWAP
   vpshufb  W2, W2, YMM_SHUFB_BSWAP
   vpshufb  W3, W3, YMM_SHUFB_BSWAP
   vpaddd   yT1, W0, ymmword [rbp]
   vpaddd   yT2, W1, ymmword [rbp+1*sizeof(ymmword)]
   vpaddd   yT3, W2, ymmword [rbp+2*sizeof(ymmword)]
   vpaddd   yT4, W3, ymmword [rbp+3*sizeof(ymmword)]
   add      rbp, 4*sizeof(ymmword)
   vmovdqa  ymmword [rsp+_dataW], yT1
   vmovdqa  ymmword [rsp+_dataW+1*sizeof(ymmword)], yT2
   vmovdqa  ymmword [rsp+_dataW+2*sizeof(ymmword)], yT3
   vmovdqa  ymmword [rsp+_dataW+3*sizeof(ymmword)], yT4

   mov      T5, hB   ; T5 = b^c
   xor      T3, T3   ; T3 = 0
   mov      T1, hF   ; T1 = f
   xor      T5, hC

   mov   qword [rsp+_block], rsi ; store block addres
   lea   rsi, [rsp+_dataW]       ; use rsi as stack pointer

align IPP_ALIGN_FACTOR
.block1_shed_proc:
   SHA256_4ROUND_SHED 0
   SHA256_4ROUND_SHED 4
   SHA256_4ROUND_SHED 8
   SHA256_4ROUND_SHED 12

   add      rsi, 4*sizeof(ymmword)
   add      rbp, 4*sizeof(ymmword)

   ;; and repeat
   cmp      dword [rbp-sizeof(dword)],0c67178f2h
   jne      .block1_shed_proc

   ;; the rest 16 rounds
   SHA256_ROUND  0, hA,hB,hC,hD,hE,hF,hG,hH
   SHA256_ROUND  1, hH,hA,hB,hC,hD,hE,hF,hG
   SHA256_ROUND  2, hG,hH,hA,hB,hC,hD,hE,hF
   SHA256_ROUND  3, hF,hG,hH,hA,hB,hC,hD,hE
   SHA256_ROUND  4, hE,hF,hG,hH,hA,hB,hC,hD
   SHA256_ROUND  5, hD,hE,hF,hG,hH,hA,hB,hC
   SHA256_ROUND  6, hC,hD,hE,hF,hG,hH,hA,hB
   SHA256_ROUND  7, hB,hC,hD,hE,hF,hG,hH,hA
   SHA256_ROUND  8, hA,hB,hC,hD,hE,hF,hG,hH
   SHA256_ROUND  9, hH,hA,hB,hC,hD,hE,hF,hG
   SHA256_ROUND 10, hG,hH,hA,hB,hC,hD,hE,hF
   SHA256_ROUND 11, hF,hG,hH,hA,hB,hC,hD,hE
   SHA256_ROUND 12, hE,hF,hG,hH,hA,hB,hC,hD
   SHA256_ROUND 13, hD,hE,hF,hG,hH,hA,hB,hC
   SHA256_ROUND 14, hC,hD,hE,hF,hG,hH,hA,hB
   SHA256_ROUND 15, hB,hC,hD,hE,hF,hG,hH,hA
   add          hA, T3

   sub      rsi, (16-4)*sizeof(ymmword)   ; restore stack to W

   mov   rdi, qword [rsp+_hash] ; restore hash pointer
   mov   r14, qword [rsp+_len]  ; restore data length

   ;; update hash values by 1-st data block
   UPDATE_HASH    dword [rdi],   hA
   UPDATE_HASH    dword [rdi+1*sizeof(dword)], hB
   UPDATE_HASH    dword [rdi+2*sizeof(dword)], hC
   UPDATE_HASH    dword [rdi+3*sizeof(dword)], hD
   UPDATE_HASH    dword [rdi+4*sizeof(dword)], hE
   UPDATE_HASH    dword [rdi+5*sizeof(dword)], hF
   UPDATE_HASH    dword [rdi+6*sizeof(dword)], hG
   UPDATE_HASH    dword [rdi+7*sizeof(dword)], hH

   cmp   r14, MBS_SHA256*2
   jl    .done

   ;; do 64 rounds for the next block
   add      rsi, 4*sizeof(dword)          ; restore stack to next block W
   lea      rbp, [rsi+16*sizeof(ymmword)] ; use rbp for loop limiter

   mov      T5, hB   ; T5 = b^c
   xor      T3, T3   ; T3 = 0
   mov      T1, hF   ; T1 = f
   xor      T5, hC

align IPP_ALIGN_FACTOR
.block2_proc:
   SHA256_ROUND  0, hA,hB,hC,hD,hE,hF,hG,hH
   SHA256_ROUND  1, hH,hA,hB,hC,hD,hE,hF,hG
   SHA256_ROUND  2, hG,hH,hA,hB,hC,hD,hE,hF
   SHA256_ROUND  3, hF,hG,hH,hA,hB,hC,hD,hE
   SHA256_ROUND  4, hE,hF,hG,hH,hA,hB,hC,hD
   SHA256_ROUND  5, hD,hE,hF,hG,hH,hA,hB,hC
   SHA256_ROUND  6, hC,hD,hE,hF,hG,hH,hA,hB
   SHA256_ROUND  7, hB,hC,hD,hE,hF,hG,hH,hA
   add      rsi, 2*sizeof(ymmword)
   cmp      rsi, rbp
   jb       .block2_proc
   add      hA, T3

   mov   rdi, qword [rsp+_hash] ; restore hash pointer
   mov   r14, qword [rsp+_len]  ; restore data length

   ;; update hash values by 2-nd data block
   UPDATE_HASH    dword [rdi],   hA
   UPDATE_HASH    dword [rdi+1*sizeof(dword)], hB
   UPDATE_HASH    dword [rdi+2*sizeof(dword)], hC
   UPDATE_HASH    dword [rdi+3*sizeof(dword)], hD
   UPDATE_HASH    dword [rdi+4*sizeof(dword)], hE
   UPDATE_HASH    dword [rdi+5*sizeof(dword)], hF
   UPDATE_HASH    dword [rdi+6*sizeof(dword)], hG
   UPDATE_HASH    dword [rdi+7*sizeof(dword)], hH

   mov      rsi, qword [rsp+_block]   ; restore block addres
   add      rsi, MBS_SHA256*2                ; move data pointer
   sub      r14, MBS_SHA256*2                ; update data length
   mov      qword [rsp+_len], r14
   jg       .sha256_block2_loop

.done:
   mov   rsp, qword [rsp+_frame]
   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC UpdateSHA256

%endif    ;; _IPP32E_L9 and above
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA256_

