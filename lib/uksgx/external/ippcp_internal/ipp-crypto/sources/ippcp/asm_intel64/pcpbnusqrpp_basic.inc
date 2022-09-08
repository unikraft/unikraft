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

%ifndef _PCPBNSQR_BASIC_ADCX_INC_
%assign _PCPBNSQR_BASIC_ADCX_INC_  1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Fixed-size (1-8 qwords) square operations
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; 1*qword squarer
;;
%macro SQR_64 2.nolist
  %xdefine %%pDst %1
  %xdefine %%pA %2

   mov   rdx, qword [%%pA]
gsmulx   rdx, rax, rdx
   mov   qword [%%pDst], rax
   mov   qword [%%pDst+sizeof(qword)], rdx
%endmacro

align IPP_ALIGN_FACTOR
sqr_1:
   SQR_64       rdi, rsi
   ret


;;
;; 2*qword squarer
;;
%macro SQR_128 13.nolist
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

   mov      rdx, qword [%%pA+sizeof(qword)]
   gsmulx   rax, %%x1, qword [%%pA]             ; (rax:x1) = a[0]*a[1]
   gsmulx   %%x3,  %%x2, rdx                        ; (x3 :x2) = a[1]^2
   mov      rdx, qword [%%pA]                 ; a[0]
   gsmulx   rdx, %%x0, rdx                        ; (rdx:x0) = a[0]^2

   add      %%x1, %%x1                              ; (rax:x1) = a[0]*a[1]*2
   adc      rax,rax
   adc      %%x3, 0

   mov      qword [%%pDst+sizeof(qword)*0], %%x0
   add      %%x1, rdx
   mov      qword [%%pDst+sizeof(qword)*1], %%x1
   adc      %%x2, rax
   mov      qword [%%pDst+sizeof(qword)*2], %%x2
   adc      %%x3, 0
   mov      qword [%%pDst+sizeof(qword)*3], %%x3
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_2,PRIVATE
   SQR_128    rdi, rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx, rbx, rbp
   ret
ENDFUNC sqr_2



;;
;; 3*qword squarer
;;
%macro SQR_192 13.nolist
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

   mov      rdx, [%%pA]                     ;; a[0]*{a[1],a[2]}
   gsmulx   %%x1, %%x0, [%%pA+sizeof(qword)*1]
   gsmulx   %%x2, rax,[%%pA+sizeof(qword)*2]
   add      %%x1, rax
   adc      %%x2, 0
   mov      rdx, [%%pA+sizeof(qword)*1]     ;; a[1]*a[2]
   gsmulx   %%x3, rax, [%%pA+sizeof(qword)*2]
   add      %%x2, rax
   adc      %%x3, 0

   xor      %%A, %%A
   ;; square a[0],a[1],a[2] and add double x0,x1,x2,x3
   mov      rdx, [%%pA+sizeof(qword)*0]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, %%x0
   gsadox   rdx, %%x0
   mov      qword [%%pDst+sizeof(qword)], rdx

   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x1
   gsadox   rax, %%x1
   mov      qword [%%pDst+sizeof(qword)*2], rax
   gsadcx   rdx, %%x2
   gsadox   rdx, %%x2
   mov      qword [%%pDst+sizeof(qword)*3], rdx

   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x3
   gsadox   rax, %%x3
   mov      qword [%%pDst+sizeof(qword)*4], rax
   gsadcx   rdx, %%A
   gsadox   rdx, %%A
   mov      qword [%%pDst+sizeof(qword)*5], rdx
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_3,PRIVATE
   SQR_192  rdi,rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx,rbx,rbp
   ret
ENDFUNC sqr_3



;;
;; 4*qword squarer
;;
%macro SQR_256 13.nolist
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

   ;; 1-st pass a[0]*{a[1],a[2],a[3]}
   mov      rdx, [%%pA]
   gsmulx   %%x1, %%x0, [%%pA+sizeof(qword)*1]
   xor      %%A,  %%A
   gsmulx   %%x2, rax,[%%pA+sizeof(qword)*2]
   add      %%x1, rax
   gsmulx   %%x3, rax,[%%pA+sizeof(qword)*3]
   adc      %%x2, rax
   adc      %%x3, 0

   ;; 2-nd pass a[1]*{a[2],a[3]}
   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*2]
   gsadcx   %%x2, rax
   gsadox   %%x3, %%t0
   gsmulx   %%x4, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x3, rax
   gsadox   %%x4, %%A
   adc      %%x4, 0

   ;; 3-rd pass a[2]*a[3]
   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   %%x5, rax, [%%pA+sizeof(qword)*3]
   add      %%x4, rax
   adc      %%x5, 0

   ;; square a[0],...,a[3] and add double x0,...,,x5
   mov      rdx, [%%pA+sizeof(qword)*0]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, %%x0
   gsadox   rdx, %%x0
   mov      qword [%%pDst+sizeof(qword)], rdx

   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x1
   gsadox   rax, %%x1
   mov      qword [%%pDst+sizeof(qword)*2], rax
   gsadcx   rdx, %%x2
   gsadox   rdx, %%x2
   mov      qword [%%pDst+sizeof(qword)*3], rdx

   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x3
   gsadox   rax, %%x3
   mov      qword [%%pDst+sizeof(qword)*4], rax
   gsadcx   rdx, %%x4
   gsadox   rdx, %%x4
   mov      qword [%%pDst+sizeof(qword)*5], rdx

   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x5
   gsadox   rax, %%x5
   mov      qword [%%pDst+sizeof(qword)*6], rax
   gsadcx   rdx, %%A
   gsadox   rdx, %%A
   mov      qword [%%pDst+sizeof(qword)*7], rdx
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_4,PRIVATE
   SQR_256  rdi,rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx,rbx,rbp
   ret
ENDFUNC sqr_4



