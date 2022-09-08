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
;               Low level Big Number reduction Support
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Fixed-size Montgomery reduction
;;

;
;  X7,X6,X5,X4,X3,X2,X1,X0 contains already preloaded (low) product
;
%macro MRED_FIX 8-15.nolist
  %xdefine %%mSize %1
  %xdefine %%rRed %2
  %xdefine %%rProduct %3
  %xdefine %%rModulus %4
  %xdefine %%M0 %5
  %xdefine %%TMPH %6
  %xdefine %%TMP %7
  %xdefine %%X0 %8
  %xdefine %%X1 %9
  %xdefine %%X2 %10
  %xdefine %%X3 %11
  %xdefine %%X4 %12
  %xdefine %%X5 %13
  %xdefine %%X6 %14
  %xdefine %%X7 %15

   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%if %%mSize > 1
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif
%if %%mSize > 2
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif
%if %%mSize > 3
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif
%if %%mSize > 4
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif
%if %%mSize > 5
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif
%if %%mSize > 6
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif
%if %%mSize > 7
   mov      rdx, %%M0
   imul     rdx, %%X0
   MLA_FIX  {%%mSize},{},{%%rModulus},{%%TMPH},{%%TMP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
%endif

   xor      %%TMP, %%TMP
   add      %%X0, qword [%%rProduct+sizeof(qword)*(%%mSize+0)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+0)], %%X0
%if %%mSize > 1
   adc      %%X1, qword [%%rProduct+sizeof(qword)*(%%mSize+1)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+1)], %%X1
%endif
%if %%mSize > 2
   adc      %%X2, qword [%%rProduct+sizeof(qword)*(%%mSize+2)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+2)], %%X2
%endif
%if %%mSize > 3
   adc      %%X3, qword [%%rProduct+sizeof(qword)*(%%mSize+3)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+3)], %%X3
%endif
%if %%mSize > 4
   adc      %%X4, qword [%%rProduct+sizeof(qword)*(%%mSize+4)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+4)], %%X4
%endif
%if %%mSize > 5
   adc      %%X5, qword [%%rProduct+sizeof(qword)*(%%mSize+5)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+5)], %%X5
%endif
%if %%mSize > 6
   adc      %%X6, qword [%%rProduct+sizeof(qword)*(%%mSize+6)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+6)], %%X6
%endif
%if %%mSize > 7
   adc      %%X7, qword [%%rProduct+sizeof(qword)*(%%mSize+7)]
   mov      qword [%%rProduct+sizeof(qword)*(%%mSize+7)], %%X7
%endif
   adc      %%TMP, 0

   sub      %%X0, qword [%%rModulus+sizeof(qword)*0]
%if %%mSize > 1
   sbb      %%X1, qword [%%rModulus+sizeof(qword)*1]
%endif
%if %%mSize > 2
   sbb      %%X2, qword [%%rModulus+sizeof(qword)*2]
%endif
%if %%mSize > 3
   sbb      %%X3, qword [%%rModulus+sizeof(qword)*3]
%endif
%if %%mSize > 4
   sbb      %%X4, qword [%%rModulus+sizeof(qword)*4]
%endif
%if %%mSize > 5
   sbb      %%X5, qword [%%rModulus+sizeof(qword)*5]
%endif
%if %%mSize > 6
   sbb      %%X6, qword [%%rModulus+sizeof(qword)*6]
%endif
%if %%mSize > 7
   sbb      %%X7, qword [%%rModulus+sizeof(qword)*7]
%endif
   sbb      %%TMP, 0

   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+0)]
   cmovae   rax, %%X0
   mov      qword [%%rRed+sizeof(qword)*0], rax
%if %%mSize > 1
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+1)]
   cmovae   rax, %%X1
   mov      qword [%%rRed+sizeof(qword)*1], rax
%endif
%if %%mSize > 2
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+2)]
   cmovae   rax, %%X2
   mov      qword [%%rRed+sizeof(qword)*2], rax
