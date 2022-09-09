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
;               Message block processing according to SHA512
;
;     Content:
;        UpdateSHA512
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "ia_32e_regs.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA512_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_L9 )

;;
;; assignments
;;
%xdefine hA  rax  ;; hash values into GPU registers
%xdefine hB  rbx
%xdefine hC  rcx
%xdefine hD  rdx
%xdefine hE  r8
%xdefine hF  r9
%xdefine hG  r10
%xdefine hH  r11

%xdefine T1  r12  ;; scratch
%xdefine T2  r13
%xdefine T3  r14
%xdefine T4  r15
%xdefine T5  rdi

%xdefine W0  ymm0 ;; W values into YMM registers
%xdefine W1  ymm1
%xdefine W2  ymm2
%xdefine W3  ymm3
%xdefine W4  ymm4
%xdefine W5  ymm5
%xdefine W6  ymm6
%xdefine W7  ymm7

%xdefine yT1 ymm8 ;; scratch
%xdefine yT2 ymm9
%xdefine yT3 ymm10
%xdefine yT4 ymm11

%xdefine W0L xmm0
%xdefine W1L xmm1
%xdefine W2L xmm2
%xdefine W3L xmm3
%xdefine W4L xmm4
%xdefine W5L xmm5
%xdefine W6L xmm6
%xdefine W7L xmm7

%xdefine YMM_SHUFB_BSWAP  ymm12  ;; byte swap constant

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; textual rotation of W args
;;
%macro ROTATE_W 0.nolist
  %xdefine %%_X   W0
  %xdefine W0   W1
  %xdefine W1   W2
  %xdefine W2   W3
  %xdefine W3   W4
  %xdefine W4   W5
  %xdefine W5   W6
  %xdefine W6   W7
  %xdefine W7   %%_X
%endmacro

;;
;; textual rotation of HASH arguments
;;
%macro ROTATE_H 0.nolist
  %xdefine %%_X   hH
  %xdefine hH  hG
  %xdefine hG  hF
  %xdefine hF  hE
  %xdefine hE  hD
  %xdefine hD  hC
  %xdefine hC  hB
  %xdefine hB  hA
  %xdefine hA  %%_X
 %endmacro

%macro ROTATE_T4_T5 0.nolist
  %xdefine %%T T4
  %xdefine T4 T5
  %xdefine T5 %%T
%endmacro

