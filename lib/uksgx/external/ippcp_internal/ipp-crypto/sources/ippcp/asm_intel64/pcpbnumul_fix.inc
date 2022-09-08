;===============================================================================
; Copyright 2013-2021 Intel Corporation
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

%ifndef _PCPBNUMUL_FIX_ADCX_INC_
%assign _PCPBNUMUL_FIX_ADCX_INC_  1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; macro used to replace op reg, mem instructions
%macro SWAP 2.nolist
  %xdefine %%r1 %1
  %xdefine %%r2 %2

   xor      %%r1, %%r2
   xor      %%r2, %%r1
   xor      %%r1, %%r2
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; macro used extract proc ep from the table
%macro GET_EP 3-4.nolist
  %xdefine %%regEp %1
  %xdefine %%Table %2
  %xdefine %%regIdx %3
  %xdefine %%tmp %4

   lea   %%regEp, [rel %%Table]
   %ifnempty %%tmp
   mov   %%tmp, [%%regEp+%%regIdx*sizeof(qword)-sizeof(qword)]
   add   %%regEp, %%tmp
   %else
   mov   %%regIdx, [%%regEp+%%regIdx*sizeof(qword)-sizeof(qword)]
   add   %%regEp, %%regIdx
   %endif
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; op DST, mem_src
%macro op_reg_mem 4.nolist
  %xdefine %%opCode %1
  %xdefine %%dst %2
  %xdefine %%mem_src %3
  %xdefine %%tmp %4

   mov      %%tmp, %%mem_src
   %%opCode   %%dst, %%tmp
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MEM_DST = (op src1, mem_src2)
%macro op_mem_reg_mem 5.nolist
  %xdefine %%opCode %1
  %xdefine %%mem_dst %2
  %xdefine %%src1 %3
  %xdefine %%mem_src2 %4
  %xdefine %%tmp %5

   op_reg_mem  %%opCode, %%src1, %%mem_src2, %%tmp
   mov         %%mem_dst, %%src1
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MEM_DST = op mem_src1, mem_src2
%macro op_mem_mem 5.nolist
  %xdefine %%opCode %1
  %xdefine %%mem_dst %2
  %xdefine %%mem_src1 %3
  %xdefine %%mem_src2 %4
  %xdefine %%tmp %5

   mov         %%tmp, %%mem_src1
   %%opCode      %%tmp, %%mem_src2
   mov         %%mem_dst, %%tmp
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;; Inputs: pDst: Destination  (upto 1024 bits, 16 qwords)
;;;         pA:   Multiplicand (upto 512 bits, 8 qwords)
;;;
;;;
;;; Uses registers:
;;;   rdx      - implicit multiplicand
;;;   (HIP:LOP)- 64x64-bit product (mulx)
;;;   X7,,X0   - high result
;;;
%macro MUL_FIX 12-13.nolist
  %xdefine %%N %1
  %xdefine %%pDst %2
  %xdefine %%pA %3
  %xdefine %%HIP %4
  %xdefine %%LOP %5
  %xdefine %%X0 %6
  %xdefine %%X1 %7
  %xdefine %%X2 %8
  %xdefine %%X3 %9
  %xdefine %%X4 %10
  %xdefine %%X5 %11
  %xdefine %%X6 %12
  %xdefine %%X7 %13

 gsmulx  %%X0, %%LOP, [%%pA]                 ; (X0:LO) = a[0]*b
   %ifnempty  %%pDst
   mov   [%%pDst], %%LOP
   %endif

   %if %%N > 1
 gsmulx  %%X1, %%HIP, [%%pA+sizeof(qword)]   ; (X1:LO) = a[1]*b
   add   %%X0, %%HIP
   %if %%N == 2
   adc   %%X1, 0
   %endif

   %if %%N > 2
 gsmulx  %%X2, %%LOP, [%%pA+sizeof(qword)*2] ; (X2:LO) = a[2]*b
   adc   %%X1, %%LOP
   %if %%N == 3
   adc   %%X2, 0
   %endif

   %if %%N > 3
 gsmulx  %%X3, %%HIP, [%%pA+sizeof(qword)*3] ; (X3:LO) = a[3]*b
   adc   %%X2, %%HIP
   %if %%N == 4
   adc   %%X3, 0
   %endif

   %if %%N > 4
 gsmulx  %%X4, %%LOP, [%%pA+sizeof(qword)*4] ; (X4:LO) = a[4]*b
   adc   %%X3, %%LOP
   %if %%N == 5
   adc   %%X4, 0
   %endif

   %if %%N > 5
 gsmulx  %%X5, %%HIP, [%%pA+sizeof(qword)*5] ; (X5:LO) = a[5]*b
   adc   %%X4, %%HIP
   %if %%N == 6
   adc   %%X5, 0
   %endif

   %if %%N > 6
 gsmulx  %%X6, %%LOP, [%%pA+sizeof(qword)*6] ; (X6:LO) = a[6]*b
   adc   %%X5, %%LOP
   %if %%N == 7
   adc   %%X6, 0
   %endif

   %if %%N > 7
 gsmulx  %%X7, %%HIP, [%%pA+sizeof(qword)*7] ; (X7:LO) = a[7]*b
   adc   %%X6, %%HIP
   adc   %%X7, 0
   %endif
   %endif
   %endif
   %endif
   %endif
   %endif
   %endif
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; {x7,x6,x5,x4,x3,x2,x1,x0},DCT[0] = {x7,x6,x5,x4,x3,x2,x1,x0} + A[7:0]*B[i]
;;
;; uses rax, rdx
;;    rdx      - implicit multiplicand B[i] (should to be preloaded)
;;    rax      - temporary product
;;    (HIP:LOP)- 64x64-bit product (mulx)
;;    X7,,X0   - high result
;;
%macro MLA_FIX 6-13.nolist
  %xdefine %%N %1
  %xdefine %%pDst %2
  %xdefine %%pA %3
  %xdefine %%HIP %4
  %xdefine %%LOP %5
  %xdefine %%X0 %6
  %xdefine %%X1 %7
  %xdefine %%X2 %8
  %xdefine %%X3 %9
  %xdefine %%X4 %10
  %xdefine %%X5 %11
  %xdefine %%X6 %12
  %xdefine %%X7 %13

 gsmulx  %%HIP, rax, [%%pA]                ; (HI:rax) = a[0]*b[i]
   add   rax, %%X0
   adc   %%HIP, 0
   %ifnempty  %%pDst
   mov   [%%pDst], rax
   %endif
   %if %%N == 1
   mov   %%X0, %%HIP
   %endif

   %if %%N > 1
 gsmulx  %%LOP,%%X0, [%%pA+sizeof(qword)]    ; (LO:X0) = a[1]*b[i]
   add   %%X0, %%X1
   adc   %%LOP, 0
   add   %%X0, %%HIP
   adc   %%LOP, 0
   %if %%N == 2
   mov   %%X1, %%LOP
   %endif

   %if %%N > 2
 gsmulx  %%HIP,%%X1, [%%pA+sizeof(qword)*2]  ; (HI:X1) = a[2]*b[i]
   add   %%X1, %%X2
   adc   %%HIP, 0
   add   %%X1, %%LOP
   adc   %%HIP, 0
   %if %%N == 3
   mov   %%X2, %%HIP
   %endif

   %if %%N > 3
 gsmulx  %%LOP,%%X2, [%%pA+sizeof(qword)*3]  ; (LO,X2) = a[3]*b[i]
   add   %%X2, %%X3
   adc   %%LOP, 0
   add   %%X2, %%HIP
   adc   %%LOP, 0
   %if %%N == 4
   mov   %%X3, %%LOP
   %endif

   %if %%N > 4
 gsmulx  %%HIP,%%X3, [%%pA+sizeof(qword)*4]  ; (HI:X3) = a[4]*b[i]
   add   %%X3, %%X4
   adc   %%HIP, 0
   add   %%X3, %%LOP
   adc   %%HIP, 0
   %if %%N == 5
   mov   %%X4, %%HIP
   %endif

   %if %%N > 5
 gsmulx  %%LOP,%%X4, [%%pA+sizeof(qword)*5]  ; (LO:X4) = a[5]*b[i]
   add   %%X4, %%X5
   adc   %%LOP, 0
   add   %%X4, %%HIP
   adc   %%LOP, 0
   %if %%N == 6
   mov   %%X5, %%LOP
   %endif

   %if %%N > 6
 gsmulx  %%HIP,%%X5, [%%pA+sizeof(qword)*6]  ; (HI:X5) = a[6]*b[i]
   add   %%X5, %%X6
   adc   %%HIP, 0
   add   %%X5, %%LOP
   adc   %%HIP, 0
   %if %%N == 7
   mov   %%X6, %%HIP
   %endif

   %if %%N > 7
 gsmulx  %%LOP, %%X6, [%%pA+sizeof(qword)*7]  ; (LO:X6) = a[7]*b[i]
   add   %%X6, %%X7
   adc   %%LOP, 0
   add   %%X6, %%HIP
   adc   %%LOP, 0
   mov   %%X7, %%LOP
   %endif
   %endif
   %endif
   %endif
   %endif
   %endif
   %endif
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Inputs: pDst: Destination  (upto 1024 bits, 16 qwords)
;;;         pA:   Multiplicand (upto 512 bits, 8 qwords)
;;;         pB:   Multiplicand (upto 512 bits, 8 qwords)
;;;
;;;
;;; Uses registers:
;;;   rdx      - implicit multiplicand
;;;   (HIP:LOP)- 64x64-bit product (mulx)
;;;   X0,,X7   - high result
;;;
%macro MUL_NxN 7-14.nolist
  %xdefine %%N %1
  %xdefine %%pDst %2
  %xdefine %%pA %3
  %xdefine %%pB %4
  %xdefine %%HIP %5
  %xdefine %%LOP %6
  %xdefine %%X0 %7
  %xdefine %%X1 %8
  %xdefine %%X2 %9
  %xdefine %%X3 %10
  %xdefine %%X4 %11
  %xdefine %%X5 %12
  %xdefine %%X6 %13
  %xdefine %%X7 %14

   mov      rdx, [%%pB]
    MUL_FIX  {%%N},%%pDst,{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}

   %if %%N > 1
   mov      rdx, [%%pB+sizeof(qword)*1]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*1},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 2
   mov      rdx, [%%pB+sizeof(qword)*2]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*2},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 3
   mov      rdx, [%%pB+sizeof(qword)*3]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*3},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 4
   mov      rdx, [%%pB+sizeof(qword)*4]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*4},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 5
   mov      rdx, [%%pB+sizeof(qword)*5]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*5},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 6
   mov      rdx, [%%pB+sizeof(qword)*6]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*6},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 7
   mov      rdx, [%%pB+sizeof(qword)*7]
   MLA_FIX  {%%N},{%%pDst+sizeof(qword)*7},{%%pA},{%%HIP},{%%LOP},{%%X0},{%%X1},{%%X2},{%%X3},{%%X4},{%%X5},{%%X6},{%%X7}
   %endif

   %if %%N > 7
   mov   qword [%%pDst+sizeof(qword)*8],  %%X0
   mov   qword [%%pDst+sizeof(qword)*9],  %%X1
   mov   qword [%%pDst+sizeof(qword)*10], %%X2
   mov   qword [%%pDst+sizeof(qword)*11], %%X3
   mov   qword [%%pDst+sizeof(qword)*12], %%X4
   mov   qword [%%pDst+sizeof(qword)*13], %%X5
   mov   qword [%%pDst+sizeof(qword)*14], %%X6
   mov   qword [%%pDst+sizeof(qword)*15], %%X7
   %elif %%N > 6
   mov   qword [%%pDst+sizeof(qword)*7],  %%X0
   mov   qword [%%pDst+sizeof(qword)*8],  %%X1
   mov   qword [%%pDst+sizeof(qword)*9],  %%X2
   mov   qword [%%pDst+sizeof(qword)*10], %%X3
   mov   qword [%%pDst+sizeof(qword)*11], %%X4
   mov   qword [%%pDst+sizeof(qword)*12], %%X5
   mov   qword [%%pDst+sizeof(qword)*13], %%X6
   %elif %%N > 5
   mov   qword [%%pDst+sizeof(qword)*6],  %%X0
   mov   qword [%%pDst+sizeof(qword)*7],  %%X1
   mov   qword [%%pDst+sizeof(qword)*8],  %%X2
   mov   qword [%%pDst+sizeof(qword)*9],  %%X3
   mov   qword [%%pDst+sizeof(qword)*10], %%X4
   mov   qword [%%pDst+sizeof(qword)*11], %%X5
   %elif %%N > 4
   mov   qword [%%pDst+sizeof(qword)*5],  %%X0
   mov   qword [%%pDst+sizeof(qword)*6],  %%X1
   mov   qword [%%pDst+sizeof(qword)*7],  %%X2
   mov   qword [%%pDst+sizeof(qword)*8],  %%X3
   mov   qword [%%pDst+sizeof(qword)*9],  %%X4
   %elif %%N > 3
   mov   qword [%%pDst+sizeof(qword)*4],  %%X0
   mov   qword [%%pDst+sizeof(qword)*5],  %%X1
   mov   qword [%%pDst+sizeof(qword)*6],  %%X2
   mov   qword [%%pDst+sizeof(qword)*7],  %%X3
   %elif %%N > 2
   mov   qword [%%pDst+sizeof(qword)*3],  %%X0
   mov   qword [%%pDst+sizeof(qword)*4],  %%X1
   mov   qword [%%pDst+sizeof(qword)*5],  %%X2
   %elif %%N > 1
   mov   qword [%%pDst+sizeof(qword)*2],  %%X0
   mov   qword [%%pDst+sizeof(qword)*3],  %%X1
   %else
   mov   qword [%%pDst+sizeof(qword)*1],  %%X0
   %endif ;; N==2

%endmacro

%endif ;; _PCPBNUMUL_FIX_ADCX_INC_