%endif
%if %%mSize > 3
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+3)]
   cmovae   rax, %%X3
   mov      qword [%%rRed+sizeof(qword)*3], rax
%endif
%if %%mSize > 4
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+4)]
   cmovae   rax, %%X4
   mov      qword [%%rRed+sizeof(qword)*4], rax
%endif
%if %%mSize > 5
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+5)]
   cmovae   rax, %%X5
   mov      qword [%%rRed+sizeof(qword)*5], rax
%endif
%if %%mSize > 6
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+6)]
   cmovae   rax, %%X6
   mov      qword [%%rRed+sizeof(qword)*6], rax
%endif
%if %%mSize > 7
   mov      rax, qword [%%rProduct+sizeof(qword)*(%%mSize+7)]
   cmovae   rax, %%X7
   mov      qword [%%rRed+sizeof(qword)*7], rax
%endif

%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sub_N,PRIVATE
   xor      rax, rax    ; cf = 0
.sub_next:
   lea      rdi, [rdi+sizeof(qword)]
   mov      r8, qword [rsi]
   mov      r9, qword [rcx]
   lea      rsi, [rsi+sizeof(qword)]
   lea      rcx, [rcx+sizeof(qword)]
   sbb      r8, r9
   mov      qword [rdi-sizeof(qword)], r8
   dec      rdx
   jnz      .sub_next
   adc      rax, 0
   ret
ENDFUNC sub_N


align IPP_ALIGN_FACTOR
DECLARE_FUNC copy_ae_N,PRIVATE
   lea      rdi, [rdi+sizeof(qword)]
   mov      r8, qword [rsi]      ; src1[]
   mov      r9, qword [rcx]      ; src2[]
   lea      rsi, [rsi+sizeof(qword)]
   lea      rcx, [rcx+sizeof(qword)]
   cmovae   r8, r9
   mov      qword [rdi-sizeof(qword)], r8
   dec      rdx
   jnz      copy_ae_N
   ret
ENDFUNC copy_ae_N


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; mredN_start procedures
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred1_start,PRIVATE
   MLA_FIX  1,, rsi, rbx,rbp, r8
   ret
ENDFUNC mred1_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred2_start,PRIVATE
   MLA_FIX  2,, rsi, rbx,rbp, r8,r9
   ret
ENDFUNC mred2_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred3_start,PRIVATE
   MLA_FIX  3,, rsi, rbx,rbp, r8,r9,r10
   ret
ENDFUNC mred3_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred4_start,PRIVATE
   MLA_FIX  4,, rsi, rbx,rbp, r8,r9,r10,r11
   ret
ENDFUNC mred4_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred5_start,PRIVATE
   MLA_FIX  5,, rsi, rbx,rbp, r8,r9,r10,r11,r12
   ret
ENDFUNC mred5_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred6_start,PRIVATE
   MLA_FIX  6,, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13
   ret
ENDFUNC mred6_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred7_start,PRIVATE
   MLA_FIX  7,, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14
   ret
ENDFUNC mred7_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8_start,PRIVATE
   MLA_FIX  8,, rsi, rbx,rbp, r8,r9,r10,r11,r12,r13,r14,r15
   ret
ENDFUNC mred8_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x1_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x1_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x2_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x2_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x3_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*2], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*2], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x3_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x4_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*2], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*2], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*3], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*3], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x4_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x5_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*2], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*2], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*3], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*3], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*4], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*4], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x5_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x6_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*2], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*2], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*3], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*3], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*4], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*4], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*5], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*5], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x6_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x7_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*2], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*2], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*3], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*3], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*4], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*4], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*5], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*5], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*6], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*6], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x7_start


align IPP_ALIGN_FACTOR
DECLARE_FUNC mred8x8_start,PRIVATE
   push     rdx      ; save m'

   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*0], rdx
   call     mred8_start
   mov      [rdi], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*1], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*1], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*2], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*2], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*3], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*3], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*4], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*4], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*5], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*5], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*6], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*6], rax

   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   mov      qword [rcx+sizeof(qword)*7], rdx
   call     mred8_start
   mov      [rdi+sizeof(qword)*7], rax

   pop      rdx      ; resrore m'
   ret
