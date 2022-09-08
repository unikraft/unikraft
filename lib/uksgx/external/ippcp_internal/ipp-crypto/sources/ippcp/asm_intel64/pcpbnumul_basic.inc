;===============================================================================
; Copyright 2013-2021 Intel Corporation
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
;               Low level Big Number multiplication Support
;
;

%ifndef _PCPBNUMUL_BASIC_INC_
%assign _PCPBNUMUL_BASIC_INC_  1

%include "pcpmulx.inc"
%include "pcpbnumul_fix.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; short size (1-8 qwords) operand mla and mul operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; 1x1 multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_1x1,PRIVATE
   mov      rdx, qword [rcx]
   MLA_FIX  1, rdi, rsi, rbx,rbp, r8
   ret
ENDFUNC mla_1x1


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_1x1,PRIVATE
   mov      rdx,qword [rcx]
   gsmulx   rdx, rax, qword [rsi]
   mov      qword [rdi], rax
   mov      qword [rdi+sizeof(qword)], rdx
   ret
ENDFUNC mul_1x1


;;
;; 2x2 multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_2x2,PRIVATE
   mov      rdx, qword [rcx]
   MLA_FIX  2, rdi, rsi, rbx,rbp, r8,r9
   mov      rdx, qword [rcx+sizeof(qword)]
   MLA_FIX  2, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9
   ret
ENDFUNC mla_2x2


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_2x2,PRIVATE
   call     mla_2x2
   mov      qword [rdi+sizeof(qword)*2], r8
   mov      qword [rdi+sizeof(qword)*3], r9
   ret
ENDFUNC mul_2x2



;;
;; 3x3 multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_3x3,PRIVATE
   mov      rdx, qword [rcx]
   MLA_FIX  3, rdi, rsi, rbx,rbp, r8,r9,r10
   mov      rdx, qword [rcx+sizeof(qword)]
   MLA_FIX  3, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9,r10
   mov      rdx, qword [rcx+sizeof(qword)*2]
   MLA_FIX  3, {rdi+sizeof(qword)*2}, rsi, rbx,rbp, r8,r9,r10
   ret
ENDFUNC mla_3x3


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_3x3,PRIVATE
   call     mla_3x3
   mov      qword [rdi+sizeof(qword)*3], r8
   mov      qword [rdi+sizeof(qword)*4], r9
   mov      qword [rdi+sizeof(qword)*5], r10
   ret
ENDFUNC mul_3x3



;;
;; 4x{2,4} multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_4x2,PRIVATE
   mov      rdx, qword [rcx]
   MLA_FIX  4, rdi, rsi, rbx,rbp, r8,r9,r10,r11
   mov      rdx, qword [rcx+sizeof(qword)]
   MLA_FIX  4, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9,r10,r11
   ret
ENDFUNC mla_4x2


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_4x4,PRIVATE
   call     mla_4x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_4x2
   sub      rdi, sizeof(qword)*2
   sub      rcx, sizeof(qword)*2
   ret
ENDFUNC mla_4x4


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_4x4,PRIVATE
   call     mla_4x4
   mov      qword [rdi+sizeof(qword)*4], r8
   mov      qword [rdi+sizeof(qword)*5], r9
   mov      qword [rdi+sizeof(qword)*6], r10
   mov      qword [rdi+sizeof(qword)*7], r11
   ret
ENDFUNC mul_4x4



;;
;; 5x{2,5} multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_5x2,PRIVATE
   mov      rdx, [rcx+sizeof(qword)*0]
   MLA_FIX  5, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12
   mov      rdx, [rcx+sizeof(qword)*1]
   MLA_FIX  5, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9,r10,r11,r12
   ret
ENDFUNC mla_5x2


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_5x5,PRIVATE
   mov      rdx, [rcx]
   MLA_FIX  5, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12

   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_5x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_5x2

   sub      rdi, sizeof(qword)*3
   sub      rcx, sizeof(qword)*3
   ret
ENDFUNC mla_5x5


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_5x5,PRIVATE
   call     mla_5x5
   mov      qword [rdi+sizeof(qword)*5], r8
   mov      qword [rdi+sizeof(qword)*6], r9
   mov      qword [rdi+sizeof(qword)*7], r10
   mov      qword [rdi+sizeof(qword)*8], r11
   mov      qword [rdi+sizeof(qword)*9], r12
   ret
