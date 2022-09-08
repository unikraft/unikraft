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
;               Big Number macros
;
;     Content:
;        SWAP
;        EXPAND_BNU
;
;        COPY_BNU
;        FIX_BNU      CMP_BNU
;
;        ADD_BNU      SUB_BNU
;        ECARRY       EBORROW
;
;        MUL_BNU_D32  MAC_BNU_D32
;        MUL_BNU_D64  MAC_BNU_D64
;
;        MAC          MAC2
;
;        ADD_FIX_BNU  SUB_FIX_BNU
;

;;
;; Swap values or addresses
;;
%macro SWAP 4.nolist
  %xdefine %%cc %1
  %xdefine %%dst %2
  %xdefine %%src %3
  %xdefine %%tmp %4

   cmov&%%cc  %%tmp,%%src
   cmov&%%cc  %%src,%%dst
   cmov&%%cc  %%dst,%%tmp
%endmacro

;;
;; EXPAND_BNU  expands BNU by zero %if one has odd size
;;
%macro EXPAND_BNU 2.nolist
  %xdefine %%rBNU %1
  %xdefine %%rLen %2

   test  %%rLen,1
   jz    %%expand_quit
   mov   DWORD [%%rBNU+%%rLen*4],0
   add   %%rLen,1
%%expand_quit:
%endmacro

;;
;; COPY_BNU
;;
%macro COPY_BNU 5.nolist
  %xdefine %%rSrc %1
  %xdefine %%rDst %2
  %xdefine %%rLen %3
  %xdefine %%rIdx %4
  %xdefine %%rTmp %5

   xor      %%rIdx,%%rIdx
%%copy_bnu:
   mov      %%rTmp&d,[%%rSrc+%%rIdx*4]
   mov      [%%rDst+%%rIdx*4],%%rTmp&d
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
;;    rSrc     points BNU
;;    rLen     actual BNU size
;;
%macro FIX_BNU 3.nolist
  %xdefine %%rSrc %1
  %xdefine %%rLen %2
  %xdefine %%tmp %3

%%fix_bnu_loop:
   mov      %%tmp&d,[%%rSrc+%%rLen*4-4]   ;; value
   test     %%tmp&d,%%tmp&d             ;; test BNU component
   jnz      %%fix_bnu_quit
   sub      %%rLen,1
   jg       %%fix_bnu_loop
   add      %%rLen,1
%%fix_bnu_quit:
%endmacro

;;
;; CMP_BNU     comare BNUs
;;
;; input
;;    rSrc1    points BNU1
;;    rSrc2    points BNU2
;;    rLen     size of BNUs
;;
;; output
;;    rCode    -1/0/1
;;
%macro CMP_BNU 4.nolist
  %xdefine %%rCode %1
  %xdefine %%rSrc1 %2
  %xdefine %%rSrc2 %3
  %xdefine %%rLen %4

%%cmp_bnu_loop:
   mov      %%rCode&d,[%%rSrc1+%%rLen*4-4]   ;; src1[]
   cmp      %%rCode&d,[%%rSrc2+%%rLen*4-4]   ;; src1[] ~  src2[]
   jnz      %%cmp_bnu_quit               ;; src1[] != src2[]
   sub      %%rLen,1
   jg       %%cmp_bnu_loop
%%cmp_bnu_quit:
   seta     %%rCode&b        ;; rCode = (src1[]>src2[])? 1:0
   setb     %%rLen&b         ;; rLen  = (src1[]<src2[])? 1:0
   sub      %%rCode&b,%%rLen&b
   movsx    %%rCode,%%rCode&b
%endmacro

