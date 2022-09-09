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
;               Message block processing according to SHA1
;
;     Content:
;        UpdateSHA1
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA1_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP32E >= _IPP32E_M7) && (_IPP32E < _IPP32E_U8 )


;;
;; Magic functions defined in FIPS 180-1
;;
%macro MAGIC_F0 4-5.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regT %5

    mov  %%regF,%%regC
    xor  %%regF,%%regD
    and  %%regF,%%regB
    xor  %%regF,%%regD
%endmacro

%macro MAGIC_F1 4-5.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regT %5

    mov  %%regF,%%regD
    xor  %%regF,%%regC
    xor  %%regF,%%regB
%endmacro

%macro MAGIC_F2 5.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regT %5

    mov  %%regF,%%regB
    mov  %%regT,%%regB
    or   %%regF,%%regC
    and  %%regT,%%regC
    and  %%regF,%%regD
    or   %%regF,%%regT
%endmacro

%macro MAGIC_F3 4-5.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regT %5

    MAGIC_F1 {%%regF},{%%regB},{%%regC},{%%regD},{%%regT}
%endmacro

;;
;; single SHA1 step
;;
;;  Ipp32u tmp =  ROL(A,5) + MAGIC_Fi(B,C,D) + E + W[t] + CNT[i];
;;  E = D;
;;  D = C;
;;  C = ROL(B,30);
;;  B = A;
;;  A = tmp;
;;
%macro SHA1_STEP 10.nolist
  %xdefine %%regA %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regE %5
  %xdefine %%regT %6
  %xdefine %%regF %7
  %xdefine %%memW %8
  %xdefine %%immCNT %9
  %xdefine %%MAGIC %10

    add   %%regE,%%immCNT
    add   %%regE,[%%memW]
    mov   %%regT,%%regA
    rol   %%regT,5
    add   %%regE,%%regT
    %%MAGIC     %%regF,%%regB,%%regC,%%regD,%%regT      ;; FUN  = MAGIC_Fi(B,C,D)
    rol   %%regB,30
    add   %%regE,%%regF
%endmacro

%macro SHA1_RND0 8.nolist
  %xdefine %%regA %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regE %5
  %xdefine %%regT %6
  %xdefine %%regF %7
  %xdefine %%nr %8

  %assign %%immCNT  05A827999h
   mov      r13d,%%immCNT
   MAGIC_F0 %%regF,%%regB,%%regC,%%regD          ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[rsp+(((%%nr) & 0Fh)*4)]
;   lea      regE,[regE+regF+immCNT] ; substituted with 2 adds because of gnu as bug
   add      r13d, %%regF
   add      %%regE, r13d
   add      %%regE,%%regT
%endmacro

%macro SHA1_RND1 8.nolist
  %xdefine %%regA %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regE %5
  %xdefine %%regT %6
  %xdefine %%regF %7
  %xdefine %%nr %8

  %assign %%immCNT  06ED9EBA1h
   mov      r13d,%%immCNT
   MAGIC_F1 {%%regF},{%%regB},{%%regC},{%%regD}   ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[rsp+(((%%nr) & 0Fh)*4)]
;   lea      regE,[regE+regF+immCNT] ; substituted with 2 adds because of gnu as bug
   add      r13d, %%regF
   add      %%regE, r13d
   add      %%regE,%%regT
%endmacro

%macro SHA1_RND2 8.nolist
  %xdefine %%regA %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regE %5
  %xdefine %%regT %6
  %xdefine %%regF %7
  %xdefine %%nr %8

  %ifndef _VXWORKS
  %assign %%immCNT  08F1BBCDCh
  %else
  %assign %%immCNT  -1894007588
  %endif
   mov      r13d,%%immCNT
   MAGIC_F2 %%regF,%%regB,%%regC,%%regD,%%regT  ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[rsp+(((%%nr) & 0Fh)*4)]
;   lea      regE,[regE+regF+immCNT] ; substituted with 2 adds because of gnu as bug
   add      r13d, %%regF
   add      %%regE, r13d
   add      %%regE,%%regT
%endmacro

%macro SHA1_RND3 8.nolist
  %xdefine %%regA %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4
  %xdefine %%regE %5
  %xdefine %%regT %6
  %xdefine %%regF %7
  %xdefine %%nr %8

  %ifndef _VXWORKS
  %assign %%immCNT  0CA62C1D6h
  %else
  %assign %%immCNT  -899497514
  %endif
   mov      r13d,%%immCNT
   MAGIC_F3 {%%regF},{%%regB},{%%regC},{%%regD}       ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[rsp+(((%%nr) & 0Fh)*4)]
