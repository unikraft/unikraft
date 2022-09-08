;===============================================================================
; Copyright 2006-2021 Intel Corporation
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
;               Big Number macros
;
;     Content:
;        COPY_BNU
;        CMP_BNU
;        FIX_BNU
;
;        SUB_FIX_BNU
;        ADD_FIX_BNU
;

;;
;; CMP_BNU     comare arbitrary BNUs
;;
;; input
;;    rSrc1    points BNU1
;;    rSrc2    points BNU2
;;    rLen     size of BNUs
;;
;; output
;;    flags
;;
%macro CMP_BNU 4.nolist
  %xdefine %%rSrc1 %1
  %xdefine %%rSrc2 %2
  %xdefine %%rLen %3
  %xdefine %%tmp %4

%%cmp_loop:
   mov   %%tmp,[%%rSrc1+%%rLen*4-4]
   cmp   %%tmp,[%%rSrc2+%%rLen*4-4]
   jne   %%cmp_quit
   sub   %%rLen,1
   jg    %%cmp_loop
%%cmp_quit:
%endmacro


;;
;; SUB_FIX_BNU subtract fixed size BNUs
;;
;; input
;;    rVal     points src/dst BNU
;;    rSrc     points source BNU
;;    immLen   BNU size
;;
%macro SUB_FIX_BNU 3.nolist
  %xdefine %%rVal %1
  %xdefine %%rSrc %2
  %xdefine %%immLen %3

   pxor     mm0,mm0
   %assign %%i 0
   %rep %%immLen
   movd     mm1,DWORD [%%rVal+%%i*4]
   movd     mm2,DWORD [%%rSrc+%%i*4]
   %if %%i != 0
    paddq    mm1,mm0
   %endif
    psubq    mm1,mm2
   movd     DWORD [%%rVal+%%i*4],mm1
   %if %%i < (%%immLen-1)
    pshufw   mm0,mm1,11111110b
   %endif
   %assign %%i %%i+1
   %endrep
%endmacro


;;
;; ADD_FIX_BNU add fixed size BNUs
;;
;; input
;;    rVal     points src/dst BNU
;;    rSrc     points source BNU
;;    immLen   BNU size
;;
%macro ADD_FIX_BNU 3.nolist
  %xdefine %%rVal %1
  %xdefine %%rSrc %2
  %xdefine %%immLen %3

    pxor     mm0,mm0
  %assign %%i 0
   %rep %%immLen
   movd     mm1,DWORD [%%rVal+%%i*4]
   movd     mm2,DWORD [%%rSrc+%%i*4]
   %if %%i != 0
    paddq    mm1,mm0
   %endif
    paddq    mm1,mm2
   movd     DWORD [%%rVal+%%i*4],mm1
   %if %%i < (%%immLen-1)
    pshufw   mm0,mm1,11111110b
   %endif
  %assign %%i %%i+1
   %endrep
%endmacro


;;
;; COPY_BNU copy arbitrary BNU
;;
%macro COPY_BNU 5.nolist
  %xdefine %%rSrc %1
  %xdefine %%rDst %2
  %xdefine %%rLen %3
  %xdefine %%rIdx %4
  %xdefine %%rTmp %5

   xor      %%rIdx,%%rIdx
%%copy_bnu:
   mov      %%rTmp,[%%rSrc+%%rIdx*4]
   mov      [%%rDst+%%rIdx*4],%%rTmp
   add      %%rIdx,1
   cmp      %%rIdx,%%rLen
   jl       %%copy_bnu
%endmacro


;;
;; FIX_BNU     returns actual length of BNU
;;
;; input
;;    rSrc     points BNU
;;    rLen     initial BNU size
;;
;; output
;;    rLen     actual BNU size
;;
%macro FIX_BNU 3.nolist
  %xdefine %%rSrc %1
  %xdefine %%rLen %2
  %xdefine %%tmp %3

%%fix_bnu_loop:
   mov      %%tmp,[%%rSrc+%%rLen*4-4]     ;; value
   test     %%tmp,%%tmp                 ;; test BNU component
   jnz      %%fix_bnu_quit
   sub      %%rLen,1
   jg       %%fix_bnu_loop
   add      %%rLen,1
%%fix_bnu_quit:
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro MULADD_START 2.nolist
  %xdefine %%i %1
  %xdefine %%j %2

    movd mm1,DWORD [eax + 4*%%j]
    movd mm3,DWORD [eax + 4*%%j]
    pmuludq mm1,mm0
    paddq mm7,mm1
    movd DWORD [edx + 4*(%%i+%%j)],mm7
    pand mm3,mm6
    psrlq mm7,32
    paddq mm7,mm3
%endmacro


%macro MULADD 2.nolist
  %xdefine %%i %1
  %xdefine %%j %2

    movd mm1,DWORD [eax + 4*%%j]
    movd mm3,DWORD [eax + 4*%%j]
    movd mm2,DWORD [edx + 4*(%%i+%%j)]
    pmuludq mm1,mm0
    pand mm3,mm6
    paddq mm1,mm2
    paddq mm7,mm1
    movd DWORD [edx + 4*(%%i+%%j)],mm7
    psrlq mm7,32
    paddq mm7,mm3
