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
;        cpSqrAdc_BNU_school()
;
;

%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpbnumulschool.inc"
%include "pcpbnusqrschool.inc"
%include "pcpvariant.inc"

%if (_ADCOX_NI_ENABLING_ == _FEATURE_OFF_) || (_ADCOX_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_M7) && (_IPP32E < _IPP32E_L9)

; acc:a1 = src * mem + a1
; clobbers rax, rdx
%macro MULADD0 4.nolist
  %xdefine %%acc %1
  %xdefine %%a1 %2
  %xdefine %%src %3
  %xdefine %%mem %4

   mov   rax, %%mem
   mul   %%src
   xor   %%acc, %%acc
   add   %%a1, rax
   adc   %%acc, rdx
%endmacro


; acc:a1 = src * mem + a1 + acc
; clobbers rax, rdx
%macro MULADD 4.nolist
  %xdefine %%acc %1
  %xdefine %%a1 %2
  %xdefine %%src %3
  %xdefine %%mem %4

   mov   rax, %%mem
   mul   %%src
   add   %%a1, rax
   adc   rdx, 0
   add   %%a1, %%acc
   adc   rdx, 0
   mov   %%acc, rdx
%endmacro


; acc:a1 = src * mem + a1
; clobbers rax, rdx
%macro MULADD1 4.nolist
  %xdefine %%acc %1
  %xdefine %%a1 %2
  %xdefine %%src %3
  %xdefine %%mem %4

   mov   rax, %%mem
   mul   %%src
   add   %%a1, rax
   adc   rdx, 0
   mov   %%acc, rdx
%endmacro


; Macro to allow us to do an adc as an adc_0/add pair
%macro adc2 2.nolist
  %xdefine %%a1 %1
  %xdefine %%a2 %2

%if 1
   adc   %%a1, %%a2
%else
   adc   %%a2, 0
   add   %%a1, %%a2