ENDFUNC mred8x8_start



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; rdi - temporary product buffer/temporary reduction
;; rsi - modulus
;; r8  - m'
;; r15 - target reduction address
;;

;;
;; 1*qword modulus length
;;

;;
;; 2*qword modulus length
;;

;;
;; 3*qword modulus length
;;

;;
;; 4*qword modulus length
;;

;;
;; 5*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_5,PRIVATE
%assign MSIZE  5
   push     r8       ; m'

   ; load low half of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]

   ; ui = x[i]*m'
   ; reduction += ui*modulus
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred5_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred5_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred5_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred5_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred5_start

   pop      rax      ; remove m'

   ; finalize cf, reduction += U0*modulus operation
   xor      rax, rax
   op_reg_mem add, r8, [rdi+sizeof(qword)*(MSIZE+0)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+0)], r8
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE+1)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+1)], r9
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE+2)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+2)], r10
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE+3)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+3)], r11
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE+4)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+4)], r12
   adc      rax, 0

   ; reduction -= modulus
   op_reg_mem sub, r8, [rsi+sizeof(qword)*0], rbx
   op_reg_mem sbb, r9, [rsi+sizeof(qword)*1], rbx
   op_reg_mem sbb, r10,[rsi+sizeof(qword)*2], rbx
   op_reg_mem sbb, r11,[rsi+sizeof(qword)*3], rbx
   op_reg_mem sbb, r12,[rsi+sizeof(qword)*4], rbx
   sbb      rax, 0

   ; copy under cf   [r15] = cf? [r15] : {r8-r12}
   mov      rax, qword [rdi+sizeof(qword)*(MSIZE+0)]
   mov      rbx, qword [rdi+sizeof(qword)*(MSIZE+1)]
   mov      rcx, qword [rdi+sizeof(qword)*(MSIZE+2)]
   mov      rdx, qword [rdi+sizeof(qword)*(MSIZE+3)]
   mov      rbp, qword [rdi+sizeof(qword)*(MSIZE+4)]
   cmovae   rax, r8
   cmovae   rbx, r9
   cmovae   rcx, r10
   cmovae   rdx, r11
   cmovae   rbp, r12
   mov      qword [r15+sizeof(qword)*0], rax
   mov      qword [r15+sizeof(qword)*1], rbx
   mov      qword [r15+sizeof(qword)*2], rcx
   mov      qword [r15+sizeof(qword)*3], rdx
   mov      qword [r15+sizeof(qword)*4], rbp
   ret
ENDFUNC mred_5


;;
;; 6*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_6,PRIVATE
%assign MSIZE  6
   push     r8       ; m'

   ; load low half of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]

   ; ui = x[i]*m'
   ; reduction += ui*modulus
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred6_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred6_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred6_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred6_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred6_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred6_start

   pop      rax      ; remove m'

   ; finalize cf, reduction += U0*modulus operation
   xor      rax, rax
   op_reg_mem add, r8, [rdi+sizeof(qword)*(MSIZE+0)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+0)], r8
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE+1)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+1)], r9
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE+2)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+2)], r10
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE+3)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+3)], r11
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE+4)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+4)], r12
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(MSIZE+5)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+5)], r13
   adc      rax, 0

   ; reduction -= modulus
   op_reg_mem sub, r8, [rsi+sizeof(qword)*0], rbx
   op_reg_mem sbb, r9, [rsi+sizeof(qword)*1], rbx
   op_reg_mem sbb, r10,[rsi+sizeof(qword)*2], rbx
   op_reg_mem sbb, r11,[rsi+sizeof(qword)*3], rbx
   op_reg_mem sbb, r12,[rsi+sizeof(qword)*4], rbx
   op_reg_mem sbb, r13,[rsi+sizeof(qword)*5], rbx
   sbb      rax, 0

   ; copy under cf   [r15] = cf? [r15] : {r8-r13}
   mov      rax, qword [rdi+sizeof(qword)*(MSIZE+0)]
   mov      rbx, qword [rdi+sizeof(qword)*(MSIZE+1)]
   mov      rcx, qword [rdi+sizeof(qword)*(MSIZE+2)]
   mov      rdx, qword [rdi+sizeof(qword)*(MSIZE+3)]
   mov      rbp, qword [rdi+sizeof(qword)*(MSIZE+4)]
   mov      rsi, qword [rdi+sizeof(qword)*(MSIZE+5)]
   cmovae   rax, r8
   cmovae   rbx, r9
   cmovae   rcx, r10
   cmovae   rdx, r11
   cmovae   rbp, r12
   cmovae   rsi, r13
   mov      qword [r15+sizeof(qword)*0], rax
   mov      qword [r15+sizeof(qword)*1], rbx
   mov      qword [r15+sizeof(qword)*2], rcx
   mov      qword [r15+sizeof(qword)*3], rdx
   mov      qword [r15+sizeof(qword)*4], rbp
   mov      qword [r15+sizeof(qword)*5], rsi
   ret