ENDFUNC mul_5x5



;;
;; 6x{2,6} multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_6x2,PRIVATE
   mov      rdx, [rcx+sizeof(qword)*0]
   MLA_FIX  6, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13
   mov      rdx, [rcx+sizeof(qword)*1]
   MLA_FIX  6, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13
   ret
ENDFUNC mla_6x2


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_6x6,PRIVATE
   call     mla_6x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_6x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_6x2

   sub      rdi, sizeof(qword)*4
   sub      rcx, sizeof(qword)*4
   ret
ENDFUNC mla_6x6


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_6x6,PRIVATE
   call     mla_6x6
   mov      qword [rdi+sizeof(qword)*6], r8
   mov      qword [rdi+sizeof(qword)*7], r9
   mov      qword [rdi+sizeof(qword)*8], r10
   mov      qword [rdi+sizeof(qword)*9], r11
   mov      qword [rdi+sizeof(qword)*10],r12
   mov      qword [rdi+sizeof(qword)*11],r13
   ret
ENDFUNC mul_6x6



;;
;; 7x{2,7} multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_7x2,PRIVATE
   mov      rdx, [rcx+sizeof(qword)*0]
   MLA_FIX  7, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14
   mov      rdx, [rcx+sizeof(qword)*1]
   MLA_FIX  7, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14
   ret
ENDFUNC mla_7x2


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_7x7,PRIVATE
   mov      rdx, [rcx]
   MLA_FIX  7, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14

   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_7x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_7x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_7x2

   sub      rdi, sizeof(qword)*5
   sub      rcx, sizeof(qword)*5
   ret
ENDFUNC mla_7x7


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_7x7,PRIVATE
   call     mla_7x7
   mov      qword [rdi+sizeof(qword)*7], r8
   mov      qword [rdi+sizeof(qword)*8], r9
   mov      qword [rdi+sizeof(qword)*9], r10
   mov      qword [rdi+sizeof(qword)*10],r11
   mov      qword [rdi+sizeof(qword)*11],r12
   mov      qword [rdi+sizeof(qword)*12],r13
   mov      qword [rdi+sizeof(qword)*13],r14
   ret
ENDFUNC mul_7x7



;;
;; 8x{1,2,3,4,5,6,7,8} multiplication
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x1,PRIVATE
   mov      rdx, [rcx]
   MLA_FIX  8, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14,r15
   ret
ENDFUNC mla_8x1


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x2,PRIVATE
   mov      rdx, [rcx+sizeof(qword)*0]
   MLA_FIX  8, rdi, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14,r15
   mov      rdx, [rcx+sizeof(qword)*1]
   MLA_FIX  8, {rdi+sizeof(qword)}, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14,r15
   ret
ENDFUNC mla_8x2


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x3,PRIVATE
   call     mla_8x1
   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_8x2
   sub      rdi, sizeof(qword)
   sub      rcx, sizeof(qword)
   ret
ENDFUNC mla_8x3


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x4,PRIVATE
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   sub      rdi, sizeof(qword)*2
   sub      rcx, sizeof(qword)*2
   ret
ENDFUNC mla_8x4


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x5,PRIVATE
   call     mla_8x1
   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_8x4
   sub      rdi, sizeof(qword)
   sub      rcx, sizeof(qword)
   ret
ENDFUNC mla_8x5


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x6,PRIVATE
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x4
   sub      rdi, sizeof(qword)*2
   sub      rcx, sizeof(qword)*2
   ret
ENDFUNC mla_8x6


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x7,PRIVATE
   call     mla_8x1
   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_8x6
   sub      rdi, sizeof(qword)
   sub      rcx, sizeof(qword)
   ret
ENDFUNC mla_8x7


align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_8x8,PRIVATE
   call     mla_8x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2

   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2

   sub      rdi, sizeof(qword)*6
   sub      rcx, sizeof(qword)*6
   ret