;;
;; update next 2 W[t] and W[t+1], t=16,...79
;;
;; W[j] = sig1(W[t- 2]) + W[t- 7]
;;       +sig0(W[t-15]) + W[t-16]
;;
;; sig0(x) = ROR(x,1) ^ROR(x,8)  ^SHR(x,7)
;;         = SHR(x,7)
;;          ^SHR(x,1) ^SHL(x,63)
;;          ^SHR(x,8) ^SHL(x,56)
;; sig1(x) = ROR(x,19)^ROR(x,61) ^SHR(x,6)
;;         = SHR(x,6)
;;          ^SHR(x,19) ^SHL(x,45)
;;          ^SHR(x,61) ^SHL(x,3)
;;
%macro UPDATE_W 9.nolist
  %xdefine %%nr %1
  %xdefine %%W0 %2
  %xdefine %%W1 %3
  %xdefine %%W2 %4
  %xdefine %%W3 %5
  %xdefine %%W4 %6
  %xdefine %%W5 %7
  %xdefine %%W6 %8
  %xdefine %%W7 %9

  %assign %%W_AHEAD  16

   vpalignr yT1,%%W1,%%W0,sizeof(qword)    ;; T1 = {w2:w1} args sig0()
   vpalignr yT4,%%W5,%%W4,sizeof(qword)    ;; T4 = {w10:w9} additive
   vpsrlq   yT3,yT1,1            ;; sig0(): T3 = {w2:w1}>>1
   vpaddq   %%W0,%%W0,yT4            ;; W0 += {w10:w9}
   vpsrlq   yT4,yT1,7            ;; sig0(): {w2:w1}>>7
   vpsllq   yT2,yT1,56           ;; sig0(): {w2:w1}<<(64-8)
   vpxor    yT1,yT4,yT3          ;; sig0(): {w2:w1}>>7 ^ {w2:w1}>>1
   vpsrlq   yT3,yT3, 7           ;; sig0(): {w2:w1}>>1>>7 = {w2:w1}>>8
   vpxor    yT1,yT1,yT2          ;; sig0(): {w2:w1}>>7 ^ {w2:w1}>>1 ^ {w2:w1}<<(64-8)
   vpsllq   yT2,yT2, 7           ;; sig0(): {w2:w1}<<(64-8)<<7 = {w2:w1}<<63
   vpxor    yT1,yT1,yT3          ;; sig0(): {w2:w1}>>7 ^ {w2:w1}>>1 ^ {w2:w1}<<(64-8) ^ {w2:w1}>>8
   vpsrlq   yT4,%%W7, 6            ;; sig1(): {w15:w14}>>6
   vpxor    yT1,yT1,yT2          ;; sig0()= {w2:w1}>>7
                                 ;;        ^{w2:w1}>>1 ^ {w2:w1}<<63
                                 ;;        ^{w2:w1}<<(64-8) ^ {w2:w1}>>8
   vpsllq   yT3,%%W7, 3            ;; sig1(): {w15:w14}<<3
   vpaddq   %%W0,%%W0,yT1            ;; W0 += sig0()
   vpsrlq   yT2,%%W7, 19           ;; sig1(): {w15:w14}>>19
   vpxor    yT4,yT4,yT3          ;; sig1(): {w15:w14}>>6 ^ {w15:w14}<<3
   vpsllq   yT3,yT3, 42          ;; sig1(): {w15:w14}<<3<<42 = {w15:w14}<<45
   vpxor    yT4,yT4,yT2          ;; sig1(): {w15:w14}>>6 ^ {w15:w14}<<3 ^ {w15:w14}>>19
   vpsrlq   yT2,yT2,42           ;; sig1(): {w15:w14}>>19>>42= {w15:w14}>>61
   vpxor    yT4,yT4,yT3          ;; sig1(): {w15:w14}>>6 ^ {w15:w14}<<3 ^ {w15:w14}>>19 ^ {w15:w14}<<45
   vpxor    yT4,yT4,yT2          ;; sig1()= {w15:w14}>>6
                                 ;;        ^{w15:w14}<<3 ^ {w15:w14}>>61
                                 ;;        ^{w15:w14}>>19 ^ {w15:w14}<<45
   vpaddq   %%W0,%%W0,yT4            ;; W0 += sig1()
   vpaddq   yT3,%%W0,YMMWORD [rbp+(%%nr/2)*sizeof(ymmword)]
   vmovdqa  YMMWORD [rsi+(%%W_AHEAD/2)*sizeof(ymmword)+(%%nr/2)*sizeof(ymmword)],yT3
   ROTATE_W
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
;; sum1(e) = (e>>>14)^(e>>>18)^(e>>>41)
;; sum0(a) = (a>>>28)^(a>>>34)^(a>>>39)
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
%macro SHA512_ROUND 9.nolist
  %xdefine %%nr %1
  %xdefine %%hA %2
  %xdefine %%hB %3
  %xdefine %%hC %4
  %xdefine %%hD %5
  %xdefine %%hE %6
  %xdefine %%hF %7
  %xdefine %%hG %8
  %xdefine %%hH %9

   add   %%hH, qword [rsi+(%%nr/2)*sizeof(ymmword)+(%%nr & 1)*sizeof(qword)] ;; h += (k[t]+w[t])
   and   T1, %%hE       ;; ch(e,f,g): (f&e)
   rorx  T2, %%hE, 41   ;; sum1(e): e>>>41
   rorx  T4, %%hE, 18   ;; sum1(e): e>>>18
   add   %%hA, T3       ;; complete computation a += sum0(a[t-1])
   add   %%hH, T1       ;; h += (k[t]+w[t]) + (f&e)
   andn  T1, %%hE, %%hG   ;; ch(e,f,g): (~e&g)
   xor   T2, T4       ;; sum1(e): (e>>>41)^(e>>>18)
   rorx  T3, %%hE, 14   ;; sum1(e): e>>>14
   add   %%hH, T1       ;; h += (k[t]+w[t]) + (f&e) + (~e&g)
   xor   T2, T3       ;; sum1(e) = (e>>>41)^(e>>>18)^(e>>>14)
   mov   T4, %%hA       ;; maj(a,b,c): a
   rorx  T1, %%hA, 39   ;; sum0(a): a>>>39
   add   %%hH, T2       ;; h += (k[t]+w[t]) +(f&e) +(~e&g) +sig1(e)
   xor   T4, %%hB       ;; maj(a,b,c): (a^b)
   rorx  T3, %%hA, 34   ;; sum0(a): a>>>34
   rorx  T2, %%hA, 28   ;; sum0(a): a>>>28
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
;; does 2 regular rounds and computes next 2 W values
;; (just 2 instances of SHA512_ROUND merged together woth UPDATE_W)
;;
%macro SHA512_2ROUND_SHED 1.nolist
  %xdefine %%round %1

  %assign %%W_AHEAD  16
  %assign %%nr  %%round