ENDFUNC mred_6


;;
;; 7*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_7,PRIVATE
%assign MSIZE  7
   push     r8       ; m'

   ; load low half of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]

   ; ui = x[i]*m'
   ; reduction += ui*modulus
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred7_start

   pop      rax      ; remove m'

   ; finalize cf, reduction += U0*modulus operation
   xor      rax, rax
   op_reg_mem add, r8, [rdi+sizeof(qword)*(MSIZE+0)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+0)], r8
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE+1)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+1)], r9
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE+2)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+2)], r10
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE+3)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+3)], r11
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE+4)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+4)], r12
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(MSIZE+5)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+5)], r13
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(MSIZE+6)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+6)], r14
   adc      rax, 0

   ; reduction -= modulus
   op_reg_mem sub, r8, [rsi+sizeof(qword)*0], rbx
   op_reg_mem sbb, r9, [rsi+sizeof(qword)*1], rbx
   op_reg_mem sbb, r10,[rsi+sizeof(qword)*2], rbx
   op_reg_mem sbb, r11,[rsi+sizeof(qword)*3], rbx
   op_reg_mem sbb, r12,[rsi+sizeof(qword)*4], rbx
   op_reg_mem sbb, r13,[rsi+sizeof(qword)*5], rbx
   op_reg_mem sbb, r14,[rsi+sizeof(qword)*6], rbx
   sbb      rax, 0

   ; copy under cf   [r15] = cf? [r15] : {r8-r14}
   mov      rax, qword [rdi+sizeof(qword)*(MSIZE+0)]
   mov      rbx, qword [rdi+sizeof(qword)*(MSIZE+1)]
   mov      rcx, qword [rdi+sizeof(qword)*(MSIZE+2)]
   mov      rdx, qword [rdi+sizeof(qword)*(MSIZE+3)]
   mov      rbp, qword [rdi+sizeof(qword)*(MSIZE+4)]
   mov      rsi, qword [rdi+sizeof(qword)*(MSIZE+5)]
   mov      rdi, qword [rdi+sizeof(qword)*(MSIZE+6)]
   cmovae   rax, r8
   cmovae   rbx, r9
   cmovae   rcx, r10
   cmovae   rdx, r11
   cmovae   rbp, r12
   cmovae   rsi, r13
   cmovae   rdi, r14
   mov      qword [r15+sizeof(qword)*0], rax
   mov      qword [r15+sizeof(qword)*1], rbx
   mov      qword [r15+sizeof(qword)*2], rcx
   mov      qword [r15+sizeof(qword)*3], rdx
   mov      qword [r15+sizeof(qword)*4], rbp
   mov      qword [r15+sizeof(qword)*5], rsi
   mov      qword [r15+sizeof(qword)*6], rdi
   ret
ENDFUNC mred_7