ENDFUNC mla_8x8


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_8x8,PRIVATE
   call     mla_8x8
   mov      qword [rdi+sizeof(qword)*8], r8
   mov      qword [rdi+sizeof(qword)*9], r9
   mov      qword [rdi+sizeof(qword)*10],r10
   mov      qword [rdi+sizeof(qword)*11],r11
   mov      qword [rdi+sizeof(qword)*12],r12
   mov      qword [rdi+sizeof(qword)*13],r13
   mov      qword [rdi+sizeof(qword)*14],r14
   mov      qword [rdi+sizeof(qword)*15],r15
   ret
ENDFUNC mul_8x8



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; middle size (9-16 qwords) operand mla and mul operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_9x9,PRIVATE
   call     mla_8x8

   mov      rdx, [rcx+sizeof(qword)*8]
   MLA_FIX  8, {rdi+sizeof(qword)*8}, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14,r15
   push     r15

   mov      rdx, [rsi+sizeof(qword)*8]
   mov      r15, [rdi+sizeof(qword)*8]
   MLA_FIX  8, {rdi+sizeof(qword)*8}, rcx, rbx,rbp, r15,r8,r9,r10,r11,r12,r13,r14
   mov      qword [rdi+sizeof(qword)*9], r15

   gsmulx   r15, rbp, [rcx+sizeof(qword)*8]
   pop      rax
   add      r14, rax
   adc      r15,0
   add      r14, rbp
   adc      r15, 0

   mov      qword [rdi+sizeof(qword)*10],r8
   mov      qword [rdi+sizeof(qword)*11],r9
   mov      qword [rdi+sizeof(qword)*12],r10
   mov      qword [rdi+sizeof(qword)*13],r11
   mov      qword [rdi+sizeof(qword)*14],r12
   mov      qword [rdi+sizeof(qword)*15],r13
   mov      qword [rdi+sizeof(qword)*16],r14
   mov      qword [rdi+sizeof(qword)*17],r15
   ret
ENDFUNC mul_9x9


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_10x10,PRIVATE
; A0*B0
   call     mla_8x8
   add      rdi, sizeof(qword)*8

; + A0*B1
   add      rcx, sizeof(qword)*8
   call     mla_8x2
   push     r15
   push     r14

; + A1*B0
   add      rsi, sizeof(qword)*8
   sub      rcx, sizeof(qword)*8
   SWAP     rcx, rsi

   mov      r15, r13
   mov      r14, r12
   mov      r13, r11
   mov      r12, r10
   mov      r11, r9
   mov      r10, r8
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r8, qword [rdi+sizeof(qword)*0]
   call     mla_8x2

   mov      qword [rdi+sizeof(qword)*2],r8
   mov      qword [rdi+sizeof(qword)*3],r9
   mov      qword [rdi+sizeof(qword)*4],r10
   mov      qword [rdi+sizeof(qword)*5],r11
   mov      qword [rdi+sizeof(qword)*6],r12
   mov      qword [rdi+sizeof(qword)*7],r13
   add      rdi, sizeof(qword)*8

   xor      r10, r10
   pop      r8
   pop      r9
   add      r8, r14
   adc      r9, r15
   adc      r10,0

; + A1*B1
   SWAP     rcx, rsi
   add      rcx, sizeof(qword)*8
   call     mla_2x2
   add      rdi, sizeof(qword)*2

   add      r8, r10
   adc      r9, 0
   mov      qword [rdi+sizeof(qword)*0],r8
   mov      qword [rdi+sizeof(qword)*1],r9
   ret
ENDFUNC mul_10x10


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_11x11,PRIVATE
; A0*B0
   call     mla_8x8
   add      rdi, sizeof(qword)*8

; + A0*B1
   add      rcx, sizeof(qword)*8
   call     mla_8x3
   push     r15
   push     r14
   push     r13

; + A1*B0
   add      rsi, sizeof(qword)*8
   sub      rcx, sizeof(qword)*8
   SWAP     rcx, rsi

   mov      r15, r12
   mov      r14, r11
   mov      r13, r10
   mov      r12, r9
   mov      r11, r8
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r8, qword [rdi+sizeof(qword)*0]
   call     mla_8x3

   mov      qword [rdi+sizeof(qword)*3],r8
   mov      qword [rdi+sizeof(qword)*4],r9
   mov      qword [rdi+sizeof(qword)*5],r10
   mov      qword [rdi+sizeof(qword)*6],r11
   mov      qword [rdi+sizeof(qword)*7],r12
   add      rdi, sizeof(qword)*8

   xor      r11, r11
   pop      r8
   pop      r9
   pop      r10
   add      r8, r13
   adc      r9, r14
   adc      r10,r15
   adc      r11,0

