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
;               Big Number Operations
;
;     Content:
;        cpMontRedAdc_BNU()
;
;
;     History:
;      Extra reduction (R=A-M) has been added to perform MontReduction safe
;
;

%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpbnumulschool.inc"
%include "pcpvariant.inc"

%if (_ADCOX_NI_ENABLING_ == _FEATURE_OFF_) || (_ADCOX_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_M7) && (_IPP32E < _IPP32E_L9)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; fixed size reduction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%macro MREDUCE_FIX_STEP 12.nolist
  %xdefine %%mSize %1
  %xdefine %%X7 %2
  %xdefine %%X6 %3
  %xdefine %%X5 %4
  %xdefine %%X4 %5
  %xdefine %%X3 %6
  %xdefine %%X2 %7
  %xdefine %%X1 %8
  %xdefine %%X0 %9
  %xdefine %%rSrc %10
  %xdefine %%U %11
  %xdefine %%rCarry %12

%if %%mSize > 0
   mul   %%U
   xor   %%rCarry, %%rCarry
   add   %%X0, rax

%if %%mSize > 1
   mov   rax, qword [%%rSrc+sizeof(qword)]
   adc   %%rCarry, rdx
   mul   %%U
   add   %%X1, %%rCarry
   adc   rdx, 0
   xor   %%rCarry, %%rCarry
   add   %%X1, rax

%if %%mSize > 2
   mov   rax, qword [%%rSrc+sizeof(qword)*2]
   adc   %%rCarry, rdx
   mul   %%U
   add   %%X2, %%rCarry
   adc   rdx, 0
   xor   %%rCarry, %%rCarry
   add   %%X2, rax

%if %%mSize > 3
   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   adc   %%rCarry, rdx
   mul   %%U
   add   %%X3, %%rCarry
   adc   rdx, 0
   xor   %%rCarry, %%rCarry
   add   %%X3, rax
%if %%mSize  == 4
   adc   %%X4, rdx
   adc   %%rCarry, 0
   add   %%X4, qword [rsp+carry]
%endif
%endif ;; mSize==4

%if %%mSize == 3
   adc   %%X3, rdx
   adc   %%rCarry, 0
   add   %%X3, qword [rsp+carry]
%endif
%endif ;; mSize==3

%if %%mSize == 2
   adc   %%X2, rdx
   adc   %%rCarry, 0
   add   %%X2, qword [rsp+carry]
%endif
%endif ;; mSize==2

%if %%mSize == 1
   adc   %%X1, rdx
   adc   %%rCarry, 0
   add   %%X1, qword [rsp+carry]
%endif
%endif ;; mSize==1

   adc   %%rCarry, 0
   mov   qword [rsp+carry], %%rCarry
%endmacro

%macro MREDUCE_FIX 14.nolist
  %xdefine %%mSize %1
  %xdefine %%rDst %2
  %xdefine %%rSrc %3
  %xdefine %%M0 %4
  %xdefine %%X7 %5
  %xdefine %%X6 %6
  %xdefine %%X5 %7
  %xdefine %%X4 %8
  %xdefine %%X3 %9
  %xdefine %%X2 %10
  %xdefine %%X1 %11
  %xdefine %%X0 %12
  %xdefine %%U %13
  %xdefine %%rCarry %14

%if %%mSize > 0
   mov   %%U, %%X0
   imul  %%U, %%M0
   mov   rax, qword [%%rSrc]
   MREDUCE_FIX_STEP %%mSize, %%X7,%%X6,%%X5,%%X4,%%X3,%%X2,%%X1,%%X0, %%rSrc, %%U, %%rCarry

%if %%mSize > 1
   mov   %%U, %%X1
   imul  %%U, %%M0
   mov   rax, qword [%%rSrc]
   MREDUCE_FIX_STEP %%mSize, %%X0,%%X7,%%X6,%%X5,%%X4,%%X3,%%X2,%%X1, %%rSrc, %%U, %%rCarry

%if %%mSize > 2
   mov   %%U, %%X2
   imul  %%U, %%M0
   mov   rax, qword [%%rSrc]
   MREDUCE_FIX_STEP %%mSize, %%X1,%%X0,%%X7,%%X6,%%X5,%%X4,%%X3,%%X2, %%rSrc, %%U, %%rCarry

%if %%mSize > 3 ; mSize == 4
   mov   %%U, %%X3
   imul  %%U, %%M0
   mov   rax, qword [%%rSrc]
   MREDUCE_FIX_STEP %%mSize, %%X2,%%X1,%%X0,%%X7,%%X6,%%X5,%%X4,%%X3, %%rSrc, %%U, %%rCarry

   mov   %%X0, %%X4                              ; {X3:X2:X1:X0} = {X7:X6:X5:X4}
   sub   %%X4, qword [%%rSrc]                ; {X7:X6:X5:X4}-= modulus
   mov   %%X1, %%X5
   sbb   %%X5, qword [%%rSrc+sizeof(qword)]
   mov   %%X2, %%X6
   sbb   %%X6, qword [%%rSrc+sizeof(qword)*2]
   mov   %%X3, %%X7
   sbb   %%X7, qword [%%rSrc+sizeof(qword)*3]
   sbb   %%rCarry, 0

   cmovc %%X4, %%X0                              ; dst = borrow? {X3:X2:X1:X0} : {X7:X6:X5:X4}
   mov   qword [%%rDst], %%X4
   cmovc %%X5, %%X1
   mov   qword [%%rDst+sizeof(qword)], %%X5
   cmovc %%X6, %%X2
   mov   qword [%%rDst+sizeof(qword)*2], %%X6
   cmovc %%X7, %%X3
   mov   qword [%%rDst+sizeof(qword)*3], %%X7
%endif
%endif
%endif
%endif