%endif
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; This version does the first half of the off-diagonal terms, then adds
; in the first half of the diagonal terms, then does the second half.
; This is to avoid the writing and reading of the off-diagonal terms to mem.
; Due to extra overhead in the arithmetic, this may or may not be faster
; than the simple version (which follows)
;
; Input in memory [pA] and also in x7...x0
; Uses all argument registers plus rax and rdx
;
%macro SQR_512 13.nolist
  %xdefine %%pDst %1
  %xdefine %%pA %2
  %xdefine %%x7 %3
  %xdefine %%x6 %4
  %xdefine %%x5 %5
  %xdefine %%x4 %6
  %xdefine %%x3 %7
  %xdefine %%x2 %8
  %xdefine %%x1 %9
  %xdefine %%x0 %10
  %xdefine %%A %11
  %xdefine %%x8 %12
  %xdefine %%t0 %13

   ;; ------------------
   ;; first pass 01...07
   ;; ------------------
   mov   %%A, %%x0

   mov      rax, %%x1
   mul      %%A
   mov      %%x0, rax
   mov      %%x1, rdx

   MULADD1  %%x2, %%x1, %%A, %%x2
   MULADD1  %%x3, %%x2, %%A, %%x3
   MULADD1  %%x4, %%x3, %%A, %%x4
   MULADD1  %%x5, %%x4, %%A, %%x5
   MULADD1  %%x6, %%x5, %%A, %%x6
   MULADD1  %%x7, %%x6, %%A, %%x7

   ;; ------------------
   ;; second pass 12...16
   ;; ------------------
   mov      %%A, [%%pA + 8*1]
   xor      %%x8, %%x8

   mov      rax, [%%pA + 8*2]
   mul      %%A
   add      %%x2, rax
   adc      rdx, 0
   mov      %%t0, rdx

   MULADD   %%t0, %%x3, %%A, [%%pA + 8*3]
   MULADD   %%t0, %%x4, %%A, [%%pA + 8*4]
   MULADD   %%t0, %%x5, %%A, [%%pA + 8*5]
   MULADD   %%t0, %%x6, %%A, [%%pA + 8*6]
   add      %%x7, %%t0
   adc      %%x8, 0

   ;; ------------------
   ;; third pass 23...25
   ;; ------------------
   mov      %%A, [%%pA + 8*2]

   mov      rax, [%%pA + 8*3]
   mul      %%A
   add      %%x4, rax
   adc      rdx, 0
   mov      %%t0, rdx

   MULADD   %%t0, %%x5, %%A, [%%pA + 8*4]
   MULADD   %%t0, %%x6, %%A, [%%pA + 8*5]
   add      %%x7, %%t0
   adc      %%x8, 0

   ;; ------------------
   ;; fourth pass 34
   ;; ------------------
   mov      rax, [%%pA + 8*3]
   mul      qword [%%pA + 8*4]
   add      %%x6, rax
   adc      rdx, 0
   add      %%x7, rdx
   adc      %%x8, 0

   ;; --- double x0...x6
   xor      %%A, %%A
   add      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%x4, %%x4
   adc      %%x5, %%x5
   adc      %%x6, %%x6
   adc      %%A, 0

   mov      rax, [%%pA + 8*0]
   mul      rax
   mov      [%%pDst + 8*0], rax
   mov      %%t0, rdx

   mov      rax, [%%pA + 8*1]
   mul      rax
   add      %%x0, %%t0
   adc      %%x1, rax
   mov      [%%pDst + 8*1], %%x0
   adc      rdx, 0
   mov      [%%pDst + 8*2], %%x1
   mov      %%t0, rdx
   mov      rax, [%%pA + 8*2]
   mul      rax
   mov      %%x0, rax
   mov      %%x1, rdx
   mov      rax, [%%pA + 8*3]
   mul      rax
   add      %%x2, %%t0
   adc      %%x3, %%x0
   mov      [%%pDst + 8*3], %%x2
   adc2     %%x4, %%x1
   mov      [%%pDst + 8*4], %%x3
   adc      %%x5, rax
   mov      [%%pDst + 8*5], %%x4
   adc2     %%x6, rdx
   mov      [%%pDst + 8*6], %%x5
   adc      %%A, 0
   mov      [%%pDst + 8*7], %%x6

   ;; ------------------
   ;; second pass 17
   ;; ------------------
   mov      rax, [%%pA + 8*1]
   xor      %%x0, %%x0

   mul      qword [%%pA + 8*7]
   add      %%x7, rax
   adc      rdx, 0
   add      %%x8, rdx
   adc      %%x0, 0

   ;; ------------------
   ;; third pass 26...27
   ;; ------------------
   mov      %%x6, [%%pA + 8*2]

   mov      rax, [%%pA + 8*6]
   mul      %%x6
   add      %%x7, rax
   adc      rdx, 0
   add      %%x8, rdx
   adc      %%x0, 0

   mov      rax, [%%pA + 8*7]
   xor      %%x1, %%x1
   mul      %%x6
   add      %%x8, rax
   adc      rdx, 0
   add      %%x0, rdx
   adc      %%x1, 0

   ;; ------------------
   ;; fourth pass 35...37
   ;; ------------------
   mov      %%x6, [%%pA + 8*3]

   mov      rax, [%%pA + 8*5]
   mul      %%x6
   add      %%x7, rax
   adc      rdx, 0
   add      %%x8, rdx
   adc      %%x0, 0
   adc      %%x1, 0

   mov      rax, [%%pA + 8*6]
   mul      %%x6
   add      %%x8, rax
   adc      rdx, 0
   add      %%x0, rdx
   adc      %%x1, 0

   mov      rax, [%%pA + 8*7]
   mul      %%x6
   add      %%x0, rax
   adc      rdx, 0
   add      %%x1, rdx
   ;; carry out should be 0

   ;; ------------------
   ;; f%ifth pass 45...47
   ;; ------------------
   mov      %%x6, [%%pA + 8*4]

   mov      rax, [%%pA + 8*5]
   mul      %%x6
   add      %%x8, rax
   adc      rdx, 0
   mov      %%x2, rdx

   MULADD   %%x2, %%x0, %%x6, [%%pA + 8*6]
   MULADD   %%x2, %%x1, %%x6, [%%pA + 8*7]

   ;; ------------------
   ;; sixth pass 56...57 & seventh pass 67
   ;; ------------------
   mov      %%x6, [%%pA + 8*5]

   mov      rax, [%%pA + 8*6]
   mul      %%x6
   add      %%x1, rax
   adc      rdx, 0
   mov      %%x3, rdx

   MULADD   %%x3, %%x2, %%x6, [%%pA + 8*7]

   mov      rax, [%%pA + 8*6]
   mul      qword [%%pA + 8*7]
   add      %%x3, rax
   adc      rdx, 0
   mov      %%x4, rdx

   ;; --- double x7, x8, x0, ..., x4
   xor      %%x5, %%x5
   add      %%x7, %%x7
   adc      %%x8, %%x8
   adc      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%x4, %%x4
   adc      %%x5, 0

   mov      rax, [%%pA + 8*4]
   mul      rax
   add      rax, %%A
   adc      rdx, 0
   add      rax, %%x7
   adc      rdx, 0
   mov      [%%pDst + 8*8], rax
   mov      %%A, rdx
   mov      rax, [%%pA + 8*5]
   mul      rax
   add      %%x8, %%A
   adc      %%x0, rax
   mov      [%%pDst + 8*9], %%x8
   adc      rdx, 0
   mov      [%%pDst + 8*10], %%x0
   mov      %%A, rdx
   mov      rax, [%%pA + 8*6]
   mul      rax
   mov      %%x7, rax
   mov      %%x8, rdx
   mov      rax, [%%pA + 8*7]
   mul      rax
   add      %%x1, %%A
   adc      %%x2, %%x7
   mov      [%%pDst + 8*11], %%x1
   adc2     %%x3, %%x8
   mov      [%%pDst + 8*12], %%x2
   adc      %%x4, rax
   mov      [%%pDst + 8*13], %%x3
   adc2     %%x5, rdx
   mov      [%%pDst + 8*14], %%x4
   mov      [%%pDst + 8*15], %%x5
%endmacro


