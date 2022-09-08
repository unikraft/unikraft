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
;               P384r1 basic arithmetic function
;
;     Content:
;      p384r1_add
;      p384r1_sub
;      p384r1_neg
;      p384r1_div_by_2
;      p384r1_mul_mont_slm
;      p384r1_sqr_mont_slm
;      p384r1_mred
;      p384r1_select_ap_w5
;






%include "asmdefs.inc"
%include "ia_emm.inc"
%include "pcpvariant.inc"

%if (_IPP >= _IPP_P8)

segment .text align=IPP_ALIGN_FACTOR

;;
;; some p384r1 constants
;;
p384r1_data:
_prime384r1 DD 0FFFFFFFFh,000000000h,000000000h,0FFFFFFFFh,0FFFFFFFEh,0FFFFFFFFh
            DD 0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh

%assign LEN384  (384/32) ; dword's length of operands

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Ipp32u add_384(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
;; input:   edi = r
;;          esi = a
;;          ebx = b
;;
;; output:  eax = carry = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM add_384,PRIVATE
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
      mov   eax, 0
      adc   eax, 0
      ret
ENDFUNC add_384

;;
;; Ipp32u sub_384(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
;; input:   edi = r
;;          esi = a
;;          ebx = b
;;
;; output:  eax = borrow = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM sub_384,PRIVATE
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
      mov   eax, 0
      adc   eax, 0
      ret
ENDFUNC sub_384

;;
;; Ipp32u shl_384(Ipp32u* r, const Ipp32u* a)
;;
;; input:   edi = r
;;          esi = a
;;
;; output:  eax = extension = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM shl_384,PRIVATE
      mov      eax, dword [esi+(LEN384-1)*sizeof(dword)]
      ; r = a<<1
      movdqu   xmm3, oword [esi+sizeof(oword)*2]
      movdqu   xmm2, oword [esi+sizeof(oword)]
      movdqu   xmm1, oword [esi]

      movdqa   xmm4, xmm3
      palignr  xmm4, xmm2, sizeof(qword)
      psllq    xmm3, 1
      psrlq    xmm4, 63
      por      xmm3, xmm4
      movdqu   oword [edi+sizeof(oword)*2], xmm3

      movdqa   xmm4, xmm2
      palignr  xmm4, xmm1, sizeof(qword)
      psllq    xmm2, 1
      psrlq    xmm4, 63
      por      xmm2, xmm4
      movdqu   oword [edi+sizeof(oword)], xmm2

      movdqa   xmm4, xmm1
      pslldq   xmm4, sizeof(qword)
      psllq    xmm1, 1
      psrlq    xmm4, 63
      por      xmm1, xmm4
      movdqu   oword [edi], xmm1

      shr     eax, 31
      ret
ENDFUNC shl_384

;;
;; void shr_384(Ipp32u* r, const Ipp32u* a)
;;
;; input:   edi = r
;;          esi = a
;;          eax = ext
;; output:  eax = extension = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM shr_384,PRIVATE
      ; r = a>>1
      movdqu   xmm3, oword [esi]
      movdqu   xmm2, oword [esi+sizeof(oword)]
      movdqu   xmm1, oword [esi+sizeof(oword)*2]

      movdqa   xmm4, xmm2
      palignr  xmm4, xmm3, sizeof(qword)
      psrlq    xmm3, 1
      psllq    xmm4, 63
      por      xmm3, xmm4
      movdqu   oword [edi], xmm3

      movdqa   xmm4, xmm1
      palignr  xmm4, xmm2, sizeof(qword)
      psrlq    xmm2, 1
      psllq    xmm4, 63
      por      xmm2, xmm4
      movdqu   oword [edi+sizeof(oword)], xmm2

      movdqa   xmm4, xmm0
      palignr  xmm4, xmm1, sizeof(qword)
      psrlq    xmm1, 1
      psllq    xmm4, 63
      por      xmm1, xmm4
      movdqu   oword [edi+sizeof(oword)*2], xmm1

      ret
ENDFUNC shr_384

;;
;; void cpy_384(Ipp32u* r, const Ipp32u* a)
;;
%macro cpy_384 2.nolist
  %xdefine %%pdst %1
  %xdefine %%psrc %2

   movdqu   xmm0, oword [%%psrc]
   movdqu   xmm1, oword [%%psrc+sizeof(oword)]
   movdqu   xmm2, oword [%%psrc+sizeof(oword)*2]
   movdqu   oword [%%pdst], xmm0
   movdqu   oword [%%pdst+sizeof(oword)], xmm1
   movdqu   oword [%%pdst+sizeof(oword)*2], xmm2
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_add(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_add,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [ebp + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN384]
%assign _sp_  _buf_+(LEN384)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov      edi, pR                 ; pR
      mov      esi, pA                 ; pA
      mov      ebx, pB                 ; pB
      CALL_IPPASM  add_384                 ; R = A+B
      mov      edx, eax

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p384r1_data        ; modulus
      lea      ebx, [ebx+(_prime384r1-p384r1_data)]
      CALL_IPPASM  sub_384                 ; T = R-modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      sub      edx, eax                ; R = T<0? R : T
      cmovnz   esi, edi
      cpy_384  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p384r1_add


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_sub(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_sub,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [ebp + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN384]
%assign _sp_  _buf_+(LEN384)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov      edi, pR                 ; pR
      mov      esi, pA                 ; pA
      mov      ebx, pB                 ; pB
      CALL_IPPASM  sub_384                 ; R = A-B
      mov      edx, eax

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p384r1_data        ; modulus
      lea      ebx, [ebx+(_prime384r1-p384r1_data)]
      CALL_IPPASM  add_384                 ; T = R+modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      test     edx, edx                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_384  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p384r1_sub


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_neg(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_neg,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN384]
%assign _sp_  _buf_+(LEN384)*sizeof(dword)  ; esp[1]
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
      sbb   edx,edx

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p384r1_data        ; modulus
      lea      ebx, [ebx+(_prime384r1-p384r1_data)]
      CALL_IPPASM  add_384                 ; T = R+modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      test     edx, edx                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_384  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p384r1_neg


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_mul_by_2(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_mul_by_2,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN384]
%assign _sp_  _buf_+(LEN384)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pA                 ; pA
      CALL_IPPASM  shl_384                 ; T = A<<1
      mov      edx, eax

      mov      esi, edi                ; T
      mov      edi, pR                 ; R
      LD_ADDR  ebx, p384r1_data        ; modulus
      lea      ebx, [ebx+(_prime384r1-p384r1_data)]
      CALL_IPPASM  sub_384                 ; R = T-modulus

      sub      edx, eax                ; R = R<0? T : R
      cmovz    esi, edi
      cpy_384  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p384r1_mul_by_2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_mul_by_3(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_mul_by_3,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _bufT_  0                             ; T buffer[LEN384]
%assign _bufU_  _bufT_+(LEN384)*sizeof(dword) ; U buffer[LEN384]
%assign _mod_  _bufU_+(LEN384)*sizeof(dword) ; modulus address [1]
%assign _sp_  _mod_+sizeof(dword)           ; esp [1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      LD_ADDR  eax, p384r1_data        ; srore modulus address
      lea      eax, [eax+(_prime384r1-p384r1_data)]
      mov      dword [esp+_mod_], eax

      lea      edi, [esp+_bufT_]       ; T
      mov      esi, pA                 ; A
      CALL_IPPASM  shl_384                 ; T = A<<1
      mov      edx, eax

      mov      esi, edi                ; T
      lea      edi, [esp+_bufU_]       ; U
      mov      ebx, [esp+_mod_]        ; modulus
      CALL_IPPASM  sub_384                 ; U = T-modulus

      sub      edx, eax                ; T = U<0? T : U
      cmovz    esi, edi
      cpy_384  edi, esi

      mov      esi, edi
      mov      ebx, pA
      CALL_IPPASM  add_384                 ; T +=A
      mov      edx, eax

      mov      edi, pR                 ; R
      mov      ebx, [esp+_mod_]        ; modulus
      CALL_IPPASM  sub_384                 ; R = T-modulus

      sub      edx, eax                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_384  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p384r1_mul_by_3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_div_by_2(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_div_by_2,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN384]
%assign _sp_  _buf_+(LEN384)*sizeof(dword)  ; esp[1]
%assign _frame_  _sp_ +sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pA                 ; A
      LD_ADDR  ebx, p384r1_data        ; modulus
      lea      ebx, [ebx+(_prime384r1-p384r1_data)]
      CALL_IPPASM  add_384                 ; R = A+modulus
      mov      edx, 0

      mov      ecx, dword [esi]     ; shifted_data = (a[0]&1)? T : A
      and      ecx, 1
      cmovnz   esi, edi
      cmovz    eax, edx
      movd     xmm0, eax
      mov      edi, pR
      CALL_IPPASM  shr_384

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p384r1_div_by_2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_mul_mont_slm(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_mul_mont_slm,PUBLIC
  USES_GPR ebp,ebx,esi,edi

%xdefine pR [eax + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [eax + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [eax + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_  0
%assign _rp_  _buf_+(LEN384+1)*sizeof(dword) ; pR
%assign _ap_  _rp_ +sizeof(dword)            ; pA
%assign _bp_  _ap_+sizeof(dword)             ; pB
%assign _sp_  _bp_+sizeof(dword)             ; esp storage
%assign _ssize_  _sp_+sizeof(dword)             ; size allocated stack

      mov   eax, esp                   ; save esp
      sub   esp, _ssize_               ; allocate stack
      and   esp, -16                   ; provide 16-byte stack alignment
      mov   dword [esp+_sp_], eax   ; store original esp

      ; clear buffer
      pxor  mm0, mm0
      movq  [esp+_buf_], mm0
      movq  [esp+_buf_+sizeof(qword)], mm0
      movq  [esp+_buf_+sizeof(qword)*2], mm0
      movq  [esp+_buf_+sizeof(qword)*3], mm0
      movq  [esp+_buf_+sizeof(qword)*4], mm0
      movq  [esp+_buf_+sizeof(qword)*5], mm0
      movq  [esp+_buf_+sizeof(qword)*6], mm0

      ; store parameters into the stack
      ; note: eax here stores an original esp, so it can be used to reach function parameters
      mov   edi, pR
      mov   esi, pA
      mov   ebp, pB
      mov   dword [esp+_rp_], edi
      mov   dword [esp+_ap_], esi
      mov   dword [esp+_bp_], ebp

      mov   eax, LEN384

      movd  mm1, dword [esi+sizeof(dword)]      ; pre load a[1], a[2], a[3], a[4]
      movd  mm2, dword [esi+sizeof(dword)*2]
      movd  mm3, dword [esi+sizeof(dword)*3]
      movd  mm4, dword [esi+sizeof(dword)*4]


align IPP_ALIGN_FACTOR
.mmul_loop:
;
; i-st pass
; modulus = 2^384 -2^128 -2^96 +2^32 -1
;           [12]   [4]    [3]   [1]  [0]
; m0 = 1
;
; T0 = a[ 0]*b[i] + p[0]
; e0 = HI(T0), u = LO(T0)
;
; T1  = a[ 1]*b[i] +e0  +p[ 1], (cf=0),  e1 = HI( T1), p1 =LO( T1),  (cf1 ,p1 ) = p1 +u,        p[ 0] = p1,  cf1
; T2  = a[ 2]*b[i] +e1  +p[ 2], (cf=0),  e2 = HI( T2), p2 =LO( T2),  (cf2 ,p2 ) = p2 +cf1,      p[ 1] = p2,  cf2
; T3  = a[ 3]*b[i] +e2  +p[ 3], (cf=0),  e3 = HI( T3), p3 =LO( T3),  (bf3 ,p3 ) = p3 +cf2 -u,   p[ 2] = p3,  bf3
; T4  = a[ 4]*b[i] +e3  +p[ 4], (cf=0),  e4 = HI( T4), p4 =LO( T4),  (bf4 ,p4 ) = p4 -bf3 -u,   p[ 3] = p4,  bf4
; T5  = a[ 5]*b[i] +e4  +p[ 5], (cf=0),  e5 = HI( T5), p5 =LO( T5),  (bf5 ,p5 ) = p5 -bf4,      p[ 4] = p5,  bf5
; T6  = a[ 6]*b[i] +e5  +p[ 6], (cf=0),  e6 = HI( T6), p6 =LO( T6),  (bf6 ,p6 ) = p6 -bf5,      p[ 5] = p6,  bf6
; T7  = a[ 7]*b[i] +e6  +p[ 7], (cf=0),  e7 = HI( T7), p7 =LO( T7),  (bf7 ,p7 ) = p7 -bf6,      p[ 6] = p7,  bf7
; T8  = a[ 8]*b[i] +e7  +p[ 8], (cf=0),  e8 = HI( T8), p8 =LO( T8),  (bf8 ,p8 ) = p8 -bf7,      p[ 7] = p8,  bf8
; T9  = a[ 9]*b[i] +e8  +p[ 9], (cf=0),  e9 = HI( T9), p9 =LO( T9),  (bf9 ,p9 ) = p9 -bf8,      p[ 8] = p9,  bf9
; T10 = a[10]*b[i] +e9  +p[10], (cf=0),  e10= HI(T10), p10=LO(T10),  (bf10,p10) = p10-bf9,      p[ 9] = p10, bf9
; T11 = a[11]*b[i] +e10 +p[11], (cf=0),  e11= HI(T11), p11=LO(T11),  (bf11,p11) = p11-bf10,     p[10] = p11, bf10
;                                                                    (cf12,p12) = e11-bf11+u    p[11] = p12, cf12
;                                                                                               p[13] = cf12
      movd     mm7, eax                   ; save pass counter

      mov      edx, dword [ebp]     ; b = b[i]
      mov      eax, dword [esi]     ; a[0]
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
      adc      edx, 0
     ;pmuludq  mm1, mm0

      movd     ebx, mm2                   ; p = a[2]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*2]
      movd     mm2, dword [esi+sizeof(dword)*6]
      adc      edx, 0
      pmuludq  mm1, mm0
      pmuludq  mm2, mm0

      movd     ebp, mm3                   ; p = a[3]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*3]
      movd     mm3, dword [esi+sizeof(dword)*7]
      adc      edx, 0
     ;pmuludq  mm3, mm0

      movd     edi, mm4                   ; p = a[4]*b[i] + E
      psrlq    mm4, 32
      add      edi, edx
      movd     edx, mm4
      adc      edx, 0
      add      edi, dword [esp+_buf_+sizeof(dword)*4]
      movd     mm4, dword [esi+sizeof(dword)*8]
      adc      edx, 0
      pmuludq  mm3, mm0
      pmuludq  mm4, mm0

;;; and reduction ;;;
      add      ecx, eax                                     ; +u0
      mov      dword [esp+_buf_+sizeof(dword)*0], ecx
      mov      ecx, eax                                     ; save u0
      adc      ebx, 0                                       ; +cf
      mov      dword [esp+_buf_+sizeof(dword)*1], ebx
      sbb      eax, 0                                       ; eax = u0-cf
      sub      ebp, eax                                     ; ebp-eax = ebp+cf-u0
      mov      dword [esp+_buf_+sizeof(dword)*2], ebp
      sbb      edi, ecx                                     ; edi-u0-bf
      mov      eax, 0
      mov      dword [esp+_buf_+sizeof(dword)*3], edi
      movd     mm6, ecx
     ;sbb      eax, eax                                     ; save bf signu: eax = bf? 0xffffffff: 0x00000000
     adc       eax, 0

; multiplication round 5 - round 8
      movd     ecx, mm1                   ; p = a[5]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*5]
      movd     mm1, dword [esi+sizeof(dword)*9]
      adc      edx, 0
     ;pmuludq  mm1, mm0

      movd     ebx, mm2                   ; p = a[6]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*6]
      movd     mm2, dword [esi+sizeof(dword)*10]
      adc      edx, 0
      pmuludq  mm1, mm0
      pmuludq  mm2, mm0

      movd     ebp, mm3                   ; p = a[7]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*7]
      movd     mm3, dword [esi+sizeof(dword)*11]
      adc      edx, 0
      pmuludq  mm3, mm0

      movd     edi, mm4                   ; p = a[8]*b[i] + E
      psrlq    mm4, 32
      add      edi, edx
      movd     edx, mm4
      adc      edx, 0
      add      edi, dword [esp+_buf_+sizeof(dword)*8]
      adc      edx, 0

;;; and reduction ;;;
     ;add      eax, eax                                  ; restore bf
     ;sbb      ecx, 0                                    ; -bf
      sub      ecx, eax
      movd     eax, mm6
      mov      dword [esp+_buf_+sizeof(dword)*4], ecx
      sbb      ebx, 0                                    ; -bf
      mov      dword [esp+_buf_+sizeof(dword)*5], ebx
      sbb      ebp, 0                                    ; -bf
      mov      dword [esp+_buf_+sizeof(dword)*6], ebp
      sbb      edi, 0                                    ; -bf
      mov      eax, 0
      mov      dword [esp+_buf_+sizeof(dword)*7], edi
     ;sbb      eax, eax
      adc      eax, 0

; multiplication round 9 - round 11
      movd     ecx, mm1                   ; p = a[9]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*9]
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[10]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*10]
      adc      edx, 0

      movd     ebp, mm3                   ; p = a[11]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*11]
      adc      edx, 0

;;; and reduction ;;;
     ;add      eax, eax                                     ; restore bf
     ;sbb      ecx, 0                                       ; u0-bf
      sub      ecx, eax
      mov      dword [esp+_buf_+sizeof(dword)*8], ecx
      movd     ecx, mm6
      sbb      ebx, 0                                       ; -bf
      mov      dword [esp+_buf_+sizeof(dword)*9], ebx
      sbb      ebp, 0                                       ; -bf
      mov      dword [esp+_buf_+sizeof(dword)*10], ebp
      sbb      ecx, 0                                       ; u0-bf
      mov      ebx, 0

; last multiplication round 12
      movd     eax, mm7                                     ; restore pass counter

      add      edx, dword [esp+_buf_+sizeof(dword)*12]
      adc      ebx, 0
      add      edx, ecx
      adc      ebx, 0
      mov      dword [esp+_buf_+sizeof(dword)*11], edx
      mov      dword [esp+_buf_+sizeof(dword)*12], ebx

      sub      eax, 1
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
      LD_ADDR  ebx, p384r1_data        ; modulus
      lea      ebx, [ebx+(_prime384r1-p384r1_data)]
      CALL_IPPASM  sub_384
      mov      edx, dword [esp+_buf_+sizeof(dword)*12]
      sub      edx, eax

; copy
      cmovz    esi, edi
      cpy_384  edi, esi

      mov   esp, [esp+_sp_]            ; release stack
   REST_GPR
   ret
ENDFUNC p384r1_mul_mont_slm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_sqr_mont_slm(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p384r1_sqr_mont_slm,PUBLIC
  USES_GPR esi,edi

%xdefine pR [esp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [esp + ARG_1 + 1*sizeof(dword)] ; source A address

      ;; use p384r1_mul_mont_slm to compute sqr
      mov   esi, pA
      mov   edi, pR
      push  esi
      push  esi
      push  edi
      CALL_IPPASM  p384r1_mul_mont_slm,PUBLIC
      add   esp, sizeof(dword)*3
   REST_GPR
   ret
ENDFUNC p384r1_sqr_mont_slm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p384r1_mred(Ipp32u* r, Ipp32u* prod)
;;
; modulus = 2^384 -2^128 -2^96 +2^32 -1
;           [12]   [4]    [3]   [1]  [0]
; m0 = 1
;
align IPP_ALIGN_FACTOR
IPPASM p384r1_mred,PUBLIC
  USES_GPR ebx,esi,edi

%xdefine pR [esp + ARG_1 + 0*sizeof(dword)] ; reduction address
%xdefine pA [esp + ARG_1 + 1*sizeof(dword)] ; source product address

   ; get parameters:
   mov   esi, pA

   mov   ecx, LEN384
   xor   edx, edx
align IPP_ALIGN_FACTOR
.mred_loop:
   mov   eax, dword [esi]

   mov   ebx, dword [esi+sizeof(dword)]
   add   ebx, eax
   mov   dword [esi+sizeof(dword)], ebx

   mov   ebx, dword [esi+sizeof(dword)*2]
   adc   ebx, 0
   mov   dword [esi+sizeof(dword)*2], ebx

   mov   ebx, dword [esi+sizeof(dword)*3]
   sbb   eax, 0
   sub   ebx, eax
   mov   eax, dword [esi]
   mov   dword [esi+sizeof(dword)*3], ebx

   mov   ebx, dword [esi+sizeof(dword)*4]
   sbb   ebx, eax
   mov   dword [esi+sizeof(dword)*4], ebx

   mov   ebx, dword [esi+sizeof(dword)*5]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*5], ebx

   mov   ebx, dword [esi+sizeof(dword)*6]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*6], ebx

   mov   ebx, dword [esi+sizeof(dword)*7]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*7], ebx

   mov   ebx, dword [esi+sizeof(dword)*8]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*8], ebx

   mov   ebx, dword [esi+sizeof(dword)*9]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*9], ebx

   mov   ebx, dword [esi+sizeof(dword)*10]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*10], ebx

   mov   ebx, dword [esi+sizeof(dword)*11]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*11], ebx

   mov   ebx, dword [esi+sizeof(dword)*12]
   sbb   eax, 0
   add   eax, edx
   mov   edx, 0
   adc   edx, 0
   add   ebx, eax
   mov   dword [esi+sizeof(dword)*12], ebx
   adc   edx, 0

   lea   esi, [esi+sizeof(dword)]
   sub   ecx, 1
   jnz   .mred_loop

   ; final reduction
   mov      edi, pR           ; result
   LD_ADDR  ebx, p384r1_data  ; addres of the modulus
   lea      ebx, [ebx+(_prime384r1-p384r1_data)]
   CALL_IPPASM  sub_384

   sub      edx, eax
   cmovz    esi, edi
   cpy_384  edi, esi

   REST_GPR
   ret
ENDFUNC p384r1_mred

%endif    ;; _IPP >= _IPP_P8