;   lea      regE,[regE+regF+immCNT] ; substituted with 2 adds because of gnu as bug
   add      r13d, %%regF
   add      %%regE, r13d
   add      %%regE,%%regT
%endmacro

;;
;; ENDIANNESS
;;
%macro ENDIANNESS 2.nolist
  %xdefine %%dst %1
  %xdefine %%src %2

  %ifnidn %%dst,%%src
    mov %%dst,%%src
  %endif
    bswap %%dst
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Following Macros are especially for new implementation of SHA1
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
%macro UPDATE 2-3.nolist
  %xdefine %%nr %1
  %xdefine %%regU %2
  %xdefine %%regT %3

   %ifempty %%regT
      mov      %%regU,[rsp+((%%nr-16) & 0Fh)*4]
      xor      %%regU,[rsp+((%%nr-14) & 0Fh)*4]
      xor      %%regU,[rsp+((%%nr-8)  & 0Fh)*4]
      xor      %%regU,[rsp+((%%nr-3)  & 0Fh)*4]
   %else
      mov      %%regU,[rsp+((%%nr-16) & 0Fh)*4]
      mov      %%regT,[rsp+((%%nr-14) & 0Fh)*4]
      xor      %%regU,%%regT
      mov      %%regT,[rsp+((%%nr-8)  & 0Fh)*4]
      xor      %%regU,%%regT
      mov      %%regT,[rsp+((%%nr-3)  & 0Fh)*4]
      xor      %%regU,%%regT
   %endif
   rol      %%regU,1
   mov      [rsp+(%%nr & 0Fh)*4],%%regU
%endmacro

segment .text align=IPP_ALIGN_FACTOR