%macro SQR_448 13.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%x7 %3
  %xdefine %%x6 %4
  %xdefine %%x5 %5
  %xdefine %%x4 %6
  %xdefine %%x3 %7
  %xdefine %%x2 %8
  %xdefine %%x1 %9
  %xdefine %%x0 %10
  %xdefine %%A %11
  %xdefine %%x8 %12
  %xdefine %%t0 %13

   ;; ------------------
   ;; first pass 01...06
   ;; ------------------
   mov   %%A, %%x0

   mov      rax, %%x1
   mul      %%A
   mov      %%x0, rax
   mov      %%x1, rdx

   MULADD1  %%x2, %%x1, %%A, %%x2
   MULADD1  %%x3, %%x2, %%A, %%x3
   MULADD1  %%x4, %%x3, %%A, %%x4
   MULADD1  %%x5, %%x4, %%A, %%x5
   MULADD1  %%x6, %%x5, %%A, %%x6

   ;; ------------------
   ;; second pass 12...16
   ;; ------------------
   mov      %%A, [%%rSrc + sizeof(qword)*1]
   mov      rax, [%%rSrc+ sizeof(qword)*2]
   mul      %%A
   add      %%x2, rax
   adc      rdx, 0
   mov      %%t0, rdx
   MULADD   %%t0, %%x3, %%A, [%%rSrc + sizeof(qword)*3]
   MULADD   %%t0, %%x4, %%A, [%%rSrc + sizeof(qword)*4]
   MULADD   %%t0, %%x5, %%A, [%%rSrc + sizeof(qword)*5]
   MULADD   %%t0, %%x6, %%A, [%%rSrc + sizeof(qword)*6]
   mov      %%x7, %%t0

   ;; ------------------
   ;; third pass 23...25
   ;; ------------------
   mov      %%A, [%%rSrc + sizeof(qword)*2]
   xor      %%x8, %%x8
   mov      rax, [%%rSrc + sizeof(qword)*3]
   mul      %%A
   add      %%x4, rax
   adc      rdx, 0
   mov      %%t0, rdx
   MULADD   %%t0, %%x5, %%A, [%%rSrc+ sizeof(qword)*4]
   MULADD   %%t0, %%x6, %%A, [%%rSrc+ sizeof(qword)*5]
   add      %%x7, %%t0
   adc      %%x8, 0

   ;; ------------------
   ;; fourth pass 34
   ;; ------------------
   mov      rax, [%%rSrc + sizeof(qword)*3]
   mul      qword [%%rSrc + sizeof(qword)*4]
   add      %%x6, rax
   adc      rdx, 0
   add      %%x7, rdx
   adc      %%x8, 0


   mov      rax, [%%rSrc + sizeof(qword)*0]

   ;; --- double x0...x6
   xor      %%A, %%A
   add      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%x4, %%x4
   adc      %%x5, %%x5
   adc      %%x6, %%x6
   adc      %%A, 0

   mul      rax      ; a[0]^2
   mov      [%%rDst + sizeof(qword)*0], rax
   mov      rax, [%%rSrc + sizeof(qword)*1]
   mov      %%t0, rdx

   mul      rax      ; a[1]^2
   add      %%x0, %%t0
   adc      %%x1, rax
   mov      rax, [%%rSrc + sizeof(qword)*2]
   mov      [%%rDst + sizeof(qword)*1], %%x0
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*2], %%x1
   mov      %%t0, rdx

   mul      rax      ; a[2]^2
   add      %%x2, %%t0
   adc      %%x3, rax
   mov      rax, [%%rSrc + sizeof(qword)*3]
   mov      [%%rDst + sizeof(qword)*3], %%x2
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*4], %%x3
   mov      %%t0, rdx

   mul      rax      ; a[3]^2
   add      %%x4, %%t0
   adc      %%x5, rax
   mov      [%%rDst + sizeof(qword)*5], %%x4
   adc      %%x6, rdx
   mov      [%%rDst + sizeof(qword)*6], %%x5
   adc      %%A, 0
   mov      [%%rDst + sizeof(qword)*7], %%x6

   ;; ------------------
   ;; third pass complete 26
   ;; ------------------
   mov      rax, [%%rSrc + sizeof(qword)*2]
   xor      %%x0, %%x0

   mul      qword [%%rSrc + sizeof(qword)*6]
   add      %%x7, rax
   adc      rdx, 0
   add      %%x8, rdx
   adc      %%x0, 0

   ;; ------------------
   ;; forth pass complete 35...36
   ;; ------------------
   mov      %%x6, [%%rSrc + sizeof(qword)*3]
   mov      rax, [%%rSrc+ sizeof(qword)*5]
   mul      %%x6
   add      %%x7, rax
   mov      rax, [%%rSrc + sizeof(qword)*6]
   adc      %%x8, rdx
   adc      %%x0, 0
   mul      %%x6
   add      %%x8, rax
   adc      %%x0, rdx

   ;; ------------------
   ;; f%ifth pass 45...46
   ;; ------------------
   mov      %%x6, [%%rSrc + sizeof(qword)*4]
   xor      %%x1, %%x1
   mov      rax, [%%rSrc + sizeof(qword)*5]
   mul      %%x6
   add      %%x8, rax
   mov      rax, [%%rSrc + sizeof(qword)*6]
   adc      %%x0, rdx
   adc      %%x1, 0
   mul      %%x6
   add      %%x0, rax
   adc      %%x1, rdx

   ;; ------------------
   ;; six pass 56
   ;; ------------------
   mov      %%x6, [%%rSrc + sizeof(qword)*5]
   xor      %%x2, %%x2
   mov      rax, [%%rSrc + sizeof(qword)*6]
   mul      %%x6
   add      %%x1, rax
   adc      %%x2, rdx

   mov      rax, [%%rSrc + sizeof(qword)*4]

   ;; --- double x7, x8, x0, x1, x2
   xor      %%x3, %%x3
   add      %%x7, %%x7
   adc      %%x8, %%x8
   adc      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, 0

   mul      rax         ; a[4]^2
   add      %%x7, %%A
   adc      rdx, 0
   xor      %%A, %%A
   add      %%x7, rax
   mov      rax, [%%rSrc + sizeof(qword)*5]
   adc      %%x8, rdx
   mov      [%%rDst+ sizeof(qword)*8], %%x7
   adc      %%A, 0
   mov      [%%rDst + sizeof(qword)*9], %%x8

   mul      rax         ; a[5]^2
   add      %%x0, %%A
   adc      rdx, 0
   xor      %%A, %%A
   add      %%x0, rax
   mov      rax, [%%rSrc + sizeof(qword)*6]
   adc      %%x1, rdx
   mov      [%%rDst + sizeof(qword)*10], %%x0
   adc      %%A, 0
   mov      [%%rDst + sizeof(qword)*11], %%x1

   mul      rax         ; a[6]^2
   add      %%x2, %%A
   adc      rdx, 0
   add      %%x2, rax
   adc      rdx, %%x3
   mov      [%%rDst + sizeof(qword)*12], %%x2
   mov      [%%rDst + sizeof(qword)*13], rdx
