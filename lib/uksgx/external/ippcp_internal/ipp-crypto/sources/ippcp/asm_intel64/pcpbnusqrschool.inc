;===============================================================================
; Copyright 2010-2021 Intel Corporation
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
;               BNU squaring support
;
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MULx1 genaral-case squarer macros
;;

;; dst = src * B epilogue (srcLen=4*n+3)
%macro sMULx1_4N_3_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%update_idx %3
  %xdefine %%B %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mul   %%B
   xor   %%T1, %%T1
   add   %%T0, rax
   mov   qword [%%rDst+sizeof(qword)], %%T0
   mov   rax, qword [%%rSrc+sizeof(qword)*2]
   adc   %%T1, rdx

   mul   %%B
   xor   %%T2, %%T2
   add   %%T1, rax
   mov   qword [%%rDst+sizeof(qword)*2], %%T1
   mov   rax, qword [%%rSrc+sizeof(qword)*3]
   adc   %%T2, rdx

   mul   %%B
   %%update_idx
   add   %%T2, rax
   mov   qword [%%rDst+sizeof(qword)*3], %%T2
  ;mov   rax, qword [rSrc+idx*sizeof(qword)]
   adc   rdx, 0

   mov   qword [%%rDst+sizeof(qword)*4], rdx
   add   %%rDst, sizeof(qword)
%endmacro

;; dst = src * B epilogue (srcLen=4*n+1)
%macro sMULx1_4N_1_ELOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%update_idx %3
  %xdefine %%B %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mul   %%B
   %%update_idx
   add   %%T0, rax
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
  ;mov   rax, qword [rSrc+idx*sizeof(qword)]
   adc   rdx, 0

   mov   qword [%%rDst+sizeof(qword)*4], rdx
   add   %%rDst, sizeof(qword)
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MULx2 genaral-case multiplier macros
;;

;; dst = src * {B1:B0} epilogue (srcLen=4*n+1)
%macro sMULx2_4N_1_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%update_idx %3
  %xdefine %%B0 %4
  %xdefine %%B1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%B1                                                       ; {T2:T1} += a[lenA-1]*B1
  ;add   rDst, sizeof(qword)*2
   %%update_idx
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
   add   %%T1, rax
  ;mov   rax, qword [rSrc+idx*sizeof(qword)]
   adc   rdx, %%T2

   mov   qword [%%rDst+sizeof(qword)*4], %%T1
   mov   qword [%%rDst+sizeof(qword)*5], rdx
%endmacro

;; dst = src * {B1:B0} epilogue (srcLen=4*n+3)
%macro sMULx2_4N_3_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%update_idx %3
  %xdefine %%B0 %4
  %xdefine %%B1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%B1                                                       ; {T2:T1} += a[lenA-3]*B1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[lenA-2]
   adc   %%T2, rdx

   mul   %%B0                                                       ; {T3:T2:T1} += a[LenA-2]*B0
   mov   qword [%%rDst+sizeof(qword)], %%T0
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[lenA-2]
   adc   %%T2, rdx
   adc   %%T3, 0

   mul   %%B1                                                       ; {T3:T2} += a[lenA-2]*B1
   xor   %%T0, %%T0
   add   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[lenA-1]
   adc   %%T3, rdx

   mul   %%B0                                                       ; {T0:T3:T2} += a[lenA-1]*B0
   mov   qword [%%rDst+sizeof(qword)*2], %%T1
   add   %%T2, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[lenA-1]
   adc   %%T3, rdx
   adc   %%T0, 0

   mul   %%B1                                                       ; {T0:T3} += a[lenA-1]*B1
  ;add   rDst, sizeof(qword)*2
   %%update_idx
   mov   qword [%%rDst+sizeof(qword)*3], %%T2
   add   %%T3, rax
  ;mov   rax, qword [rSrc+idx*sizeof(qword)]
   adc   rdx, %%T0

   mov   qword [%%rDst+sizeof(qword)*4], %%T3
   mov   qword [%%rDst+sizeof(qword)*5], rdx
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MLAx2 genaral-case multiplier macros
;;