;;
;; ADD_BNU     add BNUs
;;
;; input
;;    rDst     points resultant BNU
;;    rSrc1    points source BNU
;;    rSrc2    points source BNU
;;    rLen     BNU size
;;    rIdx     source, resultant index
;;
;; output
;;    rCarry   carry value (byte)
;;
%macro ADD_BNU 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rCarry %2
  %xdefine %%rSrc1 %3
  %xdefine %%rSrc2 %4
  %xdefine %%rLen %5
  %xdefine %%rIdx %6
  %xdefine %%tmp1 %7
  %xdefine %%tmp2 %8

   xor      %%rCarry,%%rCarry           ;; carry=0
   xor      %%rIdx,%%rIdx               ;; index=0

   sub      %%rLen,2                  ;; test BNU size
   jl       %%short_bnu

   clc                              ;; CF=0
%%add_bnu_loop:
   mov      %%tmp1,[%%rSrc1+%%rIdx*8]     ;; src1[]
   mov      %%tmp2,[%%rSrc2+%%rIdx*8]     ;; src2[]
   adc      %%tmp1,%%tmp2               ;; x = src1[]+src[2]+CF
   mov      [%%rDst+%%rIdx*8],%%tmp1      ;; dst[] = x

   inc      %%rIdx                    ;; advance index
   dec      %%rLen                    ;; decrease length
   dec      %%rLen
   jge      %%add_bnu_loop            ;; continue
   setc     %%rCarry&b                ;; save CF

   add      %%rIdx,%%rIdx               ;; restore ordinal index
   add      %%rLen,2                  ;; restore length
   jz       %%add_bnu_exit

%%short_bnu:
   shr      %%rCarry&d,1              ;; restore CF
   mov      %%tmp1&d,[%%rSrc1+%%rIdx*4]   ;; src1[]
   mov      %%tmp2&d,[%%rSrc2+%%rIdx*4]   ;; src2[]
   adc      %%tmp1&d,%%tmp2&d           ;; x = src1[]-src[2]-CF
   mov      [%%rDst+%%rIdx*4],%%tmp1&d    ;; dst[] = x
   setc     %%rCarry&b                ;; save CF
   add      %%rIdx,1                  ;; advance index
%%add_bnu_exit:
%endmacro

;;
;; ECARRY  expand carry
;;
;; input
;;    rDst     points resultant BNU
;;    rCarry   contains borrow
;;    rSrc     points source BNU
;;    rLen     BNU size
;;    rIdx     source, resultant index
;;
;; output
;;    rCarry   carry  value (byte)
;;
%macro ECARRY 6.nolist
  %xdefine %%rDst %1
  %xdefine %%rCarry %2
  %xdefine %%rSrc %3
  %xdefine %%rLen %4
  %xdefine %%rIdx %5
  %xdefine %%tmp %6

   shr      %%rCarry&d,1              ;; restore CF
%%ecarry_loop:
   mov      %%tmp&d,[%%rSrc+%%rIdx*4]     ;; a[i]
   adc      %%tmp&d,0                 ;; x = a[]+CF
   mov      [%%rDst+%%rIdx*4],%%tmp&d     ;; dst[] = x
   inc      %%rIdx                    ;; advance index
   dec      %%rLen                    ;; decrease length
   jge      %%ecarry_loop             ;; continue
   setc     %%rCarry&b                ;; save CF
%endmacro

;;
;; SUB_BNU     sub BNUs
;;
;; input
;;    rDst     points resultant BNU
;;    rSrc1    points source BNU
;;    rSrc2    points source BNU
;;    rLen     BNU size
;;    rIdx     source, resultant index
;;
;; output
;;    rBorrow  borrow value (byte)
;;
%macro SUB_BNU 8.nolist
  %xdefine %%rDst %1
  %xdefine %%rBorrow %2
  %xdefine %%rSrc1 %3
  %xdefine %%rSrc2 %4
  %xdefine %%rLen %5
  %xdefine %%rIdx %6
  %xdefine %%tmp1 %7
  %xdefine %%tmp2 %8

   xor      %%rBorrow,%%rBorrow         ;; borrow=0
   xor      %%rIdx,%%rIdx               ;; index=0

   sub      %%rLen,2                  ;; test BNU size
   jl       %%short_bnu

   clc                              ;; CF=0