%endmacro


%macro SQR_DECOMPOSE 1.nolist
  %xdefine %%i %1

    movd mm7,DWORD [eax + 4*%%i]
    movd mm0,DWORD [eax + 4*%%i]
    movd mm6,DWORD [eax + 4*%%i]
    %if %%i != 0
    movd mm1,DWORD [edx + 4*(2*%%i)]
    %endif
    pslld mm0,1
    pmuludq mm7,mm7
    psrad mm6,32
    %if %%i != 0
    paddq mm7,mm1
    %endif
    movd DWORD [edx + 4*(2*%%i)],mm7
    psrlq mm7,32
%endmacro


%macro STORE_CARRY 2.nolist
  %xdefine %%i %1
  %xdefine %%s %2

    movq DWORD [edx + 4*(%%i + %%s)],mm7
%endmacro


%macro STORE_CARRY_NEXT 2.nolist
  %xdefine %%i %1
  %xdefine %%s %2

    movd mm4,DWORD [edx + 4*(%%i + %%s)]
    paddq mm4,mm7
    movd DWORD [edx + 4*(%%i + %%s)],mm4
    psrlq mm7,32
    movd DWORD [edx + 4*(%%i + %%s + 1)],mm7
%endmacro


%macro LAST_STEP 1.nolist
  %xdefine %%s %1

    movd mm7,DWORD [eax + 4*(%%s - 1)]
    movd mm2,DWORD [edx + 4*(2*%%s - 2)]
    pmuludq mm7,mm7
    paddq mm7,mm2
    movd mm4,DWORD [edx + 4*(2*%%s - 1)]
    movd DWORD [edx + 4*(2*%%s - 2)],mm7
    psrlq mm7,32
    paddq mm4,mm7
    movd DWORD [edx + 4*(2*%%s - 1)],mm4
%endmacro


%macro INNER_LOOP 2.nolist
  %xdefine %%i %1
  %xdefine %%nsize %2

  %assign %%j  %%i + 1
  %assign %%s  %%nsize - %%i - 1

    SQR_DECOMPOSE %%i

    %rep %%s
   %if %%i == 0
     MULADD_START %%i,%%j
   el%%se
     MULADD %%i,%%j
   %endif
  %assign %%j  %%j + 1
    %endrep

   %if %%i == 0
     STORE_CARRY %%i,%%nsize
   el%%se
     STORE_CARRY_NEXT %%i,%%nsize
   %endif
%endmacro


%macro OUTER_LOOP 1.nolist
  %xdefine %%nsize %1

  %assign %%i  0
    %rep ns%%ize - 1
    INNER_LOOP %%i,%%nsize
  %assign %%i  %%i + 1
    %endrep

    LAST_STEP %%nsize
%endmacro


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro MULADD_START_wt_carry 1.nolist
  %xdefine %%i %1

    movd mm7,DWORD [eax]
    movd mm2,DWORD [edx + 4*(%%i)]
    pmuludq mm7,mm0
    paddq mm7,mm2
    movd DWORD [edx + 4*(%%i)],mm7
    psrlq mm7,32
%endmacro


%macro MULADD_START1 2.nolist
  %xdefine %%i %1
  %xdefine %%j %2

    movd mm1,DWORD [eax + 4*%%j]
    pmuludq mm1,mm0
    paddq mm7,mm1
    movd DWORD [edx + 4*(%%i+%%j)],mm7
    psrlq mm7,32
%endmacro


%macro MULADD_START_wt_carry1 0.nolist
    movd mm7,DWORD [eax]
    pmuludq mm7,mm0
    movd DWORD [edx],mm7
    psrlq mm7,32
%endmacro


%macro MULADD1 2.nolist
  %xdefine %%i %1
  %xdefine %%j %2

    movd mm1,DWORD [eax + 4*%%j]
    movd mm2,DWORD [edx + 4*(%%i+%%j)]
    pmuludq mm1,mm0
    paddq mm1,mm2
    paddq mm7,mm1
    movd DWORD [edx + 4*(%%i+%%j)],mm7
    psrlq mm7,32
%endmacro


%macro INNER_LOOP1 2.nolist
  %xdefine %%i %1
  %xdefine %%nsize %2

    %assign %%j  0
    movd mm0,DWORD [ebx + 4*%%i]

    %rep %%nsize
%if %%i == 0
    %if %%j == 0
        MULADD_START_wt_carry1
    %else
        MULADD_START1 %%i,%%j
    %endif
%else
    %if %%j == 0
        MULADD_START_wt_carry %%i
    %else
        MULADD1 %%i,%%j
    %endif
%endif
    %assign %%j  %%j + 1
    %endrep
    movd DWORD [edx + 4*(%%i + %%nsize)],mm7
%endmacro


%macro OUTER_LOOP1 1.nolist
  %xdefine %%nsize %1

  %assign %%i  0
    %rep %%nsize
    INNER_LOOP1 %%i,%%nsize
  %assign %%i  %%i + 1
    %endrep
%endmacro

