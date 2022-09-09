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
;               secp p224r1 specific implementation
;


%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

%assign _xEMULATION_  1
%assign _ADCX_ADOX_  1

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR

;; The p224r1 polynomial
Lpoly DQ 00000000000000001h,0ffffffff00000000h,0ffffffffffffffffh,000000000ffffffffh

;; mont(1)
;; ffffffff00000000 ffffffffffffffff 0000000000000000 0000000000000000

;; 2^(2*224) mod P precomputed for p224r1 polynomial
LRR   DQ 0ffffffff00000001h,0ffffffff00000000h,0fffffffe00000000h,000000000ffffffffh

LOne     DD    1,1,1,1,1,1,1,1
LTwo     DD    2,2,2,2,2,2,2,2
LThree   DD    3,3,3,3,3,3,3,3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_mul_by_2(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_mul_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12
%xdefine t4  r13

   xor   t4, t4

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]

   shld  t4, a3, 1
   shld  a3, a2, 1
   shld  a2, a1, 1
   shld  a1, a0, 1
   shl   a0, 1

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   t4, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2
   cmovz a3, t3

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_mul_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_div_by_2(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_div_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13,r14
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12
%xdefine t4  r13

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]

   xor   t4,  t4
   xor   r14, r14

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   add   t0, qword [rel Lpoly+sizeof(qword)*0]
   adc   t1, qword [rel Lpoly+sizeof(qword)*1]
   adc   t2, qword [rel Lpoly+sizeof(qword)*2]
   adc   t3, qword [rel Lpoly+sizeof(qword)*3]
   adc   t4, 0
   test  a0, 1

   cmovnz a0, t0
   cmovnz a1, t1
   cmovnz a2, t2
   cmovnz a3, t3
   cmovnz r14,t4

   shrd  a0, a1, 1
   shrd  a1, a2, 1
   shrd  a2, a3, 1
   shrd  a3, r14,1

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_div_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_mul_by_3(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_mul_by_3,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12
%xdefine t4  r13

   xor   t4, t4

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]

   shld  t4, a3, 1
   shld  a3, a2, 1
   shld  a2, a1, 1
   shld  a1, a0, 1
   shl   a0, 1

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   t4, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2
   cmovz a3, t3

   xor   t4, t4
   add   a0, qword [rsi+sizeof(qword)*0]
   adc   a1, qword [rsi+sizeof(qword)*1]
   adc   a2, qword [rsi+sizeof(qword)*2]
   adc   a3, qword [rsi+sizeof(qword)*3]
   adc   t4, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   t4, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2
   cmovz a3, t3

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_mul_by_3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_add(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_add,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 3

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12
%xdefine t4  r13

   xor   t4,  t4

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]

   add   a0, qword [rdx+sizeof(qword)*0]
   adc   a1, qword [rdx+sizeof(qword)*1]
   adc   a2, qword [rdx+sizeof(qword)*2]
   adc   a3, qword [rdx+sizeof(qword)*3]
   adc   t4, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   t4, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2
   cmovz a3, t3

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_add

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_sub(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_sub,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 3

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12
%xdefine t4  r13

   xor   t4,  t4

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]

   sub   a0, qword [rdx+sizeof(qword)*0]
   sbb   a1, qword [rdx+sizeof(qword)*1]
   sbb   a2, qword [rdx+sizeof(qword)*2]
   sbb   a3, qword [rdx+sizeof(qword)*3]
   sbb   t4, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   add   t0, qword [rel Lpoly+sizeof(qword)*0]
   adc   t1, qword [rel Lpoly+sizeof(qword)*1]
   adc   t2, qword [rel Lpoly+sizeof(qword)*2]
   adc   t3, qword [rel Lpoly+sizeof(qword)*3]
   test  t4, t4

   cmovnz a0, t0
   cmovnz a1, t1
   cmovnz a2, t2
   cmovnz a3, t3

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_sub

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_neg(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_neg,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12
%xdefine t4  r13

   xor   t4, t4

   xor   a0, a0
   xor   a1, a1
   xor   a2, a2
   xor   a3, a3

   sub   a0, qword [rsi+sizeof(qword)*0]
   sbb   a1, qword [rsi+sizeof(qword)*1]
   sbb   a2, qword [rsi+sizeof(qword)*2]
   sbb   a3, qword [rsi+sizeof(qword)*3]
   sbb   t4, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2
   mov   t3, a3

   add   t0, qword [rel Lpoly+sizeof(qword)*0]
   adc   t1, qword [rel Lpoly+sizeof(qword)*1]
   adc   t2, qword [rel Lpoly+sizeof(qword)*2]
   adc   t3, qword [rel Lpoly+sizeof(qword)*3]
   test  t4, t4

   cmovnz a0, t0
   cmovnz a1, t1
   cmovnz a2, t2
   cmovnz a3, t3

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_neg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_mul_montl(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
; void p224r1_mul_montx(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; was working on GFp functionality the problem (in reduction spep) has been found
;; 1) "sqr" impementation has been changed by "mul"
;; 2) fortunately "mont_back" stay as is because of operand zero extensioned

; on entry p5=0
; on exit  p0=0
;
%macro p224r1_prod_redstep 5.nolist
  %xdefine %%p4 %1
  %xdefine %%p3 %2
  %xdefine %%p2 %3
  %xdefine %%p1 %4
  %xdefine %%p0 %5

   neg   %%p0
   mov   t2, %%p0
   mov   t3, %%p0
   xor   t0, t0
   xor   t1, t1
   shr   t3, 32
   shl   t2, 32
   sub   t0, t2
   sbb   t1, t3
   sbb   t2, 0
   sbb   t3, 0

   neg   %%p0
   adc   %%p1, t0
   adc   %%p2, t1
   adc   %%p3, t2
   adc   %%p4, t3
%endmacro

align IPP_ALIGN_FACTOR
p224r1_mmull:

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12
%xdefine acc5  r13
%xdefine acc6  r14
%xdefine acc7  r15

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  rbp
%xdefine t4  rbx

;        rdi   assumed as result
%xdefine aPtr  rsi
%xdefine bPtr  rbx

   xor   acc5, acc5

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[0]
   mov   rax, qword [bPtr+sizeof(qword)*0]
   mul   qword [aPtr+sizeof(qword)*0]
   mov   acc0, rax
   mov   acc1, rdx

   mov   rax, qword [bPtr+sizeof(qword)*0]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc1, rax
   adc   rdx, 0
   mov   acc2, rdx

   mov   rax, qword [bPtr+sizeof(qword)*0]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc2, rax
   adc   rdx, 0
   mov   acc3, rdx

   mov   rax, qword [bPtr+sizeof(qword)*0]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc3, rax
   adc   rdx, 0
   mov   acc4, rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 0
   p224r1_prod_redstep  acc4,acc3,acc2,acc1,acc0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[1]
   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*0]
   add   acc1, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc2, rcx
   adc   rdx, 0
   add   acc2, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc3, rcx
   adc   rdx, 0
   add   acc3, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc4, rcx
   adc   rdx, 0
   add   acc4, rax
   adc   rdx, 0
   mov   acc5, rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 1
   p224r1_prod_redstep  acc5,acc4,acc3,acc2,acc1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[2]
   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*0]
   add   acc2, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc3, rcx
   adc   rdx, 0
   add   acc3, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc4, rcx
   adc   rdx, 0
   add   acc4, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc5, rcx
   adc   rdx, 0
   add   acc5, rax
   adc   rdx, 0
   mov   acc6, rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 2
   p224r1_prod_redstep  acc6,acc5,acc4,acc3,acc2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[3]
   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*0]
   add   acc3, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc4, rcx
   adc   rdx, 0
   add   acc4, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc5, rcx
   adc   rdx, 0
   add   acc5, rax
   adc   rdx, 0
   mov   rcx, rdx

   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc6, rcx
   adc   rdx, 0
   add   acc6, rax
   adc   rdx, 0
   mov   acc7, rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 3 (final)
   p224r1_prod_redstep  acc7,acc6,acc5,acc4,acc3
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   mov   t0, qword [rel Lpoly+sizeof(qword)*0]
   mov   t1, qword [rel Lpoly+sizeof(qword)*1]
   mov   t2, qword [rel Lpoly+sizeof(qword)*2]
   mov   t3, qword [rel Lpoly+sizeof(qword)*3]

   mov   acc0, acc4     ;; copy reducted result
   mov   acc1, acc5
   mov   acc2, acc6
   mov   acc3, acc7

   sub   acc4, t0       ;; test %if it exceeds prime value
   sbb   acc5, t1
   sbb   acc6, t2
   sbb   acc7, t3

   cmovc  acc4, acc0
   cmovc  acc5, acc1
   cmovc  acc6, acc2
   cmovc  acc7, acc3

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc6
   mov   qword [rdi+sizeof(qword)*3], acc7

   ret

