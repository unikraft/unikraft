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
;               secp p521r1 specific implementation
;


%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_M7)

%assign _xEMULATION_  1
%assign _ADCX_ADOX_  1

segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR

LOne     DD    1,1,1,1,1,1,1,1
LTwo     DD    2,2,2,2,2,2,2,2
LThree   DD    3,3,3,3,3,3,3,3

;; The p521r1 polynomial
Lpoly DQ 0ffffffffffffffffh,0ffffffffffffffffh,0ffffffffffffffffh
      DQ 0ffffffffffffffffh,0ffffffffffffffffh,0ffffffffffffffffh
      DQ 0ffffffffffffffffh,0ffffffffffffffffh,000000000000001ffh


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p521r1_mul_by_2(uint64_t res[9], uint64_t a[9]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_mul_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

%xdefine t  rdx

   xor   ex, ex

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]
   mov   a6, qword [rsi+sizeof(qword)*6]
   mov   a7, qword [rsi+sizeof(qword)*7]
   mov   a8, qword [rsi+sizeof(qword)*8]

   shld  ex, a8, 1
   shld  a8, a7, 1
   shld  a7, a6, 1
   shld  a6, a5, 1
   shld  a5, a4, 1
   shld  a4, a3, 1
   shld  a3, a2, 1
   shld  a2, a1, 1
   shld  a1, a0, 1
   shl   a0,     1

   mov   qword [rdi+sizeof(qword)*8], a8
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*0], a0

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   a6, qword [rel Lpoly+sizeof(qword)*6]
   sbb   a7, qword [rel Lpoly+sizeof(qword)*7]
   sbb   a8, qword [rel Lpoly+sizeof(qword)*8]
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
   mov      t, qword [rdi+sizeof(qword)*6]
   cmovnz   a6, t
   mov      t, qword [rdi+sizeof(qword)*7]
   cmovnz   a7, t
   mov      t, qword [rdi+sizeof(qword)*8]
   cmovnz   a8, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_mul_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p521r1_div_by_2(uint64_t res[9], uint64_t a[9]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_div_by_2,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

%xdefine t  rdx

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]
   mov   a6, qword [rsi+sizeof(qword)*6]
   mov   a7, qword [rsi+sizeof(qword)*7]
   mov   a8, qword [rsi+sizeof(qword)*8]

   xor   t,  t
   xor   ex, ex
   add   a0, qword [rel Lpoly+sizeof(qword)*0]
   adc   a1, qword [rel Lpoly+sizeof(qword)*1]
   adc   a2, qword [rel Lpoly+sizeof(qword)*2]
   adc   a3, qword [rel Lpoly+sizeof(qword)*3]
   adc   a4, qword [rel Lpoly+sizeof(qword)*4]
   adc   a5, qword [rel Lpoly+sizeof(qword)*5]
   adc   a6, qword [rel Lpoly+sizeof(qword)*6]
   adc   a7, qword [rel Lpoly+sizeof(qword)*7]
   adc   a8, qword [rel Lpoly+sizeof(qword)*8]
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
   mov      t,  qword [rsi+sizeof(qword)*6]
   cmovnz   a6, t
   mov      t,  qword [rsi+sizeof(qword)*7]
   cmovnz   a7, t
   mov      t,  qword [rsi+sizeof(qword)*8]
   cmovnz   a8, t

   shrd  a0, a1, 1
   shrd  a1, a2, 1
   shrd  a2, a3, 1
   shrd  a3, a4, 1
   shrd  a4, a5, 1
   shrd  a5, a6, 1
   shrd  a6, a7, 1
   shrd  a7, a8, 1
   shrd  a8, ex, 1

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_div_by_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p521r1_mul_by_3(uint64_t res[9], uint64_t a[9]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_mul_by_3,PUBLIC
%assign LOCAL_FRAME sizeof(qword)*9
        USES_GPR rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

%xdefine t  rdx

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]
   mov   a6, qword [rsi+sizeof(qword)*6]
   mov   a7, qword [rsi+sizeof(qword)*7]
   mov   a8, qword [rsi+sizeof(qword)*8]

   xor   ex, ex
   shld  ex, a8, 1
   shld  a8, a7, 1
   shld  a7, a6, 1
   shld  a6, a5, 1
   shld  a5, a4, 1
   shld  a4, a3, 1
   shld  a3, a2, 1
   shld  a2, a1, 1
   shld  a1, a0, 1
   shl   a0,     1

   mov   qword [rsp+sizeof(qword)*8], a8
   mov   qword [rsp+sizeof(qword)*7], a7
   mov   qword [rsp+sizeof(qword)*6], a6
   mov   qword [rsp+sizeof(qword)*5], a5
   mov   qword [rsp+sizeof(qword)*4], a4
   mov   qword [rsp+sizeof(qword)*3], a3
   mov   qword [rsp+sizeof(qword)*2], a2
   mov   qword [rsp+sizeof(qword)*1], a1
   mov   qword [rsp+sizeof(qword)*0], a0

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   a6, qword [rel Lpoly+sizeof(qword)*6]
   sbb   a7, qword [rel Lpoly+sizeof(qword)*7]
   sbb   a8, qword [rel Lpoly+sizeof(qword)*8]
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
   mov      t, qword [rsp+sizeof(qword)*6]
   cmovb    a6, t
   mov      t, qword [rsp+sizeof(qword)*7]
   cmovb    a7, t
   mov      t, qword [rsp+sizeof(qword)*8]
   cmovb    a8, t

   xor   ex, ex
   add   a0, qword [rsi+sizeof(qword)*0]
   adc   a1, qword [rsi+sizeof(qword)*1]
   adc   a2, qword [rsi+sizeof(qword)*2]
   adc   a3, qword [rsi+sizeof(qword)*3]
   adc   a4, qword [rsi+sizeof(qword)*4]
   adc   a5, qword [rsi+sizeof(qword)*5]
   adc   a6, qword [rsi+sizeof(qword)*6]
   adc   a7, qword [rsi+sizeof(qword)*7]
   adc   a8, qword [rsi+sizeof(qword)*8]
   adc   ex, 0

   mov   qword [rsp+sizeof(qword)*0], a0
   mov   qword [rsp+sizeof(qword)*1], a1
   mov   qword [rsp+sizeof(qword)*2], a2
   mov   qword [rsp+sizeof(qword)*3], a3
   mov   qword [rsp+sizeof(qword)*4], a4
   mov   qword [rsp+sizeof(qword)*5], a5
   mov   qword [rsp+sizeof(qword)*6], a6
   mov   qword [rsp+sizeof(qword)*7], a7
   mov   qword [rsp+sizeof(qword)*8], a8

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   a6, qword [rel Lpoly+sizeof(qword)*6]
   sbb   a7, qword [rel Lpoly+sizeof(qword)*7]
   sbb   a8, qword [rel Lpoly+sizeof(qword)*8]
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
   mov      t, qword [rsp+sizeof(qword)*6]
   cmovb    a6, t
   mov      t, qword [rsp+sizeof(qword)*7]
   cmovb    a7, t
   mov      t, qword [rsp+sizeof(qword)*8]
   cmovb    a8, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_mul_by_3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p521r1_add(uint64_t res[9], uint64_t a[9], uint64_t b[9]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_add,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

%xdefine t  rdx

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]
   mov   a6, qword [rsi+sizeof(qword)*6]
   mov   a7, qword [rsi+sizeof(qword)*7]
   mov   a8, qword [rsi+sizeof(qword)*8]

   xor   ex,  ex
   add   a0, qword [rdx+sizeof(qword)*0]
   adc   a1, qword [rdx+sizeof(qword)*1]
   adc   a2, qword [rdx+sizeof(qword)*2]
   adc   a3, qword [rdx+sizeof(qword)*3]
   adc   a4, qword [rdx+sizeof(qword)*4]
   adc   a5, qword [rdx+sizeof(qword)*5]
   adc   a6, qword [rdx+sizeof(qword)*6]
   adc   a7, qword [rdx+sizeof(qword)*7]
   adc   a8, qword [rdx+sizeof(qword)*8]
   adc   ex, 0

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   sub   a0, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*5]
   sbb   a6, qword [rel Lpoly+sizeof(qword)*6]
   sbb   a7, qword [rel Lpoly+sizeof(qword)*7]
   sbb   a8, qword [rel Lpoly+sizeof(qword)*8]
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
   mov   t, qword [rdi+sizeof(qword)*6]
   cmovb a6, t
   mov   t, qword [rdi+sizeof(qword)*7]
   cmovb a7, t
   mov   t, qword [rdi+sizeof(qword)*8]
   cmovb a8, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_add

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p521r1_sub(uint64_t res[9], uint64_t a[9], uint64_t b[9]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_sub,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

%xdefine t  rdx

   mov   a0, qword [rsi+sizeof(qword)*0]  ; a
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]
   mov   a6, qword [rsi+sizeof(qword)*6]
   mov   a7, qword [rsi+sizeof(qword)*7]
   mov   a8, qword [rsi+sizeof(qword)*8]

   xor   ex, ex
   sub   a0, qword [rdx+sizeof(qword)*0]  ; a-b
   sbb   a1, qword [rdx+sizeof(qword)*1]
   sbb   a2, qword [rdx+sizeof(qword)*2]
   sbb   a3, qword [rdx+sizeof(qword)*3]
   sbb   a4, qword [rdx+sizeof(qword)*4]
   sbb   a5, qword [rdx+sizeof(qword)*5]
   sbb   a6, qword [rdx+sizeof(qword)*6]
   sbb   a7, qword [rdx+sizeof(qword)*7]
   sbb   a8, qword [rdx+sizeof(qword)*8]
   sbb   ex, 0                               ; ex = a>=b? 0 : -1

   mov   qword [rdi+sizeof(qword)*0], a0  ; store (a-b)
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   add   a0, qword [rel Lpoly+sizeof(qword)*0] ; (a-b) +poly
   adc   a1, qword [rel Lpoly+sizeof(qword)*1]
   adc   a2, qword [rel Lpoly+sizeof(qword)*2]
   adc   a3, qword [rel Lpoly+sizeof(qword)*3]
   adc   a4, qword [rel Lpoly+sizeof(qword)*4]
   adc   a5, qword [rel Lpoly+sizeof(qword)*5]
   adc   a6, qword [rel Lpoly+sizeof(qword)*6]
   adc   a7, qword [rel Lpoly+sizeof(qword)*7]
   adc   a8, qword [rel Lpoly+sizeof(qword)*8]

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
   mov      t, qword [rdi+sizeof(qword)*6]
   cmovz    a6, t
   mov      t, qword [rdi+sizeof(qword)*7]
   cmovz    a7, t
   mov      t, qword [rdi+sizeof(qword)*8]
   cmovz    a8, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_sub

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; void p521r1_neg(uint64_t res[9], uint64_t a[9]);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_neg,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

