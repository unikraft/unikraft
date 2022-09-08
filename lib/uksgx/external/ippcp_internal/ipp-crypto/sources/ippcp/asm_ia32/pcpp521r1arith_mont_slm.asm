;===============================================================================
; Copyright 2016-2021 Intel Corporation
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
;               P521r1 basic arithmetic function
;
;     Content:
;      p521r1_add
;      p521r1_sub
;      p521r1_neg
;      p521r1_div_by_2
;      p521r1_mul_mont_slm
;      p521r1_sqr_mont_slm
;      p521r1_mred
;      p521r1_mont_back
;      p521r1_select_ap_w7
;






%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP >= _IPP_P8)

segment .text align=IPP_ALIGN_FACTOR

;;
;; some p256r1 constants
;;
p521r1_data:
_prime521r1 DD 0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh, 0FFFFFFFFh
            DD 0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh, 0FFFFFFFFh
            DD 0000001FFh

%assign LEN521  ((521+32-1)/32) ; dword's length of operands


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Ipp32u add_521(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
;; input:   edi = r
;;          esi = a
;;          ebx = b
;;
;; output:  eax = carry = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM add_521,PRIVATE
      ; r = a+b
      mov   eax, dword [esi]
      add   eax, dword [ebx]
      mov   dword [edi], eax

      mov   eax, dword [esi+sizeof(dword)]
      adc   eax, dword [ebx+sizeof(dword)]
      mov   dword [edi+sizeof(dword)], eax

      mov   eax, dword [esi+sizeof(dword)*2]
      adc   eax, dword [ebx+sizeof(dword)*2]
      mov   dword [edi+sizeof(dword)*2], eax

      mov   eax, dword [esi+sizeof(dword)*3]
      adc   eax, dword [ebx+sizeof(dword)*3]
      mov   dword [edi+sizeof(dword)*3], eax

      mov   eax, dword [esi+sizeof(dword)*4]
      adc   eax, dword [ebx+sizeof(dword)*4]
      mov   dword [edi+sizeof(dword)*4], eax

      mov   eax, dword [esi+sizeof(dword)*5]
      adc   eax, dword [ebx+sizeof(dword)*5]
      mov   dword [edi+sizeof(dword)*5], eax

      mov   eax, dword [esi+sizeof(dword)*6]
      adc   eax, dword [ebx+sizeof(dword)*6]
      mov   dword [edi+sizeof(dword)*6], eax

      mov   eax, dword [esi+sizeof(dword)*7]
      adc   eax, dword [ebx+sizeof(dword)*7]
      mov   dword [edi+sizeof(dword)*7], eax

      mov   eax, dword [esi+sizeof(dword)*8]
      adc   eax, dword [ebx+sizeof(dword)*8]
      mov   dword [edi+sizeof(dword)*8], eax

      mov   eax, dword [esi+sizeof(dword)*9]
      adc   eax, dword [ebx+sizeof(dword)*9]
      mov   dword [edi+sizeof(dword)*9], eax

      mov   eax, dword [esi+sizeof(dword)*10]
      adc   eax, dword [ebx+sizeof(dword)*10]
      mov   dword [edi+sizeof(dword)*10], eax

      mov   eax, dword [esi+sizeof(dword)*11]
      adc   eax, dword [ebx+sizeof(dword)*11]
      mov   dword [edi+sizeof(dword)*11], eax

      mov   eax, dword [esi+sizeof(dword)*12]
      adc   eax, dword [ebx+sizeof(dword)*12]
      mov   dword [edi+sizeof(dword)*12], eax

      mov   eax, dword [esi+sizeof(dword)*13]
      adc   eax, dword [ebx+sizeof(dword)*13]
      mov   dword [edi+sizeof(dword)*13], eax

      mov   eax, dword [esi+sizeof(dword)*14]
      adc   eax, dword [ebx+sizeof(dword)*14]
      mov   dword [edi+sizeof(dword)*14], eax

      mov   eax, dword [esi+sizeof(dword)*15]
      adc   eax, dword [ebx+sizeof(dword)*15]
      mov   dword [edi+sizeof(dword)*15], eax

      mov   eax, dword [esi+sizeof(dword)*16]
      adc   eax, dword [ebx+sizeof(dword)*16]
      mov   dword [edi+sizeof(dword)*16], eax

      mov   eax, 0
      adc   eax, 0
      ret
ENDFUNC add_521

;;
;; Ipp32u sub_521(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
;; input:   edi = r
;;          esi = a
;;          ebx = b
;;
;; output:  eax = borrow = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM sub_521,PRIVATE
      ; r = a-b
      mov   eax, dword [esi]
      sub   eax, dword [ebx]
      mov   dword [edi], eax

      mov   eax, dword [esi+sizeof(dword)]
      sbb   eax, dword [ebx+sizeof(dword)]
      mov   dword [edi+sizeof(dword)], eax

      mov   eax, dword [esi+sizeof(dword)*2]
      sbb   eax, dword [ebx+sizeof(dword)*2]
      mov   dword [edi+sizeof(dword)*2], eax

      mov   eax, dword [esi+sizeof(dword)*3]
      sbb   eax, dword [ebx+sizeof(dword)*3]
      mov   dword [edi+sizeof(dword)*3], eax

      mov   eax, dword [esi+sizeof(dword)*4]
      sbb   eax, dword [ebx+sizeof(dword)*4]
      mov   dword [edi+sizeof(dword)*4], eax

      mov   eax, dword [esi+sizeof(dword)*5]
      sbb   eax, dword [ebx+sizeof(dword)*5]
      mov   dword [edi+sizeof(dword)*5], eax

      mov   eax, dword [esi+sizeof(dword)*6]
      sbb   eax, dword [ebx+sizeof(dword)*6]
      mov   dword [edi+sizeof(dword)*6], eax

      mov   eax, dword [esi+sizeof(dword)*7]
      sbb   eax, dword [ebx+sizeof(dword)*7]
      mov   dword [edi+sizeof(dword)*7], eax

      mov   eax, dword [esi+sizeof(dword)*8]
      sbb   eax, dword [ebx+sizeof(dword)*8]
      mov   dword [edi+sizeof(dword)*8], eax

      mov   eax, dword [esi+sizeof(dword)*9]
      sbb   eax, dword [ebx+sizeof(dword)*9]
      mov   dword [edi+sizeof(dword)*9], eax

      mov   eax, dword [esi+sizeof(dword)*10]
      sbb   eax, dword [ebx+sizeof(dword)*10]
      mov   dword [edi+sizeof(dword)*10], eax

      mov   eax, dword [esi+sizeof(dword)*11]
      sbb   eax, dword [ebx+sizeof(dword)*11]
      mov   dword [edi+sizeof(dword)*11], eax

      mov   eax, dword [esi+sizeof(dword)*12]
      sbb   eax, dword [ebx+sizeof(dword)*12]
      mov   dword [edi+sizeof(dword)*12], eax

      mov   eax, dword [esi+sizeof(dword)*13]
      sbb   eax, dword [ebx+sizeof(dword)*13]
      mov   dword [edi+sizeof(dword)*13], eax

      mov   eax, dword [esi+sizeof(dword)*14]
      sbb   eax, dword [ebx+sizeof(dword)*14]
      mov   dword [edi+sizeof(dword)*14], eax

      mov   eax, dword [esi+sizeof(dword)*15]
      sbb   eax, dword [ebx+sizeof(dword)*15]
      mov   dword [edi+sizeof(dword)*15], eax

      mov   eax, dword [esi+sizeof(dword)*16]
      sbb   eax, dword [ebx+sizeof(dword)*16]
      mov   dword [edi+sizeof(dword)*16], eax

      mov   eax, 0
      adc   eax, 0
      ret
ENDFUNC sub_521

;;
;; Ipp32u shl_521(Ipp32u* r, const Ipp32u* a)
;;
;; input:   edi = r
;;          esi = a
;;
;; output:  eax = extension = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM shl_521,PRIVATE
      ; r = a<<1
      push  ecx
      mov   ecx, LEN521-4

      pxor  xmm1, xmm1
.shl_loop:
      movdqu   xmm0, oword [esi]
      movdqa   xmm2, xmm0
      add      esi, sizeof(oword)
      palignr  xmm2, xmm1, sizeof(qword)
      movdqa   xmm1, xmm0

      psllq    xmm0, 1
      psrlq    xmm2, 63
      por      xmm0, xmm2
      movdqu   oword [edi], xmm0
      add      edi, sizeof(oword)
      sub      ecx, 4
      jg       .shl_loop

      movd     xmm0, dword [esi]
      palignr  xmm0, xmm1, sizeof(dword)*3
      psllq    xmm0, 1
      psrldq   xmm0, sizeof(dword)
      movd     dword [edi], xmm0

      sub   esi, sizeof(oword)*4
      sub   edi, sizeof(oword)*4
      pop   ecx

      xor   eax, eax
      ret
ENDFUNC shl_521

;;
;; void shr_521(Ipp32u* r, const Ipp32u* a)
;;
;; input:   edi = r
;;          esi = a
;;          eax = ext
;; output:  eax = extension = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM shr_521,PRIVATE
      ; r = a>>1
      mov   eax, dword [esi+(LEN521-1)*sizeof(dword)]

      push  ecx
      mov   ecx, LEN521-(4*2)
.shr_loop:
      movdqu   xmm0, oword [esi]
      movdqu   xmm1, oword [esi+sizeof(oword)]
      palignr  xmm1, xmm0, sizeof(qword)
      add      esi, sizeof(oword)

      psrlq    xmm0, 1
      psllq    xmm1, 63
      por      xmm0, xmm1
      movdqu   oword [edi], xmm0
      add      edi, sizeof(oword)

      sub      ecx, 4
      jg       .shr_loop
      pop      ecx

      movdqu   xmm0, oword [esi]
      movd     xmm1, eax
      palignr  xmm1, xmm0, sizeof(qword)

      psrlq    xmm0, 1
      psllq    xmm1, 63
      por      xmm0, xmm1
      movdqu   oword [edi], xmm0
      shr      eax, 1
      mov      dword [edi+sizeof(oword)], eax

      sub      esi, sizeof(qword)*3
      sub      esi, sizeof(qword)*3
      ret
ENDFUNC shr_521
;;
;; void cpy_521(Ipp32u* r, const Ipp32u* a)
;;
%macro cpy_521 2.nolist
  %xdefine %%pdst %1
  %xdefine %%psrc %2

   movdqu   xmm0, oword [%%psrc]
   movdqu   xmm1, oword [%%psrc+sizeof(oword)]
   movdqu   xmm2, oword [%%psrc+sizeof(oword)*2]
   movdqu   xmm3, oword [%%psrc+sizeof(oword)*3]
   mov      eax,  dword [%%psrc+sizeof(oword)*4]
   movdqu   oword [%%pdst], xmm0
   movdqu   oword [%%pdst+sizeof(oword)], xmm1
   movdqu   oword [%%pdst+sizeof(oword)*2], xmm2
   movdqu   oword [%%pdst+sizeof(oword)*3], xmm3
   mov      dword [%%pdst+sizeof(oword)*4], eax
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_add(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_add,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [ebp + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN521]
%assign _sp_  _buf_+(LEN521)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov      edi, pR                 ; pR
      mov      esi, pA                 ; pA
      mov      ebx, pB                 ; pB
      CALL_IPPASM  add_521                 ; R = A+B
      mov      edx, eax

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p521r1_data        ; modulus
      lea      ebx, [ebx+(_prime521r1-p521r1_data)]
      CALL_IPPASM  sub_521                 ; T = R-modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      sub      edx, eax                ; R = T<0? R : T
      cmovnz   esi, edi
      cpy_521  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p521r1_add


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_sub(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_sub,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [ebp + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN521]
%assign _sp_  _buf_+(LEN521)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov      edi, pR                 ; pR
      mov      esi, pA                 ; pA
      mov      ebx, pB                 ; pB
      CALL_IPPASM  sub_521                 ; R = A-B
      mov      edx, eax

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p521r1_data        ; modulus
      lea      ebx, [ebx+(_prime521r1-p521r1_data)]
      CALL_IPPASM  add_521                 ; T = R+modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      test     edx, edx                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_521  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p521r1_sub


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_neg(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_neg,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN521]
%assign _sp_  _buf_+(LEN521)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov   edi, pR                    ; outpur pR
      mov   esi, pA                    ; input pA

      ; r = 0-a
      mov   eax, 0
      sub   eax, dword [esi]
      mov   dword [edi], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)]
      mov   dword [edi+sizeof(dword)], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*2]
      mov   dword [edi+sizeof(dword)*2], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*3]
      mov   dword [edi+sizeof(dword)*3], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*4]
      mov   dword [edi+sizeof(dword)*4], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*5]
      mov   dword [edi+sizeof(dword)*5], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*6]
      mov   dword [edi+sizeof(dword)*6], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*7]
      mov   dword [edi+sizeof(dword)*7], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*8]
      mov   dword [edi+sizeof(dword)*8], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*9]
      mov   dword [edi+sizeof(dword)*9], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*10]
      mov   dword [edi+sizeof(dword)*10], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*11]
      mov   dword [edi+sizeof(dword)*11], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*12]
      mov   dword [edi+sizeof(dword)*12], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*13]
      mov   dword [edi+sizeof(dword)*13], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*14]
      mov   dword [edi+sizeof(dword)*14], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*15]
      mov   dword [edi+sizeof(dword)*15], eax
      mov   eax, 0
      sbb   eax, dword [esi+sizeof(dword)*16]
      mov   dword [edi+sizeof(dword)*16], eax

      sbb   edx,edx

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p521r1_data        ; modulus
      lea      ebx, [ebx+(_prime521r1-p521r1_data)]
      CALL_IPPASM  add_521                 ; T = R+modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      test     edx, edx                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_521  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p521r1_neg


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_mul_by_2(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_mul_by_2,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN521]
%assign _sp_  _buf_+(LEN521)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pA                 ; pA
      CALL_IPPASM  shl_521                 ; T = A<<1
      mov      edx, eax

      mov      esi, edi                ; T
      mov      edi, pR                 ; R
      LD_ADDR  ebx, p521r1_data        ; modulus
      lea      ebx, [ebx+(_prime521r1-p521r1_data)]
      CALL_IPPASM  sub_521                 ; R = T-modulus

      sub      edx, eax                ; R = R<0? T : R
      cmovz    esi, edi
      cpy_521  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p521r1_mul_by_2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_mul_by_3(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_mul_by_3,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _bufT_  0                             ; T buffer[LEN521]
%assign _bufU_  _bufT_+(LEN521)*sizeof(dword) ; U buffer[LEN521]
%assign _mod_  _bufU_+(LEN521)*sizeof(dword) ; modulus address [1]
%assign _sp_  _mod_+sizeof(dword)           ; esp [1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      LD_ADDR  eax, p521r1_data        ; srore modulus address
      lea      eax, [eax+(_prime521r1-p521r1_data)]
      mov      dword [esp+_mod_], eax

      lea      edi, [esp+_bufT_]       ; T
      mov      esi, pA                 ; A
      CALL_IPPASM  shl_521                 ; T = A<<1
      mov      edx, eax

      mov      esi, edi                ; T
      lea      edi, [esp+_bufU_]       ; U
      mov      ebx, [esp+_mod_]        ; modulus
      CALL_IPPASM  sub_521                 ; U = T-modulus

      sub      edx, eax                ; T = U<0? T : U
      cmovz    esi, edi
      cpy_521  edi, esi

      mov      esi, edi
      mov      ebx, pA
      CALL_IPPASM  add_521                 ; T +=A
      mov      edx, eax

      mov      edi, pR                 ; R
      mov      ebx, [esp+_mod_]        ; modulus
      CALL_IPPASM  sub_521                 ; R = T-modulus

      sub      edx, eax                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_521  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p521r1_mul_by_3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_div_by_2(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_div_by_2,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN521]
%assign _sp_  _buf_+(LEN521)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pA                 ; A
      LD_ADDR  ebx, p521r1_data        ; modulus
      lea      ebx, [ebx+(_prime521r1-p521r1_data)]
      CALL_IPPASM  add_521                 ; R = A+modulus
      mov      edx, 0

      mov      ecx, dword [esi]     ; shifted_data = (a[0]&1)? T : A
      and      ecx, 1
      cmovnz   esi, edi
      cmovz    eax, edx
      mov      edi, pR
      CALL_IPPASM  shr_521

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p521r1_div_by_2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_mul_mont_slm(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_mul_mont_slm,PUBLIC
  USES_GPR ebp,ebx,esi,edi

%xdefine pR [eax + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [eax + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [eax + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_  0
%assign _rp_  _buf_+(LEN521+1)*sizeof(dword) ; pR
%assign _ap_  _rp_ +sizeof(dword)            ; pA
%assign _bp_  _ap_ +sizeof(dword)            ; pB
%assign _sp_  _bp_ +sizeof(dword)            ; esp storage
%assign _ssize_  _sp_ +sizeof(dword)            ; size allocated stack

      mov   eax, esp                   ; save esp
      sub   esp, _ssize_               ; allocate stack
      and   esp, -16                   ; provide 16-byte stack alignment
      mov   dword [esp+_sp_], eax   ; store original esp

      ; clear buffer
      pxor     xmm0, xmm0
      movdqa   oword [esp+_buf_], xmm0
      movdqa   oword [esp+_buf_+sizeof(oword)], xmm0
      movdqa   oword [esp+_buf_+sizeof(oword)*2], xmm0
      movdqa   oword [esp+_buf_+sizeof(oword)*3], xmm0
      movq     qword [esp+_buf_+sizeof(oword)*4], xmm0

      ; store parameters into the stack
      ; note: eax here stores an original esp, so it can be used to reach function parameters
      mov   edi, pR
      mov   esi, pA
      mov   ebp, pB
      mov   dword [esp+_rp_], edi
      mov   dword [esp+_ap_], esi
      mov   dword [esp+_bp_], ebp

      mov   ebx, LEN521

      movd  mm1, dword [esi+sizeof(dword)]      ; pre load a[1], a[2], a[3], a[4]
      movd  mm2, dword [esi+sizeof(dword)*2]
      movd  mm3, dword [esi+sizeof(dword)*3]
      movd  mm4, dword [esi+sizeof(dword)*4]

align IPP_ALIGN_FACTOR
.mmul_loop:
;
; i-st pass
; modulus = 2^521 -1
;           [17]  [0]
; m0 = 1
;
      movd     mm7, ebx                   ; save pass counter

      mov      edx, dword [ebp]        ; b = b[i]
      mov      eax, dword [esi]        ; a[0]
      movd     mm0, edx
      add      ebp, sizeof(dword)
      mov      dword [esp+_bp_], ebp

      pmuludq  mm1, mm0                   ; a[1]*b[i]
      pmuludq  mm2, mm0                   ; a[2]*b[i]

      mul      edx                        ; (E:u) = (edx:eax) = a[0]*b[i]+buf[0]
      add      eax, dword [esp+_buf_]
      adc      edx, 0

      pmuludq  mm3, mm0                   ; a[3]*b[i]
      pmuludq  mm4, mm0                   ; a[4]*b[i]

; multiplication round 1 - round 4
      movd     ecx, mm1                   ; p = a[1]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*1]
      movd     mm1, dword [esi+sizeof(dword)*5]
      mov      dword [esp+_buf_+sizeof(dword)*0], ecx
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[2]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*2]
      movd     mm2, dword [esi+sizeof(dword)*6]
      mov      dword [esp+_buf_+sizeof(dword)*1], ebx
      adc      edx, 0

      pmuludq  mm1, mm0                   ; a[5]*b[i]
      pmuludq  mm2, mm0                   ; a[6]*b[i]

      movd     ebp, mm3                   ; p = a[3]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*3]
      movd     mm3, dword [esi+sizeof(dword)*7]
      mov      dword [esp+_buf_+sizeof(dword)*2], ebp
      adc      edx, 0

      movd     edi, mm4                   ; p = a[4]*b[i] + E
      psrlq    mm4, 32
      add      edi, edx
      movd     edx, mm4
      adc      edx, 0
      add      edi, dword [esp+_buf_+sizeof(dword)*4]
      movd     mm4, dword [esi+sizeof(dword)*8]
      mov      dword [esp+_buf_+sizeof(dword)*3], edi
      adc      edx, 0

      pmuludq  mm3, mm0                   ; a[7]*b[i]
      pmuludq  mm4, mm0                   ; a[8]*b[i]

; multiplication round 5 - round 8
      movd     ecx, mm1                   ; p = a[5]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*5]
      movd     mm1, dword [esi+sizeof(dword)*9]
      mov      dword [esp+_buf_+sizeof(dword)*4], ecx
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[6]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*6]
      movd     mm2, dword [esi+sizeof(dword)*10]
      mov      dword [esp+_buf_+sizeof(dword)*5], ebx
      adc      edx, 0

      pmuludq  mm1, mm0                   ; a[9]*b[i]
      pmuludq  mm2, mm0                   ; a[10]*b[i]

      movd     ebp, mm3                   ; p = a[7]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*7]
      movd     mm3, dword [esi+sizeof(dword)*11]
      mov      dword [esp+_buf_+sizeof(dword)*6], ebp
      adc      edx, 0

      movd     edi, mm4                   ; p = a[8]*b[i] + E
      psrlq    mm4, 32
      add      edi, edx
      movd     edx, mm4
      adc      edx, 0
      add      edi, dword [esp+_buf_+sizeof(dword)*8]
      movd     mm4, dword [esi+sizeof(dword)*12]
      mov      dword [esp+_buf_+sizeof(dword)*7], edi
      adc      edx, 0

      pmuludq  mm3, mm0                   ; a[11]*b[i]
      pmuludq  mm4, mm0                   ; a[12]*b[i]