;;
;; 5*qword squarer
;;
%macro SQR_320 13.nolist
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

   ;; 1-st pass a[0]*{a[1],...,a[4]}
   mov      rdx, [%%pA]
   gsmulx   %%x1, %%x0, [%%pA+sizeof(qword)*1]
   xor      %%A,  %%A
   gsmulx   %%x2, rax,[%%pA+sizeof(qword)*2]
   add      %%x1, rax
   gsmulx   %%x3, rax,[%%pA+sizeof(qword)*3]
   adc      %%x2, rax
   gsmulx   %%x4, rax,[%%pA+sizeof(qword)*4]
   adc      %%x3, rax
   adc      %%x4, 0

   ;; 2-nd pass a[1]*{a[2],...,a[4]}
   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*2]
   gsadcx   %%x2, rax
   gsadox   %%x3, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x3, rax
   gsadox   %%x4, %%t0
   gsmulx   %%x5, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%A
   adc      %%x5, 0

   ;; 3-rd pass a[2]*{a[3],a[4]}
   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%x6, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%A
   adc      %%x6, 0

   ;; 4-th pass a[3]*a[4]
   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   %%x7, rax, [%%pA+sizeof(qword)*4]
   add      %%x6, rax
   adc      %%x7, 0

   ;; square a[0],...,a[4] and add double x0,...,,x7
   mov      rdx, [%%pA+sizeof(qword)*0]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, %%x0
   gsadox   rdx, %%x0
   mov      qword [%%pDst+sizeof(qword)], rdx

   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x1
   gsadox   rax, %%x1
   mov      qword [%%pDst+sizeof(qword)*2], rax
   gsadcx   rdx, %%x2
   gsadox   rdx, %%x2
   mov      qword [%%pDst+sizeof(qword)*3], rdx

   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x3
   gsadox   rax, %%x3
   mov      qword [%%pDst+sizeof(qword)*4], rax
   gsadcx   rdx, %%x4
   gsadox   rdx, %%x4
   mov      qword [%%pDst+sizeof(qword)*5], rdx

   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x5
   gsadox   rax, %%x5
   mov      qword [%%pDst+sizeof(qword)*6], rax
   gsadcx   rdx, %%x6
   gsadox   rdx, %%x6
   mov      qword [%%pDst+sizeof(qword)*7], rdx

   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x7
   gsadox   rax, %%x7
   mov      qword [%%pDst+sizeof(qword)*8], rax
   gsadcx   rdx, %%A
   gsadox   rdx, %%A
   mov      qword [%%pDst+sizeof(qword)*9], rdx
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_5,PRIVATE
   SQR_320  rdi,rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx,rbx,rbp
   ret
ENDFUNC sqr_5



;;
;; 6*qword squarer
;;
%macro SQR_384 13.nolist
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

   ;; 1-st pass a[0]*{a[1],...,a[5]}
   mov      rdx, [%%pA]
   gsmulx   %%x1, %%x0, [%%pA+sizeof(qword)*1]
   xor      %%A,  %%A
   gsmulx   %%x2, rax,[%%pA+sizeof(qword)*2]
   add      %%x1, rax
   gsmulx   %%x3, rax,[%%pA+sizeof(qword)*3]
   adc      %%x2, rax
   gsmulx   %%x4, rax,[%%pA+sizeof(qword)*4]
   adc      %%x3, rax
   gsmulx   %%x5, rax,[%%pA+sizeof(qword)*5]
   adc      %%x4, rax
   adc      %%x5, 0

   ;; 2-nd pass a[1]*{a[2],...,a[5]}
   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*2]
   gsadcx   %%x2, rax
   gsadox   %%x3, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x3, rax
   gsadox   %%x4, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%x6, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%A
   adc      %%x6, 0

   ;; 3-rd pass a[2]*{a[3],a[4],a[5]}
   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%t0
   gsmulx   %%x7, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%A
   adc      %%x7, 0

   ;; 4-th pass a[3]*{a[4],a[5]}
   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%t0
   gsmulx   %%x8, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x7, rax
   gsadox   %%x8, %%A
   adc      %%x8, 0

   ;; 5-th pass a[4]*a[5]
   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   add      %%x8, rax
   adc      %%t0, 0

   ;; square a[0],...,a[5] and add double x0,...,x7,x8,t0
   mov      rdx, [%%pA+sizeof(qword)*0]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, %%x0
   gsadox   rdx, %%x0
   mov      qword [%%pDst+sizeof(qword)], rdx

   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x1
   gsadox   rax, %%x1
   mov      qword [%%pDst+sizeof(qword)*2], rax
   gsadcx   rdx, %%x2
   gsadox   rdx, %%x2
   mov      qword [%%pDst+sizeof(qword)*3], rdx

   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x3
   gsadox   rax, %%x3
   mov      qword [%%pDst+sizeof(qword)*4], rax
   gsadcx   rdx, %%x4
   gsadox   rdx, %%x4
   mov      qword [%%pDst+sizeof(qword)*5], rdx

   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x5
   gsadox   rax, %%x5
   mov      qword [%%pDst+sizeof(qword)*6], rax
   gsadcx   rdx, %%x6
   gsadox   rdx, %%x6
   mov      qword [%%pDst+sizeof(qword)*7], rdx

   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x7
   gsadox   rax, %%x7
   mov      qword [%%pDst+sizeof(qword)*8], rax
   gsadcx   rdx, %%x8
   gsadox   rdx, %%x8
   mov      qword [%%pDst+sizeof(qword)*9], rdx

   mov      rdx, [%%pA+sizeof(qword)*5]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%t0
   gsadox   rax, %%t0
   mov      qword [%%pDst+sizeof(qword)*10], rax
   gsadcx   rdx, %%A
   gsadox   rdx, %%A
   mov      qword [%%pDst+sizeof(qword)*11], rdx
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_6,PRIVATE
   SQR_384  rdi,rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx,rbx,rbp
   ret
ENDFUNC sqr_6



