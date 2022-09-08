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
;               Message block processing according to SHA1
;
;     Content:
;        UpdateSHA1
;
;





%include "asmdefs.inc"
%include "ia_emm.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA1_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP >= _IPP_M5)

;;
;; Magic functions defined in FIPS 180-1
;;
%macro MAGIC_F0 4.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4

    mov  %%regF,%%regC
    xor  %%regF,%%regD
    and  %%regF,%%regB
    xor  %%regF,%%regD
%endmacro


%macro MAGIC_F1 4.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4

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


%macro MAGIC_F3 4.nolist
  %xdefine %%regF %1
  %xdefine %%regB %2
  %xdefine %%regC %3
  %xdefine %%regD %4

    MAGIC_F1 %%regF,%%regB,%%regC,%%regD
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
    %%MAGIC   %%regF,%%regB,%%regC,%%regD,%%regT          ;; FUN  = MAGIC_Fi(B,C,D)
    rol   %%regB,30
    add   %%regE,%%regF
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
      mov      %%regU,[esp+((%%nr-16)&0Fh)*4]
      xor      %%regU,[esp+((%%nr-14)&0Fh)*4]
      xor      %%regU,[esp+((%%nr-8) &0Fh)*4]
      xor      %%regU,[esp+((%%nr-3) &0Fh)*4]
   %else
      mov      %%regU,[esp+((%%nr-16)&0Fh)*4]
      mov      %%regT,[esp+((%%nr-14)&0Fh)*4]
      xor      %%regU,%%regT
      mov      %%regT,[esp+((%%nr-8) &0Fh)*4]
      xor      %%regU,%%regT
      mov      %%regT,[esp+((%%nr-3) &0Fh)*4]
      xor      %%regU,%%regT
   %endif
   rol      %%regU,1
   mov      [esp+(%%nr&0Fh)*4],%%regU
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

   MAGIC_F0 %%regF,%%regB,%%regC,%%regD          ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[esp+(((%%nr) & 0Fh)*4)]
   lea      %%regE,[%%regE+%%regF+%%immCNT]
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

   MAGIC_F1 %%regF,%%regB,%%regC,%%regD   ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[esp+(((%%nr)&0Fh)*4)]
   lea      %%regE,[%%regE+%%regF+%%immCNT]
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

  %assign %%immCNT  08F1BBCDCh

   MAGIC_F2 %%regF,%%regB,%%regC,%%regD,%%regT  ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[esp+(((%%nr)&0Fh)*4)]
   lea      %%regE,[%%regE+%%regF+%%immCNT]
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

  %assign %%immCNT  0CA62C1D6h

   MAGIC_F3 %%regF,%%regB,%%regC,%%regD       ;; FUN  = MAGIC_Fi(B,C,D)
   ror      %%regB,(32-30)
   mov      %%regT,%%regA
   rol      %%regT,5
   add      %%regE,[esp+(((%%nr)&0Fh)*4)]
   lea      %%regE,[%%regE+%%regF+%%immCNT]
   add      %%regE,%%regT
%endmacro



segment .text align=IPP_ALIGN_FACTOR