%if %%mSize == 3
   mov   %%X0, %%X3                              ; {X2:X1:X0} = {X5:X4:X3}
   sub   %%X3, qword [%%rSrc]                ; {X5:X4:X3}-= modulus
   mov   %%X1, %%X4
   sbb   %%X4, qword [%%rSrc+sizeof(qword)]
   mov   %%X2, %%X5
   sbb   %%X5, qword [%%rSrc+sizeof(qword)*2]
   sbb   %%rCarry, 0

   cmovc %%X3, %%X0                              ; dst = borrow? {X2:X1:X0} : {X5:X4:X3}
   mov   qword [%%rDst], %%X3
   cmovc %%X4, %%X1
   mov   qword [%%rDst+sizeof(qword)], %%X4
   cmovc %%X5, %%X2
   mov   qword [%%rDst+sizeof(qword)*2], %%X5
%endif ; mSize==3

%if %%mSize == 2
   mov   %%X0, %%X2                              ; {X1:X0} = {X3:X2}
   sub   %%X2, qword [%%rSrc]                ; {X3:X2}-= modulus
   mov   %%X1, %%X3
   sbb   %%X3, qword [%%rSrc+sizeof(qword)]
   sbb   %%rCarry, 0

   cmovc %%X2, %%X0                              ; dst = borrow? {X1:X0} : {X3:X2}
   mov   qword [%%rDst], %%X2
   cmovc %%X3, %%X1
   mov   qword [%%rDst+sizeof(qword)], %%X3
%endif ; mSize==2

%if %%mSize == 1
   mov   %%X0, %%X1                              ; X1 = X0
   sub   %%X1, qword [%%rSrc]                ; X1 -= modulus
   sbb   %%rCarry, 0

   cmovc %%X1, %%X0                              ; dst = borrow? X0 : X1
   mov   qword [%%rDst], %%X1
%endif ; mSize==1
%endmacro

%if (_IPP32E <= _IPP32E_Y8)
;;
;; Pre- Sandy Brige specific code
;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; product = + modulus * U0

;; main body: product = + modulus * U0
%macro MLAx1 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

  ALIGN IPP_ALIGN_FACTOR
%%L_1:
   mul   %%U0
   xor   %%T2, %%T2
   add   %%T0, qword [%%rDst+%%idx*sizeof(qword)-sizeof(qword)]
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)]
   adc   %%T2, rdx
   mov   qword [%%rDst+%%idx*sizeof(qword)-sizeof(qword)], %%T0

   mul   %%U0
   xor   %%T3, %%T3
   add   %%T1, qword [%%rDst+%%idx*sizeof(qword)]
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)*2]
   adc   %%T3, rdx
   mov   qword [%%rDst+%%idx*sizeof(qword)], %%T1

   mul   %%U0
   xor   %%T0, %%T0
   add   %%T2, qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)]
   adc   %%T3, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)*3]
   adc   %%T0, rdx
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)], %%T2

   mul   %%U0
   xor   %%T1, %%T1
   add   %%T3, qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*2]
   adc   %%T0, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)*4]
   adc   %%T1, rdx
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*2], %%T3

   add   %%idx, 4
   jnc   %%L_1
%endmacro

;; elipogue: product = modulus * U0, (srcLen=4*n+1)
%macro MM_MLAx1_4N_1_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   rax, qword [%%rSrc+sizeof(qword)]
   mov   %%idx, qword [rsp+counter]

   mul   %%U0
   xor   %%T3, %%T3
   add   %%T1, qword [%%rDst]
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]
   adc   %%T3, rdx
   mov   qword [%%rDst], %%T1

   mul   %%U0
   xor   %%T0, %%T0
   add   %%T2, qword [%%rDst+sizeof(qword)]
   adc   %%T3, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   adc   %%T0, rdx
   mov   qword [%%rDst+sizeof(qword)], %%T2

   mul   %%U0
   add   %%T3, qword [%%rDst+sizeof(qword)*2]
   adc   %%T0, rax
   adc   rdx, 0
   xor   rax, rax
   mov   qword [%%rDst+sizeof(qword)*2], %%T3

   add   %%T0, qword [%%rDst+sizeof(qword)*3]
   adc   rdx, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
   mov   qword [%%rDst+sizeof(qword)*4], rdx
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

;; elipogue: product = modulus * U0, (srcLen=4*n+4)
%macro MM_MLAx1_4N_4_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   rax, qword [%%rSrc+sizeof(qword)*2]
   mov   %%idx, qword [rsp+counter]

   mul   %%U0
   xor   %%T3, %%T0
   add   %%T1, qword [%%rDst+sizeof(qword)]
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   adc   %%T3, rdx
   mov   qword [%%rDst+sizeof(qword)], %%T1

   mul   %%U0
   add   %%T2, qword [%%rDst+sizeof(qword)*2]
   adc   %%T3, rax
   adc   rdx, 0
   xor   rax, rax
   mov   qword [%%rDst+sizeof(qword)*2], %%T2

   add   %%T3, qword [%%rDst+sizeof(qword)*3]
   adc   rdx, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T3
   mov   qword [%%rDst+sizeof(qword)*4], rdx
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

;; elipogue: product = modulus * U0, (srcLen=4*n+3)
%macro MM_MLAx1_4N_3_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   mov   %%idx, qword [rsp+counter]

   mul   %%U0
   add   %%T1, qword [%%rDst+sizeof(qword)*2]
   adc   %%T2, rax
   adc   rdx, 0
   xor   rax, rax
   mov   qword [%%rDst+sizeof(qword)*2], %%T1

   add   %%T2, qword [%%rDst+sizeof(qword)*3]
   adc   rdx, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T2
   mov   qword [%%rDst+sizeof(qword)*4], rdx
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

;; elipogue: product = modulus * U0, (srcLen=4*n+2)
%macro MM_MLAx1_4N_2_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   %%idx, qword [rsp+counter]
   xor   rax, rax

   add   %%T1, qword [%%rDst+sizeof(qword)*3]
   adc   %%T2, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T1
   mov   qword [%%rDst+sizeof(qword)*4], %%T2
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

