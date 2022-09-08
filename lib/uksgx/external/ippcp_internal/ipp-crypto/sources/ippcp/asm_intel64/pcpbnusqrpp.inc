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
;               Low level Big Number squaring Support
;
;

%ifndef _PCPBNUSQR_INC_
%assign _PCPBNUSQR_INC_  1

%include "pcpmulx.inc"
%include "pcpbnusqrpp_basic.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; (8*n) squarer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_8N_adcox,PRIVATE
   push     rdi         ; save diagonal loop parameters
   push     rsi
   push     rdx

   push     rdi         ; save initial triangle product parameters
   push     rsi
   push     rdx
;
; init upper triangle product
;
   push     rdx
   call     sqr8_triangle
   pop      rdx

   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15
   add      rdi, sizeof(qword)*8

   sub      rdx, 8

   mov      rcx, rsi
   add      rsi, sizeof(qword)*8
.initLoop:
   push     rdx
   call     mla_8x8
   pop      rdx
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   sub      rdx, 8
   jnz      .initLoop

   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   mov      qword [rdi+sizeof(qword)*7],r15
   jmp      .update_Triangle

;
; update upper triangle product
;
.outerLoop:
   push     rdi      ; update triangle product parameters
   push     rsi
   push     rdx

   xor      rax, rax    ; c-flag
   push     rax

   mov      r8,  qword [rdi+sizeof(qword)*0]
   mov      r9,  qword [rdi+sizeof(qword)*1]
   mov      r10, qword [rdi+sizeof(qword)*2]
   mov      r11, qword [rdi+sizeof(qword)*3]
   mov      r12, qword [rdi+sizeof(qword)*4]
   mov      r13, qword [rdi+sizeof(qword)*5]
   mov      r14, qword [rdi+sizeof(qword)*6]
   mov      r15, qword [rdi+sizeof(qword)*7]

.innerLoop_entry:
   push     rdx
   call     sqr8_triangle
   pop      rdx

   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15
   add      rdi, sizeof(qword)*8

   sub      rdx, 8
   jz       .skipInnerLoop

   mov      rcx, rsi
   add      rsi, sizeof(qword)*8
.innerLoop:
   pop      rax      ; restore c-flag
   neg      rax
   op_reg_mem  adc,  r8,  qword [rdi+sizeof(qword)*0], rax
   op_reg_mem  adc,  r9,  qword [rdi+sizeof(qword)*1], rax
   op_reg_mem  adc,  r10, qword [rdi+sizeof(qword)*2], rax
   op_reg_mem  adc,  r11, qword [rdi+sizeof(qword)*3], rax
   op_reg_mem  adc,  r12, qword [rdi+sizeof(qword)*4], rax
   op_reg_mem  adc,  r13, qword [rdi+sizeof(qword)*5], rax
   op_reg_mem  adc,  r14, qword [rdi+sizeof(qword)*6], rax
   op_reg_mem  adc,  r15, qword [rdi+sizeof(qword)*7], rax
   sbb      rax, rax ; save c-flag
   push     rax

   push     rdx
   call     mla_8x8
   pop      rdx
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   sub      rdx, 8
   jnz      .innerLoop

.skipInnerLoop:
   pop      rax      ; restore c-flag
   neg      rax
   adc      r8, 0
   mov      qword [rdi+sizeof(qword)*0], r8
   adc      r9, 0
   mov      qword [rdi+sizeof(qword)*1], r9
   adc      r10,0
   mov      qword [rdi+sizeof(qword)*2],r10
   adc      r11,0
   mov      qword [rdi+sizeof(qword)*3],r11
   adc      r12,0
   mov      qword [rdi+sizeof(qword)*4],r12
   adc      r13,0
   mov      qword [rdi+sizeof(qword)*5],r13
   adc      r14,0
   mov      qword [rdi+sizeof(qword)*6],r14
   adc      r15,0
   mov      qword [rdi+sizeof(qword)*7],r15

.update_Triangle:
   pop      rdx
   pop      rsi
   pop      rdi
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*(8*2)
   sub      rdx, 8
   jnz      .outerLoop

;
; add diagonal terms
;
   pop      rcx
   pop      rsi
   pop      rdi
   call     finalize
   ret