; + A1*B1
   SWAP     rcx, rsi
   add      rcx, sizeof(qword)*8
   call     mla_3x3
   add      rdi, sizeof(qword)*3

   add      r8, r11
   adc      r9, 0
   adc      r10,0
   mov      qword [rdi+sizeof(qword)*0],r8
   mov      qword [rdi+sizeof(qword)*1],r9
   mov      qword [rdi+sizeof(qword)*2],r10
   ret
ENDFUNC mul_11x11


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_12x12,PRIVATE
; A0*B0
   call     mla_8x8
   add      rdi, sizeof(qword)*8

; + A0*B1
   add      rcx, sizeof(qword)*8
   call     mla_8x4
   push     r15
   push     r14
   push     r13
   push     r12

; + A1*B0
   add      rsi, sizeof(qword)*8
   sub      rcx, sizeof(qword)*8
   SWAP     rcx, rsi

   mov      r15, r11
   mov      r14, r10
   mov      r13, r9
   mov      r12, r8
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r8, qword [rdi+sizeof(qword)*0]
   call     mla_8x4

   mov      qword [rdi+sizeof(qword)*4],r8
   mov      qword [rdi+sizeof(qword)*5],r9
   mov      qword [rdi+sizeof(qword)*6],r10
   mov      qword [rdi+sizeof(qword)*7],r11
   add      rdi, sizeof(qword)*8

   xor      rax, rax
   pop      r8
   pop      r9
   pop      r10
   pop      r11
   add      r8, r12
   adc      r9, r13
   adc      r10,r14
   adc      r11,r15
   adc      rax,0
   push     rax

; + A1*B1
   SWAP     rcx, rsi
   add      rcx, sizeof(qword)*8
   call     mla_4x4
   add      rdi, sizeof(qword)*4

   pop      rax
   add      r8, rax
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   mov      qword [rdi+sizeof(qword)*0],r8
   mov      qword [rdi+sizeof(qword)*1],r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   ret
ENDFUNC mul_12x12


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_13x13,PRIVATE
; A0*B0
   call     mla_8x8
   add      rdi, sizeof(qword)*8

; + A0*B1
   add      rcx, sizeof(qword)*8
   call     mla_8x5
   push     r15
   push     r14
   push     r13
   push     r12
   push     r11

; + A1*B0
   add      rsi, sizeof(qword)*8
   sub      rcx, sizeof(qword)*8
   SWAP     rcx, rsi

   mov      r15, r10
   mov      r14, r9
   mov      r13, r8
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r8, qword [rdi+sizeof(qword)*0]
   call     mla_8x5

   mov      qword [rdi+sizeof(qword)*5], r8
   mov      qword [rdi+sizeof(qword)*6], r9
   mov      qword [rdi+sizeof(qword)*7], r10
   add      rdi, sizeof(qword)*8

   xor      rax, rax
   pop      r8
   pop      r9
   pop      r10
   pop      rbx
   pop      rbp
   add      r8, r11
   adc      r9, r12
   adc      r10,r13
   adc      rbx,r14
   adc      rbp,r15
   adc      rax,0
   push     rax

   mov      r11, rbx
   mov      r12, rbp

; + A1*B1
   SWAP     rcx, rsi
   add      rcx, sizeof(qword)*8
   call     mla_5x5
   add      rdi, sizeof(qword)*5

   pop      rax
   add      r8, rax
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   adc      r12,0
   mov      qword [rdi+sizeof(qword)*0],r8
   mov      qword [rdi+sizeof(qword)*1],r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   ret