%endif


%if (_IPP32E >= _IPP32E_E9)
;;
;; Sandy Brige specific code
;;
%macro MLAx1 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

  ALIGN IPP_ALIGN_FACTOR
%%L_1:
   mul   %%U0
   xor   %%T2, %%T2
   add   %%T0, qword [%%rDst+%%idx*sizeof(qword)-sizeof(qword)]
   mov   qword [%%rDst+%%idx*sizeof(qword)-sizeof(qword)], %%T0
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)]
   adc   %%T2, rdx

   mul   %%U0
   xor   %%T3, %%T3
   add   %%T1, qword [%%rDst+%%idx*sizeof(qword)]
   mov   qword [%%rDst+%%idx*sizeof(qword)], %%T1
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)*2]
   adc   %%T3, rdx

   mul   %%U0
   xor   %%T0, %%T0
   add   %%T2, qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)]
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)], %%T2
   adc   %%T3, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)*3]
   adc   %%T0, rdx

   mul   %%U0
   xor   %%T1, %%T1
   add   %%T3, qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*2]
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*2], %%T3
   adc   %%T0, rax
   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)*4]
   adc   %%T1, rdx

   add   %%idx, 4
   jnc   %%L_1
%endmacro

%macro MM_MLAx1_4N_1_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   rax, qword [%%rSrc+sizeof(qword)]
   mov   %%idx, qword [rsp+counter]

   mul   %%U0
   xor   %%T3, %%T3
   add   %%T1, qword [%%rDst]
   mov   qword [%%rDst], %%T1
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]
   adc   %%T3, rdx

   mul   %%U0
   xor   %%T0, %%T0
   add   %%T2, qword [%%rDst+sizeof(qword)]
   mov   qword [%%rDst+sizeof(qword)], %%T2
   adc   %%T3, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   adc   %%T0, rdx

   mul   %%U0
   add   %%T3, qword [%%rDst+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T3
   adc   %%T0, rax
   adc   rdx, 0
   xor   rax, rax

   add   %%T0, qword [%%rDst+sizeof(qword)*3]
   adc   rdx, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
   mov   qword [%%rDst+sizeof(qword)*4], rdx
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

%macro MM_MLAx1_4N_4_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   rax, qword [%%rSrc+sizeof(qword)*2]
   mov   %%idx, qword [rsp+counter]

   mul   %%U0
   xor   %%T3, %%T0
   add   %%T1, qword [%%rDst+sizeof(qword)]
   mov   qword [%%rDst+sizeof(qword)], %%T1
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   adc   %%T3, rdx

   mul   %%U0
   add   %%T2, qword [%%rDst+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T2
   adc   %%T3, rax
   adc   rdx, 0
   xor   rax, rax

   add   %%T3, qword [%%rDst+sizeof(qword)*3]
   adc   rdx, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T3
   mov   qword [%%rDst+sizeof(qword)*4], rdx
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

%macro MM_MLAx1_4N_3_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   mov   %%idx, qword [rsp+counter]

   mul   %%U0
   add   %%T1, qword [%%rDst+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T1
   adc   %%T2, rax
   adc   rdx, 0
   xor   rax, rax

   add   %%T2, qword [%%rDst+sizeof(qword)*3]
   adc   rdx, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T2
   mov   qword [%%rDst+sizeof(qword)*4], rdx
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

%macro MM_MLAx1_4N_2_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov   %%idx, qword [rsp+counter]
   xor   rax, rax

   add   %%T1, qword [%%rDst+sizeof(qword)*3]
   adc   %%T2, qword [%%rDst+sizeof(qword)*4]
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*3], %%T1
   mov   qword [%%rDst+sizeof(qword)*4], %%T2
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)
%endmacro

%endif

;
; prologue:
; pre-compute:
;  - u0 = product[0]*m0
;  - u1 = (product[1] + LO(modulo[1]*u0) + HI(modulo[0]*u0) carry(product[0]+LO(modulo[0]*u0)))*m0
;
%macro MMx2_PLOG 10.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%m0 %4
  %xdefine %%U0 %5
  %xdefine %%U1 %6
  %xdefine %%T0 %7
  %xdefine %%T1 %8
  %xdefine %%T2 %9
  %xdefine %%T3 %10

   mov   %%U0, qword [%%rDst+%%idx*sizeof(qword)]                ; product[0]
   imul  %%U0, %%m0                                                ; u0 = product[0]*m0

   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)]               ; modulo[0]*u0
   mul   %%U0

   mov   %%U1, qword [%%rSrc+%%idx*sizeof(qword)+sizeof(qword)]  ; modulo[1]*u0
   imul  %%U1, %%U0

   mov   %%T0, rax                                               ; save  modulo[0]*u0
   mov   %%T1, rdx

   add   rax, qword [rdi+%%idx*sizeof(qword)]                ; a[1] = product[1] + LO(modulo[1]*u0)
   adc   rdx, qword [rdi+%%idx*sizeof(qword)+sizeof(qword)]  ;      + HI(modulo[0]*u0)
   add   %%U1, rdx                                               ;      + carry(product[0]+LO(modulo[0]*u0))
   imul  %%U1, %%m0                                                ; u1 = a[1]*m0

   mov   rax, qword [%%rSrc+%%idx*sizeof(qword)]
   xor   %%T2, %%T2
%endmacro