ENDFUNC sqr_8N_adcox


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; general case N>16 squarer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_N_adcox,PRIVATE
   push     rdi         ; save diagonal loop parameters
   push     rsi
   push     rdx

   push     rdi         ; save initial triangle product parameters
   push     rsi
   push     rdx

   mov      rbp, rdx
   and      rbp, 7
   GET_EP   rax, mla_8xl_tail, rbp  ; get tail procedure
   push     rax

;
; init upper triangle product
;
   sub      rdx, 8

   push     rdx
   call     sqr8_triangle
   pop      rdx

   mov      qword [rdi+sizeof(qword)*7], r15
   add      rdi, sizeof(qword)*8
   xor      r15, r15

   mov      rcx, rsi
   add      rsi, sizeof(qword)*8
   sub      rdx, 8

.initLoop:
   push     rdx
   call     mla_8x8
   pop      rdx
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   sub      rdx, 8
   jnc      .initLoop

   add      rdx, 8
;
; tail
;
   SWAP     rsi, rcx
   mov      rax, [rsp]  ; procedure
   push     rdx
   call     rax
   pop      rdx
   lea      rdi, [rdi+rdx*sizeof(qword)]

   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   mov      qword [rdi+sizeof(qword)*7],r15
   jmp      .update_Triangle

;
; update upper triangle product
;
.outerLoop:
   push     rdi      ; update triangle product parameters
   push     rsi
   push     rdx
   push     rax      ; tail procedure

   xor      rax, rax    ; c-flag
   push     rax

   mov      r8,  qword [rdi+sizeof(qword)*0]
   mov      r9,  qword [rdi+sizeof(qword)*1]
   mov      r10, qword [rdi+sizeof(qword)*2]
   mov      r11, qword [rdi+sizeof(qword)*3]
   mov      r12, qword [rdi+sizeof(qword)*4]
   mov      r13, qword [rdi+sizeof(qword)*5]
   mov      r14, qword [rdi+sizeof(qword)*6]
   mov      r15, qword [rdi+sizeof(qword)*7]

   sub      rdx, 8

   push     rdx
   call     sqr8_triangle
   pop      rdx

   mov      qword [rdi+sizeof(qword)*7], r15
   add      rdi, sizeof(qword)*8
   xor      r15, r15

   mov      rcx, rsi
   add      rsi, sizeof(qword)*8
   sub      rdx, 8

.innerLoop:
   pop      rax      ; restore c-flag
   neg      rax
   op_reg_mem  adc,  r8,  qword [rdi+sizeof(qword)*0], rax
   op_reg_mem  adc,  r9,  qword [rdi+sizeof(qword)*1], rax
   op_reg_mem  adc,  r10, qword [rdi+sizeof(qword)*2], rax
   op_reg_mem  adc,  r11, qword [rdi+sizeof(qword)*3], rax
   op_reg_mem  adc,  r12, qword [rdi+sizeof(qword)*4], rax
   op_reg_mem  adc,  r13, qword [rdi+sizeof(qword)*5], rax
   op_reg_mem  adc,  r14, qword [rdi+sizeof(qword)*6], rax
   op_reg_mem  adc,  r15, qword [rdi+sizeof(qword)*7], rax
   sbb      rax, rax ; save c-flag
   push     rax

   push     rdx
   call     mla_8x8
   pop      rdx

   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   sub      rdx, 8
   jnc      .innerLoop

   add      rdx, 8
