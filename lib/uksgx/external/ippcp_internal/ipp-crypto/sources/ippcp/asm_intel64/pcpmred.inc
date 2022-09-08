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

%ifndef _PCPMRED_INC_
%assign _PCPMRED_INC_  1

%include "pcpmred_basic.inc"


;
;  r15 - reduction
;  rdi - buffer
;  rsi - modulus
;  rdx - modulus legth
;  r8  - m'
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_8N,PRIVATE
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8 ; allocate U space
   mov      rcx, rsp

   mov      rbx, rdx ; number of passes == modulus length

   xor      rax, rax ; init global carry

.passLoop:
                     ; save:
   push     rdi      ;  - buffer address
   push     rsi      ;  - modulus address
   push     rdx      ;  - modulus length
   push     r8       ;  - m'
   push     rbx      ;  - rest of passes
   push     rax      ;  - global carry

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;
   ; signgle pass
   ;
   push     rdx      ; init modulus counter
   mov      rdx, r8  ; copy m'

   ; load low part of the product into r8,r9,r10,r11,r12,r13,r14,r15
   mov      r8, qword [rdi]
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start
   pop      rdx      ; restore modulus counter

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

   jmp      .entryInnerLoop

.innerLoop:
   push     rdx         ; save modulus counter
   call     mla_8x8
   pop      rdx         ; restore modulus counter

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

.entryInnerLoop:
   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8
   sub      rdx, 8
   jg       .innerLoop

   ; single pass completion
   pop      rax         ; new global carry

   pop      rbx         ; prev global carry
   add      r8,  rbx
   adc      r9,  0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0
   adc      r15, 0
   adc      rax, 0      ; update global carry

   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*5], r13
   mov      qword [rdi+sizeof(qword)*6], r14
   mov      qword [rdi+sizeof(qword)*7], r15
   ;
   ; end of single pass
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                     ; restore:
   pop      rbx      ;  - rest of passes
   pop      r8       ;  - m'
   pop      rdx      ;  - modulus length
   pop      rsi      ;  - modulus address
   pop      rdi      ;  - buffer address
   add      rdi, sizeof(qword)*8
   sub      rbx, 8
   jg       .passLoop

   add      rsp, sizeof(qword)*8    ; release U space

   mov      r14, rdx                ; save modulus length
   lea      r15, [rdx*sizeof(qword)]; save modulus length (bytes)

   mov      rbx, rax       ; global carry
   mov      rcx, rsi       ; modulus
   mov      rsi, rdi       ; buffer
   pop      rdi            ; reduction address

   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   mov      rdx, r14       ; length
   sub      rdi, r15       ; reduction
   sub      rsi, r15       ; buffer (src1)
   mov      rcx, rdi       ; reduction (src2)
   shr      rbx,1          ; restore cf

   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_8N


;
;  r15 - reduction
;  rdi - buffer
;  rsi - modulus
;  rdx - modulus legth
;  r8  - m'
align IPP_ALIGN_FACTOR
DECLARE_FUNC mred_N,PRIVATE
   push     r15      ; save reduction address

   sub      rsp, sizeof(qword)*8 ; allocate U space
   mov      rcx, rsp

   mov      rbx, rdx ; number of passes counter == (modulus_length -8)
   sub      rbx, 8

   xor      rax, rax ; init carryGBL

   mov      r15, dword 7                  ; n = modulus_len%8
   and      r15, rdx
   GET_EP   rbp, mla_8xl_tail, r15        ; tail procedure (mla_8xn) address

.passLoop:
                     ; save:
   push     rdi      ;  - buffer address
   push     rsi      ;  - modulus address
   push     rdx      ;  - modulus length
   push     r8       ;  - m'
   push     rbx      ;  - rest of passes
   push     rax      ;  - carryGBL
   push     rbp      ;  - mla_8xn procedure

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; regular pass
;;
   sub      rdx, 8   ; init modulus counter
   push     rdx
   mov      rdx, r8  ; copy m'

   ; load low part of the product into r8,r9,r10,r11,r12,r13,r14,r15
   mov      r8, qword [rdi]
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   call     mred8x8_start
   pop      rdx         ; restore modulus counter

   xor      rax, rax    ; init carryLCL
   push     rax

   jmp      .entryInnerLoop

.innerLoop:
   push     rdx         ; save modulus counter
   call     mla_8x8
   pop      rdx         ; restore modulus counter

.entryInnerLoop:
   pop      rax         ; restore carryLCL
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
   push     rax         ; update store carryLCL

   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8
   sub      rdx, 8
   jnc      .innerLoop

   add      rdx, 8
   jz       .complete_regular_pass

   ; tail of single pass
   mov      rax, [rsp+sizeof(qword)]      ; procedure address

   SWAP     rcx, rsi
   push     rdx
   call     rax
   pop      rdx
   SWAP     rcx, rsi

   lea      rdi, [rdi+rdx*sizeof(qword)]
   pop      rax         ; restore carryLCL
   shr      rax, 1
   mov      rbx, rdx

   dec      rbx
   jz       .mt_1
   dec      rbx
   jz       .mt_2
   dec      rbx
   jz       .mt_3
   dec      rbx
   jz       .mt_4
   dec      rbx
   jz       .mt_5
   dec      rbx
   jz       .mt_6