%if (_IPP32E <= _IPP32E_Y8)
;;
;; Pre- Sandy Brige specific code
;;
%macro MM_MLAx2_4N_1_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += modulus[mSize-1]*U1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T0, qword [%%rDst+sizeof(qword)*3]
   adc   %%T1, rax
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
   adc   %%T2, rdx
   xor   rax, rax

   add   %%T1, qword [%%rDst+sizeof(qword)*4]
   adc   %%T2, qword [%%rDst+sizeof(qword)*5]
   adc   rax, 0
   add   %%T1, qword [rsp+carry]
   adc   %%T2, 0
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*4], %%T1
   mov   qword [%%rDst+sizeof(qword)*5], %%T2
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%macro MM_MLAx2_4N_2_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += a[mSize-2]*U1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T2, rdx

   mul   %%U0                                                       ; {T3:T2:T1} += a[mSize-1]*U0 + r[mSize-2]
   add   %%T0, qword [%%rDst+sizeof(qword)*2]
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T2, rdx
   mov   qword [%%rDst+sizeof(qword)*2], %%T0
   adc   %%T3, 0

   mul   %%U1                                                       ; {T3:T2} += a[mSize-1]*B1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T1, qword [%%rDst+sizeof(qword)*3]
   adc   %%T2, rax
   mov   qword [%%rDst+sizeof(qword)*3], %%T1
   adc   %%T3, rdx
   xor   rax, rax

   add   %%T2, qword [%%rDst+sizeof(qword)*4]
   adc   %%T3, qword [%%rDst+sizeof(qword)*5]
   adc   rax, 0
   add   %%T2, qword [rsp+carry]
   adc   %%T3, 0
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*4], %%T2
   mov   qword [%%rDst+sizeof(qword)*5], %%T3
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%macro MM_MLAx2_4N_3_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += a[mSize-3]*U1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[mSize-2]
   adc   %%T2, rdx

   mul   %%U0                                                       ; {T3:T2:T1} += a[mSize-2]*U0 + r[mSize-3]
   add   %%T0, qword [%%rDst+sizeof(qword)]
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[mSize-2]
   adc   %%T2, rdx
   mov   qword [%%rDst+sizeof(qword)], %%T0
   adc   %%T3, 0

   mul   %%U1                                                       ; {T3:T2} += a[mSize-2]*U1
   xor   %%T0, %%T0
   add   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T3, rdx

   mul   %%U0                                                       ; {T0:T3:T2} += a[mSize-1]*U0 + r[mSize-2]
   add   %%T1, qword [rdi+sizeof(qword)*2]
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T3, rdx
   mov   qword [rdi+sizeof(qword)*2], %%T1
   adc   %%T0, 0

   mul   %%U1                                                       ; {T0:T3} += a[mSize-1]*U1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T2, qword [rdi+sizeof(qword)*3]
   adc   %%T3, rax
   mov   qword [rdi+sizeof(qword)*3], %%T2
   adc   %%T0, rdx
   xor   rax, rax

   add   %%T3, qword [rdi+sizeof(qword)*4]
   adc   %%T0, qword [rdi+sizeof(qword)*5]
   adc   rax, 0
   add   %%T3, qword [rsp+carry]
   adc   %%T0, 0
   adc   rax, 0
   mov   qword [rdi+sizeof(qword)*4], %%T3
   mov   qword [rdi+sizeof(qword)*5], %%T0
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%macro MM_MLAx2_4N_4_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += a[mSize-4]*U1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)]                      ; a[lenA-3]
   adc   %%T2, rdx

   mul   %%U0                                                       ; {T3:T2:T1} += a[mSize-3]*U0 + r[mSize-4]
   add   %%T0, qword [%%rDst]
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)]                      ; a[mSize-3]
   adc   %%T2, rdx
   mov   qword [%%rDst], %%T0
   adc   %%T3, 0

   mul   %%U1                                                       ; {T3:T2} += a[mSize-3]*U1
   xor   %%T0, %%T0
   add   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[mSize-2]
   adc   %%T3, rdx

   mul   %%U0                                                       ; {T0:T3:T2} += a[mSize-2]*U0 + r[mSize-3]
   add   %%T1, qword [%%rDst+sizeof(qword)]
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a{lenA-2]
   adc   %%T3, rdx
   mov   qword [%%rDst+sizeof(qword)], %%T1
   adc   %%T0, 0

   mul   %%U1                                                       ; {T0:T3} += a[mSize-2]*U1
   xor   %%T1, %%T1
   add   %%T3, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T0, rdx

   mul   %%U0                                                       ; {T1:T0:T3} += a[mSize-1]*U0 + r[mSize-2]
   add   %%T2, qword [%%rDst+sizeof(qword)*2]
   adc   %%T3, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T0, rdx
   mov   qword [%%rDst+sizeof(qword)*2], %%T2
   adc   %%T1, 0

   mul   %%U1                                                       ; {T1:T0} += a[mSize-1]*U1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T3, qword [%%rDst+sizeof(qword)*3]
   adc   %%T0, rax
   mov   qword [%%rDst+sizeof(qword)*3], %%T3
   adc   %%T1, rdx
   xor   rax, rax

   add   %%T0, qword [%%rDst+sizeof(qword)*4]
   adc   %%T1, qword [%%rDst+sizeof(qword)*5]
   adc   rax, 0
   add   %%T0, qword [rsp+carry]
   adc   %%T1, 0
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*4], %%T0
   mov   qword [%%rDst+sizeof(qword)*5], %%T1
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%endif