;
; tail
;
   ; clear in advance
    pxor    xmm0, xmm0
    movdqu  xmmword [rdi+rdx*sizeof(qword)], xmm0
    movdqu  xmmword [rdi+rdx*sizeof(qword)+sizeof(qword)*2], xmm0
    movdqu  xmmword [rdi+rdx*sizeof(qword)+sizeof(qword)*4], xmm0
    movdqu  xmmword [rdi+rdx*sizeof(qword)+sizeof(qword)*6], xmm0

   ; updates registers before mla operation
   pop      rax      ; restore c-flag
   neg      rax
   op_reg_mem  adc,  r8,  qword [rdi+sizeof(qword)*0], rax
   op_reg_mem  adc,  r9,  qword [rdi+sizeof(qword)*1], rax
   op_reg_mem  adc,  r10, qword [rdi+sizeof(qword)*2], rax
   op_reg_mem  adc,  r11, qword [rdi+sizeof(qword)*3], rax
   op_reg_mem  adc,  r12, qword [rdi+sizeof(qword)*4], rax
   op_reg_mem  adc,  r13, qword [rdi+sizeof(qword)*5], rax
   op_reg_mem  adc,  r14, qword [rdi+sizeof(qword)*6], rax
   op_reg_mem  adc,  r15, qword [rdi+sizeof(qword)*7], rax

   ; store carry for future
   sbb      rax, rax
   neg      rax
   mov      qword [rdi+sizeof(qword)*8], rax

   ; mla_8xn operation
   SWAP     rsi, rcx
   mov      rax, [rsp]  ; procedure
   push     rdx
   call     rax
   pop      rdx
   lea      rdi, [rdi+rdx*sizeof(qword)]

   ; updates registers before store
   xor      rax, rax
   mov      rax, qword [rdi+sizeof(qword)*0]
   add      r8,  rax
   mov      qword [rdi+sizeof(qword)*0], r8
   mov      rax, qword [rdi+sizeof(qword)*1]
   adc      r9,  rax
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      rax, qword [rdi+sizeof(qword)*2]
   adc      r10, rax
   mov      qword [rdi+sizeof(qword)*2], r10
   mov      rax, qword [rdi+sizeof(qword)*3]
   adc      r11, rax
   mov      qword [rdi+sizeof(qword)*3], r11
   mov      rax, qword [rdi+sizeof(qword)*4]
   adc      r12, rax
   mov      qword [rdi+sizeof(qword)*4], r12
   mov      rax, qword [rdi+sizeof(qword)*5]
   adc      r13, rax
   mov      qword [rdi+sizeof(qword)*5], r13
   mov      rax, qword [rdi+sizeof(qword)*6]
   adc      r14, rax
   mov      qword [rdi+sizeof(qword)*6], r14
   mov      rax, qword [rdi+sizeof(qword)*7]
   adc      r15, rax
   mov      qword [rdi+sizeof(qword)*7], r15

.update_Triangle:
   pop      rax                  ; tail procedure
   pop      rdx
   pop      rsi
   pop      rdi
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*(8*2)
   sub      rdx, 8
   cmp      rdx, 16
   jg       .outerLoop

;
; tail
;
   mov      rbp, rdx
   sub      rbp, 8
   GET_EP   rax, sqrN_triangle, rbp    ; get triangle proc

   sub      rsp, sizeof(qword)*32
   push     rdi
   push     rdx

   mov      r8, qword [rdi+sizeof(qword)*0]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   lea      rdi, [rsp+sizeof(qword)*2]
   call     rax

   mov      rsi, rdi
   pop      rdx
   pop      rdi

   ; copy 8 terms
   movdqu  xmm0, xmmword [rsi]
   movdqu  xmm1, xmmword [rsi+sizeof(qword)*2]
   movdqu  xmm2, xmmword [rsi+sizeof(qword)*4]
   movdqu  xmm3, xmmword [rsi+sizeof(qword)*6]
   add      rsi, sizeof(qword)*8
   movdqu  xmmword [rdi], xmm0
   movdqu  xmmword [rdi+sizeof(qword)*2], xmm1
   movdqu  xmmword [rdi+sizeof(qword)*4], xmm2
   movdqu  xmmword [rdi+sizeof(qword)*6], xmm3
   add      rdi, sizeof(qword)*8

   ; update rdx-8 terms
   lea      rax, [rdx-8]
   xor      rbx, rbx
.update1:
   mov      r8, qword [rsi]
   mov      r9, qword [rdi]
   add      rsi, sizeof(qword)
   neg      rbx
   adc      r8, r9
   sbb      rbx, rbx
   mov      qword [rdi], r8
   add      rdi, sizeof(qword)
   sub      rax, 1
   jg       .update1

   ; update rdx terms
.update2:
   mov      r8, qword [rsi]
   add      rsi, sizeof(qword)
   neg      rbx
   adc      r8, 0
   sbb      rbx, rbx
   mov      qword [rdi], r8
   add      rdi, sizeof(qword)
   sub      rdx, 1
   jg       .update2

   add      rsp, sizeof(qword)*32

;
; add diagonal terms
;
.add_diagonals:
   pop      rcx
   pop      rsi
   pop      rdi
   call     finalize

.quit:
   ret
ENDFUNC sqr_N_adcox


%endif ;; _PCPBNUSQR_INC_
