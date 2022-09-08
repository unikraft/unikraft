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
;               SM2 specific implementation
;

%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

%assign _xEMULATION_ 1
%assign _ADCX_ADOX_  1

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR

;; The SM2 polynomial
Lpoly DQ 0FFFFFFFFFFFFFFFFh,0FFFFFFFF00000000h,0FFFFFFFFFFFFFFFFh,0FFFFFFFEFFFFFFFFh

;; 2^512 mod P precomputed for SM2 polynomial
LRR   DQ 00000000200000003h,000000002ffffffffh,00000000100000001h,00000000400000002h

LOne     DD    1,1,1,1,1,1,1,1
LTwo     DD    2,2,2,2,2,2,2,2
LThree   DD    3,3,3,3,3,3,3,3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_mul_by_2(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_mul_by_2,PUBLIC
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
ENDFUNC sm2_mul_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_div_by_2(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_div_by_2,PUBLIC
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
ENDFUNC sm2_div_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_mul_by_3(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_mul_by_3,PUBLIC
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
ENDFUNC sm2_mul_by_3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_add(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_add,PUBLIC
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
ENDFUNC sm2_add

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_sub(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_sub,PUBLIC
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
ENDFUNC sm2_sub

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_neg(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_neg,PUBLIC
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
ENDFUNC sm2_neg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_mul_montl(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; on entry p5=0
; on exit  p0=0, p5=0/1
;
%macro sm2_mul_redstep 6.nolist
  %xdefine %%p5 %1
  %xdefine %%p4 %2
  %xdefine %%p3 %3
  %xdefine %%p2 %4
  %xdefine %%p1 %5
  %xdefine %%p0 %6

   mov   acc6, %%p0   ;; (u1:u0) = p0*2^32
   shl   acc6, 32
   mov   acc7, %%p0
   shr   acc7, 32

   mov   t0, %%p0
   mov   t4, %%p0
   xor   t1, t1
   xor   t3, t3

   sub   t0, acc6 ;; (t3:t2:t1:t0) = p0*2^256 -p0*2^224 +p0*2^64 -p0*2^96
   sbb   t1, acc7
   sbb   t3, acc6
   sbb   t4, acc7

   add   %%p1, t0   ;; (p5:p4:p3:p2:p1) -= (t3:t2:t1:t0)
   adc   %%p2, t1
   adc   %%p3, t3
   adc   %%p4, t4
   adc   %%p5, 0    ;; p5 extension
   xor   %%p0, %%p0   ;; p0 = 0;
%endmacro

align IPP_ALIGN_FACTOR
sm2_mmull:

%xdefine acc0  r8
%xdefine acc1  r9
%xdefine acc2  r10
%xdefine acc3  r11
%xdefine acc4  r12
%xdefine acc5  r13
%xdefine acc6  r14
%xdefine acc7  r15

%xdefine t0  rcx
%xdefine t1  rbp
%xdefine t2  rbx
%xdefine t3  rdx
%xdefine t4  rax
%xdefine t5  r14

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
   ;; 1-st reduction step
   sm2_mul_redstep acc5,acc4,acc3,acc2,acc1,acc0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[1]
   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*0]
   add   acc1, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc2, t0
   adc   rdx, 0
   add   acc2, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc3, t0
   adc   rdx, 0
   add   acc3, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*1]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc4, t0
   adc   rdx, 0
   add   acc4, rax
   adc   acc5, rdx
   adc   acc0, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; 2-ndt reduction step
   sm2_mul_redstep acc0,acc5,acc4,acc3,acc2,acc1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[2]
   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*0]
   add   acc2, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc3, t0
   adc   rdx, 0
   add   acc3, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc4, t0
   adc   rdx, 0
   add   acc4, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*2]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc5, t0
   adc   rdx, 0
   add   acc5, rax
   adc   acc0, rdx
   adc   acc1, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; 3-rd reduction step
   sm2_mul_redstep acc1,acc0,acc5,acc4,acc3,acc2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[3]
   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*0]
   add   acc3, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*1]
   add   acc4, t0
   adc   rdx, 0
   add   acc4, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*2]
   add   acc5, t0
   adc   rdx, 0
   add   acc5, rax
   adc   rdx, 0
   mov   t0, rdx

   mov   rax, qword [bPtr+sizeof(qword)*3]
   mul   qword [aPtr+sizeof(qword)*3]
   add   acc0, t0
   adc   rdx, 0
   add   acc0, rax
   adc   acc1, rdx
   adc   acc2, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; final reduction step
   sm2_mul_redstep acc2,acc1,acc0,acc5,acc4,acc3

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t0, qword [rel Lpoly+sizeof(qword)*0]
   mov   t1, qword [rel Lpoly+sizeof(qword)*1]
   mov   t2, qword [rel Lpoly+sizeof(qword)*2]
   mov   t3, qword [rel Lpoly+sizeof(qword)*3]

   mov   t4, acc4
   mov   acc3, acc5
   mov   acc6, acc0
   mov   acc7, acc1

   sub   t4,   t0       ;; test %if it exceeds prime value
   sbb   acc3, t1
   sbb   acc6, t2
   sbb   acc7, t3
   sbb   acc2, 0

   cmovnc acc4, t4
   cmovnc acc5, acc3
   cmovnc acc0, acc6
   cmovnc acc1, acc7

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc0
   mov   qword [rdi+sizeof(qword)*3], acc1

   ret