%if (_IPP32E >= _IPP32E_E9)
;;
;; Sandy Brige specific code
;;
%macro MM_MLAx2_4N_1_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += modulus[mSize-1]*U1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T0, qword [%%rDst+sizeof(qword)*3]
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
   adc   %%T1, rax
   adc   %%T2, rdx
   xor   rax, rax

   add   %%T1, qword [%%rDst+sizeof(qword)*4]
   adc   %%T2, qword [%%rDst+sizeof(qword)*5]
   adc   rax, 0
   add   %%T1, qword [rsp+carry]
   adc   %%T2, 0
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*4], %%T1
   mov   qword [%%rDst+sizeof(qword)*5], %%T2
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%macro MM_MLAx2_4N_2_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += a[mSize-2]*U1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T2, rdx

   mul   %%U0                                                       ; {T3:T2:T1} += a[mSize-1]*U0 + r[mSize-2]
   add   %%T0, qword [%%rDst+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T0
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T2, rdx
   adc   %%T3, 0

   mul   %%U1                                                       ; {T3:T2} += a[mSize-1]*B1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T1, qword [%%rDst+sizeof(qword)*3]
   mov   qword [%%rDst+sizeof(qword)*3], %%T1
   adc   %%T2, rax
   adc   %%T3, rdx
   xor   rax, rax

   add   %%T2, qword [%%rDst+sizeof(qword)*4]
   adc   %%T3, qword [%%rDst+sizeof(qword)*5]
   adc   rax, 0
   add   %%T2, qword [rsp+carry]
   adc   %%T3, 0
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*4], %%T2
   mov   qword [%%rDst+sizeof(qword)*5], %%T3
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%macro MM_MLAx2_4N_3_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += a[mSize-3]*U1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[mSize-2]
   adc   %%T2, rdx

   mul   %%U0                                                       ; {T3:T2:T1} += a[mSize-2]*U0 + r[mSize-3]
   add   %%T0, qword [%%rDst+sizeof(qword)]
   mov   qword [%%rDst+sizeof(qword)], %%T0
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[mSize-2]
   adc   %%T2, rdx
   adc   %%T3, 0

   mul   %%U1                                                       ; {T3:T2} += a[mSize-2]*U1
   xor   %%T0, %%T0
   add   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T3, rdx

   mul   %%U0                                                       ; {T0:T3:T2} += a[mSize-1]*U0 + r[mSize-2]
   add   %%T1, qword [rdi+sizeof(qword)*2]
   mov   qword [rdi+sizeof(qword)*2], %%T1
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T3, rdx
   adc   %%T0, 0

   mul   %%U1                                                       ; {T0:T3} += a[mSize-1]*U1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T2, qword [rdi+sizeof(qword)*3]
   mov   qword [rdi+sizeof(qword)*3], %%T2
   adc   %%T3, rax
   adc   %%T0, rdx
   xor   rax, rax

   add   %%T3, qword [rdi+sizeof(qword)*4]
   adc   %%T0, qword [rdi+sizeof(qword)*5]
   adc   rax, 0
   add   %%T3, qword [rsp+carry]
   adc   %%T0, 0
   adc   rax, 0
   mov   qword [rdi+sizeof(qword)*4], %%T3
   mov   qword [rdi+sizeof(qword)*5], %%T0
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%macro MM_MLAx2_4N_4_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%idx %3
  %xdefine %%U0 %4
  %xdefine %%U1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%U1                                                       ; {T2:T1} += a[mSize-4]*U1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)]                      ; a[lenA-3]
   adc   %%T2, rdx

   mul   %%U0                                                       ; {T3:T2:T1} += a[mSize-3]*U0 + r[mSize-4]
   add   %%T0, qword [%%rDst]
   mov   qword [%%rDst], %%T0
   adc   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)]                      ; a[mSize-3]
   adc   %%T2, rdx
   adc   %%T3, 0

   mul   %%U1                                                       ; {T3:T2} += a[mSize-3]*U1
   xor   %%T0, %%T0
   add   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[mSize-2]
   adc   %%T3, rdx

   mul   %%U0                                                       ; {T0:T3:T2} += a[mSize-2]*U0 + r[mSize-3]
   add   %%T1, qword [%%rDst+sizeof(qword)]
   mov   qword [%%rDst+sizeof(qword)], %%T1
   adc   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a{lenA-2]
   adc   %%T3, rdx
   adc   %%T0, 0

   mul   %%U1                                                       ; {T0:T3} += a[mSize-2]*U1
   xor   %%T1, %%T1
   add   %%T3, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T0, rdx

   mul   %%U0                                                       ; {T1:T0:T3} += a[mSize-1]*U0 + r[mSize-2]
   add   %%T2, qword [%%rDst+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T2
   adc   %%T3, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[mSize-1]
   adc   %%T0, rdx
   adc   %%T1, 0

   mul   %%U1                                                       ; {T1:T0} += a[mSize-1]*U1 + r[mSize-1]
   mov   %%idx, qword [rsp+counter]
   add   %%T3, qword [%%rDst+sizeof(qword)*3]
   mov   qword [%%rDst+sizeof(qword)*3], %%T3
   adc   %%T0, rax
   adc   %%T1, rdx
   xor   rax, rax

   add   %%T0, qword [%%rDst+sizeof(qword)*4]
   adc   %%T1, qword [%%rDst+sizeof(qword)*5]
   adc   rax, 0
   add   %%T0, qword [rsp+carry]
   adc   %%T1, 0
   adc   rax, 0
   mov   qword [%%rDst+sizeof(qword)*4], %%T0
   mov   qword [%%rDst+sizeof(qword)*5], %%T1
   mov   qword [rsp+carry], rax

   add   %%rDst, sizeof(qword)*2
%endmacro

%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro SBB_x4 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc1 %2
  %xdefine %%rSrc2 %3
  %xdefine %%idx %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8
  %xdefine %%rcf %9

  ALIGN IPP_ALIGN_FACTOR
%%L_1:
   add   %%rcf, %%rcf    ; restore cf

   mov   %%T0, qword [%%rSrc1+%%idx*sizeof(qword)]
   sbb   %%T0, qword [%%rSrc2+%%idx*sizeof(qword)]
   mov   qword [%%rDst+%%idx*sizeof(qword)], %%T0

   mov   %%T1, qword [%%rSrc1+%%idx*sizeof(qword)+sizeof(qword)]
   sbb   %%T1, qword [%%rSrc2+%%idx*sizeof(qword)+sizeof(qword)]
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)], %%T1

   mov   %%T2, qword [%%rSrc1+%%idx*sizeof(qword)+sizeof(qword)*2]
   sbb   %%T2, qword [%%rSrc2+%%idx*sizeof(qword)+sizeof(qword)*2]
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*2], %%T2

   mov   %%T3, qword [%%rSrc1+%%idx*sizeof(qword)+sizeof(qword)*3]
   sbb   %%T3, qword [%%rSrc2+%%idx*sizeof(qword)+sizeof(qword)*3]
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*3], %%T3

   sbb   %%rcf, %%rcf    ; save cf
   add   %%idx, 4
   jnc   %%L_1