%xdefine t  rdx

   xor   a0, a0
   xor   a1, a1
   xor   a2, a2
   xor   a3, a3
   xor   a4, a4
   xor   a5, a5
   xor   a6, a6
   xor   a7, a7
   xor   a8, a8

   xor   ex, ex
   sub   a0, qword [rsi+sizeof(qword)*0]
   sbb   a1, qword [rsi+sizeof(qword)*1]
   sbb   a2, qword [rsi+sizeof(qword)*2]
   sbb   a3, qword [rsi+sizeof(qword)*3]
   sbb   a4, qword [rsi+sizeof(qword)*4]
   sbb   a5, qword [rsi+sizeof(qword)*5]
   sbb   a6, qword [rsi+sizeof(qword)*6]
   sbb   a7, qword [rsi+sizeof(qword)*7]
   sbb   a8, qword [rsi+sizeof(qword)*8]
   sbb   ex, 0

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   add   a0, qword [rel Lpoly+sizeof(qword)*0]
   adc   a1, qword [rel Lpoly+sizeof(qword)*1]
   adc   a2, qword [rel Lpoly+sizeof(qword)*2]
   adc   a3, qword [rel Lpoly+sizeof(qword)*3]
   adc   a4, qword [rel Lpoly+sizeof(qword)*4]
   adc   a5, qword [rel Lpoly+sizeof(qword)*5]
   adc   a6, qword [rel Lpoly+sizeof(qword)*6]
   adc   a7, qword [rel Lpoly+sizeof(qword)*7]
   adc   a8, qword [rel Lpoly+sizeof(qword)*8]
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
   mov      t, qword [rdi+sizeof(qword)*6]
   cmovz    a6, t
   mov      t, qword [rdi+sizeof(qword)*7]
   cmovz    a7, t
   mov      t, qword [rdi+sizeof(qword)*8]
   cmovz    a8, t

   mov   qword [rdi+sizeof(qword)*0], a0
   mov   qword [rdi+sizeof(qword)*1], a1
   mov   qword [rdi+sizeof(qword)*2], a2
   mov   qword [rdi+sizeof(qword)*3], a3
   mov   qword [rdi+sizeof(qword)*4], a4
   mov   qword [rdi+sizeof(qword)*5], a5
   mov   qword [rdi+sizeof(qword)*6], a6
   mov   qword [rdi+sizeof(qword)*7], a7
   mov   qword [rdi+sizeof(qword)*8], a8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_neg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; p521r1_mred(uint64_t res[9], const uint64_t product[9*2]);