ENDFUNC mul_13x13


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_14x14,PRIVATE
   call     mla_7x7

   add      rdi, sizeof(qword)*7
   add      rsi, sizeof(qword)*7
   call     mla_7x7
   mov      qword [rdi+sizeof(qword)*7], r8
   mov      qword [rdi+sizeof(qword)*8], r9
   mov      qword [rdi+sizeof(qword)*9], r10
   mov      qword [rdi+sizeof(qword)*10],r11
   mov      qword [rdi+sizeof(qword)*11],r12
   mov      qword [rdi+sizeof(qword)*12],r13
   mov      qword [rdi+sizeof(qword)*13],r14

   mov      r8, qword [rdi+sizeof(qword)*0]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]

   add      rcx, sizeof(qword)*7
   sub      rsi, sizeof(qword)*7
   call     mla_7x7

   xor      rdx, rdx
   op_reg_mem add, r8, qword [rdi+sizeof(qword)*7], rax
   op_reg_mem adc, r9, qword [rdi+sizeof(qword)*8], rax
   op_reg_mem adc, r10,qword [rdi+sizeof(qword)*9], rax
   op_reg_mem adc, r11,qword [rdi+sizeof(qword)*10],rax
   op_reg_mem adc, r12,qword [rdi+sizeof(qword)*11],rax
   op_reg_mem adc, r13,qword [rdi+sizeof(qword)*12],rax
   op_reg_mem adc, r14,qword [rdi+sizeof(qword)*13],rax
   adc      rdx, 0
   push     rdx

   add      rdi, sizeof(qword)*7
   add      rsi, sizeof(qword)*7
   call     mla_7x7

   sub      rdi, sizeof(qword)*14
   sub      rsi, sizeof(qword)*7
   sub      rcx, sizeof(qword)*7

   pop      rdx
   add      r8, rdx
   adc      r9, 0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0

   mov      qword [rdi+sizeof(qword)*21],r8
   mov      qword [rdi+sizeof(qword)*22],r9
   mov      qword [rdi+sizeof(qword)*23],r10
   mov      qword [rdi+sizeof(qword)*24],r11
   mov      qword [rdi+sizeof(qword)*25],r12
   mov      qword [rdi+sizeof(qword)*26],r13
   mov      qword [rdi+sizeof(qword)*27],r14
   ret
ENDFUNC mul_14x14


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_15x15,PRIVATE
; A0*B0
   call     mla_8x8
   add      rdi, sizeof(qword)*8

; + A0*B1
   add      rcx, sizeof(qword)*8
   call     mla_8x7
   mov      qword [rdi+sizeof(qword)*7], r8
   mov      qword [rdi+sizeof(qword)*8], r9
   mov      qword [rdi+sizeof(qword)*9], r10
   mov      qword [rdi+sizeof(qword)*10],r11
   mov      qword [rdi+sizeof(qword)*11],r12
   mov      qword [rdi+sizeof(qword)*12],r13
   mov      qword [rdi+sizeof(qword)*13],r14
   mov      qword [rdi+sizeof(qword)*14],r15

; + A1*B0
   add      rsi, sizeof(qword)*8
   sub      rcx, sizeof(qword)*8
   SWAP     rcx, rsi

   mov      r8, qword [rdi+sizeof(qword)*0]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]
   call     mla_8x7

   mov      qword [rdi+sizeof(qword)*7], r8
   add      rdi, sizeof(qword)*8

; + A1*B1
   SWAP     rcx, rsi
   add      rcx, sizeof(qword)*8

   mov      r8, r9
   mov      r9, r10
   mov      r10,r11
   mov      r11,r12
   mov      r12,r13
   mov      r13,r14
   mov      r14,r15
   xor      rdx, rdx
   op_reg_mem add, r8, qword [rdi+sizeof(qword)*0], rax
   op_reg_mem adc, r9, qword [rdi+sizeof(qword)*1], rax
   op_reg_mem adc, r10,qword [rdi+sizeof(qword)*2], rax
   op_reg_mem adc, r11,qword [rdi+sizeof(qword)*3], rax
   op_reg_mem adc, r12,qword [rdi+sizeof(qword)*4], rax
   op_reg_mem adc, r13,qword [rdi+sizeof(qword)*5], rax
   op_reg_mem adc, r14,qword [rdi+sizeof(qword)*6], rax
   adc      rdx, 0
   push     rdx

   call     mla_7x7
   add      rdi, sizeof(qword)*7

   pop      rax
   add      r8, rax
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   adc      r12,0
   adc      r13,0
   adc      r14,0
   mov      qword [rdi+sizeof(qword)*0],r8
   mov      qword [rdi+sizeof(qword)*1],r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   ret