%endmacro

%macro SBB_TAIL 9.nolist
  %xdefine %%N %1
  %xdefine %%rDst %2
  %xdefine %%rSrc1 %3
  %xdefine %%rSrc2 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8
  %xdefine %%rcf %9

   add   %%rcf, %%rcf                      ; restore cf
   mov   idx, qword [rsp+counter]  ; restore counter

%if %%N > 3
   mov   %%T0, qword [%%rSrc1]
   sbb   %%T0, qword [%%rSrc2]
   mov   qword [%%rDst], %%T0
%endif

%if %%N > 2
   mov   %%T1, qword [%%rSrc1+sizeof(qword)]
   sbb   %%T1, qword [%%rSrc2+sizeof(qword)]
   mov   qword [%%rDst+sizeof(qword)], %%T1
%endif

%if %%N > 1
   mov   %%T2, qword [%%rSrc1+sizeof(qword)*2]
   sbb   %%T2, qword [%%rSrc2+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T2
%endif

%if %%N > 0
   mov   %%T3, qword [%%rSrc1+sizeof(qword)*3]
   sbb   %%T3, qword [%%rSrc2+sizeof(qword)*3]
   mov   qword [%%rDst+sizeof(qword)*3], %%T3
%endif

   sbb   rax, 0
   sbb   %%rcf, %%rcf    ; set cf
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; dst[] = cf? src1[] : src2[]
%macro CMOV_x4 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc1 %2
  %xdefine %%rSrc2 %3
  %xdefine %%idx %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8
  %xdefine %%rcf %9

   mov   %%T3, %%rcf                 ; copy cf
  ALIGN IPP_ALIGN_FACTOR
%%L_1:
   add   %%T3, %%T3                  ; restore cf

   mov   %%T0, qword [%%rSrc2+%%idx*sizeof(qword)]
   mov   %%T1, qword [%%rSrc1+%%idx*sizeof(qword)]
   mov   %%T2, qword [%%rSrc2+%%idx*sizeof(qword)+sizeof(qword)]
   mov   %%T3, qword [%%rSrc1+%%idx*sizeof(qword)+sizeof(qword)]
   cmovc %%T0, %%T1
   mov   qword [%%rDst+%%idx*sizeof(qword)], %%T0
   cmovc %%T2, %%T3
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)], %%T2

   mov   %%T0, qword [%%rSrc2+%%idx*sizeof(qword)+sizeof(qword)*2]
   mov   %%T1, qword [%%rSrc1+%%idx*sizeof(qword)+sizeof(qword)*2]
   mov   %%T2, qword [%%rSrc2+%%idx*sizeof(qword)+sizeof(qword)*3]
   mov   %%T3, qword [%%rSrc1+%%idx*sizeof(qword)+sizeof(qword)*3]
   cmovc %%T0, %%T1
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*2], %%T0
   cmovc %%T2, %%T3
   mov   qword [%%rDst+%%idx*sizeof(qword)+sizeof(qword)*3], %%T2

   mov   %%T3, %%rcf                 ; copy cf
   add   %%idx, 4
   jnc   %%L_1
%endmacro

%macro CMOV_TAIL 9.nolist
  %xdefine %%N %1
  %xdefine %%rDst %2
  %xdefine %%rSrc1 %3
  %xdefine %%rSrc2 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8
  %xdefine %%rcf %9

   add   %%rcf, %%rcf                      ; restore cf
   mov   idx, qword [rsp+counter]  ; restore counter

%if %%N > 3
   mov   %%T0, qword [%%rSrc2]
   mov   %%T1, qword [%%rSrc1]
   cmovc %%T0, %%T1
   mov   qword [%%rDst], %%T0
%endif

%if %%N > 2
   mov   %%T2, qword [%%rSrc2+sizeof(qword)]
   mov   %%T3, qword [%%rSrc1+sizeof(qword)]
   cmovc %%T2, %%T3
   mov   qword [%%rDst+sizeof(qword)], %%T2
%endif

%if %%N > 1
   mov   %%T0, qword [%%rSrc2+sizeof(qword)*2]
   mov   %%T1, qword [%%rSrc1+sizeof(qword)*2]
   cmovc %%T0, %%T1
   mov   qword [%%rDst+sizeof(qword)*2], %%T0
%endif

%if %%N > 0
   mov   %%T2, qword [%%rSrc2+sizeof(qword)*3]
   mov   %%T3, qword [%%rSrc1+sizeof(qword)*3]
   cmovc %%T2, %%T3
   mov   qword [%%rDst+sizeof(qword)*3], %%T2
%endif
%endmacro

segment .text align=IPP_ALIGN_FACTOR


;*************************************************************
;* void cpMontRedAdc_BNU(Ipp64u* pR,
;*                       Ipp64u* pBuffer,
;*                 const Ipp64u* pModulo, int mSize, Ipp64u m0)
;*
;*************************************************************

;;
;; Lib = M7
;;
;; Caller = ippsMontMul
;;
align IPP_ALIGN_FACTOR
IPPASM cpMontRedAdc_BNU,PUBLIC
%assign LOCAL_FRAME (1+1+1+1)*sizeof(qword)
        USES_GPR rbx,rbp,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 5

;pR        (rdi) address of the reduction
;pBuffer   (rsi) address of the temporary product
;pModulo   (rdx) address of the modulus
;mSize     (rcx) size    of the modulus
;m0        (r8)  helper