; multiplication round 9 - round 12
      movd     ecx, mm1                   ; p = a[9]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*9]
      movd     mm1, dword [esi+sizeof(dword)*13]
      mov      dword [esp+_buf_+sizeof(dword)*8], ecx
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[10]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*10]
      movd     mm2, dword [esi+sizeof(dword)*14]
      mov      dword [esp+_buf_+sizeof(dword)*9], ebx
      adc      edx, 0

      pmuludq  mm1, mm0                   ; a[13]*b[i]
      pmuludq  mm2, mm0                   ; a[14]*b[i]

      movd     ebp, mm3                   ; p = a[11]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*11]
      movd     mm3, dword [esi+sizeof(dword)*15]
      mov      dword [esp+_buf_+sizeof(dword)*10], ebp
      adc      edx, 0

      movd     edi, mm4                   ; p = a[12]*b[i] + E
      psrlq    mm4, 32
      add      edi, edx
      movd     edx, mm4
      adc      edx, 0
      add      edi, dword [esp+_buf_+sizeof(dword)*12]
      movd     mm4, dword [esi+sizeof(dword)*16]
      mov      dword [esp+_buf_+sizeof(dword)*11], edi
      adc      edx, 0

      pmuludq  mm3, mm0                   ; a[15]*b[i]
      pmuludq  mm4, mm0                   ; a[16]*b[i]