%%sub_bnu_loop:
   mov      %%tmp1,[%%rSrc1+%%rIdx*8]     ;; src1[]
   mov      %%tmp2,[%%rSrc2+%%rIdx*8]     ;; src2[]
   sbb      %%tmp1,%%tmp2               ;; x = src1[]-src[2]-CF
   mov      [%%rDst+%%rIdx*8],%%tmp1      ;; dst[] = x

   inc      %%rIdx                    ;; advance index
   dec      %%rLen                    ;; decrease length
   dec      %%rLen
   jge      %%sub_bnu_loop            ;; continue
   setc     %%rBorrow&b               ;; save CF

   add      %%rIdx,%%rIdx               ;; restore ordinal index
   add      %%rLen,2                  ;; restore length
   jz       %%sub_bnu_exit

%%short_bnu:
   shr      %%rBorrow&d,1             ;; restore CF
   mov      %%tmp1&d,[%%rSrc1+%%rIdx*4]   ;; src1[]
   mov      %%tmp2&d,[%%rSrc2+%%rIdx*4]   ;; src2[]
   sbb      %%tmp1&d,%%tmp2&d           ;; x = src1[]-src[2]-CF
   mov      [%%rDst+%%rIdx*4],%%tmp1&d    ;; dst[] = x
   setc     %%rBorrow&b               ;; save CF
   add      %%rIdx,1                  ;; advance index
%%sub_bnu_exit:
%endmacro

;;
;; EBORROW  expand borrow
;;
;; input
;;    rDst     points resultant BNU
;;    rBorrow  contains borrow
;;    rSrc     points source BNU
;;    rLen     BNU size
;;    rIdx     source, resultant index
;;
;; output
;;    rBorrow  borrow value (byte)
;;
%macro EBORROW 6.nolist
  %xdefine %%rDst %1
  %xdefine %%rBorrow %2
  %xdefine %%rSrc %3
  %xdefine %%rLen %4
  %xdefine %%rIdx %5
  %xdefine %%tmp %6

   shr      %%rBorrow,1               ;; restore CF
%%eborrow_loop:
   mov      %%tmp&d,[%%rSrc+%%rIdx*4]     ;; a[i]
   sbb      %%tmp&d,0                 ;; x = a[]-CF
   mov      [%%rDst+%%rIdx*4],%%tmp&d     ;; dst[] = x
   inc      %%rIdx                    ;; advance index
   dec      %%rLen                    ;; decrease length
   jge      %%eborrow_loop            ;; continue
   setc     %%rBorrow&b               ;; save CF
%endmacro

;;
;; MUL_BNU_D32 multiplay BNU by 32-bit Digit
;;
;; input
;;    rSrc     points source BNU
;;    rDgt     32-bit digit value
;;    rDst     points resultant BNU
;;    rLen     BNU size
;;    rIdx     (scratch) source/target index
;;    rExp     (scratch) expansion
;;
;; output
;;    rDgt     32-bit expansion
;;
;; Note
;;    rdx and rax used in mul instruction,
;;    this mean any macro argument may be neither rax nor rdx
;;
%macro MUL_BNU_D32 6.nolist
  %xdefine %%rSrc %1
  %xdefine %%rDgt %2
  %xdefine %%rDst %3
  %xdefine %%rLen %4
  %xdefine %%rIdx %5
  %xdefine %%rExp %6

   xor      %%rIdx,%%rIdx               ;; index = 0
   xor      %%rExp,%%rExp               ;; expansion = 0

   sub      %%rLen,2                  ;; test source size
   jl       %%mul_bn_short