ENDFUNC mul_15x15


align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_16x16,PRIVATE
   call     mla_8x8

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8
   call     mla_8x8
   mov      qword [rdi+sizeof(qword)*8], r8
   mov      qword [rdi+sizeof(qword)*9], r9
   mov      qword [rdi+sizeof(qword)*10],r10
   mov      qword [rdi+sizeof(qword)*11],r11
   mov      qword [rdi+sizeof(qword)*12],r12
   mov      qword [rdi+sizeof(qword)*13],r13
   mov      qword [rdi+sizeof(qword)*14],r14
   mov      qword [rdi+sizeof(qword)*15],r15

   mov      r8, qword [rdi+sizeof(qword)*0]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   add      rcx, sizeof(qword)*8
   sub      rsi, sizeof(qword)*8
   call     mla_8x8

   xor      rdx, rdx
   op_reg_mem add, r8, qword [rdi+sizeof(qword)*8], rax
   op_reg_mem adc, r9, qword [rdi+sizeof(qword)*9], rax
   op_reg_mem adc, r10,qword [rdi+sizeof(qword)*10],rax
   op_reg_mem adc, r11,qword [rdi+sizeof(qword)*11],rax
   op_reg_mem adc, r12,qword [rdi+sizeof(qword)*12],rax
   op_reg_mem adc, r13,qword [rdi+sizeof(qword)*13],rax
   op_reg_mem adc, r14,qword [rdi+sizeof(qword)*14],rax
   op_reg_mem adc, r15,qword [rdi+sizeof(qword)*15],rax
   adc      rdx, 0
   push     rdx

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8
   call     mla_8x8

   sub      rdi, sizeof(qword)*16
   sub      rsi, sizeof(qword)*8
   sub      rcx, sizeof(qword)*8

   pop      rdx
   add      r8, rdx
   adc      r9, 0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0
   adc      r15, 0

   mov      qword [rdi+sizeof(qword)*24],r8
   mov      qword [rdi+sizeof(qword)*25],r9
   mov      qword [rdi+sizeof(qword)*26],r10
   mov      qword [rdi+sizeof(qword)*27],r11
   mov      qword [rdi+sizeof(qword)*28],r12
   mov      qword [rdi+sizeof(qword)*29],r13
   mov      qword [rdi+sizeof(qword)*30],r14
   mov      qword [rdi+sizeof(qword)*31],r15
   ret
ENDFUNC mul_16x16


mul_lxl_basic  DQ    mul_1x1 - mul_lxl_basic
               DQ    mul_2x2 - mul_lxl_basic
               DQ    mul_3x3 - mul_lxl_basic
               DQ    mul_4x4 - mul_lxl_basic
               DQ    mul_5x5 - mul_lxl_basic
               DQ    mul_6x6 - mul_lxl_basic
               DQ    mul_7x7 - mul_lxl_basic
               DQ    mul_8x8 - mul_lxl_basic
               DQ    mul_9x9 - mul_lxl_basic
               DQ    mul_10x10-mul_lxl_basic
               DQ    mul_11x11-mul_lxl_basic
               DQ    mul_12x12-mul_lxl_basic
               DQ    mul_13x13-mul_lxl_basic
               DQ    mul_14x14-mul_lxl_basic
               DQ    mul_15x15-mul_lxl_basic
               DQ    mul_16x16-mul_lxl_basic

mla_lxl_short  DQ    mla_1x1 - mla_lxl_short
               DQ    mla_2x2 - mla_lxl_short
               DQ    mla_3x3 - mla_lxl_short
               DQ    mla_4x4 - mla_lxl_short
               DQ    mla_5x5 - mla_lxl_short
               DQ    mla_6x6 - mla_lxl_short
               DQ    mla_7x7 - mla_lxl_short

mla_8xl_tail   DQ    mla_8x1 - mla_8xl_tail
               DQ    mla_8x2 - mla_8xl_tail
               DQ    mla_8x3 - mla_8xl_tail
               DQ    mla_8x4 - mla_8xl_tail
               DQ    mla_8x5 - mla_8xl_tail
               DQ    mla_8x6 - mla_8xl_tail
               DQ    mla_8x7 - mla_8xl_tail

%endif ;; _PCPBNUMUL_BASIC_INC_