;;
;; 8*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_8,PRIVATE
%assign MSIZE  8
   push     r15      ; save reduction address
   push     r8       ; m'

   ; load low half of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   ; ui = x[i]*m'
   ; reduction += ui*modulus
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start
   mov      rdx, [rsp]
   gsmulx   rbx, rdx, r8
   call     mred8_start

   pop      rax      ; remove m'

   ; finalize cf, reduction += U0*modulus operation
   xor      rax, rax
   op_reg_mem add, r8, [rdi+sizeof(qword)*(MSIZE+0)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+0)], r8
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE+1)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+1)], r9
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE+2)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+2)], r10
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE+3)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+3)], r11
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE+4)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+4)], r12
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(MSIZE+5)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+5)], r13
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(MSIZE+6)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+6)], r14
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(MSIZE+7)], rbx
   mov        qword [rdi+sizeof(qword)*(MSIZE+7)], r15
   adc      rax, 0

   ; reduction -= modulus
   op_reg_mem sub, r8, [rsi+sizeof(qword)*0], rbx
   op_reg_mem sbb, r9, [rsi+sizeof(qword)*1], rbx
   op_reg_mem sbb, r10,[rsi+sizeof(qword)*2], rbx
   op_reg_mem sbb, r11,[rsi+sizeof(qword)*3], rbx
   op_reg_mem sbb, r12,[rsi+sizeof(qword)*4], rbx
   op_reg_mem sbb, r13,[rsi+sizeof(qword)*5], rbx
   op_reg_mem sbb, r14,[rsi+sizeof(qword)*6], rbx
   op_reg_mem sbb, r15,[rsi+sizeof(qword)*7], rbx
   sbb      rax, 0

   pop      rsi         ; address of reduction
   ; copy under cf   [rsi] = cf? [rdi] : {r8-r15}
   mov      rax, qword [rdi+sizeof(qword)*(MSIZE+0)]
   mov      rbx, qword [rdi+sizeof(qword)*(MSIZE+1)]
   mov      rcx, qword [rdi+sizeof(qword)*(MSIZE+2)]
   mov      rdx, qword [rdi+sizeof(qword)*(MSIZE+3)]
   cmovae   rax, r8
   cmovae   rbx, r9
   cmovae   rcx, r10
   cmovae   rdx, r11
   mov      qword [rsi+sizeof(qword)*0], rax
   mov      qword [rsi+sizeof(qword)*1], rbx
   mov      qword [rsi+sizeof(qword)*2], rcx
   mov      qword [rsi+sizeof(qword)*3], rdx

   mov      rax, qword [rdi+sizeof(qword)*(MSIZE+4)]
   mov      rbx, qword [rdi+sizeof(qword)*(MSIZE+5)]
   mov      rcx, qword [rdi+sizeof(qword)*(MSIZE+6)]
   mov      rdx, qword [rdi+sizeof(qword)*(MSIZE+7)]
   cmovae   rax, r12
   cmovae   rbx, r13
   cmovae   rcx, r14
   cmovae   rdx, r15
   mov      qword [rsi+sizeof(qword)*4], rax
   mov      qword [rsi+sizeof(qword)*5], rbx
   mov      qword [rsi+sizeof(qword)*6], rcx
   mov      qword [rsi+sizeof(qword)*7], rdx
   ret
ENDFUNC mred_8


;;
;; 9*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_9,PRIVATE
%assign MSIZE  9
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x1
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x1_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14
   mov      r8, r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_1x1

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8,  rbx
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_9


;;
;; 10*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_10,PRIVATE
%assign MSIZE  10
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x2
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x2_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13
   mov      r8, r14
   mov      r9, r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6]
   adc      r9, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_2x2

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8, rbx
   adc      r9, 0
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_10


;;
;; 11*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_11,PRIVATE
%assign MSIZE  11
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x3
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x3_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      r8, r13
   mov      r9, r14
   mov      r10,r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5]
   adc      r9, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6]
   adc      r10,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_3x3

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8, rbx
   adc      r9, 0
   adc      r10,0
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_11