;;
;; 7*qword squarer
;;
%macro SQR_448 13.nolist
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

   ;; 1-st pass a[0]*{a[1],...,a[6]}
   mov      rdx, [%%pA]
   gsmulx   %%x1, %%x0, [%%pA+sizeof(qword)*1]
   xor      %%A,  %%A
   gsmulx   %%x2, rax,[%%pA+sizeof(qword)*2]
   add      %%x1, rax
   gsmulx   %%x3, rax,[%%pA+sizeof(qword)*3]
   adc      %%x2, rax
   gsmulx   %%x4, rax,[%%pA+sizeof(qword)*4]
   adc      %%x3, rax
   gsmulx   %%x5, rax,[%%pA+sizeof(qword)*5]
   adc      %%x4, rax
   gsmulx   %%x6, rax,[%%pA+sizeof(qword)*6]
   adc      %%x5, rax
   adc      %%x6, 0

   mov      [%%pDst+sizeof(qword)*1], %%x0
   mov      [%%pDst+sizeof(qword)*2], %%x1

   ;; 2-nd pass a[1]*{a[2],...,a[6]}
   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*2]
   gsadcx   %%x2, rax
   gsadox   %%x3, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x3, rax
   gsadox   %%x4, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%t0
   gsmulx   %%x7, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%A
   adc      %%x7, 0

   mov      [%%pDst+sizeof(qword)*3], %%x2

   ;; 3-rd pass a[2]*{a[3],...,a[6]}
   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%t0
   gsmulx   %%x8, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x7, rax
   gsadox   %%x8, %%A
   adc      %%x8, 0

   ;; 4-rd pass a[3]*{a[4],...,a[6]}
   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x7, rax
   gsadox   %%x8, %%t0
   gsmulx   %%x0, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x8, rax
   gsadox   %%x0, %%A
   adc      %%x0, 0

   ;; 5-rd pass a[4]*{a[5],a[6]}
   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x8, rax
   gsadox   %%x0, %%t0
   gsmulx   %%x1, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x0, rax
   gsadox   %%x1, %%A
   adc      %%x1, 0

   ;; 6-rd pass a[5]*a[6]
   mov      rdx, [%%pA+sizeof(qword)*5]
   gsmulx   %%x2, rax, [%%pA+sizeof(qword)*6]
   add      %%x1, rax
   adc      %%x2, 0

   ;; square a[0],...,a[5] and add double
   mov      rdx, [%%pA+sizeof(qword)*0]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, qword [%%pDst+sizeof(qword)]
   gsadox   rdx, qword [%%pDst+sizeof(qword)]
   mov      qword [%%pDst+sizeof(qword)], rdx

   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, qword [%%pDst+sizeof(qword)*2]
   gsadox   rax, qword [%%pDst+sizeof(qword)*2]
   mov      qword [%%pDst+sizeof(qword)*2], rax
   gsadcx   rdx, qword [%%pDst+sizeof(qword)*3]
   gsadox   rdx, qword [%%pDst+sizeof(qword)*3]
   mov      qword [%%pDst+sizeof(qword)*3], rdx

   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x3
   gsadox   rax, %%x3
   mov      qword [%%pDst+sizeof(qword)*4], rax
   gsadcx   rdx, %%x4
   gsadox   rdx, %%x4
   mov      qword [%%pDst+sizeof(qword)*5], rdx

   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x5
   gsadox   rax, %%x5
   mov      qword [%%pDst+sizeof(qword)*6], rax
   gsadcx   rdx, %%x6
   gsadox   rdx, %%x6
   mov      qword [%%pDst+sizeof(qword)*7], rdx

   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x7
   gsadox   rax, %%x7
   mov      qword [%%pDst+sizeof(qword)*8], rax
   gsadcx   rdx, %%x8
   gsadox   rdx, %%x8
   mov      qword [%%pDst+sizeof(qword)*9], rdx

   mov      rdx, [%%pA+sizeof(qword)*5]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x0
   gsadox   rax, %%x0
   mov      qword [%%pDst+sizeof(qword)*10], rax
   gsadcx   rdx, %%x1
   gsadox   rdx, %%x1
   mov      qword [%%pDst+sizeof(qword)*11], rdx

   mov      rdx, [%%pA+sizeof(qword)*6]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x2
   gsadox   rax, %%x2
   mov      qword [%%pDst+sizeof(qword)*12], rax
   gsadcx   rdx, %%A
   gsadox   rdx, %%A
   mov      qword [%%pDst+sizeof(qword)*13], rdx
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_7,PRIVATE
   SQR_448  rdi,rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx,rbx,rbp
   ret
ENDFUNC sqr_7



