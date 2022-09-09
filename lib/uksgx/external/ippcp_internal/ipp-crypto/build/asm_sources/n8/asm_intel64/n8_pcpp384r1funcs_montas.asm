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
;               secp p384r1 specific implementation
;


%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

%assign _xEMULATION_  1
%assign _ADCX_ADOX_  1

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR

;; The p384r1 polynomial
Lpoly DQ 000000000ffffffffh,0ffffffff00000000h,0fffffffffffffffeh
      DQ 0ffffffffffffffffh,0ffffffffffffffffh,0ffffffffffffffffh

;; 2^(2*384) mod P precomputed for p384r1 polynomial
;LRR   DQ 0fffffffe00000001h,00000000200000000h,0fffffffe00000000h
;      DQ 00000000200000000h,00000000000000001h,00000000000000000h

LOne     DD    1,1,1,1,1,1,1,1
LTwo     DD    2,2,2,2,2,2,2,2
LThree   DD    3,3,3,3,3,3,3,3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p384r1_mul_by_2(uint64_t res[6], uint64_t a[6]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_mul_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine a0  rax
%xdefine a1  rcx
%xdefine a2  rdx
%xdefine a3  r8
%xdefine a4  r9
%xdefine a5  r10
%xdefine ex  r11

%xdefine t  r12

   xor   ex, ex

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]

   shld  ex, a5, 1
   shld  a5, a4, 1
   mov   qword [rdi+sizeof(qword)*5], a5
   shld  a4, a3, 1
   mov   qword [rdi+sizeof(qword)*4], a4
   shld  a3, a2, 1
   mov   qword [rdi+sizeof(qword)*3], a3
   shld  a2, a1, 1
   mov   qword [rdi+sizeof(qword)*2], a2
   shld  a1, a0, 1
   mov   qword [rdi+sizeof(qword)*1], a1
   shl   a0, 1
   mov   qword [rdi+sizeof(qword)*0], a0

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   ex, 0

   mov      t, qword [rdi+sizeof(qword)*0]
   cmovnz   a0, t
   mov      t, qword [rdi+sizeof(qword)*1]
   cmovnz   a1, t
   mov      t, qword [rdi+sizeof(qword)*2]
   cmovnz   a2, t
   mov      t, qword [rdi+sizeof(qword)*3]
   cmovnz   a3, t
   mov      t, qword [rdi+sizeof(qword)*4]
   cmovnz   a4, t
   mov      t, qword [rdi+sizeof(qword)*5]
   cmovnz   a5, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_mul_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p384r1_div_by_2(uint64_t res[6], uint64_t a[6]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_div_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine a0  rax
%xdefine a1  rcx
%xdefine a2  rdx
%xdefine a3  r8
%xdefine a4  r9
%xdefine a5  r10
%xdefine ex  r11

%xdefine t  r12

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]

   xor   t,  t
   xor   ex, ex
   add   a0, qword [rel Lpoly+sizeof(qword)*0]
   adc   a1, qword [rel Lpoly+sizeof(qword)*1]
   adc   a2, qword [rel Lpoly+sizeof(qword)*2]
   adc   a3, qword [rel Lpoly+sizeof(qword)*3]
   adc   a4, qword [rel Lpoly+sizeof(qword)*4]
   adc   a5, qword [rel Lpoly+sizeof(qword)*5]
   adc   ex, 0

   test  a0, 1
   cmovnz   ex, t
   mov      t,  qword [rsi+sizeof(qword)*0]
   cmovnz   a0, t
   mov      t,  qword [rsi+sizeof(qword)*1]
   cmovnz   a1, t
   mov      t,  qword [rsi+sizeof(qword)*2]
   cmovnz   a2, t
   mov      t,  qword [rsi+sizeof(qword)*3]
   cmovnz   a3, t
   mov      t,  qword [rsi+sizeof(qword)*4]
   cmovnz   a4, t
   mov      t,  qword [rsi+sizeof(qword)*5]
   cmovnz   a5, t

   shrd  a0, a1, 1
   shrd  a1, a2, 1
   shrd  a2, a3, 1
   shrd  a3, a4, 1
   shrd  a4, a5, 1
   shrd  a5, ex, 1

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_div_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p384r1_mul_by_3(uint64_t res[6], uint64_t a[6]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_mul_by_3,PUBLIC
%assign LOCAL_FRAME sizeof(qword)*6
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine a0  rax
%xdefine a1  rcx
%xdefine a2  rdx
%xdefine a3  r8
%xdefine a4  r9
%xdefine a5  r10
%xdefine ex  r11

%xdefine t  r12

   xor   ex, ex

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]

   shld  ex, a5, 1
   shld  a5, a4, 1
   mov   qword [rsp+sizeof(qword)*5], a5
   shld  a4, a3, 1
   mov   qword [rsp+sizeof(qword)*4], a4
   shld  a3, a2, 1
   mov   qword [rsp+sizeof(qword)*3], a3
   shld  a2, a1, 1
   mov   qword [rsp+sizeof(qword)*2], a2
   shld  a1, a0, 1
   mov   qword [rsp+sizeof(qword)*1], a1
   shl   a0, 1
   mov   qword [rsp+sizeof(qword)*0], a0

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   ex, 0

   mov      t, qword [rsp+0*sizeof(qword)]
   cmovb    a0, t
   mov      t, qword [rsp+1*sizeof(qword)]
   cmovb    a1, t
   mov      t, qword [rsp+2*sizeof(qword)]
   cmovb    a2, t
   mov      t, qword [rsp+3*sizeof(qword)]
   cmovb    a3, t
   mov      t, qword [rsp+4*sizeof(qword)]
   cmovb    a4, t
   mov      t, qword [rsp+5*sizeof(qword)]
   cmovb    a5, t

   xor   ex, ex
   add   a0, qword [rsi+sizeof(qword)*0]
   mov   qword [rsp+sizeof(qword)*0], a0
   adc   a1, qword [rsi+sizeof(qword)*1]
   mov   qword [rsp+sizeof(qword)*1], a1
   adc   a2, qword [rsi+sizeof(qword)*2]
   mov   qword [rsp+sizeof(qword)*2], a2
   adc   a3, qword [rsi+sizeof(qword)*3]
   mov   qword [rsp+sizeof(qword)*3], a3
   adc   a4, qword [rsi+sizeof(qword)*4]
   mov   qword [rsp+sizeof(qword)*4], a4
   adc   a5, qword [rsi+sizeof(qword)*5]
   mov   qword [rsp+sizeof(qword)*5], a5
   adc   ex, 0

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   ex, 0

   mov      t, qword [rsp+sizeof(qword)*0]
   cmovb    a0, t
   mov      t, qword [rsp+sizeof(qword)*1]
   cmovb    a1, t
   mov      t, qword [rsp+sizeof(qword)*2]
   cmovb    a2, t
   mov      t, qword [rsp+sizeof(qword)*3]
   cmovb    a3, t
   mov      t, qword [rsp+sizeof(qword)*4]
   cmovb    a4, t
   mov      t, qword [rsp+sizeof(qword)*5]
   cmovb    a5, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_mul_by_3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p384r1_add(uint64_t res[6], uint64_t a[6], uint64_t b[6]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_add,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12
        USES_XMM
        COMP_ABI 3

%xdefine a0  rax
%xdefine a1  rcx
%xdefine a2  rbx
%xdefine a3  r8
%xdefine a4  r9
%xdefine a5  r10
%xdefine ex  r11

%xdefine t  r12

   xor   ex,  ex

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]

   add   a0, qword [rdx+sizeof(qword)*0]
   adc   a1, qword [rdx+sizeof(qword)*1]
   adc   a2, qword [rdx+sizeof(qword)*2]
   adc   a3, qword [rdx+sizeof(qword)*3]
   adc   a4, qword [rdx+sizeof(qword)*4]
   adc   a5, qword [rdx+sizeof(qword)*5]
   adc   ex, 0

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   ex, 0

   mov   t, qword [rdi+sizeof(qword)*0]
   cmovb a0, t
   mov   t, qword [rdi+sizeof(qword)*1]
   cmovb a1, t
   mov   t, qword [rdi+sizeof(qword)*2]
   cmovb a2, t
   mov   t, qword [rdi+sizeof(qword)*3]
   cmovb a3, t
   mov   t, qword [rdi+sizeof(qword)*4]
   cmovb a4, t
   mov   t, qword [rdi+sizeof(qword)*5]
   cmovb a5, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_add

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p384r1_sub(uint64_t res[6], uint64_t a[6], uint64_t b[6]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_sub,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12
        USES_XMM
        COMP_ABI 3

%xdefine a0  rax
%xdefine a1  rcx
%xdefine a2  rbx
%xdefine a3  r8
%xdefine a4  r9
%xdefine a5  r10
%xdefine ex  r11

%xdefine t  r12

   xor   ex, ex

   mov   a0, qword [rsi+sizeof(qword)*0]  ; a
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]

   sub   a0, qword [rdx+sizeof(qword)*0]  ; a-b
   sbb   a1, qword [rdx+sizeof(qword)*1]
   sbb   a2, qword [rdx+sizeof(qword)*2]
   sbb   a3, qword [rdx+sizeof(qword)*3]
   sbb   a4, qword [rdx+sizeof(qword)*4]
   sbb   a5, qword [rdx+sizeof(qword)*5]

   sbb   ex, 0                               ; ex = a>=b? 0 : -1

   mov   qword [rdi+sizeof(qword)*0], a0  ; store (a-b)
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   add   a0, qword [rel Lpoly+sizeof(qword)*0] ; (a-b) +poly
   adc   a1, qword [rel Lpoly+sizeof(qword)*1]
   adc   a2, qword [rel Lpoly+sizeof(qword)*2]
   adc   a3, qword [rel Lpoly+sizeof(qword)*3]
   adc   a4, qword [rel Lpoly+sizeof(qword)*4]
   adc   a5, qword [rel Lpoly+sizeof(qword)*5]

   test  ex, ex                              ; r = (ex)? ((a-b)+poly) : (a-b)

   mov      t, qword [rdi+sizeof(qword)*0]
   cmovz    a0, t
   mov      t, qword [rdi+sizeof(qword)*1]
   cmovz    a1, t
   mov      t, qword [rdi+sizeof(qword)*2]
   cmovz    a2, t
   mov      t, qword [rdi+sizeof(qword)*3]
   cmovz    a3, t
   mov      t, qword [rdi+sizeof(qword)*4]
   cmovz    a4, t
   mov      t, qword [rdi+sizeof(qword)*5]
   cmovz    a5, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_sub

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p384r1_neg(uint64_t res[6], uint64_t a[6]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_neg,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine a0  rax
%xdefine a1  rcx
%xdefine a2  rdx
%xdefine a3  r8
%xdefine a4  r9
%xdefine a5  r10
%xdefine ex  r11

%xdefine t  r12

   xor   ex, ex

   xor   a0, a0
   xor   a1, a1
   xor   a2, a2
   xor   a3, a3
   xor   a4, a4
   xor   a5, a5

   sub   a0, qword [rsi+sizeof(qword)*0]
   sbb   a1, qword [rsi+sizeof(qword)*1]
   sbb   a2, qword [rsi+sizeof(qword)*2]
   sbb   a3, qword [rsi+sizeof(qword)*3]
   sbb   a4, qword [rsi+sizeof(qword)*4]
   sbb   a5, qword [rsi+sizeof(qword)*5]
   sbb   ex, 0

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   add   a0, qword [rel Lpoly+sizeof(qword)*0]
   adc   a1, qword [rel Lpoly+sizeof(qword)*1]
   adc   a2, qword [rel Lpoly+sizeof(qword)*2]
   adc   a3, qword [rel Lpoly+sizeof(qword)*3]
   adc   a4, qword [rel Lpoly+sizeof(qword)*4]
   adc   a5, qword [rel Lpoly+sizeof(qword)*5]
   test  ex, ex

   mov      t, qword [rdi+sizeof(qword)*0]
   cmovz    a0, t
   mov      t, qword [rdi+sizeof(qword)*1]
   cmovz    a1, t
   mov      t, qword [rdi+sizeof(qword)*2]
   cmovz    a2, t
   mov      t, qword [rdi+sizeof(qword)*3]
   cmovz    a3, t
   mov      t, qword [rdi+sizeof(qword)*4]
   cmovz    a4, t
   mov      t, qword [rdi+sizeof(qword)*5]
   cmovz    a5, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_neg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; projective point selector
;
; void p384r1_mred(Ipp464u* res, Ipp64u product);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro mred_step 9.nolist
  %xdefine %%a6 %1
  %xdefine %%a5 %2
  %xdefine %%a4 %3
  %xdefine %%a3 %4
  %xdefine %%a2 %5
  %xdefine %%a1 %6
  %xdefine %%a0 %7
  %xdefine %%t2 %8
  %xdefine %%t1 %9

   mov   rax, %%a0     ;; u = (m0*a0) mod 2^64= ((2^32+1)*a0) mod 2^64
   shl   rax, 32
   add   rax, %%a0

   mov   %%t2, rax     ;; (t2:t1) = u*2^32, store
   shr   %%t2, (64-32)
   push  %%t2
   mov   %%t1, rax
   shl   %%t1, 32
   push  %%t1

   sub   %%t1, rax     ;; {t2:t1} = (2^32 -1)*u
   sbb   %%t2, 0

   add   %%a0, %%t1      ;; {a0:a1} += {t2:t1}
   pop   %%t1          ;; restore {t2:t1} = u*2^32
   adc   %%a1, %%t2      ;; and accomodate carry
   pop   %%t2
   sbb   %%t2, 0

   sub   %%a1, %%t1      ;; {a1:a2} -= {t1:t2}
   mov   %%t1, dword 0
   sbb   %%a2, %%t2
   adc   %%t1, 0

   sub   %%a2, rax     ;; a2 -= u
   adc   %%t1, 0

   sub   %%a3, %%t1      ;; a3 -= borrow
   sbb   %%a4, 0       ;; a4 -= borrow
   sbb   %%a5, 0       ;; a5 -= borrow

   sbb   rax, 0
   add   rax, rdx
   mov   rdx, dword 0
   adc   rdx, 0
   add   %%a6, rax
   adc   rdx, 0
%endmacro

align IPP_ALIGN_FACTOR
IPPASM p384r1_mred,PUBLIC

        USES_GPR rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

;; rdi = result
;; rsi = product buffer

   xor   rdx, rdx
   mov   r8,  qword [rsi]
   mov   r9,  qword [rsi+sizeof(qword)]
   mov   r10, qword [rsi+sizeof(qword)*2]
   mov   r11, qword [rsi+sizeof(qword)*3]
   mov   r12, qword [rsi+sizeof(qword)*4]
   mov   r13, qword [rsi+sizeof(qword)*5]
   mov   r14, qword [rsi+sizeof(qword)*6]
   mred_step   r14,r13,r12,r11,r10,r9,r8, r15,rcx
  ;mov   qword [rdi+sizeof(qword)*0], r9
  ;mov   qword [rdi+sizeof(qword)*1], r10
  ;mov   qword [rdi+sizeof(qword)*2], r11
  ;mov   qword [rdi+sizeof(qword)*3], r12
  ;mov   qword [rdi+sizeof(qword)*4], r13
  ;mov   qword [rdi+sizeof(qword)*5], r14

   mov   r8, qword [rsi+sizeof(qword)*7]
   mred_step   r8,r14,r13,r12,r11,r10,r9, r15,rcx
  ;mov   qword [rdi+sizeof(qword)*0], r10
  ;mov   qword [rdi+sizeof(qword)*1], r11
  ;mov   qword [rdi+sizeof(qword)*2], r12
  ;mov   qword [rdi+sizeof(qword)*3], r13
  ;mov   qword [rdi+sizeof(qword)*4], r14
  ;mov   qword [rdi+sizeof(qword)*5], r8

   mov   r9, qword [rsi+sizeof(qword)*8]
   mred_step   r9,r8,r14,r13,r12,r11,r10, r15,rcx
  ;mov   qword [rdi+sizeof(qword)*0], r11
  ;mov   qword [rdi+sizeof(qword)*1], r12
  ;mov   qword [rdi+sizeof(qword)*2], r13
  ;mov   qword [rdi+sizeof(qword)*3], r14
  ;mov   qword [rdi+sizeof(qword)*4], r8
  ;mov   qword [rdi+sizeof(qword)*5], r9

   mov   r10, qword [rsi+sizeof(qword)*9]
   mred_step   r10,r9,r8,r14,r13,r12,r11, r15,rcx
  ;mov   qword [rdi+sizeof(qword)*0], r12
  ;mov   qword [rdi+sizeof(qword)*1], r13
  ;mov   qword [rdi+sizeof(qword)*2], r14
  ;mov   qword [rdi+sizeof(qword)*3], r8
  ;mov   qword [rdi+sizeof(qword)*4], r9
  ;mov   qword [rdi+sizeof(qword)*5], r10

   mov   r11, qword [rsi+sizeof(qword)*10]
   mred_step   r11,r10,r9,r8,r14,r13,r12, r15,rcx
  ;mov   qword [rdi+sizeof(qword)*0], r13
  ;mov   qword [rdi+sizeof(qword)*1], r14
  ;mov   qword [rdi+sizeof(qword)*2], r8
  ;mov   qword [rdi+sizeof(qword)*3], r9
  ;mov   qword [rdi+sizeof(qword)*4], r10
  ;mov   qword [rdi+sizeof(qword)*5], r11

   mov   r12, qword [rsi+sizeof(qword)*11]
   mred_step   r12,r11,r10,r9,r8,r14,r13, r15,rcx     ; {r12,r11,r10,r9,r8,r14} - result
   mov   qword [rdi+sizeof(qword)*0], r14
   mov   qword [rdi+sizeof(qword)*1], r8
   mov   qword [rdi+sizeof(qword)*2], r9
   mov   qword [rdi+sizeof(qword)*3], r10
   mov   qword [rdi+sizeof(qword)*4], r11
   mov   qword [rdi+sizeof(qword)*5], r12

   sub   r14, qword [rel Lpoly+sizeof(qword)*0]
   sbb   r8,  qword [rel Lpoly+sizeof(qword)*1]
   sbb   r9,  qword [rel Lpoly+sizeof(qword)*2]
   sbb   r10, qword [rel Lpoly+sizeof(qword)*3]
   sbb   r11, qword [rel Lpoly+sizeof(qword)*4]
   sbb   r12, qword [rel Lpoly+sizeof(qword)*5]
   sbb   rdx, 0

   mov      rax, qword [rdi+sizeof(qword)*0]
   cmovnz   r14, rax
   mov      rax, qword [rdi+sizeof(qword)*1]
   cmovnz   r8,  rax
   mov      rax, qword [rdi+sizeof(qword)*2]
   cmovnz   r9,  rax
   mov      rax, qword [rdi+sizeof(qword)*3]
   cmovnz   r10, rax
   mov      rax, qword [rdi+sizeof(qword)*4]
   cmovnz   r11, rax
   mov      rax, qword [rdi+sizeof(qword)*5]
   cmovnz   r12, rax

   mov   qword [rdi+sizeof(qword)*0], r14
   mov   qword [rdi+sizeof(qword)*1], r8
   mov   qword [rdi+sizeof(qword)*2], r9
   mov   qword [rdi+sizeof(qword)*3], r10
   mov   qword [rdi+sizeof(qword)*4], r11
   mov   qword [rdi+sizeof(qword)*5], r12

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_mred

%ifndef _DISABLE_ECP_384R1_HARDCODED_BP_TBL_
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; affine point selector
;
; void p384r1_select_ap_w5(AF_POINT *val, const AF_POINT *tbl, int idx);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_select_ap_w5,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14
        COMP_ABI 3

%xdefine val      rdi
%xdefine in_t     rsi
%xdefine idx      edx

%xdefine Xa       xmm0
%xdefine Xb       xmm1
%xdefine Xc       xmm2
%xdefine Ya       xmm3
%xdefine Yb       xmm4
%xdefine Yc       xmm5

%xdefine TXa    xmm6
%xdefine TXb    xmm7
%xdefine TXc    xmm8
%xdefine TYa    xmm9
%xdefine TYb    xmm10
%xdefine TYc    xmm11

%xdefine REQ_IDX  xmm12
%xdefine CUR_IDX  xmm13
%xdefine MASKDATA xmm14

   movdqa   CUR_IDX, oword [rel LOne]

   movd     REQ_IDX, idx
   pshufd   REQ_IDX, REQ_IDX, 0

   pxor     Xa, Xa
   pxor     Xb, Xb
   pxor     Xc, Xc
   pxor     Ya, Ya
   pxor     Yb, Yb
   pxor     Yc, Yc

   ; Skip index = 0, is implicictly infty -> load with offset -1
   mov      rcx, dword 16
.select_loop:
      movdqa   MASKDATA, CUR_IDX  ; MASK = CUR_IDX==REQ_IDX? 0xFF : 0x00
      pcmpeqd  MASKDATA, REQ_IDX  ;
      paddd    CUR_IDX, oword [rel LOne]

      movdqa   TXa, oword [in_t+sizeof(oword)*0]
      movdqa   TXb, oword [in_t+sizeof(oword)*1]
      movdqa   TXc, oword [in_t+sizeof(oword)*2]
      movdqa   TYa, oword [in_t+sizeof(oword)*3]
      movdqa   TYb, oword [in_t+sizeof(oword)*4]
      movdqa   TYc, oword [in_t+sizeof(oword)*5]
      add      in_t, sizeof(oword)*6

      pand     TXa, MASKDATA
      pand     TXb, MASKDATA
      pand     TXc, MASKDATA
      pand     TYa, MASKDATA
      pand     TYb, MASKDATA
      pand     TYc, MASKDATA

      por      Xa, TXa
      por      Xb, TXb
      por      Xc, TXc
      por      Ya, TYa
      por      Yb, TYb
      por      Yc, TYc

      dec      rcx
      jnz      .select_loop

   movdqu   oword [val+sizeof(oword)*0], Xa
   movdqu   oword [val+sizeof(oword)*1], Xb
   movdqu   oword [val+sizeof(oword)*2], Xc
   movdqu   oword [val+sizeof(oword)*3], Ya
   movdqu   oword [val+sizeof(oword)*4], Yb
   movdqu   oword [val+sizeof(oword)*5], Yc

   REST_XMM
   REST_GPR
   ret
ENDFUNC p384r1_select_ap_w5

%endif

%endif ;; _IPP32E_M7