align IPP_ALIGN_FACTOR
IPPASM sm2_mul_montl,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

%xdefine bPtr  rbx

   mov   bPtr, rdx
   call  sm2_mmull

   REST_XMM
   REST_GPR
   ret
ENDFUNC sm2_mul_montl

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_to_mont(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_to_mont,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

   lea   rbx, [rel LRR]
   call  sm2_mmull
   REST_XMM
   REST_GPR
   ret
ENDFUNC sm2_to_mont

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_mul_montx(uint64_t res[4], uint64_t a[4], uint64_t b[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%if (_IPP32E >= _IPP32E_L9)
align IPP_ALIGN_FACTOR
sm2_mmulx:

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
%xdefine t3  rcx
%xdefine t4  rbp
%xdefine t2  rbx

;        rdi   assumed as result
%xdefine aPtr  rsi
%xdefine bPtr  rbx

   xor   acc5, acc5

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[0]
   xor   rdx, rdx
   mov   rdx, qword [bPtr+sizeof(qword)*0]
   mulx  acc1,acc0, qword [aPtr+sizeof(qword)*0]
   mulx  acc2,t3,   qword [aPtr+sizeof(qword)*1]
   add   acc1,t3
   mulx  acc3,t3,   qword [aPtr+sizeof(qword)*2]
   adc   acc2,t3
   mulx  acc4,t3,   qword [aPtr+sizeof(qword)*3]
   adc   acc3,t3
   adc   acc4,0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 0
   sm2_mul_redstep acc5,acc4,acc3,acc2,acc1,acc0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[1]
   mov   rdx,    qword [bPtr+sizeof(qword)*1]

   mulx  t4, t3, qword [aPtr+sizeof(qword)*0]
   adcx  acc1, t3
   adox  acc2, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*1]
   adcx  acc2, t3
   adox  acc3, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*2]
   adcx  acc3, t3
   adox  acc4, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*3]
   adcx  acc4, t3
   adox  acc5, t4

   adcx  acc5, acc0
   adox  acc0, acc0
   adc   acc0, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 1
   sm2_mul_redstep acc0,acc5,acc4,acc3,acc2,acc1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[2]
   mov   rdx,    qword [bPtr+sizeof(qword)*2]

   mulx  t4, t3, qword [aPtr+sizeof(qword)*0]
   adcx  acc2, t3
   adox  acc3, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*1]
   adcx  acc3, t3
   adox  acc4, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*2]
   adcx  acc4, t3
   adox  acc5, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*3]
   adcx  acc5, t3
   adox  acc0, t4

   adcx  acc0, acc1
   adox  acc1, acc1

   adc   acc1, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 2
   sm2_mul_redstep acc1,acc0,acc5,acc4,acc3,acc2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; * b[3]
   mov   rdx,    qword [bPtr+sizeof(qword)*3]

   mulx  t4, t3, qword [aPtr+sizeof(qword)*0]
   adcx  acc3, t3
   adox  acc4, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*1]
   adcx  acc4, t3
   adox  acc5, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*2]
   adcx  acc5, t3
   adox  acc0, t4

   mulx  t4, t3, qword [aPtr+sizeof(qword)*3]
   adcx  acc0, t3
   adox  acc1, t4

   adcx  acc1, acc2
   adox  acc2, acc2
   adc   acc2, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;; reduction step 3 (final)
   sm2_mul_redstep acc2,acc1,acc0,acc5,acc4,acc3

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t0, qword [rel Lpoly+sizeof(qword)*0]
   mov   t1, qword [rel Lpoly+sizeof(qword)*1]
   mov   t2, qword [rel Lpoly+sizeof(qword)*2]
   mov   t3, qword [rel Lpoly+sizeof(qword)*3]

   mov   t4,   acc4     ;; copy reducted result
   mov   acc3, acc5
   mov   acc6, acc0
   mov   acc7, acc1

   sub   t4,   t0       ;; test %if it exceeds prime value
   sbb   acc3, t1
   sbb   acc6, t2
   sbb   acc7, t3
   sbb   acc2, 0

   cmovnc acc4, t4
   cmovnc acc5, acc3
   cmovnc acc0, acc6
   cmovnc acc1, acc7

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc0
   mov   qword [rdi+sizeof(qword)*3], acc1

   ret