;;
;; 8*qword squarer
;;
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

   ;; 1-st pass a[0]*{a[1],...,a[7]}
   mov      rdx, [%%pA]
   gsmulx   %%x1, %%x0, [%%pA+sizeof(qword)*1]
   xor      %%A,  %%A
   gsmulx   %%x2, rax,[%%pA+sizeof(qword)*2]
   add      %%x1, rax
   gsmulx   %%x3, rax,[%%pA+sizeof(qword)*3]
   adc      %%x2, rax
   gsmulx   %%x4, rax,[%%pA+sizeof(qword)*4]
   adc      %%x3, rax
   gsmulx   %%x5, rax,[%%pA+sizeof(qword)*5]
   adc      %%x4, rax
   gsmulx   %%x6, rax,[%%pA+sizeof(qword)*6]
   adc      %%x5, rax
   gsmulx   %%x7, rax,[%%pA+sizeof(qword)*7]
   adc      %%x6, rax
   adc      %%x7, 0

   mov      [%%pDst+sizeof(qword)*1], %%x0
   mov      [%%pDst+sizeof(qword)*2], %%x1

   ;; 2-nd pass a[1]*{a[2],...,a[7]}
   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*2]
   gsadcx   %%x2, rax
   gsadox   %%x3, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x3, rax
   gsadox   %%x4, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%t0
   gsmulx   %%x8, rax, [%%pA+sizeof(qword)*7]
   gsadcx   %%x7, rax
   gsadox   %%x8, %%A
   adc      %%x8, 0

   mov      [%%pDst+sizeof(qword)*3], %%x2
   mov      [%%pDst+sizeof(qword)*4], %%x3

   ;; 3-rd pass a[2]*{a[3],...,a[7]}
   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*3]
   gsadcx   %%x4, rax
   gsadox   %%x5, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x5, rax
   gsadox   %%x6, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x7, rax
   gsadox   %%x8, %%t0
   gsmulx   %%x0, rax, [%%pA+sizeof(qword)*7]
   gsadcx   %%x8, rax
   gsadox   %%x0, %%A
   adc      %%x0, 0

   ;; 4-rd pass a[3]*{a[4],...,a[7]}
   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*4]
   gsadcx   %%x6, rax
   gsadox   %%x7, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x7, rax
   gsadox   %%x8, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x8, rax
   gsadox   %%x0, %%t0
   gsmulx   %%x1, rax, [%%pA+sizeof(qword)*7]
   gsadcx   %%x0, rax
   gsadox   %%x1, %%A
   adc      %%x1, 0

   ;; 5-rd pass a[4]*{a[5],...,a[7]}
   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*5]
   gsadcx   %%x8, rax
   gsadox   %%x0, %%t0
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x0, rax
   gsadox   %%x1, %%t0
   gsmulx   %%x2, rax, [%%pA+sizeof(qword)*7]
   gsadcx   %%x1, rax
   gsadox   %%x2, %%A
   adc      %%x2, 0

   ;; 6-rd pass a[5]*{a[6],a[7]}
   mov      rdx, [%%pA+sizeof(qword)*5]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*6]
   gsadcx   %%x1, rax
   gsadox   %%x2, %%t0
   gsmulx   %%x3, rax, [%%pA+sizeof(qword)*7]
   gsadcx   %%x2, rax
   gsadox   %%x3, %%A
   adc      %%x3, 0

   ;; 7-rd pass a[6]*a[7]
   mov      rdx, [%%pA+sizeof(qword)*6]
   gsmulx   %%t0, rax, [%%pA+sizeof(qword)*7]
   add      %%x3, rax
   adc      %%t0, 0

   ;; square a[0],...,a[7] and add double
   mov      rdx, [%%pA+sizeof(qword)*0]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, qword [%%pDst+sizeof(qword)]
   gsadox   rdx, qword [%%pDst+sizeof(qword)]
   mov      qword [%%pDst+sizeof(qword)], rdx

   mov      rdx, [%%pA+sizeof(qword)*1]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, qword [%%pDst+sizeof(qword)*2]
   gsadox   rax, qword [%%pDst+sizeof(qword)*2]
   mov      qword [%%pDst+sizeof(qword)*2], rax
   gsadcx   rdx, qword [%%pDst+sizeof(qword)*3]
   gsadox   rdx, qword [%%pDst+sizeof(qword)*3]
   mov      qword [%%pDst+sizeof(qword)*3], rdx

   mov      rdx, [%%pA+sizeof(qword)*2]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, qword [%%pDst+sizeof(qword)*4]
   gsadox   rax, qword [%%pDst+sizeof(qword)*4]
   mov      qword [%%pDst+sizeof(qword)*4], rax
   gsadcx   rdx, %%x4
   gsadox   rdx, %%x4
   mov      qword [%%pDst+sizeof(qword)*5], rdx

   mov      rdx, [%%pA+sizeof(qword)*3]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x5
   gsadox   rax, %%x5
   mov      qword [%%pDst+sizeof(qword)*6], rax
   gsadcx   rdx, %%x6
   gsadox   rdx, %%x6
   mov      qword [%%pDst+sizeof(qword)*7], rdx

   mov      rdx, [%%pA+sizeof(qword)*4]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x7
   gsadox   rax, %%x7
   mov      qword [%%pDst+sizeof(qword)*8], rax
   gsadcx   rdx, %%x8
   gsadox   rdx, %%x8
   mov      qword [%%pDst+sizeof(qword)*9], rdx

   mov      rdx, [%%pA+sizeof(qword)*5]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x0
   gsadox   rax, %%x0
   mov      qword [%%pDst+sizeof(qword)*10], rax
   gsadcx   rdx, %%x1
   gsadox   rdx, %%x1
   mov      qword [%%pDst+sizeof(qword)*11], rdx

   mov      rdx, [%%pA+sizeof(qword)*6]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%x2
   gsadox   rax, %%x2
   mov      qword [%%pDst+sizeof(qword)*12], rax
   gsadcx   rdx, %%x3
   gsadox   rdx, %%x3
   mov      qword [%%pDst+sizeof(qword)*13], rdx

   mov      rdx, [%%pA+sizeof(qword)*7]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, %%t0
   gsadox   rax, %%t0
   mov      qword [%%pDst+sizeof(qword)*14], rax
   gsadcx   rdx, %%A
   gsadox   rdx, %%A
   mov      qword [%%pDst+sizeof(qword)*15], rdx
%endmacro

align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_8,PRIVATE
   SQR_512  rdi, rsi, r15,r14,r13,r12,r11,r10,r9,r8, rcx, rbx, rbp
   ret
ENDFUNC sqr_8



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SQR_512_TRIANGLE_STEP
; executes single line of upper triangle
;
; Inp:partial sum:   x7 x6 x5 x4 x3 x2 x1 x0
;   rdx * pA[]     p  p  p  p  p  p  p  p  p
; Out:            x0 x7 x6 x5 x4 x3 x2 x1
;                                        [dst]

%macro SQR_512_TRIANGLE_STEP 14.nolist
  %xdefine %%HEAD_X %1
  %xdefine %%X7 %2
  %xdefine %%X6 %3
  %xdefine %%X5 %4
  %xdefine %%X4 %5
  %xdefine %%X3 %6
  %xdefine %%X2 %7
  %xdefine %%X1 %8
  %xdefine %%X0 %9
  %xdefine %%TAIL_X %10
  %xdefine %%pDst %11
  %xdefine %%pA %12
  %xdefine %%TMP1 %13
  %xdefine %%TMP2 %14

   xor   rax, rax

%ifnempty %%X0
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*0]     ; TMP2:TMP1 = rdx * pA[0]
gsadcx   %%X0, %%TMP1
gsadox   %%X1, %%TMP2
%endif

%ifnempty %%TAIL_X
   mov   %%pDst, %%TAIL_X
%endif

%ifnempty %%X1
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*1]     ; TMP2:TMP1 = rdx * pA[1]
gsadcx   %%X1, %%TMP1
gsadox   %%X2, %%TMP2
%endif

%ifnempty %%X2
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*2]     ; TMP2:TMP1 = rdx * pA[2]
gsadcx   %%X2, %%TMP1
gsadox   %%X3, %%TMP2
%endif

%ifnempty %%X3
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*3]     ; TMP2:TMP1 = rdx * pA[3]
gsadcx   %%X3, %%TMP1
gsadox   %%X4, %%TMP2
%endif

%ifnempty %%X4
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*4]     ; TMP2:TMP1 = rdx * pA[4]
gsadcx   %%X4, %%TMP1
gsadox   %%X5, %%TMP2
%endif

%ifnempty %%X5
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*5]     ; TMP2:TMP1 = rdx * pA[5]
gsadcx   %%X5, %%TMP1
gsadox   %%X6, %%TMP2
%endif

%ifnempty %%X6
gsmulx   %%TMP2,%%TMP1, [%%pA+sizeof(qword)*6]     ; TMP2:TMP1 = rdx * pA[6]
gsadcx   %%X6, %%TMP1
gsadox   %%X7, %%TMP2
%endif

