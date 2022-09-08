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
;               Low level Big Number multiplication Support
;
;

%ifndef _PCPBNUMUL_INC_
%assign _PCPBNUMUL_INC_  1

%include "pcpbnumulpp_basic.inc"

%macro CLEAR 2.nolist
  %xdefine %%rPtr %1
  %xdefine %%rLen %2

%%L_1:
   mov   qword [%%rPtr], rax
   add   %%rPtr, sizeof(qword)
   sub   %%rLen, 1
   jnz   %%L_1
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; (8*n)x(8*m) multiplier
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_8Nx8M_adcox,PRIVATE
   push     rbx   ;; nsB
   push     rdi   ;; pR
   push     rsi   ;; pA
   push     rdx   ;; nsA

;;;
;;; init
;;;
.mul_loopA:
   push     rdx   ;; nsA
   call     mla_8x8
   add      rdi, sizeof(qword)*8    ; adv pR
   add      rsi, sizeof(qword)*8    ; adv pA
   pop      rdx                     ; nsA--
   sub      rdx, 8
   jnz      .mul_loopA

   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   mov      qword [rdi+sizeof(qword)*7],r15

   jmp      .mla_entry

.mla_loopB:
   push     rbx   ;; nsB

   push     rdi   ;; pR
   push     rsi   ;; pA
   push     rdx   ;; nsA

   xor      rax, rax ; c-flag
   push     rax

   mov      r8, qword [rdi+sizeof(qword)*0]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

.loopA:
   push     rdx                  ; nsA
   call     mla_8x8
   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8
   pop      rdx                  ; nsA--
   sub      rdx, 8
   jz       .exit_loopA

   pop      rax                  ; restore c-flag
   neg      rax
   op_reg_mem adc, r8, qword [rdi+sizeof(qword)*0], rax
   op_reg_mem adc, r9, qword [rdi+sizeof(qword)*1], rax
   op_reg_mem adc, r10,qword [rdi+sizeof(qword)*2], rax
   op_reg_mem adc, r11,qword [rdi+sizeof(qword)*3], rax
   op_reg_mem adc, r12,qword [rdi+sizeof(qword)*4], rax
   op_reg_mem adc, r13,qword [rdi+sizeof(qword)*5], rax
   op_reg_mem adc, r14,qword [rdi+sizeof(qword)*6], rax
   op_reg_mem adc, r15,qword [rdi+sizeof(qword)*7], rax
   sbb      rax, rax             ; save c-flag
   push     rax
   jmp      .loopA

.exit_loopA:
   pop      rax                  ; restore c-flag
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

.mla_entry:
   pop      rdx   ; restore nsA
   pop      rsi   ; restore pA
   pop      rdi
   add      rdi, sizeof(qword)*8

   add      rcx, sizeof(qword)*8
   pop      rbx   ;; nsB--
   sub      rbx, 8
   jnz      .mla_loopB
   ret
ENDFUNC mul_8Nx8M_adcox


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; simplest general case N x M multiplier
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mla_simple,PRIVATE
   xor      rax, rax       ; carry = 0

   mov      r11, rdx
   cmp      r11, rbx
   jge      .ms_mla_entry
   SWAP     r11, rbx
   SWAP     rsi, rcx
   jmp      .ms_mla_entry

.ms_loopB:
   push     rbx   ;; nsB
   push     rdi   ;; pR
   push     rsi   ;; pA
   push     r11   ;; nsA
   push     rax   ;; save previous pass carry

   mov      rdx, qword [rcx]  ; pB[]
   xor      r10, r10             ; extension
.ms_loopA:
   gsmulx   r9, r8, qword [rsi]
   add      rdi, sizeof(qword)
   add      rsi, sizeof(qword)
   add      r8, r10
   adc      r9, 0
   add      r8, qword [rdi-sizeof(qword)]
   adc      r9, 0
   mov      qword [rdi-sizeof(qword)], r8
   mov      r10, r9
   sub      r11, 1
   jnz      .ms_loopA

   pop      rax                  ; restore carry
   shr      rax, 1

   adc      r10, qword [rdi]
   mov      qword [rdi], r10
   adc      rax, 0               ; save carry

   pop      r11   ; restore nsA
   pop      rsi   ; restore pA
   pop      rdi   ; restore pR
   pop      rbx   ; nsB

   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)

.ms_mla_entry:
   sub      rbx, 1
   jnc      .ms_loopB
   ret