;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA1(DigestSHA1 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;*****************************************************************************************

;;
;; Lib = M7
;;
;; Caller = ippsSHA1Update
;; Caller = ippsSHA1Final
;; Caller = ippsSHA1MessageDigest
;;
;; Caller = ippsHMACSHA1Update
;; Caller = ippsHMACSHA1Final
;; Caller = ippsHMACSHA1MessageDigest
;;

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA1,PUBLIC
%assign LOCAL_FRAME 16*4
        USES_GPR rbx,rsi,rdi,r8,r9,r10,r11,r12,r13
        USES_XMM
        COMP_ABI 4

%xdefine MBS_SHA1    (64)

   movsxd   rdx, edx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha1_block_loop:

;;
;; init A, B, C, D, E by the internal digest
;;
   mov      r8d, [rdi+0*4]       ; r8d = digest[0] (A)
   mov      r9d, [rdi+1*4]       ; r9d = digest[1] (B)
   mov      r10d,[rdi+2*4]       ; r10d= digest[2] (C)
   mov      r11d,[rdi+3*4]       ; r11d= digest[3] (D)
   mov      r12d,[rdi+4*4]       ; r12d= digest[4] (E)

;;
;; initialize the first 16 words in the array W (remember about endian)
;;
   xor      rcx,rcx
.loop1:
   mov      eax,[rsi+rcx*4+0*4]
   ENDIANNESS eax,eax
   mov      [rsp+rcx*4+0*4],eax

   mov      ebx,[rsi+rcx*4+1*4]
   ENDIANNESS ebx,ebx
   mov      [rsp+rcx*4+1*4],ebx

   add      rcx,2
   cmp      rcx,16
   jl       .loop1

;;
;; perform 0-79 steps
;;
;;             A,  B,  C,   D,   E,    TMP,FUN, round
;;             -----------------------------------
   SHA1_RND0   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  0
   UPDATE      16, eax
   SHA1_RND0   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  1
   UPDATE      17, eax
   SHA1_RND0   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  2
   UPDATE      18, eax
   SHA1_RND0   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  3
   UPDATE      19, eax
   SHA1_RND0   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  4
   UPDATE      20, eax
   SHA1_RND0   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  5
   UPDATE      21, eax
   SHA1_RND0   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  6
   UPDATE      22, eax
   SHA1_RND0   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  7
   UPDATE      23, eax
   SHA1_RND0   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  8
   UPDATE      24, eax
   SHA1_RND0   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  9
   UPDATE      25, eax
   SHA1_RND0   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  10
   UPDATE      26, eax
   SHA1_RND0   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  11
   UPDATE      27, eax
   SHA1_RND0   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  12
   UPDATE      28, eax
   SHA1_RND0   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  13
   UPDATE      29, eax
   SHA1_RND0   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  14
   UPDATE      30, eax
   SHA1_RND0   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  15
   UPDATE      31, eax
   SHA1_RND0   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  16
   UPDATE      32, eax
   SHA1_RND0   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  17
   UPDATE      33, eax
   SHA1_RND0   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  18
   UPDATE      34, eax
   SHA1_RND0   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  19
   UPDATE      35, eax

   SHA1_RND1   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  20
   UPDATE      36, eax
   SHA1_RND1   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  21
   UPDATE      37, eax
   SHA1_RND1   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  22
   UPDATE      38, eax
   SHA1_RND1   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  23
   UPDATE      39, eax
   SHA1_RND1   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  24
   UPDATE      40, eax
   SHA1_RND1   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  25
   UPDATE      41, eax
   SHA1_RND1   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  26
   UPDATE      42, eax
   SHA1_RND1   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  27
   UPDATE      43, eax
   SHA1_RND1   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  28
   UPDATE      44, eax
   SHA1_RND1   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  29
   UPDATE      45, eax
   SHA1_RND1   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  30
   UPDATE      46, eax
   SHA1_RND1   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  31
   UPDATE      47, eax
   SHA1_RND1   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  32
   UPDATE      48, eax
   SHA1_RND1   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  33
   UPDATE      49, eax
   SHA1_RND1   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  34
   UPDATE      50, eax
   SHA1_RND1   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  35
   UPDATE      51, eax
   SHA1_RND1   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  36
   UPDATE      52, eax
   SHA1_RND1   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  37
   UPDATE      53, eax
   SHA1_RND1   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  38
   UPDATE      54, eax
   SHA1_RND1   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  39
   UPDATE      55, eax

   SHA1_RND2   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  40
   UPDATE      56, eax
   SHA1_RND2   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  41
   UPDATE      57, eax
   SHA1_RND2   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  42
   UPDATE      58, eax
   SHA1_RND2   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  43
   UPDATE      59, eax
   SHA1_RND2   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  44
   UPDATE      60, eax
   SHA1_RND2   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  45
   UPDATE      61, eax
   SHA1_RND2   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  46
   UPDATE      62, eax
   SHA1_RND2   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  47
   UPDATE      63, eax
   SHA1_RND2   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  48
   UPDATE      64, eax
   SHA1_RND2   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  49
   UPDATE      65, eax
   SHA1_RND2   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  50
   UPDATE      66, eax
   SHA1_RND2   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  51
   UPDATE      67, eax
   SHA1_RND2   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  52
   UPDATE      68, eax
   SHA1_RND2   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  53
   UPDATE      69, eax
   SHA1_RND2   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  54
   UPDATE      70, eax
   SHA1_RND2   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  55
   UPDATE      71, eax
   SHA1_RND2   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  56
   UPDATE      72, eax
   SHA1_RND2   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  57
   UPDATE      73, eax
   SHA1_RND2   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  58
   UPDATE      74, eax
   SHA1_RND2   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  59
   UPDATE      75, eax

   SHA1_RND3   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  60
   UPDATE      76, eax
   SHA1_RND3   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  61
   UPDATE      77, eax
   SHA1_RND3   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  62
   UPDATE      78, eax
   SHA1_RND3   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  63
   UPDATE      79, eax
   SHA1_RND3   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  64
   SHA1_RND3   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  65
   SHA1_RND3   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  66
   SHA1_RND3   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  67
   SHA1_RND3   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  68
   SHA1_RND3   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  69
   SHA1_RND3   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  70
   SHA1_RND3   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  71
   SHA1_RND3   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  72
   SHA1_RND3   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  73
   SHA1_RND3   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  74
   SHA1_RND3   r8d,r9d,r10d,r11d,r12d, ecx,ebx,  75
   SHA1_RND3   r12d,r8d,r9d,r10d,r11d, ecx,ebx,  76
   SHA1_RND3   r11d,r12d,r8d,r9d,r10d, ecx,ebx,  77
   SHA1_RND3   r10d,r11d,r12d,r8d,r9d, ecx,ebx,  78
   SHA1_RND3   r9d,r10d,r11d,r12d,r8d, ecx,ebx,  79

;;
;; update digest
;;
   add      [rdi+0*4],r8d     ; advance digest
   add      [rdi+1*4],r9d
   add      [rdi+2*4],r10d
   add      [rdi+3*4],r11d
   add      [rdi+4*4],r12d

   add      rsi, MBS_SHA1
   sub      rdx, MBS_SHA1
   jg       .sha1_block_loop

   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA1

%endif    ;; (_IPP32E >= _IPP32E_M7) AND (_IPP32E < _IPP32E_U8 )
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA1_