%ifnempty %%X7
gsmulx   %%HEAD_X,%%TMP1, [%%pA+sizeof(qword)*7]   ; X0:TMP1 = rdx * pA[7]
gsadcx   %%X7, %%TMP1
gsadox   %%HEAD_X, rax
   adc   %%HEAD_X, 0
%endif
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; square and add diagonal terms
;

;; rdi = dst
;; rsi = src
;; rcx = src lengh
%macro FINALIZE_FIX 3.nolist
  %xdefine %%N %1
  %xdefine %%pDst %2
  %xdefine %%pSrc %3

   xor      rax, rax
   mov      rdx, qword [%%pSrc]
   gsmulx   rdx, rax, rdx
   mov      qword [%%pDst], rax
   gsadcx   rdx, qword [%%pDst+sizeof(qword)]
   gsadox   rdx, qword [%%pDst+sizeof(qword)]
   mov      qword [rdi+sizeof(qword)], rdx

   %assign %%i 1
   %rep (%%N-2)
      mov      rdx, qword [%%pSrc+%%i*sizeof(qword)]
      gsmulx   rdx, rax, rdx
      gsadcx   rax, qword [%%pDst+(2*%%i)*sizeof(qword)]
      gsadox   rax, qword [%%pDst+(2*%%i)*sizeof(qword)]
      mov      qword [%%pDst+(2*%%i)*sizeof(qword)], rax
      gsadcx   rdx, qword [%%pDst+(2*%%i+1)*sizeof(qword)]
      gsadox   rdx, qword [%%pDst+(2*%%i+1)*sizeof(qword)]
      mov      qword [rdi+(2*%%i+1)*sizeof(qword)], rdx
      %assign %%i %%i+1
   %endrep

   mov      rdx, qword [%%pSrc+(%%N-1)*sizeof(qword)]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, qword [%%pDst+(2*%%N-2)*sizeof(qword)]
   gsadox   rax, qword [%%pDst+(2*%%N-2)*sizeof(qword)]
   mov      qword [%%pDst+(2*%%N-2)*sizeof(qword)], rax
   mov      rax, dword 0
   gsadcx   rdx, rax
   gsadox   rdx, rax
   mov      qword [rdi+(2*%%N-1)*sizeof(qword)], rdx
%endmacro


align IPP_ALIGN_FACTOR
DECLARE_FUNC finalize,PRIVATE
   push  rcx
   xor   rax, rax

   mov      rdx, qword [rsi]
   gsmulx   rdx, rax, rdx
   lea      rsi, [rsi+sizeof(qword)]
   mov      qword [rdi], rax
   gsadcx   rdx, qword [rdi+sizeof(qword)]
   gsadox   rdx, qword [rdi+sizeof(qword)]
   mov      qword [rdi+sizeof(qword)], rdx
   lea      rdi, [rdi+sizeof(qword)*2]
   lea      rcx, [rcx-2]

.next_sqr:
   mov      rdx, qword [rsi]
   gsmulx   rdx, rax, rdx
   lea      rsi, [rsi+sizeof(qword)]
   gsadcx   rax, qword [rdi]
   gsadox   rax, qword [rdi]
   mov      qword [rdi], rax
   gsadcx   rdx, qword [rdi+sizeof(qword)]
   gsadox   rdx, qword [rdi+sizeof(qword)]
   mov      qword [rdi+sizeof(qword)], rdx
   lea      rdi, [rdi+sizeof(qword)*2]
   lea      rcx, [rcx-1]
   jrcxz    .last_sqr
   jmp      .next_sqr

.last_sqr:
   mov      rdx, qword [rsi]
   gsmulx   rdx, rax, rdx
   gsadcx   rax, qword [rdi]
   gsadox   rax, qword [rdi]
   mov      qword [rdi], rax
   mov      rax, dword 0
   gsadcx   rdx, rax
   gsadox   rdx, rax
   mov      qword [rdi+sizeof(qword)], rdx

   pop   rcx
   lea   rax, [rcx*sizeof(qword)-sizeof(qword)]
   sub   rsi, rax
   sub   rdi, rax
   sub   rdi, rax
   ret
ENDFUNC finalize


;;
;; 8*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr8_triangle,PRIVATE
   ;; A[0]*A[1..7]
   mov      rdx, [rsi+sizeof(qword)*0]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,r11,r10, r9,   , r8,  [rdi+sizeof(qword)*0],rsi, rbx,rbp

   ;; A[1]*A[2..7]
   mov      rdx, [rsi+sizeof(qword)*1]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,r13,r12,r11,   ,   , r9,  [rdi+sizeof(qword)*1],rsi, rbx,rbp

   ;; A[2]*A[3..7]
   mov      rdx, [rsi+sizeof(qword)*2]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,r15,r14,r13,   ,   ,   , r10, [rdi+sizeof(qword)*2],rsi, rbx,rbp

   ;; A[3]*A[4..7]
   mov      rdx, [rsi+sizeof(qword)*3]
   SQR_512_TRIANGLE_STEP   r11, r10, r9, r8,r15,   ,   ,   ,   , r11, [rdi+sizeof(qword)*3],rsi, rbx,rbp

   ;; A[4]*A[5..7]
   mov      rdx, [rsi+sizeof(qword)*4]
   SQR_512_TRIANGLE_STEP   r12, r11,r10, r9,   ,   ,   ,   ,   , r12, [rdi+sizeof(qword)*4],rsi, rbx,rbp

   ;; A[5]*A[6..7]
   mov      rdx, [rsi+sizeof(qword)*5]
   SQR_512_TRIANGLE_STEP   r13, r12,r11,   ,   ,   ,   ,   ,   , r13, [rdi+sizeof(qword)*5],rsi, rbx,rbp

   ;; A[6]*A[7]
   mov      rdx, [rsi+sizeof(qword)*6]
   SQR_512_TRIANGLE_STEP   r14, r13,   ,   ,   ,   ,   ,   ,   , r14, [rdi+sizeof(qword)*6],rsi, rbx,rbp
   ret
ENDFUNC sqr8_triangle