%endmacro


%macro SQR_384 13.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%x7 %3
  %xdefine %%x6 %4
  %xdefine %%x5 %5
  %xdefine %%x4 %6
  %xdefine %%x3 %7
  %xdefine %%x2 %8
  %xdefine %%x1 %9
  %xdefine %%x0 %10
  %xdefine %%A %11
  %xdefine %%x8 %12
  %xdefine %%t0 %13

   mov   %%A, %%x0
   mov      rax, %%x1
   mul      %%A
   mov      %%x0, rax
   mov      %%x1, rdx
   MULADD1  %%x2, %%x1, %%A, %%x2
   MULADD1  %%x3, %%x2, %%A, %%x3
   MULADD1  %%x4, %%x3, %%A, %%x4
   MULADD1  %%x5, %%x4, %%A, %%x5

   mov      %%A, qword [%%rSrc+ sizeof(qword)*1]
   mov      rax, qword [%%rSrc+ sizeof(qword)*2]
   mul      %%A
   add      %%x2, rax
   adc      rdx, 0
   mov      %%t0, rdx
   MULADD   %%t0, %%x3, %%A, [%%rSrc + sizeof(qword)*3]
   MULADD   %%t0, %%x4, %%A, [%%rSrc + sizeof(qword)*4]
   MULADD   %%t0, %%x5, %%A, [%%rSrc + sizeof(qword)*5]
   mov      %%x6, %%t0

   mov      %%A, qword [%%rSrc+ sizeof(qword)*2]
   mov      rax, qword [%%rSrc+ sizeof(qword)*3]
   mul      %%A
   add      %%x4, rax
   adc      rdx, 0
   mov      %%t0, rdx
   MULADD   %%t0, %%x5, %%A, [%%rSrc + sizeof(qword)*4]
   MULADD   %%t0, %%x6, %%A, [%%rSrc + sizeof(qword)*5]
   mov      %%x7, %%t0

   mov      %%A, qword [%%rSrc+ sizeof(qword)*3]
   mov      rax, qword [%%rSrc+ sizeof(qword)*4]
   mul      %%A
   xor      %%x8, %%x8
   add      %%x6, rax
   mov      rax, qword [%%rSrc+ sizeof(qword)*5]
   adc      %%x7, rdx
   adc      %%x8, 0
   mul      %%A
   mov      %%A, qword [%%rSrc+ sizeof(qword)*4]
   add      %%x7, rax
   mov      rax, qword [%%rSrc+ sizeof(qword)*5]
   adc      %%x8, rdx

   mul      %%A
   xor      %%t0, %%t0
   add      %%x8, rax
   adc      %%t0, rdx

   mov      rax, [%%rSrc + sizeof(qword)*0]

   ;; --- double x0...x7,x8,t0
   xor      %%A, %%A
   add      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%x4, %%x4
   adc      %%x5, %%x5
   adc      %%x6, %%x6
   adc      %%x7, %%x7
   adc      %%x8, %%x8
   adc      %%t0, %%t0
   adc      %%A, 0
   mov      qword [rsp], %%A

   mul      rax
   mov      [%%rDst + sizeof(qword)*0], rax
   mov      rax, [%%rSrc + sizeof(qword)*1]    ; a[1]
   mov      %%A, rdx

   mul      rax
   add      %%x0, %%A
   adc      %%x1, rax
   mov      rax, [%%rSrc + sizeof(qword)*2]    ; a[2]
   mov      [%%rDst + sizeof(qword)*1], %%x0
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*2], %%x1
   mov      %%A, rdx

   mul      rax
   add      %%x2, %%A
   adc      %%x3, rax
   mov      rax, [%%rSrc + sizeof(qword)*3]    ; a[3]
   mov      [%%rDst + sizeof(qword)*3], %%x2
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*4], %%x3
   mov      %%A, rdx

   mul      rax
   add      %%x4, %%A
   adc      %%x5, rax
   mov      rax, [%%rSrc + sizeof(qword)*4]    ; a[4]
   mov      [%%rDst + sizeof(qword)*5], %%x4
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*6], %%x5
   mov      %%A, rdx

   mul      rax
   add      %%x6, %%A
   adc      %%x7, rax
   mov      rax, [%%rSrc + sizeof(qword)*5]    ; a[5]
   mov      [%%rDst + sizeof(qword)*7], %%x6
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*8], %%x7
   mov      %%A, rdx

   mul      rax
   add      %%x8, %%A
   adc      %%t0, rax
   mov      [%%rDst + sizeof(qword)*9], %%x8
   adc      rdx, qword [rsp]
   mov      [%%rDst + sizeof(qword)*10], %%t0
   mov      [%%rDst + sizeof(qword)*11], rdx