.mt_7: op_reg_mem adc, r9, [rdi+sizeof(qword)*1], rbx
.mt_6: op_reg_mem adc, r10,[rdi+sizeof(qword)*2], rbx
.mt_5: op_reg_mem adc, r11,[rdi+sizeof(qword)*3], rbx
.mt_4: op_reg_mem adc, r12,[rdi+sizeof(qword)*4], rbx
.mt_3: op_reg_mem adc, r13,[rdi+sizeof(qword)*5], rbx
.mt_2: op_reg_mem adc, r14,[rdi+sizeof(qword)*6], rbx
.mt_1: op_reg_mem adc, r15,[rdi+sizeof(qword)*7], rbx

   adc   rax, 0         ; update carryLCL
   push  rax

   ; single pass completion
.complete_regular_pass:
   pop      rax         ; carryLCL
   pop      rbp         ; mla_8xn procedure

   pop      rbx         ; carryGBL
   add      r8,  rbx
   adc      r9,  0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0
   adc      r15, 0
   adc      rax, 0      ; update carryGBL

   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2], r10
   mov      qword [rdi+sizeof(qword)*3], r11
   mov      qword [rdi+sizeof(qword)*4], r12
   mov      qword [rdi+sizeof(qword)*5], r13
   mov      qword [rdi+sizeof(qword)*6], r14
   mov      qword [rdi+sizeof(qword)*7], r15
;;
;; end of regular pass
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                     ; restore:
   pop      rbx      ;  - rest of passes
   pop      r8       ;  - m'
   pop      rdx      ;  - modulus length
   pop      rsi      ;  - modulus address
   pop      rdi      ;  - buffer address
   add      rdi, sizeof(qword)*8
   sub      rbx, 8
   jnc      .passLoop

   add      rbx, 8
   jz       .complete_reduction

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; tail pass of reduction
;;
                     ; save:
   push     rdi      ;  - buffer address
   push     rsi      ;  - modulus address
   push     rdx      ;  - modulus length
   push     r8       ;  - m'
   push     rbx      ;  - rest of passes
   push     rax      ;  - carry
   push     rbp      ;  - mla_8xn procedure

   sub      rdx, 8   ; init modulus counter
   push     rdx
   mov      rdx, r8  ; copy m'

   ; load low part of the product into r8,r9,r10,r11,r12,r13,r14,r15
   mov      r8, qword [rdi]
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   GET_EP   rax, mred8x_start, rbx, rbp   ; procedure mred8xn_start address

   call     rax
   pop      rdx      ; restore modulus counter

   xor      rax, rax    ; init carryCLC
   push     rax

   jmp      .entryTailLoop

.tailLoop:
   mov      r8, qword [rdi]
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      rax, [rsp+sizeof(qword)]      ; procedure address
   push     rdx         ; save modulus counter
   call     rax
   pop      rdx         ; restore modulus counter

.entryTailLoop:
   pop      rax         ; restore carryLCL
   shr      rax, 1
   adc      r8,  0
   adc      r9,  0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0
   adc      r15, 0
   adc      rax, 0      ; update carryGBL

   mov      rbx, [rsp+sizeof(qword)*2]    ; modulus_len%7
   cmp      rbx, 1
   jz       .tt_1
   cmp      rbx, 2
   jz       .tt_2
   cmp      rbx, 3
   jz       .tt_3
   cmp      rbx, 4
   jz       .tt_4
   cmp      rbx, 5
   jz       .tt_5
   cmp      rbx, 6
   jz       .tt_6