;;
;; 9*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_9,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x1

   mov      qword [rdi+sizeof(qword)*(1+0)], r8
   mov      qword [rdi+sizeof(qword)*(1+1)], r9
   mov      qword [rdi+sizeof(qword)*(1+2)], r10
   mov      qword [rdi+sizeof(qword)*(1+3)], r11
   mov      qword [rdi+sizeof(qword)*(1+4)], r12
   mov      qword [rdi+sizeof(qword)*(1+5)], r13
   mov      qword [rdi+sizeof(qword)*(1+6)], r14
   mov      qword [rdi+sizeof(qword)*(1+7)], r15

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(1+8)], rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 9, rdi, rsi
   ret
ENDFUNC sqr_9


;;
;; 10*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_10,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x2

   mov      qword [rdi+sizeof(qword)*(2+0)], r8
   mov      qword [rdi+sizeof(qword)*(2+1)], r9
   mov      qword [rdi+sizeof(qword)*(2+2)], r10
   mov      qword [rdi+sizeof(qword)*(2+3)], r11
   mov      qword [rdi+sizeof(qword)*(2+4)], r12
   mov      qword [rdi+sizeof(qword)*(2+5)], r13
   mov      qword [rdi+sizeof(qword)*(2+6)], r14

   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP  r8, r15, , , , , , , , , , {rsi+sizeof(qword)*2}, rbx,rbp

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(2+7)], r15
   mov      qword [rdi+sizeof(qword)*(2+8)], r8
   mov      qword [rdi+sizeof(qword)*(2+9)], rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 10, rdi, rsi
   ret
ENDFUNC sqr_10


;;
;; 10*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_11,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x1
   add      rdi, sizeof(qword)*1
   add      rcx, sizeof(qword)*1
   call     mla_8x2
   sub      rdi, sizeof(qword)*1
   sub      rcx, sizeof(qword)*1

   mov      qword [rdi+sizeof(qword)*(3+0)], r8
   mov      qword [rdi+sizeof(qword)*(3+1)], r9
   mov      qword [rdi+sizeof(qword)*(3+2)], r10
   mov      qword [rdi+sizeof(qword)*(3+3)], r11
   mov      qword [rdi+sizeof(qword)*(3+4)], r12


;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(3+5)],{rsi+sizeof(qword)*3}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(3+6)],{rsi+sizeof(qword)*3}, rbx,rbp

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(3+7)], r15
   mov      qword [rdi+sizeof(qword)*(3+8)], r8
   mov      qword [rdi+sizeof(qword)*(3+9)], r9
   mov      qword [rdi+sizeof(qword)*(3+10)],rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 11, rdi, rsi
   ret
ENDFUNC sqr_11


;;
;; 12*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_12,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   sub      rdi, sizeof(qword)*2
   sub      rcx, sizeof(qword)*2

   mov      qword [rdi+sizeof(qword)*(4+0)], r8
   mov      qword [rdi+sizeof(qword)*(4+1)], r9
   mov      qword [rdi+sizeof(qword)*(4+2)], r10
   mov      qword [rdi+sizeof(qword)*(4+3)], r11

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(4+4)],{rsi+sizeof(qword)*4}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(4+5)],{rsi+sizeof(qword)*4}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(4+6)],{rsi+sizeof(qword)*4}, rbx,rbp

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(4+7)], r15
   mov      qword [rdi+sizeof(qword)*(4+8)], r8
   mov      qword [rdi+sizeof(qword)*(4+9)], r9
   mov      qword [rdi+sizeof(qword)*(4+10)],r10
   mov      qword [rdi+sizeof(qword)*(4+11)],rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 12, rdi, rsi
   ret
ENDFUNC sqr_12


;;
;; 13*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_13,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x1
   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   sub      rdi, sizeof(qword)*3
   sub      rcx, sizeof(qword)*3

   mov      qword [rdi+sizeof(qword)*(5+0)], r8
   mov      qword [rdi+sizeof(qword)*(5+1)], r9
   mov      qword [rdi+sizeof(qword)*(5+2)], r10

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,   ,   ,   ,   ,  r11,[rdi+sizeof(qword)*(5+3)],{rsi+sizeof(qword)*5}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(5+4)],{rsi+sizeof(qword)*5}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(5+5)],{rsi+sizeof(qword)*5}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*11]
   SQR_512_TRIANGLE_STEP   r11, r10,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(5+6)],{rsi+sizeof(qword)*5}, rbx,rbp

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(5+7)], r15
   mov      qword [rdi+sizeof(qword)*(5+8)], r8
   mov      qword [rdi+sizeof(qword)*(5+9)], r9
   mov      qword [rdi+sizeof(qword)*(5+10)],r10
   mov      qword [rdi+sizeof(qword)*(5+11)],r11
   mov      qword [rdi+sizeof(qword)*(5+12)],rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 13, rdi, rsi
   ret
ENDFUNC sqr_13


;;
;; 14*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_14,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   sub      rdi, sizeof(qword)*4
   sub      rcx, sizeof(qword)*4

   mov      qword [rdi+sizeof(qword)*(6+0)], r8
   mov      qword [rdi+sizeof(qword)*(6+1)], r9

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,r11,   ,   ,   ,  r10,[rdi+sizeof(qword)*(6+2)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,r13,   ,   ,   ,   ,  r11,[rdi+sizeof(qword)*(6+3)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,r15,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(6+4)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*11]
   SQR_512_TRIANGLE_STEP   r11, r10, r9,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(6+5)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*12]
   SQR_512_TRIANGLE_STEP   r12, r11,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(6+6)],{rsi+sizeof(qword)*6}, rbx,rbp

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(6+7)], r15
   mov      qword [rdi+sizeof(qword)*(6+8)], r8
   mov      qword [rdi+sizeof(qword)*(6+9)], r9
   mov      qword [rdi+sizeof(qword)*(6+10)],r10
   mov      qword [rdi+sizeof(qword)*(6+11)],r11
   mov      qword [rdi+sizeof(qword)*(6+12)],r12
   mov      qword [rdi+sizeof(qword)*(6+13)],rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 14, rdi, rsi
   ret
ENDFUNC sqr_14