%endmacro


%macro SQR_320 13.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%x7 %3
  %xdefine %%x6 %4
  %xdefine %%x5 %5
  %xdefine %%x4 %6
  %xdefine %%x3 %7
  %xdefine %%x2 %8
  %xdefine %%x1 %9
  %xdefine %%x0 %10
  %xdefine %%A %11
  %xdefine %%x8 %12
  %xdefine %%t0 %13

   mov   %%A, %%x0
   mov      rax, %%x1
   mul      %%A
   mov      %%x0, rax
   mov      %%x1, rdx
   MULADD1  %%x2, %%x1, %%A, %%x2
   MULADD1  %%x3, %%x2, %%A, %%x3
   MULADD1  %%x4, %%x3, %%A, %%x4

   mov      %%A, qword [%%rSrc+ sizeof(qword)*1]
   mov      rax, qword [%%rSrc+ sizeof(qword)*2]
   mul      %%A
   add      %%x2, rax
   adc      rdx, 0
   mov      %%t0, rdx
   MULADD   %%t0, %%x3, %%A, [%%rSrc + sizeof(qword)*3]
   MULADD   %%t0, %%x4, %%A, [%%rSrc + sizeof(qword)*4]
   mov      %%x5, %%t0

   mov      %%A, qword [%%rSrc+ sizeof(qword)*2]
   mov      rax, qword [%%rSrc+ sizeof(qword)*3]
   mul      %%A
   xor      %%x6, %%x6
   add      %%x4, rax
   mov      rax, qword [%%rSrc+ sizeof(qword)*4]
   adc      %%x5, rdx
   adc      %%x6, 0
   mul      %%A
   mov      %%A, qword [%%rSrc+ sizeof(qword)*3]
   add      %%x5, rax
   mov      rax, qword [%%rSrc+ sizeof(qword)*4]
   adc      %%x6, rdx

   mul      %%A
   xor      %%x7, %%x7
   add      %%x6, rax
   adc      %%x7, rdx

   mov      rax, [%%rSrc + sizeof(qword)*0]

   ;; --- double x0...x5
   xor      %%A, %%A
   add      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%x4, %%x4
   adc      %%x5, %%x5
   adc      %%x6, %%x6
   adc      %%x7, %%x7
   adc      %%A, 0

   mul      rax
   mov      [%%rDst + sizeof(qword)*0], rax
   mov      rax, [%%rSrc + sizeof(qword)*1]
   mov      %%t0, rdx

   mul      rax
   add      %%x0, %%t0
   adc      %%x1, rax
   mov      rax, [%%rSrc + sizeof(qword)*2]
   mov      [%%rDst + sizeof(qword)*1], %%x0
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*2], %%x1
   mov      %%t0, rdx

   mul      rax
   add      %%x2, %%t0
   adc      %%x3, rax
   mov      rax, [%%rSrc + sizeof(qword)*3]
   mov      [%%rDst + sizeof(qword)*3], %%x2
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*4], %%x3
   mov      %%t0, rdx

   mul      rax
   add      %%x4, %%t0
   adc      %%x5, rax
   mov      rax, [%%rSrc + sizeof(qword)*4]
   mov      [%%rDst + sizeof(qword)*5], %%x4
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*6], %%x5
   mov      %%t0, rdx

   mul      rax
   add      %%x6, %%t0
   adc      %%x7, rax
   mov      [%%rDst + sizeof(qword)*7], %%x6
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*8], %%x7
   add      rdx, %%A
   mov      [%%rDst + sizeof(qword)*9], rdx
%endmacro


%macro SQR_256 13.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%x7 %3
  %xdefine %%x6 %4
  %xdefine %%x5 %5
  %xdefine %%x4 %6
  %xdefine %%x3 %7
  %xdefine %%x2 %8
  %xdefine %%x1 %9
  %xdefine %%x0 %10
  %xdefine %%A %11
  %xdefine %%x8 %12
  %xdefine %%t0 %13

   ;; ------------------
   ;; first pass 01...03
   ;; ------------------
   mov   %%A, %%x0
   mov      rax, %%x1
   mul      %%A
   mov      %%x0, rax
   mov      %%x1, rdx
   MULADD1  %%x2, %%x1, %%A, %%x2
   MULADD1  %%x3, %%x2, %%A, %%x3

   ;; ------------------
   ;; second pass 12, 13
   ;; ------------------
   mov      %%A, qword [%%rSrc+ sizeof(qword)*1]
   mov      rax, qword [%%rSrc+ sizeof(qword)*2]
   mul      %%A
   xor      %%x4, %%x4
   add      %%x2, rax
   mov      rax, qword [%%rSrc+ sizeof(qword)*3]
   adc      %%x3, rdx
   adc      %%x4, 0

   mul      %%A
   mov      %%A, qword [%%rSrc+ sizeof(qword)*2]
   add      %%x3, rax
   mov      rax, qword [%%rSrc+ sizeof(qword)*3]
   adc      %%x4, rdx

   ;; ------------------
   ;; third pass 23
   ;; ------------------
   mul      %%A
   xor      %%x5, %%x5
   add      %%x4, rax
   adc      %%x5, rdx

   mov      rax, [%%rSrc + sizeof(qword)*0]

   ;; --- double x0...x5
   xor      %%A, %%A
   add      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%x4, %%x4
   adc      %%x5, %%x5
   adc      %%A, 0

   mul      rax
   mov      [%%rDst + sizeof(qword)*0], rax
   mov      rax, [%%rSrc + sizeof(qword)*1]
   mov      %%t0, rdx

   mul      rax
   add      %%x0, %%t0
   adc      %%x1, rax
   mov      rax, [%%rSrc + sizeof(qword)*2]
   mov      [%%rDst + sizeof(qword)*1], %%x0
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*2], %%x1
   mov      %%t0, rdx

   mul      rax
   add      %%x2, %%t0
   adc      %%x3, rax
   mov      rax, [%%rSrc + sizeof(qword)*3]
   mov      [%%rDst + sizeof(qword)*3], %%x2
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*4], %%x3
   mov      %%t0, rdx

   mul      rax
   add      %%x4, %%t0
   adc      %%x5, rax
   mov      [%%rDst + sizeof(qword)*5], %%x4
   adc      rdx, 0
   mov      [%%rDst + sizeof(qword)*6], %%x5
   add      rdx, %%A
   mov      [%%rDst + sizeof(qword)*7], rdx