ENDFUNC mla_simple



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; general case N x M multiplier
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC mul_NxM_adcox,PRIVATE
;
; stack struct
;
%assign nsB_cnt   0                      ; rest of B operand
%assign ptrR      nsB_cnt+sizeof(qword)  ; current product pointer
%assign ptrA      ptrR+sizeof(qword)     ; ponter to A operand (pA)
%assign nsA       ptrA+sizeof(qword)     ; length of A operand (nsA)
%assign nsA_cnt   nsA+sizeof(qword)      ; rest of A operand
%assign carry     nsA_cnt+sizeof(qword)  ; carry
%assign tailproc  carry+sizeof(qword)    ; mla_8xn procedure
%assign stack_mem tailproc+sizeof(qword) ; size of stack allocation

   sub      rsp, stack_mem       ; allocate stack

   cmp      rbx, 8
   jge      .regular_entry
   cmp      rdx, 8
   jge      .irregular_entry

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; degradated case
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ; clear product
   mov      r8, rdx
   add      r8, rbx
   mov      rbp, rdi
   xor      rax, rax
   CLEAR    rbp, r8

   call     mla_simple
   jmp      .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; irregular init
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.irregular_entry:
   mov      [rsp+nsB_cnt], rbx
   mov      [rsp+nsA], rdx
   mov      [rsp+nsA_cnt], rdx

   GET_EP   rax, mla_8xl_tail, rbx, rbp   ; tail procedure (mla_8xn) address
   mov      [rsp+tailproc], rax

   jmp      .irr_init_entry

.irr_init_loop:
   mov      [rsp+nsA_cnt], rdx   ; save A counter
   call     rax

   mov      rbx, [rsp+nsB_cnt]
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

   xor      r8, r8
   xor      r9, r9
   xor      r10,r10
   xor      r11,r11
   xor      r12,r12
   xor      r13,r13
   xor      r14,r14
   xor      r15,r15

   ; load data depending on nsB length
   mov      r8,  qword [rdi]
   cmp      rbx, 1
   jz       .continue
   mov      r9,  qword [rdi+sizeof(qword)]
   cmp      rbx, 2
   jz       .continue
   mov      r10, qword [rdi+sizeof(qword)*2]
   cmp      rbx, 3
   jz       .continue
   mov      r11, qword [rdi+sizeof(qword)*3]
   cmp      rbx, 4
   jz       .continue
   mov      r12, qword [rdi+sizeof(qword)*4]
   cmp      rbx, 5
   jz       .continue
   mov      r13, qword [rdi+sizeof(qword)*5]
   cmp      rbx, 6
   jz       .continue
   mov      r14, qword [rdi+sizeof(qword)*6]
.continue:
   mov      rdx, [rsp+nsA_cnt]   ; nsA

.irr_init_entry:
   sub      rdx, 8
   mov      rax, [rsp+tailproc]
   jnc      .irr_init_loop

   add      rdx, 8
   jz       .quit

   ; clear uninitialized rest of product
   lea      rbp, [rdi+rbx*sizeof(qword)]
   xor      rax, rax
   CLEAR    rbp, rdx

   mov      rdx, [rsp+nsA_cnt]
   call     mla_simple
   jmp      .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; regular ep
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.regular_entry:
   sub      rbx, 8
   xor      rax, rax
   mov      [rsp+nsB_cnt], rbx
   mov      [rsp+ptrR], rdi
   mov      [rsp+ptrA], rsi
   mov      [rsp+nsA], rdx
   mov      [rsp+nsA_cnt], rdx
   mov      [rsp+carry], rax

   mov      rbp, rdx                ; n = nsA %8
   and      rbp, 7
   GET_EP   rax, mla_8xl_tail, rbp  ; tail procedure (mla_8xn) address
   mov      [rsp+tailproc], rax

;;
;; regular init
;;
   sub      rdx, 8   ;; nsA counter
.init_loopA:
   mov      [rsp+nsA_cnt], rdx      ; nsA
   call     mla_8x8
   mov      rdx, [rsp+nsA_cnt]
   add      rdi, sizeof(qword)*8    ; adv ptrR
   add      rsi, sizeof(qword)*8    ; adv ptrA
   sub      rdx, 8                  ; nsA -= 8
   jnc      .init_loopA

   add      rdx, 8
   jz       .init_complete

   mov      [rsp+nsA_cnt], rdx

   mov      rax, [rsp+tailproc]
   SWAP     rcx, rsi
   call     rax
   SWAP     rcx, rsi

   mov      rdx, [rsp+nsA_cnt]
   lea      rdi, [rdi+rdx*sizeof(qword)]

.init_complete:
   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   mov      qword [rdi+sizeof(qword)*7],r15

   jmp      .mla_entry

;;
;; regular mla passes
;;
.mla_loopB:
   mov      [rsp+nsB_cnt], rbx   ; update B conter
   mov      [rsp+ptrR], rdi        ; update current product pointer

   xor      rax, rax             ; init carry
   mov      [rsp+carry], rax

   mov      r8, qword [rdi+sizeof(qword)*0]
   mov      r9, qword [rdi+sizeof(qword)*1]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   sub      rdx, 8