;;
;; 15*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_15,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call     mla_8x1
   add      rdi, sizeof(qword)
   add      rcx, sizeof(qword)
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   add      rdi, sizeof(qword)*2
   add      rcx, sizeof(qword)*2
   call     mla_8x2
   sub      rdi, sizeof(qword)*5
   sub      rcx, sizeof(qword)*5

   mov      qword [rdi+sizeof(qword)*(7+0)], r8

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,r11,r10,   ,   ,  r9, [rdi+sizeof(qword)*(7+1)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,r13,r12,   ,   ,   ,  r10,[rdi+sizeof(qword)*(7+2)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,r15,r14,   ,   ,   ,   ,  r11,[rdi+sizeof(qword)*(7+3)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*11]
   SQR_512_TRIANGLE_STEP   r11, r10, r9, r8,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(7+4)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*12]
   SQR_512_TRIANGLE_STEP   r12, r11,r10,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(7+5)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*13]
   SQR_512_TRIANGLE_STEP   r13, r12,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(7+6)],{rsi+sizeof(qword)*7}, rbx,rbp

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*(7+7)], r15
   mov      qword [rdi+sizeof(qword)*(7+8)], r8
   mov      qword [rdi+sizeof(qword)*(7+9)], r9
   mov      qword [rdi+sizeof(qword)*(7+10)],r10
   mov      qword [rdi+sizeof(qword)*(7+11)],r11
   mov      qword [rdi+sizeof(qword)*(7+12)],r12
   mov      qword [rdi+sizeof(qword)*(7+13)],r13
   mov      qword [rdi+sizeof(qword)*(7+14)],rbx

   sub      rdi, sizeof(qword)*8

   FINALIZE_FIX 15, rdi, rsi
   ret
ENDFUNC sqr_15


;;
;; 16*qword squarer
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr_16,PRIVATE
   call     sqr8_triangle

   mov      qword [rdi+sizeof(qword)*7], r15

   mov      rcx, rsi
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   xor      r15, r15
   call   mla_8x2

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

   add      rdi, sizeof(qword)*8
   call     sqr8_triangle

   xor      rbx, rbx
   mov      qword [rdi+sizeof(qword)*7], r15
   mov      qword [rdi+sizeof(qword)*8], r8
   mov      qword [rdi+sizeof(qword)*9], r9
   mov      qword [rdi+sizeof(qword)*10],r10
   mov      qword [rdi+sizeof(qword)*11],r11
   mov      qword [rdi+sizeof(qword)*12],r12
   mov      qword [rdi+sizeof(qword)*13],r13
   mov      qword [rdi+sizeof(qword)*14],r14
   mov      qword [rdi+sizeof(qword)*15],rbx

   sub      rsi, sizeof(qword)*8
   sub      rdi, sizeof(qword)*16

   FINALIZE_FIX 16, rdi, rsi
   ret
ENDFUNC sqr_16



;;
;; 9*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr9_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x1

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(1+0)], r8
   mov      qword [rdi+sizeof(qword)*(1+1)], r9
   mov      qword [rdi+sizeof(qword)*(1+2)], r10
   mov      qword [rdi+sizeof(qword)*(1+3)], r11
   mov      qword [rdi+sizeof(qword)*(1+4)], r12
   mov      qword [rdi+sizeof(qword)*(1+5)], r13
   mov      qword [rdi+sizeof(qword)*(1+6)], r14
   mov      qword [rdi+sizeof(qword)*(1+7)], r15
   mov      qword [rdi+sizeof(qword)*(1+8)], rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr9_triangle


;;
;; 10*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr10_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x2

   mov      qword [rdi+sizeof(qword)*(2+0)], r8
   mov      qword [rdi+sizeof(qword)*(2+1)], r9
   mov      qword [rdi+sizeof(qword)*(2+2)], r10
   mov      qword [rdi+sizeof(qword)*(2+3)], r11
   mov      qword [rdi+sizeof(qword)*(2+4)], r12
   mov      qword [rdi+sizeof(qword)*(2+5)], r13
   mov      qword [rdi+sizeof(qword)*(2+6)], r14

   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP  r8, r15, , , , , , , , , , {rsi+sizeof(qword)*2}, rbx,rbp

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(2+7)], r15
   mov      qword [rdi+sizeof(qword)*(2+8)], r8
   mov      qword [rdi+sizeof(qword)*(2+9)], rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr10_triangle


;;
;; 11*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr11_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x3

   mov      qword [rdi+sizeof(qword)*(3+0)], r8
   mov      qword [rdi+sizeof(qword)*(3+1)], r9
   mov      qword [rdi+sizeof(qword)*(3+2)], r10
   mov      qword [rdi+sizeof(qword)*(3+3)], r11
   mov      qword [rdi+sizeof(qword)*(3+4)], r12

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(3+5)],{rsi+sizeof(qword)*3}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(3+6)],{rsi+sizeof(qword)*3}, rbx,rbp

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(3+7)], r15
   mov      qword [rdi+sizeof(qword)*(3+8)], r8
   mov      qword [rdi+sizeof(qword)*(3+9)], r9
   mov      qword [rdi+sizeof(qword)*(3+10)],rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr11_triangle


;;
;; 12*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr12_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x4

   mov      qword [rdi+sizeof(qword)*(4+0)], r8
   mov      qword [rdi+sizeof(qword)*(4+1)], r9
   mov      qword [rdi+sizeof(qword)*(4+2)], r10
   mov      qword [rdi+sizeof(qword)*(4+3)], r11

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(4+4)],{rsi+sizeof(qword)*4}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(4+5)],{rsi+sizeof(qword)*4}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(4+6)],{rsi+sizeof(qword)*4}, rbx,rbp

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(4+7)], r15
   mov      qword [rdi+sizeof(qword)*(4+8)], r8
   mov      qword [rdi+sizeof(qword)*(4+9)], r9
   mov      qword [rdi+sizeof(qword)*(4+10)],r10
   mov      qword [rdi+sizeof(qword)*(4+11)],rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr12_triangle


;;
;; 13*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr13_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x5

   mov      qword [rdi+sizeof(qword)*(5+0)], r8
   mov      qword [rdi+sizeof(qword)*(5+1)], r9
   mov      qword [rdi+sizeof(qword)*(5+2)], r10

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,   ,   ,   ,   ,  r11,[rdi+sizeof(qword)*(5+3)],{rsi+sizeof(qword)*5}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(5+4)],{rsi+sizeof(qword)*5}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(5+5)],{rsi+sizeof(qword)*5}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*11]
   SQR_512_TRIANGLE_STEP   r11, r10,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(5+6)],{rsi+sizeof(qword)*5}, rbx,rbp

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(5+7)], r15
   mov      qword [rdi+sizeof(qword)*(5+8)], r8
   mov      qword [rdi+sizeof(qword)*(5+9)], r9
   mov      qword [rdi+sizeof(qword)*(5+10)],r10
   mov      qword [rdi+sizeof(qword)*(5+11)],r11
   mov      qword [rdi+sizeof(qword)*(5+12)],rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr13_triangle