%endmacro


%macro SQR_192 13.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%x7 %3
  %xdefine %%x6 %4
  %xdefine %%x5 %5
  %xdefine %%x4 %6
  %xdefine %%x3 %7
  %xdefine %%x2 %8
  %xdefine %%x1 %9
  %xdefine %%x0 %10
  %xdefine %%A %11
  %xdefine %%x8 %12
  %xdefine %%t0 %13

   mov      %%A, %%x0

   mov      rax, %%x1
   mul      %%A
   mov      %%x0, rax
   mov      %%x1, rdx

   MULADD1  %%x2, %%x1, %%A, %%x2

   mov      rax, qword [%%rSrc+ sizeof(qword)*1]
   mul      qword [%%rSrc+ sizeof(qword)*2]
   xor      %%x3, %%x3
   add      %%x2, rax
   adc      %%x3, rdx

   xor      %%A, %%A
   add      %%x0, %%x0
   adc      %%x1, %%x1
   adc      %%x2, %%x2
   adc      %%x3, %%x3
   adc      %%A, %%A

   mov      rax, qword [%%rSrc+ sizeof(qword)*0]
   mul      rax
   mov      %%x4, rax
   mov      %%x5, rdx

   mov      rax, qword [%%rSrc+ sizeof(qword)*1]
   mul      rax
   mov      %%x6, rax
   mov      %%x7, rdx

   mov      rax, qword [%%rSrc+ sizeof(qword)*2]
   mul      rax

   mov      qword [%%rDst+sizeof(qword)*0], %%x4
   add      %%x5, %%x0
   mov      qword [%%rDst+sizeof(qword)*1], %%x5
   adc      %%x6, %%x1
   mov      qword [%%rDst+sizeof(qword)*2], %%x6
   adc      %%x7, %%x2
   mov      qword [%%rDst+sizeof(qword)*3], %%x7
   adc      rax, %%x3
   mov      qword [%%rDst+sizeof(qword)*4], rax
   adc      rdx, %%A
   mov      qword [%%rDst+sizeof(qword)*5], rdx
%endmacro

segment .text align=IPP_ALIGN_FACTOR

;*************************************************************
;* Ipp64u  cpSqrAdc_BNU_school(Ipp64u* pR;
;*                       const Ipp64u* pA, int  aSize)
;* returns pR[aSize+aSize-1]
;*
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpSqrAdc_BNU_school,PUBLIC
%assign LOCAL_FRAME (3*sizeof(qword))
        USES_GPR rbx,rbp,rsi,rdi,r12,r13,r14,r15
        USES_XMM
        COMP_ABI 3

;;
;; assignment
%xdefine rDst     rdi
%xdefine rSrc     rsi
%xdefine srcL     rdx

%xdefine A        rcx
%xdefine x0       r8
%xdefine x1       r9
%xdefine x2       r10
%xdefine x3       r11
%xdefine x4       r12
%xdefine x5       r13
%xdefine x6       r14
%xdefine x7       r15
%xdefine x8       rbx
%xdefine t0       rbp

   cmp     edx, 4
   jg      .more_then_4

   cmp      edx, 3
   jg       .SQR4
   je       .SQR3
   jp       .SQR2