;
; modulus = 2^521 -1
;           [17]  [0]
; m0 = 1
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%macro mred_step 12.nolist
  %xdefine %%EX %1
  %xdefine %%X8 %2
  %xdefine %%X7 %3
  %xdefine %%X6 %4
  %xdefine %%X5 %5
  %xdefine %%X4 %6
  %xdefine %%X3 %7
  %xdefine %%X2 %8
  %xdefine %%X1 %9
  %xdefine %%X0 %10
  %xdefine %%T %11
  %xdefine %%CF %12

   mov   %%T, %%X0                   ;; u0 = X0
   shr   %%T,  (64-(521-512))      ;; (T:X0) = u0<<9
   shl   %%X0, (521-512)
   add   %%T, %%CF

   add   %%X8, %%X0
   adc   %%EX, %%T
   mov   %%CF, dword 0
   adc   %%CF, 0
%endmacro

align IPP_ALIGN_FACTOR
IPPASM p521r1_mred,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 2

%xdefine a0  r8
%xdefine a1  r9
%xdefine a2  r10
%xdefine a3  r11
%xdefine a4  r12
%xdefine a5  r13
%xdefine a6  r14
%xdefine a7  r15
%xdefine a8  rax
%xdefine ex  rcx

   mov   a0, qword [rsi+sizeof(qword)*0]
   mov   a1, qword [rsi+sizeof(qword)*1]
   mov   a2, qword [rsi+sizeof(qword)*2]
   mov   a3, qword [rsi+sizeof(qword)*3]
   mov   a4, qword [rsi+sizeof(qword)*4]
   mov   a5, qword [rsi+sizeof(qword)*5]
   mov   a6, qword [rsi+sizeof(qword)*6]
   mov   a7, qword [rsi+sizeof(qword)*7]
   mov   a8, qword [rsi+sizeof(qword)*8]
   mov   ex, qword [rsi+sizeof(qword)*9]