%if _IPP32E >= _IPP32E_L9
align IPP_ALIGN_FACTOR
p224r1_mmulx:

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12
%xdefine acc5  r13
%xdefine acc6  r14
%xdefine acc7  r15

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  rbp
%xdefine t4  rbx

;        rdi   assumed as result
%xdefine aPtr  rsi
%xdefine bPtr  rbx

   xor   acc5, acc5

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[0]
   mov   rdx, qword [bPtr+sizeof(qword)*0]
   mulx  acc1,acc0, qword [aPtr+sizeof(qword)*0]
   mulx  acc2,t2,   qword [aPtr+sizeof(qword)*1]
   add   acc1,t2
   mulx  acc3,t2,   qword [aPtr+sizeof(qword)*2]
   adc   acc2,t2
   mulx  acc4,t2,   qword [aPtr+sizeof(qword)*3]
   adc   acc3,t2
   adc   acc4,0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 0
   p224r1_prod_redstep  acc4,acc3,acc2,acc1,acc0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[1]
   mov   rdx,    qword [bPtr+sizeof(qword)*1]
   xor   t0, t0

   mulx  t3, t2, qword [aPtr+sizeof(qword)*0]
   adcx  acc1, t2
   adox  acc2, t3

   mulx  t3, t2, qword [aPtr+sizeof(qword)*1]
   adcx  acc2, t2
   adox  acc3, t3

   mulx  t3, t2, qword [aPtr+sizeof(qword)*2]
   adcx  acc3, t2
   adox  acc4, t3

   mulx  acc5, t2, qword [aPtr+sizeof(qword)*3]
   adcx  acc4, t2
   adox  acc5, t0
   adc   acc5, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 1
   p224r1_prod_redstep  acc5,acc4,acc3,acc2,acc1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[2]
   mov   rdx,    qword [bPtr+sizeof(qword)*2]
   xor   t0, t0

   mulx  t3, t2, qword [aPtr+sizeof(qword)*0]
   adcx  acc2, t2
   adox  acc3, t3

   mulx  t3, t2, qword [aPtr+sizeof(qword)*1]
   adcx  acc3, t2
   adox  acc4, t3

   mulx  t3, t2, qword [aPtr+sizeof(qword)*2]
   adcx  acc4, t2
   adox  acc5, t3

   mulx  acc6, t2, qword [aPtr+sizeof(qword)*3]
   adcx  acc5, t2
   adox  acc6, t0
   adc   acc6, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 2
   p224r1_prod_redstep  acc6,acc5,acc4,acc3,acc2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[3]
   mov   rdx,    qword [bPtr+sizeof(qword)*3]
   xor   t0, t0

   mulx  t3, t2, qword [aPtr+sizeof(qword)*0]
   adcx  acc3, t2
   adox  acc4, t3

   mulx  t3, t2, qword [aPtr+sizeof(qword)*1]
   adcx  acc4, t2
   adox  acc5, t3

   mulx  t3, t2, qword [aPtr+sizeof(qword)*2]
   adcx  acc5, t2
   adox  acc6, t3

   mulx  acc7, t2, qword [aPtr+sizeof(qword)*3]
   adcx  acc6, t2
   adox  acc7, t0
   adc   acc7, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 3 (final)
   p224r1_prod_redstep  acc7,acc6,acc5,acc4,acc3
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   mov   t0, qword [rel Lpoly+sizeof(qword)*0]
   mov   t1, qword [rel Lpoly+sizeof(qword)*1]
   mov   t2, qword [rel Lpoly+sizeof(qword)*2]
   mov   t3, qword [rel Lpoly+sizeof(qword)*3]

   mov   acc0, acc4     ;; copy reducted result
   mov   acc1, acc5
   mov   acc2, acc6
   mov   acc3, acc7

   sub   acc4, t0       ;; test %if it exceeds prime value
   sbb   acc5, t1
   sbb   acc6, t2
   sbb   acc7, t3

   cmovc  acc4, acc0
   cmovc  acc5, acc1
   cmovc  acc6, acc2
   cmovc  acc7, acc3

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc6
   mov   qword [rdi+sizeof(qword)*3], acc7

   ret