%%mul_bnu_loop:
   mov      rax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt                    ;; x = a*digit
   add      rax,%%rExp                ;; x+= expansion
   adc      edx,0
   mov      [%%rDst+%%rIdx*4],rax       ;; pDst[i]   = LO64(x)
   mov      %%rExp&d,edx              ;; 32 bit expansion = HI32(x)

   add      %%rIdx,2                  ;; advance index
   sub      %%rLen,2                  ;; decrease length
   jge      %%mul_bnu_loop

   add      %%rLen,2                  ;; source size remainder
   jz       %%mul_bn_quit             ;; edx contains 32 bit expansion

%%mul_bn_short:
   mov      eax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt&d                  ;; x = a*digit
   add      eax,%%rExp&d              ;; x+= expansion
   adc      edx,0
   mov      [%%rDst+%%rIdx*4],eax       ;; pDst[i] = LO32(x)

%%mul_bn_quit:
   mov      %%rDgt&d,edx              ;; 32 bit expansion
%endmacro

;;
;; MAC_BNU_D32 multiplay BNU by 32-bit Digit and accumulate
;;
;; input
;;    rSrc     points source BNU
;;    rDgt     32-bit digit value
;;    rDst     points accumulator (resultant) BNU
;;    rLen     BNU size
;;    rIdx     (scratch) source/target index
;;    rExp     (scratch) expansion
;;
;; output
;;    rDgt     32-bit expansion
;;
;; Note
;;    rdx and rax used in mul instruction,
;;    this mean any macro argument may be neither rax nor rdx
;;
%macro MAC_BNU_D32 6.nolist
  %xdefine %%rSrc %1
  %xdefine %%rDgt %2
  %xdefine %%rDst %3
  %xdefine %%rLen %4
  %xdefine %%rIdx %5
  %xdefine %%rExp %6

   xor      %%rIdx,%%rIdx               ;; index = 0
   xor      %%rExp,%%rExp               ;; expansion = 0

   sub      %%rLen,2                  ;; test source size
   jl       %%mac_bn_short
%%mac_bnu_loop:
   mov      rax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt                    ;; x = a*digit
   add      rax,%%rExp                ;;
   adc      edx,0                   ;;
   add      rax,[%%rDst+%%rIdx*4]       ;; x = expansion + pDst[i] + x
   adc      edx,0                   ;;
   mov      [%%rDst+%%rIdx*4],rax       ;; pDst[i]   = LO64(x)
   mov      %%rExp&d,edx              ;; expansion = HI32(x)

   add      %%rIdx,2                  ;; advance index
   sub      %%rLen,2                  ;; decrease length
   jge      %%mac_bnu_loop

   add      %%rLen,2                  ;; source size remainder
   jz       %%mac_bn_quit             ;; edx contains 32 bit expansion

%%mac_bn_short:
   mov      eax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt&d                  ;; x = a*digit
   add      eax,%%rExp&d              ;;
   adc      edx,0                   ;;
   add      eax,dword [%%rDst+%%rIdx*4] ;; x = expansion + pDst[i] + x
   adc      edx,0                   ;;
   mov      [%%rDst+%%rIdx*4],eax       ;; pDst[i]   = LO64(x)

%%mac_bn_quit:
   mov      %%rDgt&d,edx              ;; 32 bit expansion
%endmacro

;;
;; MUL_BNU_D64 multiplay BNU by 64-bit Digit
;;
;; input
;;    rSrc     points source BNU
;;    rDgt     64-bit digit value
;;    rDst     points resultant BNU
;;    rLen     BNU size
;;    rIdx     (scratch) source/target index
;;    rExp     (scratch) expansion
;;
;; output
;;    rDgt     32-bit expansion (because of origin API)
;;
;; Note
;;    rdx and rax used in mul instruction,
;;    this mean any macro argument may be neither rax nor rdx
;;
%macro MUL_BNU_D64 6.nolist
  %xdefine %%rSrc %1
  %xdefine %%rDgt %2
  %xdefine %%rDst %3
  %xdefine %%rLen %4
  %xdefine %%rIdx %5
  %xdefine %%rExp %6

   xor      %%rIdx,%%rIdx               ;; index = 0
   xor      %%rExp,%%rExp               ;; expansion = 0

   sub      %%rLen,2                  ;; test source size
   jl       %%mul_bn_short