vpalignr yT1,W1,W0,sizeof(qword)
   add   hH, qword [rsi+(%%nr/2)*sizeof(ymmword)+(%%nr & 1)*sizeof(qword)]
   and   T1, hE
vpalignr yT4,W5,W4,sizeof(qword)
   rorx  T2, hE, 41
   rorx  T4, hE, 18
vpsrlq   yT3,yT1,1
   add   hA, T3
   add   hH, T1
vpaddq   W0,W0,yT4
   andn  T1, hE, hG
   xor   T2, T4
vpsrlq   yT4,yT1,7
   rorx  T3, hE, 14
   add   hH, T1
vpsllq   yT2,yT1,56
   xor   T2, T3
   mov   T4, hA
vpxor    yT1,yT4,yT3
   rorx  T1, hA, 39
   add   hH, T2
vpsrlq   yT3,yT3, 7
   xor   T4, hB
   rorx  T3, hA, 34
vpxor    yT1,yT1,yT2
   rorx  T2, hA, 28
   add   hD, hH
vpsllq   yT2,yT2, 7
   and   T5, T4
   xor   T3, T1
vpxor    yT1,yT1,yT3
   xor   T5, hB
   xor   T3, T2
vpsrlq   yT4,W7, 6
   add   hH, T5
   mov   T1, hE
vpxor    yT1,yT1,yT2
   ROTATE_T4_T5
   ROTATE_H

  %assign %%nr  %%nr+1
   add   hH, qword [rsi+(%%nr/2)*sizeof(ymmword)+(%%nr & 1)*sizeof(qword)]
   and   T1, hE
vpsllq   yT3,W7, 3
   rorx  T2, hE, 41
   rorx  T4, hE, 18
vpaddq   W0,W0,yT1
   add   hA, T3
   add   hH, T1
vpsrlq   yT2,W7, 19
   andn  T1, hE, hG
   xor   T2, T4
vpxor    yT4,yT4,yT3
   rorx  T3, hE, 14
   add   hH, T1
vpsllq   yT3,yT3, 42
   xor   T2, T3
   mov   T4, hA
vpxor    yT4,yT4,yT2
   rorx  T1, hA, 39
   add   hH, T2
vpsrlq   yT2,yT2,42
   xor   T4, hB
   rorx  T3, hA, 34
vpxor    yT4,yT4,yT3
   rorx  T2, hA, 28
   add   hD, hH
vpxor    yT4,yT4,yT2
   and   T5, T4
   xor   T3, T1
vpaddq   W0,W0,yT4
   xor   T5, hB
   xor   T3, T2
vpaddq   yT3,W0,YMMWORD [rbp+(%%nr/2)*sizeof(ymmword)]
   add   hH, T5
   mov   T1, hE
vmovdqa  YMMWORD [rsi+(%%W_AHEAD/2)*sizeof(ymmword)+(%%nr/2)*sizeof(ymmword)],yT3
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