;;
;; 12*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_12,PRIVATE
%assign MSIZE  12
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x4
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x4_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      r8, r12
   mov      r9, r13
   mov      r10,r14
   mov      r11,r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4]
   adc      r9, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5]
   adc      r10,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6]
   adc      r11,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_4x4

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8, rbx
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_12


;;
;; 13*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_13,PRIVATE
%assign MSIZE  13
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x5
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x5_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      r8, r11
   mov      r9, r12
   mov      r10,r13
   mov      r11,r14
   mov      r12,r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3]
   adc      r9, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4]
   adc      r10,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5]
   adc      r11,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6]
   adc      r12,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_5x5

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8, rbx
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   adc      r12,0
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_13


;;
;; 14*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_14,PRIVATE
%assign MSIZE  14
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x6
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x6_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      r8, r10
   mov      r9, r11
   mov      r10,r12
   mov      r11,r13
   mov      r12,r14
   mov      r13,r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2]
   adc      r9, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3]
   adc      r10,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4]
   adc      r11,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5]
   adc      r12,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6]
   adc      r13,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_6x6

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8, rbx
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   adc      r12,0
   adc      r13,0
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_14


;;
;; 15*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_15,PRIVATE
%assign MSIZE  15
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   push     r8                      ; save m'
   mov      rdx, r8

;;
;; init pass
;;

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax    ; init carryLCL
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   SWAP     rcx, rsi
   call     mla_8x7
   SWAP     rcx, rsi

   pop      rax
   shr      rax, 1
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1],  r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], r15, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7], rbx
   adc      rax, 0
   push     rax         ; store carryGBL

;;
;; last pass
;;
   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rdx, [rsp+sizeof(qword)]
   call     mred8x7_start

   xor      rax, rax    ; init carryLCL
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      r8, r9
   mov      r9, r10
   mov      r10,r11
   mov      r11,r12
   mov      r12,r13
   mov      r13,r14
   mov      r14,r15
   add      r8, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1]
   adc      r9, qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2]
   adc      r10,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3]
   adc      r11,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4]
   adc      r12,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5]
   adc      r13,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6]
   adc      r14,qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*7]
   adc      rax, 0
   push     rax         ; store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_7x7

   pop      rax         ; restore carryLCL
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], rbx
   adc      rax, 0

   pop      rbx         ; carryGBL
   add      r8, rbx
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   adc      r12,0
   adc      r13,0
   adc      r14,0
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*5], r13
   mov      qword [rdi+sizeof(qword)*(MSIZE-8)+sizeof(qword)*6], r14
   adc      rax, 0      ; update carryGBL


   pop      rcx                     ; remove m'
   add      rsp, sizeof(qword)*8    ; release U space

   lea      rcx, [rsi-sizeof(qword)*8]                            ; restore modulus
   lea      rsi, [rdi+sizeof(qword)*(MSIZE-8)-sizeof(qword)*8]    ; restore buffer
   pop      rdi                                                   ; restore reduction

   mov      rbx, rax       ; save carryGBL

   mov      rdx, dword MSIZE
   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   sub      rdi, sizeof(qword)*MSIZE      ; reduction
   sub      rsi, sizeof(qword)*MSIZE      ; buffer (src1)
   mov      rcx, rdi                      ; reduction (src2)
   mov      rdx, dword MSIZE
   shr      rbx,1          ; restore cf
   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_15


