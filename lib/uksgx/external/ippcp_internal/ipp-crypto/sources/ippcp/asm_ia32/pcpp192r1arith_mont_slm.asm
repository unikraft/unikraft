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
;               P192r1 basic arithmetic function
;
;     Content:
;      p192r1_add
;      p192r1_sub
;      p192r1_neg
;      p192r1_div_by_2
;      p192r1_mul_mont_slm
;      p192r1_sqr_mont_slm
;      p192r1_mred
;      p192r1_select_ap_w7
;






%include "asmdefs.inc"
%include "ia_emm.inc"
%include "pcpvariant.inc"

%if (_IPP >= _IPP_P8)

segment .text align=IPP_ALIGN_FACTOR

;;
;; some p384r1 constants
;;
p192r1_data:
_prime192r1 DD 0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFEh,0FFFFFFFFh,0FFFFFFFFh,0FFFFFFFFh

%assign LEN192  (192/32) ; dword's length of operands

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Ipp32u add_192(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
;; input:   edi = r
;;          esi = a
;;          ebx = b
;;
;; output:  eax = carry = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM add_192,PRIVATE
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

      mov   eax, 0
      adc   eax, 0
      ret
ENDFUNC add_192

;;
;; Ipp32u sub_192(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
;; input:   edi = r
;;          esi = a
;;          ebx = b
;;
;; output:  eax = borrow = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM sub_192,PRIVATE
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

      mov   eax, 0
      adc   eax, 0
      ret
ENDFUNC sub_192

;;
;; Ipp32u shl_192(Ipp32u* r, const Ipp32u* a)
;;
;; input:   edi = r
;;          esi = a
;;
;; output:  eax = extension = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM shl_192,PRIVATE
      mov      eax, dword [esi+(LEN192-1)*sizeof(dword)]
      ; r = a<<1
      movq     xmm2, qword [esi+sizeof(oword)]
      movdqu   xmm1, oword [esi]

      movdqa   xmm3, xmm2
      palignr  xmm3, xmm1, sizeof(qword)
      psllq    xmm2, 1
      psrlq    xmm3, 63
      por      xmm2, xmm3
      movq     qword [edi+sizeof(oword)], xmm2

      movdqa   xmm3, xmm1
      pslldq   xmm3, sizeof(qword)
      psllq    xmm1, 1
      psrlq    xmm3, 63
      por      xmm1, xmm3
      movdqu   oword [edi], xmm1

      shr     eax, 31
      ret
ENDFUNC shl_192

;;
;; void shr_192(Ipp32u* r, const Ipp32u* a)
;;
;; input:   edi = r
;;          esi = a
;;          eax = ext
;; output:  eax = extension = 0/1
;;
align IPP_ALIGN_FACTOR
IPPASM shr_192,PRIVATE
      ; r = a>>1
      movdqu   xmm2, oword [esi]
      movq     xmm1, qword [esi+sizeof(oword)]

      movdqa   xmm3, xmm1
      palignr  xmm3, xmm2, sizeof(qword)
      psrlq    xmm2, 1
      psllq    xmm3, 63
      por      xmm2, xmm3
      movdqu   oword [edi], xmm2

      movdqa   xmm3, xmm0
      psrlq    xmm1, 1
      psllq    xmm3, 63
      por      xmm1, xmm3
      movq     qword [edi+sizeof(oword)], xmm1

      ret
ENDFUNC shr_192

;;
;; void cpy_192(Ipp32u* r, const Ipp32u* a)
;;
%macro cpy_192 2.nolist
  %xdefine %%pdst %1
  %xdefine %%psrc %2

   movdqu   xmm0, oword [%%psrc]
   movq     xmm1, qword [%%psrc+sizeof(oword)]
   movdqu   oword [%%pdst], xmm0
   movq     qword [%%pdst+sizeof(oword)], xmm1
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_add(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_add,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [ebp + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_   0                            ; buffer[LEN192]
%assign _sp_    _buf_+(LEN192)*sizeof(dword) ; esp[1]
%assign _frame_ _sp_+sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov      edi, pR                 ; pR
      mov      esi, pA                 ; pA
      mov      ebx, pB                 ; pB
      CALL_IPPASM  add_192                 ; R = A+B
      mov      edx, eax

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p192r1_data        ; modulus
      lea      ebx, [ebx+(_prime192r1-p192r1_data)]
      CALL_IPPASM  sub_192                 ; T = R-modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      sub      edx, eax                ; R = T<0? R : T
      cmovnz   esi, edi
      cpy_192  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p192r1_add


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_sub(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_sub,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [ebp + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_   0                            ; buffer[LEN192]
%assign _sp_    _buf_+(LEN192)*sizeof(dword) ; esp[1]
%assign _frame_ _sp_+sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      mov      edi, pR                 ; pR
      mov      esi, pA                 ; pA
      mov      ebx, pB                 ; pB
      CALL_IPPASM  sub_192                 ; R = A-B
      mov      edx, eax

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p192r1_data        ; modulus
      lea      ebx, [ebx+(_prime192r1-p192r1_data)]
      CALL_IPPASM  add_192                 ; T = R+modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      test     edx, edx                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_192  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p192r1_sub


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_neg(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_neg,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_  0                             ; buffer[LEN192]
%assign _sp_  _buf_+(LEN192)*sizeof(dword)  ; esp[1]
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
      sbb   edx,edx

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pR                 ; R
      LD_ADDR  ebx, p192r1_data        ; modulus
      lea      ebx, [ebx+(_prime192r1-p192r1_data)]
      CALL_IPPASM  add_192                 ; T = R+modulus

      lea      esi,[esp+_buf_]
      mov      edi, pR
      test     edx, edx                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_192  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p192r1_neg


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_mul_by_2(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_by_2,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_   0                            ; buffer[LEN192]
%assign _sp_    _buf_+(LEN192)*sizeof(dword) ; esp[1]
%assign _frame_ _sp_+sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pA                 ; pA
      CALL_IPPASM  shl_192                 ; T = A<<1
      mov      edx, eax

      mov      esi, edi                ; T
      mov      edi, pR                 ; R
      LD_ADDR  ebx, p192r1_data        ; modulus
      lea      ebx, [ebx+(_prime192r1-p192r1_data)]
      CALL_IPPASM  sub_192                 ; R = T-modulus

      sub      edx, eax                ; R = R<0? T : R
      cmovz    esi, edi
      cpy_192  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p192r1_mul_by_2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_mul_by_3(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_by_3,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _bufT_  0                             ; T buffer[LEN192]
%assign _bufU_  _bufT_+(LEN192)*sizeof(dword) ; U buffer[LEN192]
%assign _mod_   _bufU_+(LEN192)*sizeof(dword) ; modulus address [1]
%assign _sp_    _mod_+sizeof(dword)           ; esp [1]
%assign _frame_ _sp_+sizeof(dword)            ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      LD_ADDR  eax, p192r1_data        ; srore modulus address
      lea      eax, [eax+(_prime192r1-p192r1_data)]
      mov      dword [esp+_mod_], eax

      lea      edi, [esp+_bufT_]       ; T
      mov      esi, pA                 ; A
      CALL_IPPASM  shl_192                 ; T = A<<1
      mov      edx, eax

      mov      esi, edi                ; T
      lea      edi, [esp+_bufU_]       ; U
      mov      ebx, [esp+_mod_]        ; modulus
      CALL_IPPASM  sub_192                 ; U = T-modulus

      sub      edx, eax                ; T = U<0? T : U
      cmovz    esi, edi
      cpy_192  edi, esi

      mov      esi, edi
      mov      ebx, pA
      CALL_IPPASM  add_192                 ; T +=A
      mov      edx, eax

      mov      edi, pR                 ; R
      mov      ebx, [esp+_mod_]        ; modulus
      CALL_IPPASM  sub_192                 ; R = T-modulus

      sub      edx, eax                ; R = T<0? R : T
      cmovz    esi, edi
      cpy_192  edi, esi

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p192r1_mul_by_3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_div_by_2(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_div_by_2,PUBLIC
  USES_GPR esi,edi,ebx,ebp

      mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pR [ebp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [ebp + ARG_1 + 1*sizeof(dword)] ; source A address
;
; stack layout:
;
%assign _buf_   0                            ; buffer[LEN192]
%assign _sp_    _buf_+(LEN192)*sizeof(dword) ; esp[1]
%assign _frame_ _sp_+sizeof(dword)           ; +16 bytes for alignment

      mov   eax, esp                   ; save esp
      sub   esp, _frame_               ; allocate frame
      and   esp, -16                   ; provide 16-byte alignment
      mov   dword [esp+_sp_], eax   ; store esp

      lea      edi, [esp+_buf_]        ; T
      mov      esi, pA                 ; A
      LD_ADDR  ebx, p192r1_data        ; modulus
      lea      ebx, [ebx+(_prime192r1-p192r1_data)]
      CALL_IPPASM  add_192                 ; R = A+modulus
      mov      edx, 0

      mov      ecx, dword [esi]     ; shifted_data = (a[0]&1)? T : A
      and      ecx, 1
      cmovnz   esi, edi
      cmovz    eax, edx
      movd     xmm0, eax
      mov      edi, pR
      CALL_IPPASM  shr_192

      mov      esp, [esp+_sp_]
   REST_GPR
   ret
ENDFUNC p192r1_div_by_2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_mul_mont_slm(Ipp32u* r, const Ipp32u* a, const Ipp32u* b)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mul_mont_slm,PUBLIC
  USES_GPR ebp,ebx,esi,edi

%xdefine pR [eax + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [eax + ARG_1 + 1*sizeof(dword)] ; source A address
%xdefine pB [eax + ARG_1 + 2*sizeof(dword)] ; source B address
;
; stack layout:
;
%assign _buf_   0
%assign _rp_    _buf_+(LEN192+1)*sizeof(dword) ; pR
%assign _ap_    _rp_+sizeof(dword)             ; pA
%assign _bp_    _ap_+sizeof(dword)             ; pB
%assign _sp_    _bp_+sizeof(dword)             ; esp storage
%assign _ssize_ _sp_+sizeof(dword)             ; size allocated stack

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

      ; store parameters into the stack
      ; note: eax here stores an original esp, so it can be used to reach function parameters
      mov   edi, pR
      mov   esi, pA
      mov   ebp, pB
      mov   dword [esp+_rp_], edi
      mov   dword [esp+_ap_], esi
      mov   dword [esp+_bp_], ebp

      mov   edi, LEN192

      movd  mm1, dword [esi+sizeof(dword)]      ; pre load a[1], a[2]
      movd  mm2, dword [esi+sizeof(dword)*2]


align IPP_ALIGN_FACTOR
.mmul_loop:
;
; i-st pass
; modulus = 2^192 -2^64 -1
;            [6]   [2]  [0]
; m0 = 1
;
      movd     mm7, edi                   ; save pass counter

      mov      edx, dword [ebp]     ; b = b[i]
      mov      eax, dword [esi]     ; a[0]
      movd     mm0, edx
      add      ebp, sizeof(dword)
      mov      dword [esp+_bp_], ebp

      pmuludq  mm1, mm0                   ; a[1]*b[i]

      mul      edx                        ; (E:u) = (edx:eax) = a[0]*b[i]+buf[0]
      add      eax, dword [esp+_buf_]
      adc      edx, 0

      pmuludq  mm2, mm0                   ; a[2]*b[i]

; multiplication round 1 - round 2
      movd     ecx, mm1                   ; p = a[1]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*1]
      movd     mm1, dword [esi+sizeof(dword)*3]
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[2]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*2]
      movd     mm2, dword [esi+sizeof(dword)*4]
      movd     mm3, dword [esi+sizeof(dword)*5]
      adc      edx, 0

      pmuludq  mm1, mm0                   ; a[3]*b[i]
      pmuludq  mm2, mm0                   ; a[4]*b[i]
      pmuludq  mm3, mm0                   ; a[5]*b[i]

;;; and reduction ;;;
      mov      dword [esp+_buf_+sizeof(dword)*0], ecx    ; +0
      sub      ebx, eax                                     ; -u0
      mov      edi, 0
      mov      dword [esp+_buf_+sizeof(dword)*1], ebx
      adc      edi, 0                                       ; ssave bf

; multiplication round 3 - round 5
      movd     ecx, mm1                   ; p = a[3]*b[i] + E
      psrlq    mm1, 32
      add      ecx, edx
      movd     edx, mm1
      adc      edx, 0
      add      ecx, dword [esp+_buf_+sizeof(dword)*3]
      adc      edx, 0

      movd     ebx, mm2                   ; p = a[4]*b[i] + E
      psrlq    mm2, 32
      add      ebx, edx
      movd     edx, mm2
      adc      edx, 0
      add      ebx, dword [esp+_buf_+sizeof(dword)*4]
      adc      edx, 0

      movd     ebp, mm3                   ; p = a[5]*b[i] + E
      psrlq    mm3, 32
      add      ebp, edx
      movd     edx, mm3
      adc      edx, 0
      add      ebp, dword [esp+_buf_+sizeof(dword)*5]
      adc      edx, 0

;;; and reduction ;;;
      sub      ecx, edi                                     ; -cb
      mov      dword [esp+_buf_+sizeof(dword)*2], ecx
      sbb      ebx, 0                                       ; -bf
      mov      dword [esp+_buf_+sizeof(dword)*3], ebx
      sbb      ebp, 0                                       ; -bf
      mov      dword [esp+_buf_+sizeof(dword)*4], ebp

;;; last multiplication round 6
      movd     edi, mm7                   ; restore pass counter

      sbb      eax, 0                     ; u0 -bf
      mov      ebx, 0
      add      edx, dword [esp+_buf_+sizeof(dword)*6]
      adc      ebx, 0
      add      edx, eax
      adc      ebx, 0
      mov      dword [esp+_buf_+sizeof(dword)*5], edx
      mov      dword [esp+_buf_+sizeof(dword)*6], ebx

      sub      edi, 1
      movd  mm1, dword [esi+sizeof(dword)]               ; speculative load a[1], a[2], a[3], a[4]
      movd  mm2, dword [esi+sizeof(dword)*2]
      jz       .exit_mmul_loop

      mov      ebp, dword [esp+_bp_]            ; restore pB
      jmp      .mmul_loop

.exit_mmul_loop:
      emms

; final reduction
      mov      edi, [esp+_rp_]         ; result
      lea      esi, [esp+_buf_]        ; buffer
      LD_ADDR  ebx, p192r1_data        ; modulus
      lea      ebx, [ebx+(_prime192r1-p192r1_data)]
      CALL_IPPASM  sub_192
      mov      edx, dword [esp+_buf_+sizeof(dword)*LEN192]
      sub      edx, eax

; copy
      cmovz    esi, edi
      cpy_192  edi, esi

      mov   esp, [esp+_sp_]            ; release stack
   REST_GPR
   ret
ENDFUNC p192r1_mul_mont_slm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_sqr_mont_slm(Ipp32u* r, const Ipp32u* a)
;;
align IPP_ALIGN_FACTOR
IPPASM p192r1_sqr_mont_slm,PUBLIC
  USES_GPR esi,edi

%xdefine pR [esp + ARG_1 + 0*sizeof(dword)] ; product address
%xdefine pA [esp + ARG_1 + 1*sizeof(dword)] ; source A address

      ;; use p192r1_mul_mont_slm to compute sqr
      mov   esi, pA
      mov   edi, pR
      push  esi
      push  esi
      push  edi
      CALL_IPPASM p192r1_mul_mont_slm,PUBLIC
      add   esp, sizeof(dword)*3
   REST_GPR
   ret
ENDFUNC p192r1_sqr_mont_slm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; void p192r1_mred(Ipp32u* r, Ipp32u* prod)
;;
; modulus = 2^192  -2^64 -1
;           [6]    [2]  [0]
; m0 = 1
;
align IPP_ALIGN_FACTOR
IPPASM p192r1_mred,PUBLIC
  USES_GPR ebx,esi,edi

%xdefine pR [esp + ARG_1 + 0*sizeof(dword)] ; reduction address
%xdefine pA [esp + ARG_1 + 1*sizeof(dword)] ; source product address

   ; get parameters:
   mov   esi, pA

   mov   ecx, LEN192
   xor   edx, edx
align IPP_ALIGN_FACTOR
.mred_loop:
   mov   eax, dword [esi]

   mov   ebx, 0
   mov   dword [esi], ebx

   mov   ebx, dword [esi+sizeof(dword)]
   mov   dword [esi+sizeof(dword)], ebx

   mov   ebx, dword [esi+sizeof(dword)*2]
   sub   ebx, eax
   mov   dword [esi+sizeof(dword)*2], ebx

   mov   ebx, dword [esi+sizeof(dword)*3]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*3], ebx

   mov   ebx, dword [esi+sizeof(dword)*4]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*4], ebx

   mov   ebx, dword [esi+sizeof(dword)*5]
   sbb   ebx, 0
   mov   dword [esi+sizeof(dword)*5], ebx

   mov   ebx, dword [esi+sizeof(dword)*6]
   sbb   eax, 0
   add   eax, edx
   mov   edx, 0
   adc   edx, 0
   add   ebx, eax
   mov   dword [esi+sizeof(dword)*6], ebx
   adc   edx, 0

   lea   esi, [esi+sizeof(dword)]
   sub   ecx, 1
   jnz   .mred_loop

   ; final reduction
   mov      edi, pR           ; result
   LD_ADDR  ebx, p192r1_data  ; addres of the modulus
   lea      ebx, [ebx+(_prime192r1-p192r1_data)]
   CALL_IPPASM  sub_192

   sub      edx, eax
   cmovz    esi, edi
   cpy_192  edi, esi

   REST_GPR
   ret
ENDFUNC p192r1_mred

%endif    ;; _IPP >= _IPP_P8