SHA512_YMM_K   dq 0428a2f98d728ae22h, 07137449123ef65cdh, 0428a2f98d728ae22h, 07137449123ef65cdh
               dq 0b5c0fbcfec4d3b2fh, 0e9b5dba58189dbbch, 0b5c0fbcfec4d3b2fh, 0e9b5dba58189dbbch
               dq 03956c25bf348b538h, 059f111f1b605d019h, 03956c25bf348b538h, 059f111f1b605d019h
               dq 0923f82a4af194f9bh, 0ab1c5ed5da6d8118h, 0923f82a4af194f9bh, 0ab1c5ed5da6d8118h
               dq 0d807aa98a3030242h, 012835b0145706fbeh, 0d807aa98a3030242h, 012835b0145706fbeh
               dq 0243185be4ee4b28ch, 0550c7dc3d5ffb4e2h, 0243185be4ee4b28ch, 0550c7dc3d5ffb4e2h
               dq 072be5d74f27b896fh, 080deb1fe3b1696b1h, 072be5d74f27b896fh, 080deb1fe3b1696b1h
               dq 09bdc06a725c71235h, 0c19bf174cf692694h, 09bdc06a725c71235h, 0c19bf174cf692694h
               dq 0e49b69c19ef14ad2h, 0efbe4786384f25e3h, 0e49b69c19ef14ad2h, 0efbe4786384f25e3h
               dq 00fc19dc68b8cd5b5h, 0240ca1cc77ac9c65h, 00fc19dc68b8cd5b5h, 0240ca1cc77ac9c65h
               dq 02de92c6f592b0275h, 04a7484aa6ea6e483h, 02de92c6f592b0275h, 04a7484aa6ea6e483h
               dq 05cb0a9dcbd41fbd4h, 076f988da831153b5h, 05cb0a9dcbd41fbd4h, 076f988da831153b5h
               dq 0983e5152ee66dfabh, 0a831c66d2db43210h, 0983e5152ee66dfabh, 0a831c66d2db43210h
               dq 0b00327c898fb213fh, 0bf597fc7beef0ee4h, 0b00327c898fb213fh, 0bf597fc7beef0ee4h
               dq 0c6e00bf33da88fc2h, 0d5a79147930aa725h, 0c6e00bf33da88fc2h, 0d5a79147930aa725h
               dq 006ca6351e003826fh, 0142929670a0e6e70h, 006ca6351e003826fh, 0142929670a0e6e70h
               dq 027b70a8546d22ffch, 02e1b21385c26c926h, 027b70a8546d22ffch, 02e1b21385c26c926h
               dq 04d2c6dfc5ac42aedh, 053380d139d95b3dfh, 04d2c6dfc5ac42aedh, 053380d139d95b3dfh
               dq 0650a73548baf63deh, 0766a0abb3c77b2a8h, 0650a73548baf63deh, 0766a0abb3c77b2a8h
               dq 081c2c92e47edaee6h, 092722c851482353bh, 081c2c92e47edaee6h, 092722c851482353bh
               dq 0a2bfe8a14cf10364h, 0a81a664bbc423001h, 0a2bfe8a14cf10364h, 0a81a664bbc423001h
               dq 0c24b8b70d0f89791h, 0c76c51a30654be30h, 0c24b8b70d0f89791h, 0c76c51a30654be30h
               dq 0d192e819d6ef5218h, 0d69906245565a910h, 0d192e819d6ef5218h, 0d69906245565a910h
               dq 0f40e35855771202ah, 0106aa07032bbd1b8h, 0f40e35855771202ah, 0106aa07032bbd1b8h
               dq 019a4c116b8d2d0c8h, 01e376c085141ab53h, 019a4c116b8d2d0c8h, 01e376c085141ab53h
               dq 02748774cdf8eeb99h, 034b0bcb5e19b48a8h, 02748774cdf8eeb99h, 034b0bcb5e19b48a8h
               dq 0391c0cb3c5c95a63h, 04ed8aa4ae3418acbh, 0391c0cb3c5c95a63h, 04ed8aa4ae3418acbh
               dq 05b9cca4f7763e373h, 0682e6ff3d6b2b8a3h, 05b9cca4f7763e373h, 0682e6ff3d6b2b8a3h
               dq 0748f82ee5defb2fch, 078a5636f43172f60h, 0748f82ee5defb2fch, 078a5636f43172f60h
               dq 084c87814a1f0ab72h, 08cc702081a6439ech, 084c87814a1f0ab72h, 08cc702081a6439ech
               dq 090befffa23631e28h, 0a4506cebde82bde9h, 090befffa23631e28h, 0a4506cebde82bde9h
               dq 0bef9a3f7b2c67915h, 0c67178f2e372532bh, 0bef9a3f7b2c67915h, 0c67178f2e372532bh
               dq 0ca273eceea26619ch, 0d186b8c721c0c207h, 0ca273eceea26619ch, 0d186b8c721c0c207h
               dq 0eada7dd6cde0eb1eh, 0f57d4f7fee6ed178h, 0eada7dd6cde0eb1eh, 0f57d4f7fee6ed178h
               dq 006f067aa72176fbah, 00a637dc5a2c898a6h, 006f067aa72176fbah, 00a637dc5a2c898a6h
               dq 0113f9804bef90daeh, 01b710b35131c471bh, 0113f9804bef90daeh, 01b710b35131c471bh
               dq 028db77f523047d84h, 032caab7b40c72493h, 028db77f523047d84h, 032caab7b40c72493h
               dq 03c9ebe0a15c9bebch, 0431d67c49c100d4ch, 03c9ebe0a15c9bebch, 0431d67c49c100d4ch
               dq 04cc5d4becb3e42b6h, 0597f299cfc657e2ah, 04cc5d4becb3e42b6h, 0597f299cfc657e2ah
               dq 05fcb6fab3ad6faech, 06c44198c4a475817h, 05fcb6fab3ad6faech, 06c44198c4a475817h