%endif

%if _IPP32E >= _IPP32E_L9
align IPP_ALIGN_FACTOR
IPPASM sm2_mul_montx,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

%xdefine bPtr  rbx

   mov   bPtr, rdx
   call  sm2_mmulx

   REST_XMM
   REST_GPR
   ret
ENDFUNC sm2_mul_montx

%endif ;; _IPP32E_L9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_sqr_montl(uint64_t res[4], uint64_t a[4]);
; void sm2_sqr_montx(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%macro sm2_prod_redstep 6.nolist
  %xdefine %%e %1
  %xdefine %%p4 %2
  %xdefine %%p3 %3
  %xdefine %%p2 %4
  %xdefine %%p1 %5
  %xdefine %%p0 %6

   mov   t4, %%p0
   mov   t0, %%p0
   mov   t3, %%p0
   xor   t1, t1
   xor   t2, t2
   shl   %%p0, 32
   shr   t4, 32  ;; (t4:p0) = p0*2^32

   sub   t0, %%p0   ;; (t3:t2:t1:t0) = p0*2^256 -p0*2^224 +p0*2^64 -p0*2^96
   sbb   t1, t4
   sbb   t2, %%p0
   sbb   t3, t4

   xor   %%p0, %%p0
   add   %%p1, t0   ;; (p5:p4:p3:p2:p1) -= (t3:t2:t1:t0)
   adc   %%p2, t1
   adc   %%p3, t2
   adc   %%p4, t3
   adc   %%p0, 0

   %ifnempty %%e
   add   %%p4, %%e
   adc   %%p0, 0
   %endif

%endmacro

align IPP_ALIGN_FACTOR
IPPASM sm2_sqr_montl,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
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

%xdefine t0  rcx
%xdefine t1  rbp
%xdefine t2  rbx
%xdefine t3  rdx
%xdefine t4  rax

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
   mov   rax,qword [aPtr+sizeof(qword)*3]
   mul   t2
   add   acc3, rax
   adc   rdx, 0
   mov   acc4, rdx
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t2, qword [aPtr+sizeof(qword)*1]
   mov   rax,qword [aPtr+sizeof(qword)*2]
   mul   t2
   add   acc3, rax
   adc   rdx, 0
   mov   t1, rdx
   mov   rax,qword [aPtr+sizeof(qword)*3]
   mul   t2
   add   acc4, rax
   adc   rdx, 0
   add   acc4, t1
   adc   rdx, 0
   mov   acc5, rdx
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   t2, qword [aPtr+sizeof(qword)*2]
   mov   rax,qword [aPtr+sizeof(qword)*3]
   mul   t2
   add   acc5, rax
   adc   rdx, 0
   mov   acc6, rdx

   xor   acc7, acc7
   shld  acc7, acc6, 1
   shld  acc6, acc5, 1
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
   adc   acc6, 0
   mov   rax,qword [aPtr+sizeof(qword)*3]
   mul   rax
   add   acc6, rax
   adc   acc7, rdx

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   sm2_prod_redstep      ,acc4,acc3,acc2,acc1,acc0
   sm2_prod_redstep  acc0,acc5,acc4,acc3,acc2,acc1
   sm2_prod_redstep  acc1,acc6,acc5,acc4,acc3,acc2
   sm2_prod_redstep  acc2,acc7,acc6,acc5,acc4,acc3
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   mov   t0, qword [rel Lpoly+sizeof(qword)*0]
   mov   t1, qword [rel Lpoly+sizeof(qword)*1]
   mov   t2, qword [rel Lpoly+sizeof(qword)*2]
   mov   t3, qword [rel Lpoly+sizeof(qword)*3]

   mov     t4, acc4
   mov   acc0, acc5
   mov   acc1, acc6
   mov   acc2, acc7

   sub     t4, t0
   sbb   acc0, t1
   sbb   acc1, t2
   sbb   acc2, t3
   sbb   acc3, 0

   cmovnc acc4, t4
   cmovnc acc5, acc0
   cmovnc acc6, acc1
   cmovnc acc7, acc2

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc6
   mov   qword [rdi+sizeof(qword)*3], acc7

   REST_XMM
   REST_GPR
   ret
ENDFUNC sm2_sqr_montl

%if (_IPP32E >= _IPP32E_L9)
align IPP_ALIGN_FACTOR
IPPASM sm2_sqr_montx,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
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

%xdefine t0  rcx
%xdefine t1  rbp
%xdefine t2  rbx
%xdefine t3  rdx
%xdefine t4  rax

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   rdx,  qword [aPtr+sizeof(qword)*0]
   mulx  acc2, acc1, qword [aPtr+sizeof(qword)*1]
   mulx  acc3, t0,   qword [aPtr+sizeof(qword)*2]
   add   acc2, t0
   mulx  acc4, t0,   qword [aPtr+sizeof(qword)*3]
   adc   acc3, t0
   adc   acc4, 0
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   rdx, qword [aPtr+sizeof(qword)*1]
   xor   acc5, acc5
   mulx  t1, t0, qword [aPtr+sizeof(qword)*2]
   adcx  acc3, t0
   adox  acc4, t1
   mulx  t1, t0, qword [aPtr+sizeof(qword)*3]
   adcx  acc4, t0
   adox  acc5, t1
   adc   acc5, 0
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   mov   rdx, qword [aPtr+sizeof(qword)*2]
   mulx  acc6, t0, qword [aPtr+sizeof(qword)*3]
   add   acc5, t0
   adc   acc6, 0

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   xor   acc7, acc7
   shld  acc7, acc6, 1
   shld  acc6, acc5, 1
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
   mov   rdx, qword [aPtr+sizeof(qword)*3]
   mulx  t1, t0, rdx
   adcx  acc6, t0
   adcx  acc7, t1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   sm2_prod_redstep      ,acc4,acc3,acc2,acc1,acc0
   sm2_prod_redstep  acc0,acc5,acc4,acc3,acc2,acc1
   sm2_prod_redstep  acc1,acc6,acc5,acc4,acc3,acc2
   sm2_prod_redstep  acc2,acc7,acc6,acc5,acc4,acc3
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   mov   t0, qword [rel Lpoly+sizeof(qword)*0]
   mov   t1, qword [rel Lpoly+sizeof(qword)*1]
   mov   t2, qword [rel Lpoly+sizeof(qword)*2]
   mov   t3, qword [rel Lpoly+sizeof(qword)*3]

   mov     t4, acc4
   mov   acc0, acc5
   mov   acc1, acc6
   mov   acc2, acc7

   sub     t4, t0
   sbb   acc0, t1
   sbb   acc1, t2
   sbb   acc2, t3
   sbb   acc3, 0

   cmovnc acc4, t4
   cmovnc acc5, acc0
   cmovnc acc6, acc1
   cmovnc acc7, acc2

   mov   qword [rdi+sizeof(qword)*0], acc4
   mov   qword [rdi+sizeof(qword)*1], acc5
   mov   qword [rdi+sizeof(qword)*2], acc6
   mov   qword [rdi+sizeof(qword)*3], acc7

   REST_XMM
   REST_GPR
   ret
ENDFUNC sm2_sqr_montx

%endif ;; _IPP32E_L9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void sm2_mont_back(uint64_t res[4], uint64_t a[4]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM sm2_mont_back,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbp,rbx,rsi,rdi,r12,r13,r14,r15
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

%xdefine t0  rcx
%xdefine t1  rbp
%xdefine t2  rbx
%xdefine t3  rdx
%xdefine t4  rax

   mov   acc2, qword [rsi+sizeof(qword)*0]
   mov   acc3, qword [rsi+sizeof(qword)*1]
   mov   acc4, qword [rsi+sizeof(qword)*2]
   mov   acc5, qword [rsi+sizeof(qword)*3]
   xor   acc0, acc0
   xor   acc1, acc1

   sm2_mul_redstep acc1,acc0,acc5,acc4,acc3,acc2
   sm2_mul_redstep acc2,acc1,acc0,acc5,acc4,acc3
   sm2_mul_redstep acc3,acc2,acc1,acc0,acc5,acc4
   sm2_mul_redstep acc4,acc3,acc2,acc1,acc0,acc5

   mov   t0, acc0
   mov   t1, acc1
   mov   t2, acc2
   mov   t3, acc3

   sub   t0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   t1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   t2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   t3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   acc4, 0

   cmovnc acc0, t0
   cmovnc acc1, t1
   cmovnc acc2, t2
   cmovnc acc3, t3

   mov   qword [rdi+sizeof(qword)*0], acc0
   mov   qword [rdi+sizeof(qword)*1], acc1
   mov   qword [rdi+sizeof(qword)*2], acc2
   mov   qword [rdi+sizeof(qword)*3], acc3

   REST_XMM
   REST_GPR
   ret
ENDFUNC sm2_mont_back

%endif ;; _IPP32E>=_IPP32E_M7