; multiplication round 13 - round 16
      movd     ecx, mm1                   ; p = a[13]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*13]
      mov      dword [esp+_buf_+sizeof(dword)*12], ecx
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[14]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*14]
      mov      dword [esp+_buf_+sizeof(dword)*13], ebx
      adc      edx, 0

      movd     ebp, mm3                   ; p = a[15]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*15]
      mov      dword [esp+_buf_+sizeof(dword)*14], ebp
      adc      edx, 0

      movd     edi, mm4                   ; p = a[16]*b[i] + E
      psrlq    mm4, 32
      add      edi, edx
      movd     edx, mm4
      adc      edx, 0
      add      edi, dword [esp+_buf_+sizeof(dword)*16]
      adc      edx, 0

;;; and reduction and last multiplication round 17
      movd     ebx, mm7                   ; restore pass counter

      mov      ecx, eax                   ; u0 <<= 9
      shl      eax, (521-512)
      shr      ecx, (32-(521-512))

      add      edi, eax
      mov      dword [esp+_buf_+sizeof(dword)*15], edi
      adc      edx, ecx
      mov      dword [esp+_buf_+sizeof(dword)*16], edx

      sub      ebx, 1
      movd  mm1, dword [esi+sizeof(dword)]               ; speculative load a[1], a[2], a[3], a[4]
      movd  mm2, dword [esi+sizeof(dword)*2]
      movd  mm3, dword [esi+sizeof(dword)*3]
      movd  mm4, dword [esi+sizeof(dword)*4]
      jz       .exit_mmul_loop

      mov      ebp, dword [esp+_bp_]            ; restore pB
      jmp      .mmul_loop