%xdefine t  rbx
%xdefine carry  rdx

   xor         carry, carry
   mred_step   ex,a8,a7,a6,a5,a4,a3,a2,a1,a0, t,carry ;; step 0   {rcx,rax,r15,r14,r13,r12,r11,r10,r9,r8} -> {rcx,rax,r15,r14,r13,r12,r11,r10,r9,--}

   mov         a0, qword [rsi+sizeof(qword)*10]
   mred_step   a0,ex,a8,a7,a6,a5,a4,a3,a2,a1, t,carry ;; step 1   {r8,rcx,rax,r15,r14,r13,r12,r11,r10,r9} -> {r8,rcx,rax,r15,r14,r13,r12,r11,r10,--}

   mov         a1, qword [rsi+sizeof(qword)*11]
   mred_step   a1,a0,ex,a8,a7,a6,a5,a4,a3,a2, t,carry ;; step 2   {r9,r8,rcx,rax,r15,r14,r13,r12,r11,r10} -> {r9,r8,rcx,rax,r15,r14,r13,r12,r11, --}

   mov         a2, qword [rsi+sizeof(qword)*12]
   mred_step   a2,a1,a0,ex,a8,a7,a6,a5,a4,a3, t,carry ;; step 3   {r10,r9,r8,rcx,rax,r15,r14,r13,r12,r11} -> {r10,r9,r8,rcx,rax,r15,r14,r13,r12, --}

   mov         a3, qword [rsi+sizeof(qword)*13]
   mred_step   a3,a2,a1,a0,ex,a8,a7,a6,a5,a4, t,carry ;; step 4   {r11,r10,r9,r8,rcx,rax,r15,r14,r13,r12} -> {r11,r10,r9,r8,rcx,rax,r15,r14,r13, --}

   mov         a4, qword [rsi+sizeof(qword)*14]
   mred_step   a4,a3,a2,a1,a0,ex,a8,a7,a6,a5, t,carry ;; step 5   {r12,r11,r10,r9,r8,rcx,rax,r15,r14,r13} -> {r12,r11,r10,r9,r8,rcx,rax,r15,r14, --}

   mov         a5, qword [rsi+sizeof(qword)*15]
   mred_step   a5,a4,a3,a2,a1,a0,ex,a8,a7,a6, t,carry ;; step 6   {r13,r12,r11,r10,r9,r8,rcx,rax,r15,r14} -> {r13,r12,r11,r10,r9,r8,rcx,rax,r15, --}

   mov         a6, qword [rsi+sizeof(qword)*16]
   mred_step   a6,a5,a4,a3,a2,a1,a0,ex,a8,a7, t,carry ;; step 7   {r14,r13,r12,r11,r10,r9,r8,rcx,rax,r15} -> {r14,r13,r12,r11,r10,r9,r8,rcx,rax, --}

   mov         a7, qword [rsi+sizeof(qword)*17]
   mred_step   a7,a6,a5,a4,a3,a2,a1,a0,ex,a8, t,carry ;; step 8   {r15,r14,r13,r12,r11,r10,r9,r8,rcx,rax} -> {r15,r14,r13,r12,r11,r10,r9,r8,rcx, --}

   ;; temporary result: a8,a7,a6,a5,a4,a3,a2,a1,a0,ex
   mov   qword [rdi+sizeof(qword)*0], ex
   mov   qword [rdi+sizeof(qword)*1], a0
   mov   qword [rdi+sizeof(qword)*2], a1
   mov   qword [rdi+sizeof(qword)*3], a2
   mov   qword [rdi+sizeof(qword)*4], a3
   mov   qword [rdi+sizeof(qword)*5], a4
   mov   qword [rdi+sizeof(qword)*6], a5
   mov   qword [rdi+sizeof(qword)*7], a6
   mov   qword [rdi+sizeof(qword)*8], a7

   ;; sub modulus
   sub   ex, qword [rel Lpoly+sizeof(qword)*0]
   sbb   a0, qword [rel Lpoly+sizeof(qword)*1]
   sbb   a1, qword [rel Lpoly+sizeof(qword)*2]
   sbb   a2, qword [rel Lpoly+sizeof(qword)*3]
   sbb   a3, qword [rel Lpoly+sizeof(qword)*4]
   sbb   a4, qword [rel Lpoly+sizeof(qword)*5]
   sbb   a5, qword [rel Lpoly+sizeof(qword)*6]
   sbb   a6, qword [rel Lpoly+sizeof(qword)*7]
   sbb   a7, qword [rel Lpoly+sizeof(qword)*8]

   ;; masked copy
   mov   t, qword [rdi+sizeof(qword)*0]
   cmovb ex, t
   mov   t, qword [rdi+sizeof(qword)*1]
   cmovb a0, t
   mov   t, qword [rdi+sizeof(qword)*2]
   cmovb a1, t
   mov   t, qword [rdi+sizeof(qword)*3]
   cmovb a2, t
   mov   t, qword [rdi+sizeof(qword)*4]
   cmovb a3, t
   mov   t, qword [rdi+sizeof(qword)*5]
   cmovb a4, t
   mov   t, qword [rdi+sizeof(qword)*6]
   cmovb a5, t
   mov   t, qword [rdi+sizeof(qword)*7]
   cmovb a6, t
   mov   t, qword [rdi+sizeof(qword)*8]
   cmovb a7, t

   ;; store result: a7,a6,a5,a4,a3,a2,a1,a0,ex
   mov   qword [rdi+sizeof(qword)*0], ex
   mov   qword [rdi+sizeof(qword)*1], a0
   mov   qword [rdi+sizeof(qword)*2], a1
   mov   qword [rdi+sizeof(qword)*3], a2
   mov   qword [rdi+sizeof(qword)*4], a3
   mov   qword [rdi+sizeof(qword)*5], a4
   mov   qword [rdi+sizeof(qword)*6], a5
   mov   qword [rdi+sizeof(qword)*7], a6
   mov   qword [rdi+sizeof(qword)*8], a7

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_mred