SHA512_YMM_BF  dq 00001020304050607h, 008090a0b0c0d0e0fh, 00001020304050607h, 008090a0b0c0d0e0fh


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; UpdateSHA512(Ipp64u digest[], Ipp8u dataBlock[], int datalen, Ipp64u K_512[])
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA512,PUBLIC
%assign LOCAL_FRAME (sizeof(qword)*4 + sizeof(qword)*80*2)
        USES_GPR rbx,rsi,rdi,rbp,rbx,r12,r13,r14,r15
        USES_XMM_AVX xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12
        COMP_ABI 4
;;
;; rdi = pointer to the updated hash
;; rsi = pointer to the data block
;; rdx = data block length
;; rcx = pointer to the SHA_512 constant (ignored)
;;

%xdefine MBS_SHA512    (128)

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

   vmovdqa  YMM_SHUFB_BSWAP, ymmword [rel SHA512_YMM_BF]   ; load byte shuffler

   mov      hA, qword [rdi]       ; load initial hash value
   mov      hB, qword [rdi+1*sizeof(qword)]
   mov      hC, qword [rdi+2*sizeof(qword)]
   mov      hD, qword [rdi+3*sizeof(qword)]
   mov      hE, qword [rdi+4*sizeof(qword)]
   mov      hF, qword [rdi+5*sizeof(qword)]
   mov      hG, qword [rdi+6*sizeof(qword)]
   mov      hH, qword [rdi+7*sizeof(qword)]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data 2 block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