;;
;; stack structure:
%assign pR      (0)                   ; reduction address
%assign mSize   (pR+sizeof(qword))    ; modulus length (qwords)
%assign carry   (mSize+sizeof(qword)) ; carry from previous MLAx1 or MLAx2 operation
%assign counter (carry+sizeof(qword)) ; counter = 4-mSize

   mov      qword [rsp+carry], 0   ; clear carry
   movsxd   rcx, ecx                   ; expand modulus length
   mov      r15, r8                    ; helper m0

   cmp   rcx, 5
   jge   .general_case

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; reducer of the fixed sizes
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   cmp   rcx, 3
   ja    .mSize_4     ; rcx=4
   jz    .mSize_3     ; rcx=3
   jp    .mSize_2     ; rcx=2
   ;     mSize_1     ; rcx=1

%xdefine X0    r8
%xdefine X1    r9
%xdefine X2    r10
%xdefine X3    r11
%xdefine X4    r12
%xdefine X5    r13
%xdefine X6    r14
%xdefine X7    rcx

align IPP_ALIGN_FACTOR
.mSize_1:
   mov   X0, qword [rsi]
   mov   X1, qword [rsi+sizeof(qword)]
   mov   rsi, rdx
                ; rDst,rSrc, U,  M0,  T0
   MREDUCE_FIX 1, rdi, rsi, r15, X7, X6, X5, X4, X3, X2, X1,X0, rbp,rbx
   jmp   .quit

align IPP_ALIGN_FACTOR
.mSize_2:
   mov   X0, qword [rsi]
   mov   X1, qword [rsi+sizeof(qword)]
   mov   X2, qword [rsi+sizeof(qword)*2]
   mov   X3, qword [rsi+sizeof(qword)*3]
   mov   rsi, rdx
   MREDUCE_FIX 2, rdi, rsi, r15, X7, X6, X5, X4, X3, X2, X1,X0, rbp,rbx
   jmp   .quit

align IPP_ALIGN_FACTOR
.mSize_3:
   mov   X0, qword [rsi]
   mov   X1, qword [rsi+sizeof(qword)]
   mov   X2, qword [rsi+sizeof(qword)*2]
   mov   X3, qword [rsi+sizeof(qword)*3]
   mov   X4, qword [rsi+sizeof(qword)*4]
   mov   X5, qword [rsi+sizeof(qword)*5]
   mov   rsi, rdx
   MREDUCE_FIX 3, rdi, rsi, r15, X7, X6, X5, X4, X3, X2, X1,X0, rbp,rbx
   jmp   .quit

align IPP_ALIGN_FACTOR
.mSize_4:
   mov   X0, qword [rsi]
   mov   X1, qword [rsi+sizeof(qword)]
   mov   X2, qword [rsi+sizeof(qword)*2]
   mov   X3, qword [rsi+sizeof(qword)*3]
   mov   X4, qword [rsi+sizeof(qword)*4]
   mov   X5, qword [rsi+sizeof(qword)*5]
   mov   X6, qword [rsi+sizeof(qword)*6]
   mov   X7, qword [rsi+sizeof(qword)*7]
   mov   rsi, rdx
   MREDUCE_FIX 4, rdi, rsi, r15, X7, X6, X5, X4, X3, X2, X1,X0, rbp,rbx
   jmp   .quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; general case reducer
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%xdefine U0    r9    ; u0, u1
%xdefine U1    r10

%xdefine T0    r11   ; temporary
%xdefine T1    r12
%xdefine T2    r13
%xdefine T3    r14

%xdefine idx   rbx   ; index
%xdefine rDst  rdi
%xdefine rSrc  rsi

align IPP_ALIGN_FACTOR
.general_case:
   lea   rdi, [rdi+rcx*sizeof(qword)-sizeof(qword)*4] ; save pR+mSzie-4
   mov   qword [rsp+pR], rdi       ; save pR
  ;mov   qword [rsp+mSize], rcx    ; save length of modulus

   mov   rdi, rsi                ; rdi = pBuffer
   mov   rsi, rdx                ; rsi = pModulo

   lea   rdi, [rdi+rcx*sizeof(qword)-sizeof(qword)*4] ; rdi = pBuffer
   lea   rsi, [rsi+rcx*sizeof(qword)-sizeof(qword)*4] ; rsi = pModulus

   mov   idx, dword 4                                       ; init negative counter
   sub   idx, rcx
   mov   qword [rsp+counter], idx
   mov   rdx, dword 3
   and   rdx, rcx

   test  rcx,1
   jz    .even_len_Modulus

;
; modulus of odd length
;
.odd_len_Modulus:
   mov   U0, qword [rDst+idx*sizeof(qword)]       ; pBuffer[0]
   imul  U0, r15                                      ; u0 = pBuffer[0]*m0

   mov   rax, qword [rSrc+idx*sizeof(qword)]      ; pModulo[0]

   mul   U0                                           ; prologue
   mov   T0, rax
   mov   rax, qword [rSrc+idx*sizeof(qword)+sizeof(qword)]
   mov   T1, rdx

   add   idx, 1
   jz    .skip_mlax1

   MLAx1 rdi, rsi, idx, U0, T0,T1,T2,T3               ; pBuffer[] += pModulo[]*u

.skip_mlax1:
   mul   U0
   xor   T2, T2
   add   qword [rDst+idx*sizeof(qword)-sizeof(qword)], T0
   adc   T1, rax
   adc   T2, rdx

   cmp      idx, 2
   ja    .fin_mla1x4n_2   ; idx=3
   jz    .fin_mla1x4n_3   ; idx=2
   jp    .fin_mla1x4n_4   ; idx=1
   ;     fin_mla1x4n_1   ; idx=0