.exit_mmul_loop:
      emms

; final reduction
      mov      edi, [esp+_rp_]         ; result
      lea      esi, [esp+_buf_]        ; buffer
      LD_ADDR  ebx, p521r1_data        ; modulus
      lea      ebx, [ebx+(_prime521r1-p521r1_data)]
      CALL_IPPASM  sub_521
      mov      edx, dword [esp+_buf_+LEN521*sizeof(dword)]

; copy
      cmovz    esi, edi
      cpy_521  edi, esi

      mov   esp, [esp+_sp_]            ; release stack
   REST_GPR
   ret
ENDFUNC p521r1_mul_mont_slm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_sqr_mont_slm(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p521r1_sqr_mont_slm,PUBLIC
  USES_GPR esi,edi

%xdefine pR [esp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [esp + ARG_1 + 1*sizeof(dword)] ; source A address

      ;; use p521r1_mul_mont_slm to compute sqr
      mov   esi, pA
      mov   edi, pR
      push  esi
      push  esi
      push  edi
      CALL_IPPASM  p521r1_mul_mont_slm,PUBLIC
      add   esp, sizeof(dword)*3
   REST_GPR
   ret
ENDFUNC p521r1_sqr_mont_slm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p521r1_mred(Ipp32u* r, Ipp32u* prod)
;;
; modulus = 2^521 -1
;           [17]  [0]
; m0 = 1
;
align IPP_ALIGN_FACTOR
IPPASM p521r1_mred,PUBLIC
  USES_GPR ebx,esi,edi

%xdefine pR [esp + ARG_1 + 0*sizeof(dword)] ; reduction address
%xdefine pA [esp + ARG_1 + 1*sizeof(dword)] ; source product address

   ; get parameters:
   mov   esi, pA

   mov   ecx, LEN521
   xor   edx, edx
align IPP_ALIGN_FACTOR
.mred_loop:
   mov   ebx, dword [esi]     ; [ebx:eax] = u0<<9
   mov   eax, dword [esi]
   shr   ebx,(32-(521-512))
   shl   eax,(521-512)
   add   ebx, edx

   ;; [esi+i*sizeof(dword)] = [esi+i*sizeof(dword)], i=1,..,16

   add   eax, dword [esi+sizeof(dword)*16]
   mov   edx, dword [esi+sizeof(dword)*17]
   adc   ebx, edx
   mov   edx, 0

   mov   dword [esi+sizeof(dword)*16], eax
   mov   dword [esi+sizeof(dword)*17], ebx
   adc   edx, 0

   lea   esi, [esi+sizeof(dword)]
   sub   ecx, 1
   jnz   .mred_loop

   ; final reduction
   mov      edi, pR           ; result
   LD_ADDR  ebx, p521r1_data  ; addres of the modulus
   lea      ebx, [ebx+(_prime521r1-p521r1_data)]
   CALL_IPPASM  sub_521

   sub      edx, eax
   cmovz    esi, edi
   cpy_521  edi, esi

   REST_GPR
   ret
ENDFUNC p521r1_mred

%endif    ;; _IPP >= _IPP_P8