.sha512_block2_loop:
   lea      r12, [rsi+MBS_SHA512]      ; next block

   cmp      r14, MBS_SHA512            ; %if single block processed
   cmovbe   r12, rsi                   ; use the same data block address

   lea      rbp, [rel SHA512_YMM_K]    ; to SHA512 consts

   vmovdqu  W0L, xmmword [rsi]           ; load data block
   vmovdqu  W1L, xmmword [rsi+1*sizeof(xmmword)]
   vmovdqu  W2L, xmmword [rsi+2*sizeof(xmmword)]
   vmovdqu  W3L, xmmword [rsi+3*sizeof(xmmword)]
   vmovdqu  W4L, xmmword [rsi+4*sizeof(xmmword)]
   vmovdqu  W5L, xmmword [rsi+5*sizeof(xmmword)]
   vmovdqu  W6L, xmmword [rsi+6*sizeof(xmmword)]
   vmovdqu  W7L, xmmword [rsi+7*sizeof(xmmword)]

   vinserti128 W0, W0, xmmword [r12], 1   ; merge next data block
   vinserti128 W1, W1, xmmword [r12+1*sizeof(xmmword)], 1
   vinserti128 W2, W2, xmmword [r12+2*sizeof(xmmword)], 1
   vinserti128 W3, W3, xmmword [r12+3*sizeof(xmmword)], 1
   vinserti128 W4, W4, xmmword [r12+4*sizeof(xmmword)], 1
   vinserti128 W5, W5, xmmword [r12+5*sizeof(xmmword)], 1
   vinserti128 W6, W6, xmmword [r12+6*sizeof(xmmword)], 1
   vinserti128 W7, W7, xmmword [r12+7*sizeof(xmmword)], 1

   vpshufb  W0, W0, YMM_SHUFB_BSWAP
   vpshufb  W1, W1, YMM_SHUFB_BSWAP
   vpshufb  W2, W2, YMM_SHUFB_BSWAP
   vpshufb  W3, W3, YMM_SHUFB_BSWAP
   vpshufb  W4, W4, YMM_SHUFB_BSWAP
   vpshufb  W5, W5, YMM_SHUFB_BSWAP
   vpshufb  W6, W6, YMM_SHUFB_BSWAP
   vpshufb  W7, W7, YMM_SHUFB_BSWAP

   vpaddq   yT1, W0, ymmword [rbp]
   vmovdqa  ymmword [rsp+_dataW], yT1
   vpaddq   yT2, W1, ymmword [rbp+1*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+1*sizeof(ymmword)], yT2
   vpaddq   yT3, W2, ymmword [rbp+2*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+2*sizeof(ymmword)], yT3
   vpaddq   yT4, W3, ymmword [rbp+3*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+3*sizeof(ymmword)], yT4

   vpaddq   yT1, W4, ymmword [rbp+4*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+4*sizeof(ymmword)], yT1
   vpaddq   yT2, W5, ymmword [rbp+5*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+5*sizeof(ymmword)], yT2
   vpaddq   yT3, W6, ymmword [rbp+6*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+6*sizeof(ymmword)], yT3
   vpaddq   yT4, W7, ymmword [rbp+7*sizeof(ymmword)]
   vmovdqa  ymmword [rsp+_dataW+7*sizeof(ymmword)], yT4

   add      rbp, 8*sizeof(ymmword)

   mov      T5, hB   ; T5 = b^c
   xor      T3, T3   ; T3 = 0
   mov      T1, hF   ; T1 = f
   xor      T5, hC

   mov   qword [rsp+_block], rsi   ; store block addres
   lea   rsi, [rsp+_dataW]         ; use rsi as stack pointer