.tt_7: op_reg_mem adc, r9, [rdi+rbx*sizeof(qword)+sizeof(qword)*1], rbp
.tt_6: op_reg_mem adc, r10,[rdi+rbx*sizeof(qword)+sizeof(qword)*2], rbp
.tt_5: op_reg_mem adc, r11,[rdi+rbx*sizeof(qword)+sizeof(qword)*3], rbp
.tt_4: op_reg_mem adc, r12,[rdi+rbx*sizeof(qword)+sizeof(qword)*4], rbp
.tt_3: op_reg_mem adc, r13,[rdi+rbx*sizeof(qword)+sizeof(qword)*5], rbp
.tt_2: op_reg_mem adc, r14,[rdi+rbx*sizeof(qword)+sizeof(qword)*6], rbp
.tt_1: op_reg_mem adc, r15,[rdi+rbx*sizeof(qword)+sizeof(qword)*7], rbp

   adc      rax, 0
   push     rax         ; update store carryLCL

   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*0], r8
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*1], r9
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*2], r10
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*3], r11
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*4], r12
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*5], r13
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*6], r14
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*7], r15

   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   sub      rdx, 8
   jnc      .tailLoop

   add      rdx, 8

   ; load data depending on length of tail
   mov      rbx, rdx

   mov      r8,  qword [rdi]
   dec      rbx
   jz       .get_tail_proc
   mov      r9,  qword [rdi+sizeof(qword)]
   dec      rbx
   jz       .get_tail_proc
   mov      r10, qword [rdi+sizeof(qword)*2]
   dec      rbx
   jz       .get_tail_proc
   mov      r11, qword [rdi+sizeof(qword)*3]
   dec      rbx
   jz       .get_tail_proc
   mov      r12, qword [rdi+sizeof(qword)*4]
   dec      rbx
   jz       .get_tail_proc
   mov      r13, qword [rdi+sizeof(qword)*5]
   dec      rbx
   jz       .get_tail_proc
   mov      r14, qword [rdi+sizeof(qword)*6]

.get_tail_proc:
   ; multiply
;  GET_EP   rax, mla_8xl_tail, rdx, rbp   ; tail procedure mla_8xn address
   GET_EP   rax, mla_lxl_short, rdx, rbp  ; tail procedure mla_lxl address

   push     rdx
   call     rax   ; mla_nxn
   pop      rdx

   lea      rdi, [rdi+rdx*sizeof(qword)]

   ; accumulate carryLCL and update hight product above
   pop      rax
   shr      rax, 1
   mov      rbx, rdx
   op_reg_mem adc, r8, [rdi+sizeof(qword)*0], rbp
   dec      rbx
   jz       .add_carry1
   op_reg_mem adc, r9, [rdi+sizeof(qword)*1], rbp
   dec      rbx
   jz       .add_carry1
   op_reg_mem adc, r10,[rdi+sizeof(qword)*2], rbp
   dec      rbx
   jz       .add_carry1
   op_reg_mem adc, r11,[rdi+sizeof(qword)*3], rbp
   dec      rbx
   jz       .add_carry1
   op_reg_mem adc, r12,[rdi+sizeof(qword)*4], rbp
   dec      rbx
   jz       .add_carry1
   op_reg_mem adc, r13,[rdi+sizeof(qword)*5], rbp
   dec      rbx
   jz       .add_carry1
   op_reg_mem adc, r14,[rdi+sizeof(qword)*6], rbp
.add_carry1:
   adc      rax, 0

   pop      rbp         ; mul_8xn procedure

   ; accumulate carryGBL and store hight product above
   pop      rbx         ; carryGBL
   add      r8,  rbx
   mov      qword [rdi+sizeof(qword)*0], r8
   dec      rdx
   jz       .add_carry2
   adc      r9,  0
   mov      qword [rdi+sizeof(qword)*1], r9
   dec      rdx
   jz       .add_carry2
   adc      r10, 0
   mov      qword [rdi+sizeof(qword)*2], r10
   dec      rdx
   jz       .add_carry2
   adc      r11, 0
   mov      qword [rdi+sizeof(qword)*3], r11
   dec      rdx
   jz       .add_carry2
   adc      r12, 0
   mov      qword [rdi+sizeof(qword)*4], r12
   dec      rdx
   jz       .add_carry2
   adc      r13, 0
   mov      qword [rdi+sizeof(qword)*5], r13
   dec      rdx
   jz       .add_carry2
   adc      r14, 0
   mov      qword [rdi+sizeof(qword)*6], r14
.add_carry2:
   adc      rax, 0      ; update carryGBL


   ; release stack
   pop      rbx      ;  - passes counter
   pop      r8       ;  - m'
   pop      rdx      ;  - modulus length
   pop      rsi      ;  - modulus
   pop      rdi      ;  - buffer
   lea      rdi, [rdi+rbx*sizeof(qword)]

.complete_reduction:
   add      rsp, sizeof(qword)*8    ; release U space

   mov      r14, rdx                ; save modulus length
   lea      r15, [rdx*sizeof(qword)]; save modulus length (bytes)

   mov      rbx, rax       ; carry
   mov      rcx, rsi       ; modulus
   mov      rsi, rdi       ; buffer
   pop      rdi            ; reduction address

   call     sub_N          ; reduction = buffer - modulus
   sub      rbx, rax       ; rbx = borrow

   mov      rdx, r14       ; length
   sub      rdi, r15       ; reduction
   sub      rsi, r15       ; buffer (src1)
   mov      rcx, rdi       ; reduction (src2)
   shr      rbx,1          ; restore cf

   call     copy_ae_N      ; copy under cf, reduction = cf? buffer : reduction

   ret
ENDFUNC mred_N


%endif ;; _PCPMRED_INC_