.loopA:
   mov      [rsp+nsA_cnt], rdx   ; save A counter
   call     mla_8x8
   mov      rdx, [rsp+nsA_cnt]   ; A counter
   add      rdi, sizeof(qword)*8
   add      rsi, sizeof(qword)*8
   sub      rdx, 8               ; nsA -= 8
   jc       .exit_loopA

   mov      rax, [rsp+carry]     ; restore carry
   shr      rax, 1
   op_reg_mem adc, r8, qword [rdi+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, qword [rdi+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,qword [rdi+sizeof(qword)*2], rbx
   op_reg_mem adc, r11,qword [rdi+sizeof(qword)*3], rbx
   op_reg_mem adc, r12,qword [rdi+sizeof(qword)*4], rbx
   op_reg_mem adc, r13,qword [rdi+sizeof(qword)*5], rbx
   op_reg_mem adc, r14,qword [rdi+sizeof(qword)*6], rbx
   op_reg_mem adc, r15,qword [rdi+sizeof(qword)*7], rbx
   adc      rax, 0               ; save carry
   mov      [rsp+carry], rax
   jmp      .loopA

.exit_loopA:
   add      rdx, 8
   jz       .complete_reg_loopB

   mov      [rsp+nsA_cnt], rdx

   ; put zeros
   xor      rax, rax
.put_zero:
   mov      qword [rdi+rdx*sizeof(qword)], rax
   add      rdx,1
   cmp      rdx, 8
   jl       .put_zero

   mov      rax, [rsp+carry]     ; restore carry
   shr      rax, 1
   op_reg_mem adc, r8, [rdi+sizeof(qword)*0], rbx
   op_reg_mem adc, r9, [rdi+sizeof(qword)*1], rbx
   op_reg_mem adc, r10,[rdi+sizeof(qword)*2], rbx
   op_reg_mem adc, r11,[rdi+sizeof(qword)*3], rbx
   op_reg_mem adc, r12,[rdi+sizeof(qword)*4], rbx
   op_reg_mem adc, r13,[rdi+sizeof(qword)*5], rbx
   op_reg_mem adc, r14,[rdi+sizeof(qword)*6], rbx
   op_reg_mem adc, r15,[rdi+sizeof(qword)*7], rbx
   adc      rax, 0               ; save carry
   mov      [rsp+carry], rax

   mov      rax, [rsp+tailproc]
   SWAP     rcx, rsi
   call     rax
   SWAP     rcx, rsi

   mov      rdx, [rsp+nsA_cnt]   ; restore nsA
   lea      rdi, [rdi+rdx*sizeof(qword)]

   mov      rax, [rsp+carry]     ; restore carry
   shr      rax, 1

   dec      rdx
   jz       .mt_1
   dec      rdx
   jz       .mt_2
   dec      rdx
   jz       .mt_3
   dec      rdx
   jz       .mt_4
   dec      rdx
   jz       .mt_5
   dec      rdx
   jz       .mt_6
.mt_7: adc   r9, 0
.mt_6: adc   r10,0
.mt_5: adc   r11,0
.mt_4: adc   r12,0
.mt_3: adc   r13,0
.mt_2: adc   r14,0
.mt_1: adc   r15,0
   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   mov      qword [rdi+sizeof(qword)*7],r15
   jmp      .mla_entry

.complete_reg_loopB:
   mov      rax, [rsp+carry]     ; restore carry
   add      r8, rax
   adc      r9, 0
   adc      r10,0
   adc      r11,0
   adc      r12,0
   adc      r13,0
   adc      r14,0
   adc      r15,0
   mov      qword [rdi+sizeof(qword)*0], r8
   mov      qword [rdi+sizeof(qword)*1], r9
   mov      qword [rdi+sizeof(qword)*2],r10
   mov      qword [rdi+sizeof(qword)*3],r11
   mov      qword [rdi+sizeof(qword)*4],r12
   mov      qword [rdi+sizeof(qword)*5],r13
   mov      qword [rdi+sizeof(qword)*6],r14
   mov      qword [rdi+sizeof(qword)*7],r15

.mla_entry:
                                 ; restore:
   mov      rbx, [rsp+nsB_cnt]   ; - B counter
   mov      rdi, [rsp+ptrR]      ; - ptrR
   mov      rdx, [rsp+nsA]       ; - nsA
   mov      rsi, [rsp+ptrA]      ; - pA

   add      rcx, sizeof(qword)*8 ; adv pB
   add      rdi, sizeof(qword)*8 ; adv ptrR
   sub      rbx, 8               ; nsB -= 8
   jnc      .mla_loopB

   add      rbx, 8
   jz       .quit

;;
;; final mla pass
;;
   mov      [rsp+nsB_cnt], rbx

   GET_EP   rax, mla_8xl_tail, rbx, rbp   ; tail procedure (mla_8xn) address
   mov      [rsp+tailproc], rax

   ; clear uninitialized rest of product
   lea      rbp, [rdi+rdx*sizeof(qword)]
   xor      rax, rax
   CLEAR    rbp, rbx

   xor      rax, rax             ; init carry
   mov      [rsp+carry], rax

   sub      rdx, 8
.tail_loopA:
   mov      r8, qword [rdi]
   mov      r9, qword [rdi+sizeof(qword)]
   mov      r10,qword [rdi+sizeof(qword)*2]
   mov      r11,qword [rdi+sizeof(qword)*3]
   mov      r12,qword [rdi+sizeof(qword)*4]
   mov      r13,qword [rdi+sizeof(qword)*5]
   mov      r14,qword [rdi+sizeof(qword)*6]
   mov      r15,qword [rdi+sizeof(qword)*7]

   mov      [rsp+nsA_cnt], rdx   ; save A counter
   mov      rax, [rsp+tailproc]
   call     rax

.entry_tail_loopA:
   mov      rax, [rsp+carry]     ; restore carry
   shr      rax, 1
   adc      r8,  0
   adc      r9,  0
   adc      r10, 0
   adc      r11, 0
   adc      r12, 0
   adc      r13, 0
   adc      r14, 0
   adc      r15, 0
   adc      rax, 0

   mov      rbx, [rsp+nsB_cnt]   ; nsB
   mov      rbp, rbx
   dec      rbp
   jz       .tt_1
   dec      rbp
   jz       .tt_2
   dec      rbp
   jz       .tt_3
   dec      rbp
   jz       .tt_4
   dec      rbp
   jz       .tt_5
   dec      rbp
   jz       .tt_6
.tt_7: op_reg_mem adc, r9, [rdi+rbx*sizeof(qword)+sizeof(qword)*1], rbp
.tt_6: op_reg_mem adc, r10,[rdi+rbx*sizeof(qword)+sizeof(qword)*2], rbp
.tt_5: op_reg_mem adc, r11,[rdi+rbx*sizeof(qword)+sizeof(qword)*3], rbp
.tt_4: op_reg_mem adc, r12,[rdi+rbx*sizeof(qword)+sizeof(qword)*4], rbp
.tt_3: op_reg_mem adc, r13,[rdi+rbx*sizeof(qword)+sizeof(qword)*5], rbp
.tt_2: op_reg_mem adc, r14,[rdi+rbx*sizeof(qword)+sizeof(qword)*6], rbp
.tt_1: op_reg_mem adc, r15,[rdi+rbx*sizeof(qword)+sizeof(qword)*7], rbp
   adc      rax, 0
   mov      [rsp+carry], rax  ; update carry

   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*0], r8
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*1], r9
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*2], r10
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*3], r11
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*4], r12
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*5], r13
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*6], r14
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*7], r15

   mov      rdx, [rsp+nsA_cnt]      ; A counter
   add      rsi, sizeof(qword)*8    ; adv pA
   add      rdi, sizeof(qword)*8    ; adv pR
   sub      rdx, 8                  ; nsA -= 8
   jnc      .tail_loopA

   add      rdx, 8
   jz       .quit

   ; carry propagation
   mov      rax, [rsp+carry]
   mov      rbp, rbx

   dec      rbp
   mov      r8, qword [rdi+rbx*sizeof(qword)+sizeof(qword)*0]
   add      r8, rax
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*0], r8
   jz       .simple
   dec      rbp
   mov      r9, qword [rdi+rbx*sizeof(qword)+sizeof(qword)*1]
   adc      r9, 0
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*1], r9
   jz       .simple
   dec      rbp
   mov      r10,qword [rdi+rbx*sizeof(qword)+sizeof(qword)*2]
   adc      r10, 0
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*2], r10
   jz       .simple
   dec      rbp
   mov      r11,qword [rdi+rbx*sizeof(qword)+sizeof(qword)*3]
   adc      r11, 0
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*3], r11
   jz       .simple
   dec      rbp
   mov      r12,qword [rdi+rbx*sizeof(qword)+sizeof(qword)*4]
   adc      r12, 0
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*4], r12
   jz       .simple
   dec      rbp
   mov      r13,qword [rdi+rbx*sizeof(qword)+sizeof(qword)*5]
   adc      r13, 0
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*5], r13
   jz       .simple
   dec      rbp
   mov      r14,qword [rdi+rbx*sizeof(qword)+sizeof(qword)*6]
   adc      r14, 0
   mov      qword [rdi+rbx*sizeof(qword)+sizeof(qword)*6], r14

.simple:
   call     mla_simple


.quit:
   add      rsp, stack_mem       ; release stack
   ret
ENDFUNC mul_NxM_adcox


%endif ;; _PCPBNUMUL_INC_