align IPP_ALIGN_FACTOR
.SQR1:    ;; len=1
   mov     rax, qword [rsi]            ; eax = a
   mul     rax                             ; eax = rL edx = rH
   mov     qword [rdi], rax            ;
   mov     rax, rdx                        ;
   mov     qword [rdi+8], rdx          ;
   mov     rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.SQR2:    ;; len=2
   mov   rax, qword [rsi]                ; a[0]
   mul   qword [rsi+sizeof(qword)*1]     ; a[0]*a[1]
   xor   A, A
   mov   x2, rax
   mov   x3, rdx

   mov   rax, qword [rsi]                ; a[0]
   mul   rax                                 ; (x1:x0) = a[1]^2

   add   x2, x2                              ; (A:x3:x2) = 2*a[0]*a[1]
   adc   x3, x3
   adc   A, 0

   mov   x0, rax
   mov   x1, rdx

   mov   rax, qword [rsi+sizeof(qword)*1]; a[1]
   mul   rax                                 ; (rdx:rax) = a[1]^2

   mov   qword [rdi+sizeof(qword)*0], x0
   add   x1, x2
   mov   qword [rdi+sizeof(qword)*1], x1
   adc   rax, x3
   mov   qword [rdi+sizeof(qword)*2], rax
   adc   rdx, A
   mov   qword [rdi+sizeof(qword)*3], rdx
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.SQR3:    ;; len=3
   mov   x0, [rSrc+ sizeof(qword)*0]
   mov   x1, [rSrc+ sizeof(qword)*1]
   mov   x2, [rSrc+ sizeof(qword)*2]
   SQR_192   rDst, rSrc, x7, x6, x5, x4, x3, x2, x1, x0, A, x8, t0
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.SQR4:    ;; len=4
   mov   x0, [rSrc+ sizeof(qword)*0]
   mov   x1, [rSrc+ sizeof(qword)*1]
   mov   x2, [rSrc+ sizeof(qword)*2]
   mov   x3, [rSrc+ sizeof(qword)*3]
   SQR_256   rDst, rSrc, x7, x6, x5, x4, x3, x2, x1, x0, A, x8, t0
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.more_then_4:
   cmp      edx, 8
   jg       .general_case

   cmp      edx, 7
   jg       .SQR8
   je       .SQR7
   jp       .SQR6

align IPP_ALIGN_FACTOR
.SQR5:    ;; len=5
   mov   x0, [rSrc+ sizeof(qword)*0]
   mov   x1, [rSrc+ sizeof(qword)*1]
   mov   x2, [rSrc+ sizeof(qword)*2]
   mov   x3, [rSrc+ sizeof(qword)*3]
   mov   x4, [rSrc+ sizeof(qword)*4]
   SQR_320   rDst, rSrc, x7, x6, x5, x4, x3, x2, x1, x0, A, x8, t0
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.SQR6:    ;; len=6
   mov   x0, [rSrc+ sizeof(qword)*0]
   mov   x1, [rSrc+ sizeof(qword)*1]
   mov   x2, [rSrc+ sizeof(qword)*2]
   mov   x3, [rSrc+ sizeof(qword)*3]
   mov   x4, [rSrc+ sizeof(qword)*4]
   mov   x5, [rSrc+ sizeof(qword)*5]
   SQR_384   rDst, rSrc, x7, x6, x5, x4, x3, x2, x1, x0, A, x8, t0
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.SQR7:    ;; len=7
   mov   x0, [rSrc+ sizeof(qword)*0]
   mov   x1, [rSrc+ sizeof(qword)*1]
   mov   x2, [rSrc+ sizeof(qword)*2]
   mov   x3, [rSrc+ sizeof(qword)*3]
   mov   x4, [rSrc+ sizeof(qword)*4]
   mov   x5, [rSrc+ sizeof(qword)*5]
   mov   x6, [rSrc+ sizeof(qword)*6]
   SQR_448  rDst, rSrc, x7, x6, x5, x4, x3, x2, x1, x0, A, x8, t0
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret

align IPP_ALIGN_FACTOR
.SQR8:    ;; len=8
   mov   x0, [rSrc+ sizeof(qword)*0]
   mov   x1, [rSrc+ sizeof(qword)*1]
   mov   x2, [rSrc+ sizeof(qword)*2]
   mov   x3, [rSrc+ sizeof(qword)*3]
   mov   x4, [rSrc+ sizeof(qword)*4]
   mov   x5, [rSrc+ sizeof(qword)*5]
   mov   x6, [rSrc+ sizeof(qword)*6]
   mov   x7, [rSrc+ sizeof(qword)*7]
   SQR_512  rDst, rSrc, x7, x6, x5, x4, x3, x2, x1, x0, A, x8, t0
   mov   rax, rdx
   REST_XMM
   REST_GPR
   ret



;********** lenSrcA > 8 **************************************
align IPP_ALIGN_FACTOR
.general_case:

;;
;; stack structure:
%assign pDst   0
%assign pSrc   pDst+sizeof(qword)
%assign len    pSrc+sizeof(qword)

;;
;; assignment
%xdefine B0    r10   ; a[i], a[i+1]
%xdefine B1    r11

%xdefine T0    r12   ; temporary
%xdefine T1    r13
%xdefine T2    r14
%xdefine T3    r15

%xdefine idx   rcx   ; indexs
%xdefine idxt  rbx

%xdefine rSrc  rsi
%xdefine rDst  rdi

   movsxd   rdx, edx    ; expand length
   mov      [rsp+pDst], rdi   ; save parameters
   mov      [rsp+pSrc], rsi
   mov      [rsp+len], rdx

   mov      r8, rdx
   mov      rax, dword 2
   mov      rbx, dword 1
   test     r8, 1
   cmove    rax, rbx          ; delta = len&1? 2:1

   sub      rdx, rax                      ; len -= delta
   lea      rsi, [rsi+rax*sizeof(qword)]  ; A' = A+delta
   lea      rdi, [rdi+rax*sizeof(qword)]  ; R' = R+delta
   lea      rsi, [rsi+rdx*sizeof(qword)-4*sizeof(qword)] ; rsi = &A'[len-4]
   lea      rdi, [rdi+rdx*sizeof(qword)-4*sizeof(qword)] ; rdi = &R'[len-4]

   mov      idx, dword 4      ; init
   sub      idx, rdx    ; negative index

   test     r8, 1
   jnz      .init_odd_len_operation

