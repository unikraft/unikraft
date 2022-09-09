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
;               Big Number Multiplicative Operations
;
;      Content:
;         cpMulAdx_BNU_school()
;         cpSqrAdx_BNU_school()
;         cpMontRedAdx_BNU()
;
;  Implementation is using mulx and adcx/adox instruvtions
;
;

%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ADCOX_NI_ENABLING_ == _FEATURE_ON_) || (_ADCOX_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_L9)

%assign _xEMULATION_ 1
%assign _ADCX_ADOX_  1

segment .text align=IPP_ALIGN_FACTOR

%include "pcpbnumulpp.inc"
%include "pcpbnusqrpp.inc"
%include "pcpmredpp.inc"

;*************************************************************
;* Ipp64u  cpMulAdx_BNU_school(Ipp64u* pR;
;*                       const Ipp64u* pA, int  aSize,
;*                       const Ipp64u* pB, int  bSize)
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpMulAdx_BNU_school,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rbp,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 5

; rdi = pR
; rsi = pA
; edx = nsA
; rcx = pB
; r8d = nsB

   movsxd   rdx, edx    ; expand length
   movsxd   rbx, r8d

   xor      r8, r8      ; clear scratch
   xor      r9, r9
   xor      r10, r10
   xor      r11, r11
   xor      r12, r12
   xor      r13, r13
   xor      r14, r14
   xor      r15, r15

   cmp      rdx, rbx
   jl       .swap_operans      ; nsA < nsB
   jg       .test_8N_case      ; test %if nsA=8*N and nsB=8*M

   cmp      rdx, 16
   jg       .test_8N_case

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; short nsA==nsB (1,..,16)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   cmp      rdx, 4
   jg       .more_then_4

   cmp      edx, 3
   ja       .mul_4_4
   jz       .mul_3_3
   jp       .mul_2_2
  ;         mul_1_1

.mul_1_1:
   MUL_NxN  1, rdi, rsi, rcx, rbx, rbp, r8
   jmp      .quit
.mul_2_2:
   MUL_NxN  2, rdi, rsi, rcx, rbx, rbp, r8, r9
   jmp      .quit
.mul_3_3:
   MUL_NxN  3, rdi, rsi, rcx, rbx, rbp, r8, r9, r10
   jmp      .quit
.mul_4_4:
   MUL_NxN  4, rdi, rsi, rcx, rbx, rbp, r8, r9, r10, r11
   jmp      .quit

.more_then_4:
   GET_EP   rax, mul_lxl_basic, rdx, rbp
   call     rax
   jmp      .quit

.swap_operans:
   SWAP     rsi, rcx       ; swap operands
   SWAP     rdx, rbx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 8*N x 8*M case multiplier
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.test_8N_case:
   mov      rax, rdx
   or       rax, rbx
   and      rax, 7
   jnz      .general_mul

   CALL_FUNC     mul_8Nx8M_adcox
   jmp      .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; general case multiplier
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.general_mul:
   CALL_FUNC  mul_NxM_adcox
   jmp   .quit

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpMulAdx_BNU_school

;*************************************************************
;*
;* Ipp64u  cpSqrAdx_BNU_school(Ipp64u* pR;
;*                       const Ipp64u* pA, int  aSize)
;*
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpSqrAdx_BNU_school,PUBLIC
%assign LOCAL_FRAME 0
        USES_GPR rbx,rbp,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

   movsxd   rdx, edx    ; expand length

   xor      r8, r8      ; clear scratch
   xor      r9, r9
   xor      r10, r10
   xor      r11, r11
   xor      r12, r12
   xor      r13, r13
   xor      r14, r14
   xor      r15, r15

   cmp      rdx, 16
   jg       .test_8N_case

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; short nsA (1,..,16)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   GET_EP   rax, sqr_l_basic, rdx, rbp
   call     rax
   jmp      .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 8N case squarer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.test_8N_case:
   test     rdx, 7
   jnz      .general_sqr

   CALL_FUNC     sqr_8N_adcox
   jmp      .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; general case squarer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.general_sqr:
   CALL_FUNC     sqr_N_adcox

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpSqrAdx_BNU_school

;*************************************************************
;*
;* Ipp64u  cpMontRedAdx_BNU(Ipp64u* pR;
;*                          Ipp64u* pProduct,
;*                    const Ipp64u* pModulus, int  mSize,
;*                          Ipp64u  m)
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpMontRedAdx_BNU,PUBLIC
%assign LOCAL_FRAME (0)
        USES_GPR rbx,rbp,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 5
;pR        (rdi) address of the reduction
;pProduct  (rsi) address of the temporary product
;pModulus  (rdx) address of the modulus
;mSize     (rcx) size    of the modulus
;m0        (r8)  montgomery helper (m')

   mov      r15, rdi    ; store reduction address

   ; reload parameters for future convinience:
   mov      rdi, rsi    ; rdi = temporary product buffer
   mov      rsi, rdx    ; rsi = modulus
   movsxd   rdx, ecx    ; rdx = length of modulus

   cmp      rdx, 16
   ja       .test_8N_case   ; length of modulus >16

   cmp      rdx, 4
   ja       .above4         ; length of modulus 4,..,16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; short modulus (1,..,4)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   cmp      rdx, 3
   ja       .red_4
   jz       .red_3
   jp       .red_2
  ;         red_1

.red_1:
   mov      r9, qword [rdi+sizeof(qword)*0]
   MRED_FIX 1, r15, rdi, rsi, r8, rbp,rbx, r9
   jmp      .quit

.red_2:
   mov      r9,  qword [rdi+sizeof(qword)*0]
   mov      r10, qword [rdi+sizeof(qword)*1]
   MRED_FIX 2, r15, rdi, rsi, r8, rbp,rbx, r9,r10
   jmp      .quit

.red_3:
   mov      r9,  qword [rdi+sizeof(qword)*0]
   mov      r10, qword [rdi+sizeof(qword)*1]
   mov      r11, qword [rdi+sizeof(qword)*2]
   MRED_FIX 3, r15, rdi, rsi, r8, rbp,rbx, r9,r10,r11
   jmp      .quit

.red_4:
   mov      r9,  qword [rdi+sizeof(qword)*0]
   mov      r10, qword [rdi+sizeof(qword)*1]
   mov      r11, qword [rdi+sizeof(qword)*2]
   mov      r12, qword [rdi+sizeof(qword)*3]
   MRED_FIX 4, r15, rdi, rsi, r8, rbp,rbx, r9,r10,r11,r12
   jmp      .quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; short modulus (5,..,16)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.above4:
   mov      rbp, rdx
   sub      rbp, 4
   GET_EP   rax, mred_short, rbp    ; mred procedure

   call     rax
   jmp      .quit


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 8N case squarer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.test_8N_case:
   test     rdx, 7
   jnz      .general_case

   CALL_FUNC     mred_8N_adcox
   jmp      .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; general case modulus
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.general_case:
   CALL_FUNC     mred_N_adcox

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpMontRedAdx_BNU

%endif

%endif ;; _ADCOX_NI_ENABLING_

