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
;               secp p192r1 specific implementation
;


%include "asmdefs.inc"
%include "ia_32e.inc"

%if _IPP32E >= _IPP32E_M7

%assign _xEMULATION_  1
%assign _ADCX_ADOX_  1

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR

;; The p192r1 polynomial
Lpoly DQ 0FFFFFFFFFFFFFFFFh,0FFFFFFFFFFFFFFFEh,0FFFFFFFFFFFFFFFFh

;; 2^(192*2) mod P precomputed for p192r1 polynomial
LRR   DQ 00000000000000001h,00000000000000002h,00000000000000001h

LOne     DD    1,1,1,1,1,1,1,1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_mul_by_2(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12

   xor   t3, t3

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]

   shld  t3, a2, 1
   shld  a2, a1, 1
   shld  a1, a0, 1
   shl   a0, 1

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_mul_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_div_by_2(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_div_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]

   xor   t3,  t3
   xor   r13, r13

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   add   t0, qword [rel Lpoly+sizeof(qword)*0]
   adc   t1, qword [rel Lpoly+sizeof(qword)*1]
   adc   t2, qword [rel Lpoly+sizeof(qword)*2]
   adc   t3, 0
   test  a0, 1

   cmovnz a0, t0
   cmovnz a1, t1
   cmovnz a2, t2
   cmovnz r13,t3

   shrd  a0, a1, 1
   shrd  a1, a2, 1
   shrd  a2, r13,1

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_div_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_mul_by_3(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_by_3,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12

   xor   t3, t3

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]

   shld  t3, a2, 1
   shld  a2, a1, 1
   shld  a1, a0, 1
   shl   a0, 1

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2

   xor   t3, t3
   add   a0, qword [rsi+sizeof(qword)*0]
   adc   a1, qword [rsi+sizeof(qword)*1]
   adc   a2, qword [rsi+sizeof(qword)*2]
   adc   t3, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, 0

   cmovz   a0, t0
   cmovz   a1, t1
   cmovz   a2, t2

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_mul_by_3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_add(uint64_t res[3], uint64_t a[3], uint64_t b[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_add,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 3

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12

   xor   t3,  t3

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]

   add   a0, qword [rdx+sizeof(qword)*0]
   adc   a1, qword [rdx+sizeof(qword)*1]
   adc   a2, qword [rdx+sizeof(qword)*2]
   adc   t3, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, 0

   cmovz a0, t0
   cmovz a1, t1
   cmovz a2, t2

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_add

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_sub(uint64_t res[3], uint64_t a[3], uint64_t b[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_sub,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 3

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12

   xor   t3,  t3

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]

   sub   a0, qword [rdx+sizeof(qword)*0]
   sbb   a1, qword [rdx+sizeof(qword)*1]
   sbb   a2, qword [rdx+sizeof(qword)*2]
   sbb   t3, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   add   t0, qword [rel Lpoly+sizeof(qword)*0]
   adc   t1, qword [rel Lpoly+sizeof(qword)*1]
   adc   t2, qword [rel Lpoly+sizeof(qword)*2]
   test  t3, t3

   cmovnz a0, t0
   cmovnz a1, t1
   cmovnz a2, t2

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_sub

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_neg(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_neg,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx
%xdefine t3  r12

   xor   t3, t3

   xor   a0, a0
   xor   a1, a1
   xor   a2, a2

   sub   a0, qword [rsi+sizeof(qword)*0]
   sbb   a1, qword [rsi+sizeof(qword)*1]
   sbb   a2, qword [rsi+sizeof(qword)*2]
   sbb   t3, 0

   mov   t0, a0
   mov   t1, a1
   mov   t2, a2

   add   t0, qword [rel Lpoly+sizeof(qword)*0]
   adc   t1, qword [rel Lpoly+sizeof(qword)*1]
   adc   t2, qword [rel Lpoly+sizeof(qword)*2]
   test  t3, t3

   cmovnz a0, t0
   cmovnz a1, t1
   cmovnz a2, t2

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_neg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_mul_montl(uint64_t res[3], uint64_t a[3], uint64_t b[3]);
; void p192r1_mul_montx(uint64_t res[3], uint64_t a[3], uint64_t b[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; product = {p3,p2,p1,p0}, p4=0
; product += 2^192*p0 -2^64*p0 -p0
;
; on entry p4=0
; on exit  p0=0
;
%macro p192r1_mul_redstep 5.nolist
  %xdefine %%p4 %1
  %xdefine %%p3 %2
  %xdefine %%p2 %3
  %xdefine %%p1 %4
  %xdefine %%p0 %5

   sub   %%p1, %%p0
   sbb   %%p2, 0
   sbb   %%p0, 0
   add   %%p3, %%p0
   adc   %%p4, 0
   xor   %%p0, %%p0
%endmacro

align IPP_ALIGN_FACTOR
p192r1_mmull:

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx

;        rdi   assumed as result
%xdefine aPtr  rsi
%xdefine bPtr  rbx

   xor   acc4, acc4

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

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 0
   p192r1_mul_redstep acc4,acc3,acc2,acc1,acc0

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
   adc   acc4, rdx
   adc   acc0, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 1
   p192r1_mul_redstep acc0,acc4,acc3,acc2,acc1

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
   adc   acc0, rdx
   adc   acc1, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 2 (final)
   p192r1_mul_redstep acc1,acc0,acc4,acc3,acc2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t0, acc3     ;; copy reducted result
   mov   t1, acc4
   mov   t2, acc0

   sub   t0, qword [rel Lpoly+sizeof(qword)*0] ;; test %if it exceeds prime value
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   acc1, 0

   cmovnc acc3, t0
   cmovnc acc4, t1
   cmovnc acc0, t2

   mov   qword [rdi+sizeof(qword)*0], acc3
   mov   qword [rdi+sizeof(qword)*1], acc4
   mov   qword [rdi+sizeof(qword)*2], acc0

   ret

%if (_IPP32E >= _IPP32E_L9)
align IPP_ALIGN_FACTOR
p192r1_mmulx:

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx

;        rdi   assumed as result
%xdefine aPtr  rsi
%xdefine bPtr  rbx

   xor   acc4, acc4

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[0]
   xor   rdx, rdx
   mov   rdx, qword [bPtr+sizeof(qword)*0]
   mulx  acc1,acc0, qword [aPtr+sizeof(qword)*0]
   mulx  acc2,t2,   qword [aPtr+sizeof(qword)*1]
   add   acc1,t2
   mulx  acc3,t2,   qword [aPtr+sizeof(qword)*2]
   adc   acc2,t2
   adc   acc3,0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 0
   p192r1_mul_redstep acc4,acc3,acc2,acc1,acc0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[1]
   mov   rdx,    qword [bPtr+sizeof(qword)*1]

   mulx  t0, t2, qword [aPtr+sizeof(qword)*0]
   adcx  acc1, t2
   adox  acc2, t0

   mulx  t0, t2, qword [aPtr+sizeof(qword)*1]
   adcx  acc2, t2
   adox  acc3, t0

   mulx  t0, t2, qword [aPtr+sizeof(qword)*2]
   adcx  acc3, t2
   adox  acc4, t0

   adcx  acc4, acc0
   adox  acc0, acc0
   adc   acc0, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 1
   p192r1_mul_redstep acc0,acc4,acc3,acc2,acc1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[2]
   mov   rdx,    qword [bPtr+sizeof(qword)*2]

   mulx  t0, t2, qword [aPtr+sizeof(qword)*0]
   adcx  acc2, t2
   adox  acc3, t0

   mulx  t0, t2, qword [aPtr+sizeof(qword)*1]
   adcx  acc3, t2
   adox  acc4, t0

   mulx  t0, t2, qword [aPtr+sizeof(qword)*2]
   adcx  acc4, t2
   adox  acc0, t0

   adcx  acc0, acc1
   adox  acc1, acc1
   adc   acc1, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 2 (final)
   p192r1_mul_redstep acc1,acc0,acc4,acc3,acc2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t0, acc3     ;; copy reducted result
   mov   t1, acc4
   mov   t2, acc0

   sub   t0, qword [rel Lpoly+sizeof(qword)*0] ;; test %if it exceeds prime value
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   acc1, 0

   cmovnc acc3, t0
   cmovnc acc4, t1
   cmovnc acc0, t2

   mov   qword [rdi+sizeof(qword)*0], acc3
   mov   qword [rdi+sizeof(qword)*1], acc4
   mov   qword [rdi+sizeof(qword)*2], acc0

   ret
%endif

align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_montl,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12
        USES_XMM
        COMP_ABI 3

%xdefine bPtr  rbx

   mov   bPtr, rdx
   call  p192r1_mmull

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_mul_montl

%if _IPP32E >= _IPP32E_L9
align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_montx,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12
        USES_XMM
        COMP_ABI 3

%xdefine bPtr  rbx

   mov   bPtr, rdx
   call  p192r1_mmulx

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_mul_montx

%endif ;; _IPP32E_L9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_to_mont(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_to_mont,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

   lea   rbx, [rel LRR]
   call  p192r1_mmull
   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_to_mont

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_sqr_montl(uint64_t res[3], uint64_t a[3]);
; void p192r1_sqr_montx(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; on entry e = expasion (previous step)
; on exit  p0= expasion (next step)
;
%macro p192r1_prod_redstep 5.nolist
  %xdefine %%e %1
  %xdefine %%p3 %2
  %xdefine %%p2 %3
  %xdefine %%p1 %4
  %xdefine %%p0 %5

   sub   %%p1, %%p0
   sbb   %%p2, 0
   sbb   %%p0, 0
   add   %%p3, %%p0
   mov   %%p0, dword 0
   adc   %%p0, 0

   %ifnempty %%e
   add   %%p3, %%e
   adc   %%p0, 0
   %endif
%endmacro

align IPP_ALIGN_FACTOR
IPPASM p192r1_sqr_montl,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12
%xdefine acc5  r13

%xdefine t0  rax
%xdefine t1  rdx
%xdefine t2  rcx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t2, qword [aPtr+sizeof(qword)*0]
   mov   rax,qword [aPtr+sizeof(qword)*1]
   mul   t2
   mov   acc1, rax
   mov   acc2, rdx
   mov   rax,qword [aPtr+sizeof(qword)*2]
   mul   t2
   add   acc2, rax
   adc   rdx, 0
   mov   acc3, rdx
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t2, qword [aPtr+sizeof(qword)*1]
   mov   rax,qword [aPtr+sizeof(qword)*2]
   mul   t2
   add   acc3, rax
   adc   rdx, 0
   mov   acc4, rdx

   xor   acc5, acc5
   shld  acc5, acc4, 1
   shld  acc4, acc3, 1
   shld  acc3, acc2, 1
   shld  acc2, acc1, 1
   shl   acc1, 1

   mov   rax,qword [aPtr+sizeof(qword)*0]
   mul   rax
   mov   acc0, rax
   add   acc1, rdx
   adc   acc2, 0
   mov   rax,qword [aPtr+sizeof(qword)*1]
   mul   rax
   add   acc2, rax
   adc   acc3, rdx
   adc   acc4, 0
   mov   rax,qword [aPtr+sizeof(qword)*2]
   mul   rax
   add   acc4, rax
   adc   acc5, rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   p192r1_prod_redstep      ,acc3,acc2,acc1,acc0
   p192r1_prod_redstep  acc0,acc4,acc3,acc2,acc1
   p192r1_prod_redstep  acc1,acc5,acc4,acc3,acc2
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   mov   t0, acc3
   mov   t1, acc4
   mov   t2, acc5

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   acc2, 0

   cmovnc acc3, t0
   cmovnc acc4, t1
   cmovnc acc5, t2

   mov   qword [rdi+sizeof(qword)*0], acc3
   mov   qword [rdi+sizeof(qword)*1], acc4
   mov   qword [rdi+sizeof(qword)*2], acc5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_sqr_montl

%if _IPP32E >= _IPP32E_L9
align IPP_ALIGN_FACTOR
IPPASM p192r1_sqr_montx,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13
        USES_XMM
        COMP_ABI 2

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12
%xdefine acc5  r13

%xdefine t0  rcx
%xdefine t1  rax
%xdefine t2  rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   rdx,  qword [aPtr+sizeof(qword)*0]
   mulx  acc2, acc1, qword [aPtr+sizeof(qword)*1]
   mulx  acc3, t0,   qword [aPtr+sizeof(qword)*2]
   add   acc2, t0
   adc   acc3, 0
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   rdx, qword [aPtr+sizeof(qword)*1]
   mulx  acc4, t0, qword [aPtr+sizeof(qword)*2]
   add   acc3, t0
   adc   acc4, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   xor   acc5, acc5
   shld  acc5, acc4, 1
   shld  acc4, acc3, 1
   shld  acc3, acc2, 1
   shld  acc2, acc1, 1
   shl   acc1, 1

   xor   acc0, acc0
   mov   rdx, qword [aPtr+sizeof(qword)*0]
   mulx  t1, acc0, rdx
   adcx  acc1, t1
   mov   rdx, qword [aPtr+sizeof(qword)*1]
   mulx  t1, t0, rdx
   adcx  acc2, t0
   adcx  acc3, t1
   mov   rdx, qword [aPtr+sizeof(qword)*2]
   mulx  t1, t0, rdx
   adcx  acc4, t0
   adcx  acc5, t1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   p192r1_prod_redstep      ,acc3,acc2,acc1,acc0
   p192r1_prod_redstep  acc0,acc4,acc3,acc2,acc1
   p192r1_prod_redstep  acc1,acc5,acc4,acc3,acc2
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   mov   t0, acc3
   mov   t1, acc4
   mov   t2, acc5

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   acc2, 0

   cmovnc acc3, t0
   cmovnc acc4, t1
   cmovnc acc5, t2

   mov   qword [rdi+sizeof(qword)*0], acc3
   mov   qword [rdi+sizeof(qword)*1], acc4
   mov   qword [rdi+sizeof(qword)*2], acc5

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_sqr_montx

%endif ;; _IPP32E_L9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_mont_back(uint64_t res[3], uint64_t a[3]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mont_back,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12
        USES_XMM
        COMP_ABI 2

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12

%xdefine t0    rax
%xdefine t1    rdx
%xdefine t2    rcx
%xdefine t3    rsi

   mov   acc2, qword [rsi+sizeof(qword)*0]
   mov   acc3, qword [rsi+sizeof(qword)*1]
   mov   acc4, qword [rsi+sizeof(qword)*2]
   xor   acc0, acc0
   xor   acc1, acc1

   p192r1_mul_redstep acc1,acc0,acc4,acc3,acc2
   p192r1_mul_redstep acc2,acc1,acc0,acc4,acc3
   p192r1_mul_redstep acc3,acc2,acc1,acc0,acc4

   mov   t0, acc0
   mov   t1, acc1
   mov   t2, acc2

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   acc4, 0

   cmovnc acc0, t0
   cmovnc acc1, t1
   cmovnc acc2, t2

   mov   qword [rdi+sizeof(qword)*0], acc0
   mov   qword [rdi+sizeof(qword)*1], acc1
   mov   qword [rdi+sizeof(qword)*2], acc2

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_mont_back


%ifndef _DISABLE_ECP_192R1_HARDCODED_BP_TBL_
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p192r1_select_ap_w7(AF_POINT *val, const AF_POINT *in_t, int index);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_select_ap_w7,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi
        USES_XMM xmm6,xmm7,xmm8,xmm9
        COMP_ABI 3

%xdefine val   rdi
%xdefine in_t  rsi
%xdefine idx   edx

%xdefine ONE   xmm0
%xdefine INDEX xmm1

%xdefine Ra    xmm2
%xdefine Rb    xmm3
%xdefine Rc    xmm4

%xdefine T0a   xmm5
%xdefine T0b   xmm6
%xdefine T0c   xmm7

%xdefine M0    xmm8
%xdefine TMP0  xmm9

   movdqa   ONE, oword [rel LOne]

   pxor     Ra, Ra
   pxor     Rb, Rb
   pxor     Rc, Rc

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
      add      in_t, sizeof(oword)*3

      pand     T0a, TMP0
      pand     T0b, TMP0
      pand     T0c, TMP0

      por      Ra, T0a
      por      Rb, T0b
      por      Rc, T0c
      dec      rcx
      jnz      .select_loop_sse_w7

   movdqu   oword [val+sizeof(oword)*0], Ra
   movdqu   oword [val+sizeof(oword)*1], Rb
   movdqu   oword [val+sizeof(oword)*2], Rc

   REST_XMM
   REST_GPR
   ret
ENDFUNC p192r1_select_ap_w7

%endif

%endif ;; _IPP32E_M7