align IPP_ALIGN_FACTOR
.block1_shed_proc:
   SHA512_2ROUND_SHED   0
   SHA512_2ROUND_SHED   2
   SHA512_2ROUND_SHED   4
   SHA512_2ROUND_SHED   6
   SHA512_2ROUND_SHED   8
   SHA512_2ROUND_SHED  10
   SHA512_2ROUND_SHED  12
   SHA512_2ROUND_SHED  14

   add      rsi, 8*sizeof(ymmword)
   add      rbp, 8*sizeof(ymmword)

   ;; and repeat
   cmp      dword [rbp-sizeof(qword)],04a475817h
   jne      .block1_shed_proc

   ;; the rest 16 rounds
   SHA512_ROUND  0, hA,hB,hC,hD,hE,hF,hG,hH
   SHA512_ROUND  1, hH,hA,hB,hC,hD,hE,hF,hG
   SHA512_ROUND  2, hG,hH,hA,hB,hC,hD,hE,hF
   SHA512_ROUND  3, hF,hG,hH,hA,hB,hC,hD,hE
   SHA512_ROUND  4, hE,hF,hG,hH,hA,hB,hC,hD
   SHA512_ROUND  5, hD,hE,hF,hG,hH,hA,hB,hC
   SHA512_ROUND  6, hC,hD,hE,hF,hG,hH,hA,hB
   SHA512_ROUND  7, hB,hC,hD,hE,hF,hG,hH,hA
   SHA512_ROUND  8, hA,hB,hC,hD,hE,hF,hG,hH
   SHA512_ROUND  9, hH,hA,hB,hC,hD,hE,hF,hG
   SHA512_ROUND 10, hG,hH,hA,hB,hC,hD,hE,hF
   SHA512_ROUND 11, hF,hG,hH,hA,hB,hC,hD,hE
   SHA512_ROUND 12, hE,hF,hG,hH,hA,hB,hC,hD
   SHA512_ROUND 13, hD,hE,hF,hG,hH,hA,hB,hC
   SHA512_ROUND 14, hC,hD,hE,hF,hG,hH,hA,hB
   SHA512_ROUND 15, hB,hC,hD,hE,hF,hG,hH,hA
   add          hA, T3

   sub      rsi, ((80-16)/2)*sizeof(ymmword)  ; restore stack to W

   mov   rdi, qword [rsi+_hash-_dataW] ; restore hash pointer
   mov   r14, qword [rsi+_len-_dataW]  ; restore data length

   ;; update hash values by 1-st data block
   UPDATE_HASH    qword [rdi],   hA
   UPDATE_HASH    qword [rdi+1*sizeof(qword)], hB
   UPDATE_HASH    qword [rdi+2*sizeof(qword)], hC
   UPDATE_HASH    qword [rdi+3*sizeof(qword)], hD
   UPDATE_HASH    qword [rdi+4*sizeof(qword)], hE
   UPDATE_HASH    qword [rdi+5*sizeof(qword)], hF
   UPDATE_HASH    qword [rdi+6*sizeof(qword)], hG
   UPDATE_HASH    qword [rdi+7*sizeof(qword)], hH

   cmp   r14, MBS_SHA512*2
   jl    .done

   ;; do 80 rounds for the next block
   add      rsi, 2*sizeof(qword)          ; restore stack to next block W
   lea      rbp, [rsi+40*sizeof(ymmword)] ; use rbp for loop limiter

   mov      T5, hB   ; T5 = b^c
   xor      T3, T3   ; T3 = 0
   mov      T1, hF   ; T1 = f
   xor      T5, hC

align IPP_ALIGN_FACTOR
.block2_proc:
   SHA512_ROUND  0, hA,hB,hC,hD,hE,hF,hG,hH
   SHA512_ROUND  1, hH,hA,hB,hC,hD,hE,hF,hG
   SHA512_ROUND  2, hG,hH,hA,hB,hC,hD,hE,hF
   SHA512_ROUND  3, hF,hG,hH,hA,hB,hC,hD,hE
   SHA512_ROUND  4, hE,hF,hG,hH,hA,hB,hC,hD
   SHA512_ROUND  5, hD,hE,hF,hG,hH,hA,hB,hC
   SHA512_ROUND  6, hC,hD,hE,hF,hG,hH,hA,hB
   SHA512_ROUND  7, hB,hC,hD,hE,hF,hG,hH,hA
   add      rsi, 4*sizeof(ymmword)
   cmp      rsi, rbp
   jb       .block2_proc
   add      hA, T3

   mov   rdi, qword [rsp+_hash] ; restore hash pointer
   mov   r14, qword [rsp+_len]  ; restore data length

   ;; update hash values by 1-st data block
   UPDATE_HASH    qword [rdi],   hA
   UPDATE_HASH    qword [rdi+1*sizeof(qword)], hB
   UPDATE_HASH    qword [rdi+2*sizeof(qword)], hC
   UPDATE_HASH    qword [rdi+3*sizeof(qword)], hD
   UPDATE_HASH    qword [rdi+4*sizeof(qword)], hE
   UPDATE_HASH    qword [rdi+5*sizeof(qword)], hF
   UPDATE_HASH    qword [rdi+6*sizeof(qword)], hG
   UPDATE_HASH    qword [rdi+7*sizeof(qword)], hH

   mov      rsi, qword [rsp+_block]   ; restore block addres
   add      rsi, MBS_SHA512*2                ; move data pointer
   sub      r14, MBS_SHA512*2                ; update data length
   mov      qword [rsp+_len], r14
   jg       .sha512_block2_loop

.done:
   mov   rsp, qword [rsp+_frame]
   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC UpdateSHA512

%endif    ;; _IPP32E_L9 and above
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA512_