.fin_mla1x4n_1:
   MM_MLAx1_4N_1_ELOG rdi, rsi, idx, U0, T0,T1,T2,T3  ; [rsp+carry] = rax = cf = pBuffer[]+pModulo[]*u
   sub   rcx, 1
   jmp   .mla2x4n_1

.fin_mla1x4n_4:
   MM_MLAx1_4N_4_ELOG rdi, rsi, idx, U0, T0,T1,T2,T3  ; [rsp+carry] = rax = cf = pBuffer[]+pModulo[]*u
   sub   rcx, 1
   jmp   .mla2x4n_4

.fin_mla1x4n_3:
   MM_MLAx1_4N_3_ELOG rdi, rsi, idx, U0, T0,T1,T2,T3  ; [rsp+carry] = rax = cf = pBuffer[]+pModulo[]*u
   sub   rcx, 1
   jmp   .mla2x4n_3

.fin_mla1x4n_2:
   MM_MLAx1_4N_2_ELOG rdi, rsi, idx, U0, T0,T1,T2,T3  ; [rsp+carry] = rax = cf = pBuffer[]+pModulo[]*u
   sub   rcx, 1
   jmp   .mla2x4n_2

align IPP_ALIGN_FACTOR
;
; modulus of even length
;
.even_len_Modulus:
   xor   rax, rax    ; clear carry
   cmp   rdx, 2
   ja    .mla2x4n_1   ; rdx=1
   jz    .mla2x4n_2   ; rdx=2
   jp    .mla2x4n_3   ; rdx=3
   ;     mla2x4n_4   ; rdx=0

align IPP_ALIGN_FACTOR
.mla2x4n_4:
   MMx2_PLOG            rdi, rsi, idx, r15, U0,U1,T0,T1,T2,T3  ; pre-compute u0 and u1
   MLAx2                rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; rax = cf = product[]+modulo[]*{u1:u0}
   MM_MLAx2_4N_4_ELOG   rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; [rsp+carry] = rax = cf
   sub               rcx, 2
   jnz               .mla2x4n_4

   ; (borrow, pR) = (carry,product) - modulo
   mov      rdx, qword [rsp+pR]
   xor      rcx, rcx
   SBB_x4      rdx, rdi, rsi, idx, T0,T1,T2,T3, rcx
   SBB_TAIL 4, rdx, rdi, rsi, T0,T1,T2,T3, rcx

   ; pR = borrow? product : pR
   CMOV_x4     rdx, rdi, rdx, idx, T0,T1,T2,T3, rcx
   CMOV_TAIL   4, rdx, rdi, rdx, T0,T1,T2,T3, rcx
   jmp      .quit

align IPP_ALIGN_FACTOR
.mla2x4n_3:
   MMx2_PLOG            rdi, rsi, idx, r15, U0,U1,T0,T1,T2,T3  ; pre-compute u0 and u1
   MLAx2                rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; rax = cf = product[]+modulo[]*{u1:u0}
   MM_MLAx2_4N_3_ELOG   rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; [rsp+carry] = rax = cf
   sub               rcx, 2
   jnz               .mla2x4n_3

   ; (borrow, pR) = (carry,product) - modulo
   mov      rdx, qword [rsp+pR]
   xor      rcx, rcx
   SBB_x4      rdx, rdi, rsi, idx, T0,T1,T2,T3, rcx
   SBB_TAIL 3, rdx, rdi, rsi, T0,T1,T2,T3, rcx

   ; pR = borrow? product : pR
   CMOV_x4     rdx, rdi, rdx, idx, T0,T1,T2,T3, rcx
   CMOV_TAIL   3, rdx, rdi, rdx, T0,T1,T2,T3, rcx
   jmp      .quit

align IPP_ALIGN_FACTOR
.mla2x4n_2:
   MMx2_PLOG            rdi, rsi, idx, r15, U0,U1,T0,T1,T2,T3  ; pre-compute u0 and u1
   MLAx2                rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; rax = cf = product[]+modulo[]*{u1:u0}
   MM_MLAx2_4N_2_ELOG   rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; [rsp+carry] = rax = cf
   sub               rcx, 2
   jnz               .mla2x4n_2

   ; (borrow, pR) = (carry,product) - modulo
   mov      rdx, qword [rsp+pR]
   xor      rcx, rcx
   SBB_x4      rdx, rdi, rsi, idx, T0,T1,T2,T3, rcx
   SBB_TAIL 2, rdx, rdi, rsi, T0,T1,T2,T3, rcx

   ; pR = borrow? product : pR
   CMOV_x4     rdx, rdi, rdx, idx, T0,T1,T2,T3, rcx
   CMOV_TAIL   2, rdx, rdi, rdx, T0,T1,T2,T3, rcx
   jmp      .quit


align IPP_ALIGN_FACTOR
.mla2x4n_1:
   MMx2_PLOG            rdi, rsi, idx, r15, U0,U1,T0,T1,T2,T3  ; pre-compute u0 and u1
   MLAx2                rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ; rax = cf = product[]+modulo[]*{u1:u0}
   MM_MLAx2_4N_1_ELOG   rdi, rsi, idx, U0,U1, T0,T1,T2,T3      ;  [rsp+carry] = rax = cf
   sub               rcx, 2
   jnz               .mla2x4n_1

   ; (borrow, pR) = (carry,product) - modulo
   mov      rdx, qword [rsp+pR]
   xor      rcx, rcx
   SBB_x4      rdx, rdi, rsi, idx, T0,T1,T2,T3, rcx
   SBB_TAIL 1, rdx, rdi, rsi, T0,T1,T2,T3, rcx

   ; pR = borrow? product : pR
   CMOV_x4     rdx, rdi, rdx, idx, T0,T1,T2,T3, rcx
   CMOV_TAIL   1, rdx, rdi, rdx, T0,T1,T2,T3, rcx

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpMontRedAdc_BNU

%endif

%endif ;; _ADCOX_NI_ENABLING_