%endif

IPPASM p224r1_mul_montl,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

%xdefine bPtr  rbx

   mov   bPtr, rdx
   call  p224r1_mmull

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_mul_montl

%if _IPP32E >= _IPP32E_L9
align IPP_ALIGN_FACTOR
IPPASM p224r1_mul_montx,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

%xdefine bPtr  rbx

   mov   bPtr, rdx
   call  p224r1_mmulx

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_mul_montx

%endif ;; _IPP32E_L9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_to_mont(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_to_mont,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

   lea   rbx, [rel LRR]
   call  p224r1_mmull
   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_to_mont

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_sqr_montl(uint64_t res[4], uint64_t a[4]);
; void p224r1_sqr_montx(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_sqr_montl,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

   mov   rbx, rsi
   call  p224r1_mmull

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_sqr_montl

%if _IPP32E >= _IPP32E_L9
align IPP_ALIGN_FACTOR
IPPASM p224r1_sqr_montx,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

   mov   rbx, rsi
   call  p224r1_mmulx

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_sqr_montx

%endif ;; _IPP32E_L9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_mont_back(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_mont_back,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12
%xdefine acc5  r13
%xdefine acc6  r14
%xdefine acc7  r15

%xdefine t0    rax
%xdefine t1    rdx
%xdefine t2    rcx
%xdefine t3    rsi

   mov   acc0, qword [rsi+sizeof(qword)*0]
   mov   acc1, qword [rsi+sizeof(qword)*1]
   mov   acc2, qword [rsi+sizeof(qword)*2]
   mov   acc3, qword [rsi+sizeof(qword)*3]
   xor   acc4, acc4
   xor   acc5, acc5
   xor   acc6, acc6
   xor   acc7, acc7

   p224r1_prod_redstep acc4,acc3,acc2,acc1,acc0
   p224r1_prod_redstep acc5,acc4,acc3,acc2,acc1
   p224r1_prod_redstep acc6,acc5,acc4,acc3,acc2
   p224r1_prod_redstep acc7,acc6,acc5,acc4,acc3

   mov   acc0, acc4
   mov   acc1, acc5
   mov   acc2, acc6
   mov   acc3, acc7

   sub   acc4, qword [rel Lpoly+sizeof(qword)*0]
   sbb   acc5, qword [rel Lpoly+sizeof(qword)*1]
   sbb   acc6, qword [rel Lpoly+sizeof(qword)*2]
   sbb   acc7, qword [rel Lpoly+sizeof(qword)*3]

   cmovc acc4, acc0
   cmovc acc5, acc1
   cmovc acc6, acc2
   cmovc acc7, acc3

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc6
   mov   qword [rdi+sizeof(qword)*3], acc7

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_mont_back

%ifndef _DISABLE_ECP_224R1_HARDCODED_BP_TBL_
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p224r1_select_ap_w7(AF_POINT *val, const AF_POINT *in_t, int index);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p224r1_select_ap_w7,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
        COMP_ABI 3

%xdefine val    rdi
%xdefine in_t    rsi
%xdefine idx    edx

%xdefine ONE    xmm0
%xdefine INDEX    xmm1

%xdefine Ra    xmm2
%xdefine Rb    xmm3
%xdefine Rc    xmm4
%xdefine Rd    xmm5

%xdefine M0  xmm8
%xdefine T0a  xmm9
%xdefine T0b  xmm10
%xdefine T0c  xmm11
%xdefine T0d  xmm12
%xdefine TMP0  xmm15

   movdqa   ONE, oword [rel LOne]

   pxor     Ra, Ra
   pxor     Rb, Rb
   pxor     Rc, Rc
   pxor     Rd, Rd

   movdqa   M0, ONE

   movd     INDEX, idx
   pshufd   INDEX, INDEX, 0

   ; Skip index = 0, is implicictly infty -> load with offset -1
   mov      rcx, dword 64
.select_loop_sse_w7:
      movdqa   TMP0, M0
      pcmpeqd  TMP0, INDEX
      paddd    M0, ONE

      movdqa   T0a, oword [in_t+sizeof(oword)*0]
      movdqa   T0b, oword [in_t+sizeof(oword)*1]
      movdqa   T0c, oword [in_t+sizeof(oword)*2]
      movdqa   T0d, oword [in_t+sizeof(oword)*3]
      add      in_t, sizeof(oword)*4

      pand     T0a, TMP0
      pand     T0b, TMP0
      pand     T0c, TMP0
      pand     T0d, TMP0

      por      Ra, T0a
      por      Rb, T0b
      por      Rc, T0c
      por      Rd, T0d
      dec      rcx
      jnz      .select_loop_sse_w7

   movdqu   oword [val+sizeof(oword)*0], Ra
   movdqu   oword [val+sizeof(oword)*1], Rb
   movdqu   oword [val+sizeof(oword)*2], Rc
   movdqu   oword [val+sizeof(oword)*3], Rd

   REST_XMM
   REST_GPR
   ret
ENDFUNC p224r1_select_ap_w7

%endif

%endif