;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA1(DigestSHA1 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;*****************************************************************************************

;;
;; Lib = W7,V8
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
  USES_GPR esi,edi,ebx,ebp

%xdefine digest [esp + ARG_1 + 0*sizeof(dword)] ; hash address
%xdefine mblk   [esp + ARG_1 + 1*sizeof(dword)] ; buffer address
%xdefine mlen   [esp + ARG_1 + 2*sizeof(dword)] ; buffer length
%xdefine pTable [esp + ARG_1 + 3*sizeof(dword)] ; pointer to SHA1 const (dummy)

%xdefine MBS_SHA1    (64)

%assign stackSize  (16+3)               ; stack size
   mov      eax, pTable          ; dummy
   mov      esi,mblk             ; source data address
   mov      eax,mlen             ; data length
   mov      edi,digest           ; hash address

   sub      esp,stackSize*4      ; allocate local buffer
   mov      [esp+stackSize*4-3*4],edi ; save hash address

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha1_block_loop:
   mov      [esp+stackSize*4-2*4],esi ; save data address
   mov      [esp+stackSize*4-1*4],eax ; save data length

;;
;; initialize the first 16 words in the array W (remember about endian)
;;
   xor      ecx,ecx
.loop1:
   mov      eax,[esi+ecx*4+0*4]
   mov      edx,[esi+ecx*4+1*4]
   ENDIANNESS eax,eax
   ENDIANNESS edx,edx
   mov      [esp+ecx*4+0*4],eax
   mov      [esp+ecx*4+1*4],edx
   add      ecx,2
   cmp      ecx,16
   jl       .loop1

;;
;; init A, B, C, D, E by the internal digest
;;
   mov      eax,[edi+0*4]        ; r2 = digest[0] (A)
   mov      ebx,[edi+1*4]        ; r3 = digest[1] (B)
   mov      ecx,[edi+2*4]        ; r4 = digest[2] (C)
   mov      edx,[edi+3*4]        ; r5 = digest[3] (D)
   mov      ebp,[edi+4*4]        ; r6 = digest[4] (E)

;;
;; perform 0-79 steps
;;
;;             A,  B,  C,  D,  E,   TMP,FUN, round
;;             -----------------------------------
   SHA1_RND0   eax,ebx,ecx,edx,ebp, esi,edi,  0
   UPDATE      16, esi
   SHA1_RND0   ebp,eax,ebx,ecx,edx, esi,edi,  1
   UPDATE      17, esi
   SHA1_RND0   edx,ebp,eax,ebx,ecx, esi,edi,  2
   UPDATE      18, esi
   SHA1_RND0   ecx,edx,ebp,eax,ebx, esi,edi,  3
   UPDATE      19, esi
   SHA1_RND0   ebx,ecx,edx,ebp,eax, esi,edi,  4
   UPDATE      20, esi
   SHA1_RND0   eax,ebx,ecx,edx,ebp, esi,edi,  5
   UPDATE      21, esi
   SHA1_RND0   ebp,eax,ebx,ecx,edx, esi,edi,  6
   UPDATE      22, esi
   SHA1_RND0   edx,ebp,eax,ebx,ecx, esi,edi,  7
   UPDATE      23, esi
   SHA1_RND0   ecx,edx,ebp,eax,ebx, esi,edi,  8
   UPDATE      24, esi
   SHA1_RND0   ebx,ecx,edx,ebp,eax, esi,edi,  9
   UPDATE      25, esi
   SHA1_RND0   eax,ebx,ecx,edx,ebp, esi,edi, 10
   UPDATE      26, esi
   SHA1_RND0   ebp,eax,ebx,ecx,edx, esi,edi, 11
   UPDATE      27, esi
   SHA1_RND0   edx,ebp,eax,ebx,ecx, esi,edi, 12
   UPDATE      28, esi
   SHA1_RND0   ecx,edx,ebp,eax,ebx, esi,edi, 13
   UPDATE      29, esi
   SHA1_RND0   ebx,ecx,edx,ebp,eax, esi,edi, 14
   UPDATE      30, esi
   SHA1_RND0   eax,ebx,ecx,edx,ebp, esi,edi, 15
   UPDATE      31, esi
   SHA1_RND0   ebp,eax,ebx,ecx,edx, esi,edi, 16
   UPDATE      32, esi
   SHA1_RND0   edx,ebp,eax,ebx,ecx, esi,edi, 17
   UPDATE      33, esi
   SHA1_RND0   ecx,edx,ebp,eax,ebx, esi,edi, 18
   UPDATE      34, esi
   SHA1_RND0   ebx,ecx,edx,ebp,eax, esi,edi, 19
   UPDATE      35, esi

   SHA1_RND1   eax,ebx,ecx,edx,ebp, esi,edi, 20
   UPDATE      36, esi
   SHA1_RND1   ebp,eax,ebx,ecx,edx, esi,edi, 21
   UPDATE      37, esi
   SHA1_RND1   edx,ebp,eax,ebx,ecx, esi,edi, 22
   UPDATE      38, esi
   SHA1_RND1   ecx,edx,ebp,eax,ebx, esi,edi, 23
   UPDATE      39, esi
   SHA1_RND1   ebx,ecx,edx,ebp,eax, esi,edi, 24
   UPDATE      40, esi
   SHA1_RND1   eax,ebx,ecx,edx,ebp, esi,edi, 25
   UPDATE      41, esi
   SHA1_RND1   ebp,eax,ebx,ecx,edx, esi,edi, 26
   UPDATE      42, esi
   SHA1_RND1   edx,ebp,eax,ebx,ecx, esi,edi, 27
   UPDATE      43, esi
   SHA1_RND1   ecx,edx,ebp,eax,ebx, esi,edi, 28
   UPDATE      44, esi
   SHA1_RND1   ebx,ecx,edx,ebp,eax, esi,edi, 29
   UPDATE      45, esi
   SHA1_RND1   eax,ebx,ecx,edx,ebp, esi,edi, 30
   UPDATE      46, esi
   SHA1_RND1   ebp,eax,ebx,ecx,edx, esi,edi, 31
   UPDATE      47, esi
   SHA1_RND1   edx,ebp,eax,ebx,ecx, esi,edi, 32
   UPDATE      48, esi
   SHA1_RND1   ecx,edx,ebp,eax,ebx, esi,edi, 33
   UPDATE      49, esi
   SHA1_RND1   ebx,ecx,edx,ebp,eax, esi,edi, 34
   UPDATE      50, esi
   SHA1_RND1   eax,ebx,ecx,edx,ebp, esi,edi, 35
   UPDATE      51, esi
   SHA1_RND1   ebp,eax,ebx,ecx,edx, esi,edi, 36
   UPDATE      52, esi
   SHA1_RND1   edx,ebp,eax,ebx,ecx, esi,edi, 37
   UPDATE      53, esi
   SHA1_RND1   ecx,edx,ebp,eax,ebx, esi,edi, 38
   UPDATE      54, esi
   SHA1_RND1   ebx,ecx,edx,ebp,eax, esi,edi, 39
   UPDATE      55, esi

   SHA1_RND2   eax,ebx,ecx,edx,ebp, esi,edi, 40
   UPDATE      56, esi
   SHA1_RND2   ebp,eax,ebx,ecx,edx, esi,edi, 41
   UPDATE      57, esi
   SHA1_RND2   edx,ebp,eax,ebx,ecx, esi,edi, 42
   UPDATE      58, esi
   SHA1_RND2   ecx,edx,ebp,eax,ebx, esi,edi, 43
   UPDATE      59, esi
   SHA1_RND2   ebx,ecx,edx,ebp,eax, esi,edi, 44
   UPDATE      60, esi
   SHA1_RND2   eax,ebx,ecx,edx,ebp, esi,edi, 45
   UPDATE      61, esi
   SHA1_RND2   ebp,eax,ebx,ecx,edx, esi,edi, 46
   UPDATE      62, esi
   SHA1_RND2   edx,ebp,eax,ebx,ecx, esi,edi, 47
   UPDATE      63, esi
   SHA1_RND2   ecx,edx,ebp,eax,ebx, esi,edi, 48
   UPDATE      64, esi
   SHA1_RND2   ebx,ecx,edx,ebp,eax, esi,edi, 49
   UPDATE      65, esi
   SHA1_RND2   eax,ebx,ecx,edx,ebp, esi,edi, 50
   UPDATE      66, esi
   SHA1_RND2   ebp,eax,ebx,ecx,edx, esi,edi, 51
   UPDATE      67, esi
   SHA1_RND2   edx,ebp,eax,ebx,ecx, esi,edi, 52
   UPDATE      68, esi
   SHA1_RND2   ecx,edx,ebp,eax,ebx, esi,edi, 53
   UPDATE      69, esi
   SHA1_RND2   ebx,ecx,edx,ebp,eax, esi,edi, 54
   UPDATE      70, esi
   SHA1_RND2   eax,ebx,ecx,edx,ebp, esi,edi, 55
   UPDATE      71, esi
   SHA1_RND2   ebp,eax,ebx,ecx,edx, esi,edi, 56
   UPDATE      72, esi
   SHA1_RND2   edx,ebp,eax,ebx,ecx, esi,edi, 57
   UPDATE      73, esi
   SHA1_RND2   ecx,edx,ebp,eax,ebx, esi,edi, 58
   UPDATE      74, esi
   SHA1_RND2   ebx,ecx,edx,ebp,eax, esi,edi, 59
   UPDATE      75, esi

   SHA1_RND3   eax,ebx,ecx,edx,ebp, esi,edi, 60
   UPDATE      76, esi
   SHA1_RND3   ebp,eax,ebx,ecx,edx, esi,edi, 61
   UPDATE      77, esi
   SHA1_RND3   edx,ebp,eax,ebx,ecx, esi,edi, 62
   UPDATE      78, esi
   SHA1_RND3   ecx,edx,ebp,eax,ebx, esi,edi, 63
   UPDATE      79, esi
   SHA1_RND3   ebx,ecx,edx,ebp,eax, esi,edi, 64
   SHA1_RND3   eax,ebx,ecx,edx,ebp, esi,edi, 65
   SHA1_RND3   ebp,eax,ebx,ecx,edx, esi,edi, 66
   SHA1_RND3   edx,ebp,eax,ebx,ecx, esi,edi, 67
   SHA1_RND3   ecx,edx,ebp,eax,ebx, esi,edi, 68
   SHA1_RND3   ebx,ecx,edx,ebp,eax, esi,edi, 69
   SHA1_RND3   eax,ebx,ecx,edx,ebp, esi,edi, 70
   SHA1_RND3   ebp,eax,ebx,ecx,edx, esi,edi, 71
   SHA1_RND3   edx,ebp,eax,ebx,ecx, esi,edi, 72
   SHA1_RND3   ecx,edx,ebp,eax,ebx, esi,edi, 73
   SHA1_RND3   ebx,ecx,edx,ebp,eax, esi,edi, 74
   SHA1_RND3   eax,ebx,ecx,edx,ebp, esi,edi, 75
   SHA1_RND3   ebp,eax,ebx,ecx,edx, esi,edi, 76
   SHA1_RND3   edx,ebp,eax,ebx,ecx, esi,edi, 77
   SHA1_RND3   ecx,edx,ebp,eax,ebx, esi,edi, 78
   SHA1_RND3   ebx,ecx,edx,ebp,eax, esi,edi, 79

;;
;; update digest
;;
   mov      edi,[esp+stackSize*4-3*4]  ; restore hash address
   mov      esi,[esp+stackSize*4-2*4]  ; restore data address

   add      [edi+0*4],eax     ; advance digest
   mov      eax,[esp+stackSize*4-1*4]  ; restore data length
   add      [edi+1*4],ebx
   add      [edi+2*4],ecx
   add      [edi+3*4],edx
   add      [edi+4*4],ebp

   add      esi, MBS_SHA1
   sub      eax, MBS_SHA1
   jg       .sha1_block_loop

   add      esp,stackSize*4   ; remove local buffer
   REST_GPR
   ret
ENDFUNC UpdateSHA1

%endif    ;; _IPP >= _IPP_M5
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA1_