%%mul_bnu_loop:
   mov      rax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt                    ;; x = a*digit
   add      rax,%%rExp                ;; x+= expansion
   adc      rdx,0
   mov      [%%rDst+%%rIdx*4],rax       ;; pDst[i]   = LO64(x)
   mov      %%rExp,rdx                ;; expansion = HI64(x)

   add      %%rIdx,2                  ;; advance index
   sub      %%rLen,2                  ;; decrease length
   jge      %%mul_bnu_loop

   add      %%rLen,2                  ;; source size remainder
   jnz      %%mul_bn_short            ;; BNU of odd length

   shr      rdx,32                  ;; extract 32 bit expansion
   mov      [%%rDst+%%rIdx*4],%%rExp&d    ;; store
   jmp      %%mul_bn_quit

%%mul_bn_short:
   mov      eax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt                    ;; x = a*digit
   add      rax,%%rExp                ;; x+= expansion
   adc      rdx,0
   mov      [%%rDst+%%rIdx*4],rax       ;; pDst[i]   = LO64(x)

%%mul_bn_quit:
   mov      %%rDgt&d,edx              ;; 32 bit expansion
%endmacro

;;
;; MAC_BNU_D64 multiplay BNU by 64-bit Digit and accumulate
;;
;; input
;;    rSrc     points source BNU
;;    rDgt     64-bit digit value
;;    rDst     points accumulator (resultant) BNU
;;    rLen     BNU size
;;    rIdx     (scratch) source/target index
;;    rExp     (scratch) expansion
;;
;; output
;;    rDgt     32-bit expansion (because of origin API)
;;
;; Note
;;    rdx and rax used in mul instruction,
;;    this mean any macro argument may be neither rax nor rdx
;;
%macro MAC_BNU_D64 6.nolist
  %xdefine %%rSrc %1
  %xdefine %%rDgt %2
  %xdefine %%rDst %3
  %xdefine %%rLen %4
  %xdefine %%rIdx %5
  %xdefine %%rExp %6

   xor      %%rIdx,%%rIdx               ;; index = 0
   xor      %%rExp,%%rExp               ;; expansion = 0

   sub      %%rLen,2                  ;; test source size
   jl       %%mac_bn_short
%%mac_bnu_loop:
   mov      rax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt                    ;; x = a*digit
   add      rax,%%rExp                ;;
   adc      rdx,0                   ;;
   add      rax,[%%rDst+%%rIdx*4]       ;; x = expansion + pDst[i] + x
   adc      rdx,0                   ;;
   mov      [%%rDst+%%rIdx*4],rax       ;; pDst[i]   = LO64(x)
   mov      %%rExp,rdx                ;; expansion = HI64(x)

   add      %%rIdx,2                  ;; advance index
   sub      %%rLen,2                  ;; decrease length
   jge      %%mac_bnu_loop

   add      %%rLen,2                  ;; source size remainder
   jnz      %%mac_bn_short            ;; BNU of odd length

   shr      rdx,32                  ;; extract 32 bit expansion
   mov      [%%rDst+%%rIdx*4],%%rExp&d    ;; store
   jmp      %%mac_bn_quit

%%mac_bn_short:
   mov      eax,[%%rSrc+%%rIdx*4]       ;; a = pSrc[i]
   mul      %%rDgt                    ;; x = a*digit
   add      rax,%%rExp                ;;
   adc      rdx,0                   ;;
   mov      %%rDgt&d,[%%rDst+%%rIdx*4]    ;; x = expansion + pDst[i] + x
   add      rax,%%rDgt                ;;
   adc      rdx,0                   ;;
   mov      [%%rDst+%%rIdx*4],rax       ;; pDst[i]   = LO64(x)