;;
;; 16*qword modulus length
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_16,PRIVATE
%assign MSIZE  16
   push     r15                     ; save reduction address

   sub      rsp, sizeof(qword)*8    ; allocate U space
   mov      rcx, rsp

   mov      rdx, r8                 ; copy m'

   ; load low part of the product
   mov      r8, qword [rdi]     ; x0,x1,x2,x3,x4,x5,x6,x7
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax

   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8

   push     rdx
   call     mla_8x8
   pop      rdx

   pop      rax
   shr      rax, 1
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+0)], r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+1)], r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+2)], r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+3)], r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+4)], r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+5)], r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+6)], r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_mem_reg_mem adc, [rdi+sizeof(qword)*(8+7)], r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax

   sub      rsi, sizeof(qword)*8

   mov      r8, qword [rdi]
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start

   xor      rax, rax
   op_reg_mem add, r8, [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0
   push     rax

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8

   call     mla_8x8

   sub      rsi, sizeof(qword)*8

   pop      rax
   shr      rax, 1
   op_reg_mem adc, r8,  [rdi+sizeof(qword)*(8+0)], rbx
   op_reg_mem adc, r9,  [rdi+sizeof(qword)*(8+1)], rbx
   op_reg_mem adc, r10, [rdi+sizeof(qword)*(8+2)], rbx
   op_reg_mem adc, r11, [rdi+sizeof(qword)*(8+3)], rbx
   op_reg_mem adc, r12, [rdi+sizeof(qword)*(8+4)], rbx
   op_reg_mem adc, r13, [rdi+sizeof(qword)*(8+5)], rbx
   op_reg_mem adc, r14, [rdi+sizeof(qword)*(8+6)], rbx
   op_reg_mem adc, r15, [rdi+sizeof(qword)*(8+7)], rbx
   adc      rax, 0

   pop      rbx
   add      r8,  rbx
   adc      r9,  0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0
   adc      r15, 0
   adc      rax, 0

   mov      qword [rdi+sizeof(qword)*(8+0)], r8
   mov      qword [rdi+sizeof(qword)*(8+1)], r9
   mov      qword [rdi+sizeof(qword)*(8+2)], r10
   mov      qword [rdi+sizeof(qword)*(8+3)], r11
   mov      qword [rdi+sizeof(qword)*(8+4)], r12
   mov      qword [rdi+sizeof(qword)*(8+5)], r13
   mov      qword [rdi+sizeof(qword)*(8+6)], r14
   mov      qword [rdi+sizeof(qword)*(8+7)], r15

   add      rsp, sizeof(qword)*8    ; release U space

   pop      rbp

   ; reduction -= modulus
   op_mem_mem  sub,   [rbp+sizeof(qword)*0 ], [rdi+sizeof(qword)*0 ], [rsi+sizeof(qword)*0 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*1 ], [rdi+sizeof(qword)*1 ], [rsi+sizeof(qword)*1 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*2 ], [rdi+sizeof(qword)*2 ], [rsi+sizeof(qword)*2 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*3 ], [rdi+sizeof(qword)*3 ], [rsi+sizeof(qword)*3 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*4 ], [rdi+sizeof(qword)*4 ], [rsi+sizeof(qword)*4 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*5 ], [rdi+sizeof(qword)*5 ], [rsi+sizeof(qword)*5 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*6 ], [rdi+sizeof(qword)*6 ], [rsi+sizeof(qword)*6 ], rbx
   op_mem_mem  sbb,   [rbp+sizeof(qword)*7 ], [rdi+sizeof(qword)*7 ], [rsi+sizeof(qword)*7 ], rbx
   op_reg_mem  sbb, r8,  [rsi+sizeof(qword)*8 ], rbx
   op_reg_mem  sbb, r9,  [rsi+sizeof(qword)*9 ], rbx
   op_reg_mem  sbb, r10, [rsi+sizeof(qword)*10], rbx
   op_reg_mem  sbb, r11, [rsi+sizeof(qword)*11], rbx
   op_reg_mem  sbb, r12, [rsi+sizeof(qword)*12], rbx
   op_reg_mem  sbb, r13, [rsi+sizeof(qword)*13], rbx
   op_reg_mem  sbb, r14, [rsi+sizeof(qword)*14], rbx
   op_reg_mem  sbb, r15, [rsi+sizeof(qword)*15], rbx
   sbb      rax, 0

   ; copy under cf   [rbp] = cf? [rdi] : [rbp]
   mov      rax, qword [rdi+sizeof(qword)*8 ]
   mov      rbx, qword [rdi+sizeof(qword)*9 ]
   mov      rcx, qword [rdi+sizeof(qword)*10]
   mov      rdx, qword [rdi+sizeof(qword)*11]
   cmovae   rax, r8
   cmovae   rbx, r9
   cmovae   rcx, r10
   cmovae   rdx, r11
   mov      qword [rbp+sizeof(qword)*8 ], rax
   mov      qword [rbp+sizeof(qword)*9 ], rbx
   mov      qword [rbp+sizeof(qword)*10], rcx
   mov      qword [rbp+sizeof(qword)*11], rdx

   mov      rax, qword [rdi+sizeof(qword)*12]
   mov      rbx, qword [rdi+sizeof(qword)*13]
   mov      rcx, qword [rdi+sizeof(qword)*14]
   mov      rdx, qword [rdi+sizeof(qword)*15]
   cmovae   rax, r12
   cmovae   rbx, r13
   cmovae   rcx, r14
   cmovae   rdx, r15
   mov      qword [rbp+sizeof(qword)*12], rax
   mov      qword [rbp+sizeof(qword)*13], rbx
   mov      qword [rbp+sizeof(qword)*14], rcx
   mov      qword [rbp+sizeof(qword)*15], rdx

   mov      r8,  qword [rbp+sizeof(qword)*0]
   mov      r9,  qword [rbp+sizeof(qword)*1]
   mov      r10, qword [rbp+sizeof(qword)*2]
   mov      r11, qword [rbp+sizeof(qword)*3]
   mov      r12, qword [rbp+sizeof(qword)*4]
   mov      r13, qword [rbp+sizeof(qword)*5]
   mov      r14, qword [rbp+sizeof(qword)*6]
   mov      r15, qword [rbp+sizeof(qword)*7]

   mov      rax, qword [rdi+sizeof(qword)*0]
   mov      rbx, qword [rdi+sizeof(qword)*1]
   mov      rcx, qword [rdi+sizeof(qword)*2]
   mov      rdx, qword [rdi+sizeof(qword)*3]
   cmovae   rax, r8
   cmovae   rbx, r9
   cmovae   rcx, r10
   cmovae   rdx, r11
   mov      qword [rbp+sizeof(qword)*0], rax
   mov      qword [rbp+sizeof(qword)*1], rbx
   mov      qword [rbp+sizeof(qword)*2], rcx
   mov      qword [rbp+sizeof(qword)*3], rdx

   mov      rax, qword [rdi+sizeof(qword)*4]
   mov      rbx, qword [rdi+sizeof(qword)*5]
   mov      rcx, qword [rdi+sizeof(qword)*6]
   mov      rdx, qword [rdi+sizeof(qword)*7]
   cmovae   rax, r12
   cmovae   rbx, r13
   cmovae   rcx, r14
   cmovae   rdx, r15
   mov      qword [rbp+sizeof(qword)*4], rax
   mov      qword [rbp+sizeof(qword)*5], rbx
   mov      qword [rbp+sizeof(qword)*6], rcx
   mov      qword [rbp+sizeof(qword)*7], rdx

   ret
ENDFUNC mred_16



mred_short  DQ    mred_5  - mred_short
            DQ    mred_6  - mred_short
            DQ    mred_7  - mred_short
            DQ    mred_8  - mred_short
            DQ    mred_9  - mred_short
            DQ    mred_10 - mred_short
            DQ    mred_11 - mred_short
            DQ    mred_12 - mred_short
            DQ    mred_13 - mred_short
            DQ    mred_14 - mred_short
            DQ    mred_15 - mred_short
            DQ    mred_16 - mred_short

mred8x_start   DQ mred8x1_start - mred8x_start
               DQ mred8x2_start - mred8x_start
               DQ mred8x3_start - mred8x_start
               DQ mred8x4_start - mred8x_start
               DQ mred8x5_start - mred8x_start
               DQ mred8x6_start - mred8x_start
               DQ mred8x7_start - mred8x_start