%ifndef _DISABLE_ECP_521R1_HARDCODED_BP_TBL_
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; affine point selector
;
; void p521r1_select_ap_w5(AF_POINT *val, const AF_POINT *tbl, int idx);
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_select_ap_w5,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rsi,rdi,r12,r13
        USES_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm14
        COMP_ABI 3

%xdefine val      rdi
%xdefine in_t     rsi
%xdefine idx      edx

%xdefine xyz0     xmm0
%xdefine xyz1     xmm1
%xdefine xyz2     xmm2
%xdefine xyz3     xmm3
%xdefine xyz4     xmm4
%xdefine xyz5     xmm5
%xdefine xyz6     xmm6
%xdefine xyz7     xmm7
%xdefine xyz8     xmm8

%xdefine REQ_IDX  xmm9
%xdefine CUR_IDX  xmm10
%xdefine MASKDATA xmm11
%xdefine TMP      xmm12

   movdqa   CUR_IDX, oword [rel LOne]

   movd     REQ_IDX, idx
   pshufd   REQ_IDX, REQ_IDX, 0

   pxor     xyz0, xyz0
   pxor     xyz1, xyz1
   pxor     xyz2, xyz2
   pxor     xyz3, xyz3
   pxor     xyz4, xyz4
   pxor     xyz5, xyz5
   pxor     xyz6, xyz6
   pxor     xyz7, xyz7
   pxor     xyz8, xyz8

   ; Skip index = 0, is implicictly infty -> load with offset -1
   mov      rcx, dword 16