;********** odd number of passes (multiply only) *********************
.init_even_len_operation:
   mov      B0,  qword [rsi+idx*sizeof(qword)-sizeof(qword)]  ; B0 = a[0]
   mov      rax, qword [rsi+idx*sizeof(qword)]                ; a[1]
   xor      T0, T0
   cmp      idx, 0
   jge      .skip_mul1

   mov      idxt, idx
   MULx1    rdi, rsi, idxt, B0, T0, T1, T2, T3

.skip_mul1:
   cmp      idxt, 1
   jne      .fin_mulx1_3 ; idx=3 (effective len=4n+1)
   ;        .fin_mulx1_1 ; idx=1 (effective len=4n+3)

.fin_mulx1_1:
   sMULx1_4N_3_ELOG rdi, rsi, {add idx,2}, B0, T0,T1,T2,T3
   jmp      .odd_pass_pairs
.fin_mulx1_3:
   sMULx1_4N_1_ELOG rdi, rsi, {add idx,2}, B0, T0,T1,T2,T3
   jmp      .even_pass_pairs


;********** even number of passes (multiply only) *********************
.init_odd_len_operation:
   mov      B0, qword [rsi+idx*sizeof(qword)-2*sizeof(qword)]    ; a[0] and a[1]
   mov      B1, qword [rsi+idx*sizeof(qword)-sizeof(qword)]

   mov      rax, B1     ; a[0]*a[1]
   mul      B0

   mov      qword [rdi+idx*sizeof(qword)-sizeof(qword)], rax
   mov      rax, qword [rsi+idx*sizeof(qword)]                   ; a[2]
   mov      T0, rdx

   mul      B0                                                       ; a[0]*a[2]
   xor      T1, T1
   xor      T2, T2
   add      T0, rax
   mov      rax, qword [rsi+idx*sizeof(qword)]                   ; a[2]
   adc      T1, rdx

   cmp      idx, 0
   jge      .skip_mul_nx2

   mov      idxt, idx
   MULx2    rdi, rsi, idxt, B0,B1, T0,T1,T2,T3

.skip_mul_nx2:
   cmp      idxt, 1
   jnz      .fin_mul2x_3  ; idx=3 (effective len=4n+3)
   ;        .fin_mul2x_1  ; idx=1 (effective len=4n+1)

.fin_mul2x_1:
   sMULx2_4N_3_ELOG  rdi, rsi, {add idx,2}, B0,B1, T0,T1,T2,T3
   add   rdi, 2*sizeof(qword)
   jmp      .odd_pass_pairs
.fin_mul2x_3:
   sMULx2_4N_1_ELOG  rdi, rsi, {add idx,2}, B0,B1, T0,T1,T2,T3
   add   rdi, 2*sizeof(qword)

align IPP_ALIGN_FACTOR
.even_pass_pairs:
   sMLAx2_PLOG {rdi+idx*sizeof(qword)}, {rsi+idx*sizeof(qword)}, B0,B1, T0,T1,T2,T3
   cmp      idx, 0
   jge      .skip1
   mov      idxt, idx
   MLAx2    rdi, rsi, idxt, B0,B1, T0,T1,T2,T3
.skip1:
   sMLAx2_4N_3_ELOG rdi, rsi, {add idx,2}, B0,B1, T0,T1,T2,T3
   add   rdi, 2*sizeof(qword)

.odd_pass_pairs:
   sMLAx2_PLOG {rdi+idx*sizeof(qword)}, {rsi+idx*sizeof(qword)}, B0,B1, T0,T1,T2,T3
   cmp      idx, 0
   jge      .skip2
   mov      idxt, idx
   MLAx2    rdi, rsi, idxt, B0,B1, T0,T1,T2,T3
.skip2:
   sMLAx2_4N_1_ELOG rdi, rsi, {add idx,2}, B0,B1, T0,T1,T2,T3
   add   rdi, 2*sizeof(qword)

   cmp   idx, 4
   jl    .even_pass_pairs


.add_diag:
   mov      rdi, [rsp+pDst]   ; restore parameters
   mov      rsi, [rsp+pSrc]
   mov      rcx, [rsp+len]

   xor      idxt, idxt
   xor      T0, T0
   xor      T1, T1
   lea      rax, [rdi+rcx*sizeof(qword)]
   lea      rsi, [rsi+rcx*sizeof(qword)]
   mov      qword [rdi], T0                                   ; r[0] = 0
   mov      qword [rax+rcx*sizeof(qword)-sizeof(qword)], T0   ; r[2*len-1] = 0
   neg      rcx

align IPP_ALIGN_FACTOR
.add_diag_loop:
   mov      rax, qword [rsi+rcx*sizeof(qword)] ; a[i]
   mul      rax
   mov      T2, qword [rdi]                    ; r[2*i]
   mov      T3, qword [rdi+sizeof(qword)]      ; r[2*i+1]
   add      T0, 1
   adc      T2, T2
   adc      T3, T3
   sbb      T0, T0
   add      T1, 1
   adc      T2, rax
   adc      T3, rdx
   sbb      T1, T1
   mov      qword [rdi], T2
   mov      qword [rdi+sizeof(qword)], T3
   add      rdi, sizeof(qword)*2
   add      rcx, 1
   jnz      .add_diag_loop

   mov      rax, T3        ; r[2*len-1]
   REST_XMM
   REST_GPR
   ret
ENDFUNC cpSqrAdc_BNU_school


%endif

%endif ;; _ADCOX_NI_ENABLING_