%%mac_bn_quit:
   mov      %%rDgt&d,edx              ;; 32 bit expansion
%endmacro

;;
;; Multiply 64-bit operands and accumulate
;;
%macro MAC 5.nolist
  %xdefine %%hAcc %1
  %xdefine %%mAcc %2
  %xdefine %%lAcc %3
  %xdefine %%a %4
  %xdefine %%b %5

%ifnidn %%a,rax
   mov   rax,%%a
%endif
   mul   %%b
   %%add   %%lAcc,rax
   %%adc   %%mAcc,rdx
%ifnempty %%hAcc
   %%adc   %%hAcc,0
%endif
%endmacro

;;
;; Multiply 64-bit operands and accumulate twice
;;
%macro MAC2 5.nolist
  %xdefine %%hAcc %1
  %xdefine %%mAcc %2
  %xdefine %%lAcc %3
  %xdefine %%a %4
  %xdefine %%b %5

%ifnidn %%a,rax
   mov   rax,%%a
%endif
   mul   %%b
   %%add   %%lAcc,rax
   %%adc   %%mAcc,rdx
%ifnempty %%hAcc
   %%adc   %%hAcc,0
%endif
   %%add   %%lAcc,rax
   %%adc   %%mAcc,rdx
%ifnempty %%hAcc
   %%adc   %%hAcc,0
%endif
%endmacro

;;
;; SUB_FIX_BNU subtract fixed size BNUs
;;
;; input
;;    rVal     points src/dst BNU
;;    rSrc     points source BNU
;;    immLen   BNU size
;;
%macro SUB_FIX_BNU 4.nolist
  %xdefine %%rVal %1
  %xdefine %%rSrc %2
  %xdefine %%immLen %3
  %xdefine %%tmp %4

  %assign %%limit  %%immLen/2

  %assign %%i  0
  %rep %%limit
  mov   %%tmp,[%%rVal+%%i*4]
     %if %%i == 0
  sub   %%tmp,[%%rSrc+%%i*4]
     %else
  sbb   %%tmp,[%%rSrc+%%i*4]
     %endif
  mov   qword [%%rVal+%%i*4],%%tmp
  %assign %%i  %%i+2
  %endrep

  %if (%%immLen & 1) != 0
  mov   %%tmp&d,[%%rVal+%%immLen*4-4]
    %if %%i == 0
  sub   %%tmp&d,[%%rSrc+%%immLen*4-4]
    %else
  sbb   %%tmp&d,[%%rSrc+%%immLen*4-4]
    %endif
  mov   [%%rVal+%%immLen*4-4],%%tmp&d
  %endif
%endmacro

;;
;; ADD_FIX_BNU add fixed size BNUs
;;
;; input
;;    rVal     points src/dst BNU
;;    rSrc     points source BNU
;;    immLen   BNU size
;;
%macro ADD_FIX_BNU 4.nolist
  %xdefine %%rVal %1
  %xdefine %%rSrc %2
  %xdefine %%immLen %3
  %xdefine %%tmp %4

  %assign %%limit  %%immLen/2

  %assign %%i  0
  %rep %%limit
  mov   %%tmp,[%%rVal+%%i*4]
    %if %%i == 0
  add   %%tmp,[%%rSrc+%%i*4]
    %else
  adc   %%tmp,[%%rSrc+%%i*4]
    %endif
  mov   qword [%%rVal+%%i*4],%%tmp
  %assign %%i  %%i+2
  %endrep

  %if (%%immLen & 1) != 0
  mov   %%tmp&d,[%%rVal+%%immLen*4-4]
    %if %%i == 0
  add   %%tmp&d,[%%rSrc+%%immLen*4-4]
    %else
  adc   %%tmp&d,[%%rSrc+%%immLen*4-4]
    %endif
  mov   [%%rVal+%%immLen*4-4],%%tmp&d
  %endif
%endmacro