.select_loop:
      movdqa   MASKDATA, CUR_IDX  ; MASK = CUR_IDX==REQ_IDX? 0xFF : 0x00
      pcmpeqd  MASKDATA, REQ_IDX  ;
      paddd    CUR_IDX, oword [rel LOne]

      movdqa   TMP, oword [in_t+sizeof(oword)*0]
      pand     TMP, MASKDATA
      por      xyz0, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*1]
      pand     TMP, MASKDATA
      por      xyz1, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*2]
      pand     TMP, MASKDATA
      por      xyz2, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*3]
      pand     TMP, MASKDATA
      por      xyz3, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*4]
      pand     TMP, MASKDATA
      por      xyz4, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*5]
      pand     TMP, MASKDATA
      por      xyz5, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*6]
      pand     TMP, MASKDATA
      por      xyz6, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*7]
      pand     TMP, MASKDATA
      por      xyz7, TMP
      movdqa   TMP, oword [in_t+sizeof(oword)*8]
      pand     TMP, MASKDATA
      por      xyz8, TMP

      add      in_t, sizeof(qword)*(9*2)
      dec      rcx
      jnz      .select_loop

   movdqu   oword [val+sizeof(oword)*0], xyz0
   movdqu   oword [val+sizeof(oword)*1], xyz1
   movdqu   oword [val+sizeof(oword)*2], xyz2
   movdqu   oword [val+sizeof(oword)*3], xyz3
   movdqu   oword [val+sizeof(oword)*4], xyz4
   movdqu   oword [val+sizeof(oword)*5], xyz5
   movdqu   oword [val+sizeof(oword)*6], xyz6
   movdqu   oword [val+sizeof(oword)*7], xyz7
   movdqu   oword [val+sizeof(oword)*8], xyz8

   REST_XMM
   REST_GPR
   ret
ENDFUNC p521r1_select_ap_w5

%endif

%endif ;; _IPP32E_M7