;;
;; B0 = rSrc[-2]
;; B1 = rSrc[-1]
;; inp_vector = rSrc
;; out_vector = rDst
%macro sMLAx2_PLOG 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%B0 %3
  %xdefine %%B1 %4
  %xdefine %%T0 %5
  %xdefine %%T1 %6
  %xdefine %%T2 %7
  %xdefine %%T3 %8

   mov      %%B0, qword [%%rSrc-2*sizeof(qword)]   ; preload a[-2]
   mov      %%B1, qword [%%rSrc-sizeof(qword)]     ; and a[i-1]

   mov      rax, %%B1
   mul      %%B0                                     ; a[-2]*a[i-1]
   xor      %%T0, %%T0

   add      qword [%%rDst-sizeof(qword)], rax
   mov      rax, qword [%%rSrc]                  ; a[i]
   adc      %%T0, rdx

   mul      %%B0                                     ; B0*a[i]
   xor      %%T1, %%T1
   xor      %%T2, %%T2
   add      %%T0, rax
   mov      rax, qword [%%rSrc]                  ; a[i]
   adc      %%T1, rdx
%endmacro

;; dst = + src * {B1:B0} epilogue (srcLen=4*n+1)
%macro sMLAx2_4N_1_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%update_idx %3
  %xdefine %%B0 %4
  %xdefine %%B1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%B1                                                       ; {T2:T1} += a[lenA-1]*B1 + r[lenA-1]
  ;add   rDst, sizeof(qword)*2
   %%update_idx
   add   %%T0, qword [%%rDst+sizeof(qword)*3]
   mov   qword [%%rDst+sizeof(qword)*3], %%T0
   adc   %%T1, rax
   adc   rdx, %%T2
  ;mov   rax, qword [rSrc+idx*sizeof(qword)]

   mov   qword [%%rDst+sizeof(qword)*4], %%T1
   mov   qword [%%rDst+sizeof(qword)*5], rdx
%endmacro

;; dst = + src * {B1:B0} epilogue (srcLen=4*n+3)
%macro sMLAx2_4N_3_ELOG 9.nolist
  %xdefine %%rDst %1
  %xdefine %%rSrc %2
  %xdefine %%update_idx %3
  %xdefine %%B0 %4
  %xdefine %%B1 %5
  %xdefine %%T0 %6
  %xdefine %%T1 %7
  %xdefine %%T2 %8
  %xdefine %%T3 %9

   mul   %%B1                                                       ; {T2:T1} += a[lenA-3]*B1
   xor   %%T3, %%T3
   add   %%T1, rax
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[lenA-2]
   adc   %%T2, rdx

   mul   %%B0                                                       ; {T3:T2:T1} += a[LenA-2]*B0 + r[len-3]
   add   %%T0, qword [%%rDst+sizeof(qword)]
   mov   qword [%%rDst+sizeof(qword)], %%T0
   adc   %%T1, rax
   adc   %%T2, rdx
   adc   %%T3, 0
   mov   rax, qword [%%rSrc+sizeof(qword)*2]                    ; a[lenA-2]

   mul   %%B1                                                       ; {T3:T2} += a[lenA-2]*B1
   xor   %%T0, %%T0
   add   %%T2, rax
   adc   %%T3, rdx
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[lenA-1]

   mul   %%B0                                                       ; {T0:T3:T2} += a[lenA-1]*B0 + r[lenA-2]
   add   %%T1, qword [%%rDst+sizeof(qword)*2]
   mov   qword [%%rDst+sizeof(qword)*2], %%T1
   adc   %%T2, rax
   adc   %%T3, rdx
   adc   %%T0, 0
   mov   rax, qword [%%rSrc+sizeof(qword)*3]                    ; a[lenA-1]

   mul   %%B1                                                       ; {T0:T3} += a[lenA-1]*B1 + r[lenA-1]
  ;add   rDst, sizeof(qword)*2
   %%update_idx
   add   %%T2, qword [%%rDst+sizeof(qword)*3]
   mov   qword [%%rDst+sizeof(qword)*3], %%T2
   adc   %%T3, rax
   adc   rdx, %%T0
  ;mov   rax, qword [rSrc+idx*sizeof(qword)]

   mov   qword [%%rDst+sizeof(qword)*4], %%T3
   mov   qword [%%rDst+sizeof(qword)*5], rdx
%endmacro