;;
;; 14*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr14_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x6

   mov      qword [rdi+sizeof(qword)*(6+0)], r8
   mov      qword [rdi+sizeof(qword)*(6+1)], r9

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,r11,   ,   ,   ,  r10,[rdi+sizeof(qword)*(6+2)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,r13,   ,   ,   ,   ,  r11,[rdi+sizeof(qword)*(6+3)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,r15,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(6+4)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*11]
   SQR_512_TRIANGLE_STEP   r11, r10, r9,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(6+5)],{rsi+sizeof(qword)*6}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*12]
   SQR_512_TRIANGLE_STEP   r12, r11,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(6+6)],{rsi+sizeof(qword)*6}, rbx,rbp

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(6+7)], r15
   mov      qword [rdi+sizeof(qword)*(6+8)], r8
   mov      qword [rdi+sizeof(qword)*(6+9)], r9
   mov      qword [rdi+sizeof(qword)*(6+10)],r10
   mov      qword [rdi+sizeof(qword)*(6+11)],r11
   mov      qword [rdi+sizeof(qword)*(6+12)],r12
   mov      qword [rdi+sizeof(qword)*(6+13)],rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr14_triangle


;;
;; 15*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr15_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   lea      rcx, [rsi+sizeof(qword)*8]
   add      rdi, sizeof(qword)*8
   call     mla_8x7

   mov      qword [rdi+sizeof(qword)*(7+0)], r8

;  SQR_512_TRIANGLE_STEP     H,  p7, p6, p5, p4, p3, p2, p1, p0,  T, <dst>,<src>, rbx,rbp
;                          ------------------------------
   mov      rdx, qword [rsi+sizeof(qword)*8]
   SQR_512_TRIANGLE_STEP    r8, r15,r14,r13,r12,r11,r10,   ,   ,  r9, [rdi+sizeof(qword)*(7+1)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*9]
   SQR_512_TRIANGLE_STEP    r9,  r8,r15,r14,r13,r12,   ,   ,   ,  r10,[rdi+sizeof(qword)*(7+2)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*10]
   SQR_512_TRIANGLE_STEP   r10,  r9, r8,r15,r14,   ,   ,   ,   ,  r11,[rdi+sizeof(qword)*(7+3)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*11]
   SQR_512_TRIANGLE_STEP   r11, r10, r9, r8,   ,   ,   ,   ,   ,  r12,[rdi+sizeof(qword)*(7+4)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*12]
   SQR_512_TRIANGLE_STEP   r12, r11,r10,   ,   ,   ,   ,   ,   ,  r13,[rdi+sizeof(qword)*(7+5)],{rsi+sizeof(qword)*7}, rbx,rbp
   mov      rdx, qword [rsi+sizeof(qword)*13]
   SQR_512_TRIANGLE_STEP   r13, r12,   ,   ,   ,   ,   ,   ,   ,  r14,[rdi+sizeof(qword)*(7+6)],{rsi+sizeof(qword)*7}, rbx,rbp

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*(7+7)], r15
   mov      qword [rdi+sizeof(qword)*(7+8)], r8
   mov      qword [rdi+sizeof(qword)*(7+9)], r9
   mov      qword [rdi+sizeof(qword)*(7+10)],r10
   mov      qword [rdi+sizeof(qword)*(7+11)],r11
   mov      qword [rdi+sizeof(qword)*(7+12)],r12
   mov      qword [rdi+sizeof(qword)*(7+13)],r13
   mov      qword [rdi+sizeof(qword)*(7+14)],rax

   sub      rdi, sizeof(qword)*8
   ret
ENDFUNC sqr15_triangle


;;
;; 16*qword triangle
;;
align IPP_ALIGN_FACTOR
DECLARE_FUNC sqr16_triangle,PRIVATE
   call     sqr8_triangle
   mov      qword [rdi+sizeof(qword)*7], r15
   xor      r15, r15

   mov      rcx, rsi
   add      rsi, sizeof(qword)*8
   add      rdi, sizeof(qword)*8
   call     mla_8x8

   add      rdi, sizeof(qword)*8
   call     sqr8_triangle

   xor      rax, rax
   mov      qword [rdi+sizeof(qword)*7], r15
   mov      qword [rdi+sizeof(qword)*8], r8
   mov      qword [rdi+sizeof(qword)*9], r9
   mov      qword [rdi+sizeof(qword)*10],r10
   mov      qword [rdi+sizeof(qword)*11],r11
   mov      qword [rdi+sizeof(qword)*12],r12
   mov      qword [rdi+sizeof(qword)*13],r13
   mov      qword [rdi+sizeof(qword)*14],r14
   mov      qword [rdi+sizeof(qword)*15],rax

   sub      rsi, sizeof(qword)*8
   sub      rdi, sizeof(qword)*16
   ret
ENDFUNC sqr16_triangle



sqr_l_basic    DQ    sqr_1 - sqr_l_basic
               DQ    sqr_2 - sqr_l_basic
               DQ    sqr_3 - sqr_l_basic
               DQ    sqr_4 - sqr_l_basic
               DQ    sqr_5 - sqr_l_basic
               DQ    sqr_6 - sqr_l_basic
               DQ    sqr_7 - sqr_l_basic
               DQ    sqr_8 - sqr_l_basic
               DQ    sqr_9 - sqr_l_basic
               DQ    sqr_10- sqr_l_basic
               DQ    sqr_11- sqr_l_basic
               DQ    sqr_12- sqr_l_basic
               DQ    sqr_13- sqr_l_basic
               DQ    sqr_14- sqr_l_basic
               DQ    sqr_15- sqr_l_basic
               DQ    sqr_16- sqr_l_basic

sqrN_triangle  DQ    sqr9_triangle  - sqrN_triangle
               DQ    sqr10_triangle - sqrN_triangle
               DQ    sqr11_triangle - sqrN_triangle
               DQ    sqr12_triangle - sqrN_triangle
               DQ    sqr13_triangle - sqrN_triangle
               DQ    sqr14_triangle - sqrN_triangle
               DQ    sqr15_triangle - sqrN_triangle
               DQ    sqr16_triangle - sqrN_triangle

%endif ;; _PCPBNSQR_BASIC_ADCX_INC_
