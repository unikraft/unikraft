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
;               Message block processing according to SHA256
;
;     Content:
;        UpdateSHA256
;
;






%include "asmdefs.inc"
%include "ia_emm.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA256_)
%if (_SHA_NI_ENABLING_ == _FEATURE_OFF_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
%if (_IPP >= _IPP_M5) && (_IPP < _IPP_V8)

;;
;; SIG0(x) = ROR32(x, 7) ^ ROR32(x,18) ^ LSR(x, 3)
;; SIG1(x) = ROR32(x,17) ^ ROR32(x,19) ^ LSR(x,10)
;; W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16], i=16,..,63
;;
%macro UPDATE 2.nolist
  %xdefine %%nr %1
  %xdefine %%wBuff %2

   mov      ebx,[%%wBuff+((%%nr-2) &0Fh)*sizeof(dword)]   ; W[i- 2]
   ror      ebx, 10
   mov      ecx,[%%wBuff+((%%nr-15)&0Fh)*sizeof(dword)]   ; W[i-15]
   ror      ecx, 3
   mov      esi,[%%wBuff+((%%nr-7) &0Fh)*sizeof(dword)]   ; +W[i- 7]
   add      esi,[%%wBuff+((%%nr-16)&0Fh)*sizeof(dword)]   ; +W[i-16]

   mov      edi,003FFFFFh  ; SIG1()
   and      edi,ebx
   ror      ebx,(17-10)
   xor      edi,ebx
   ror      ebx,(19-17)
   xor      edi,ebx

   add      edi, esi       ; SIG0() +W[i-7] +W[i-16]

   mov      esi,1FFFFFFFh  ; SIG0()
   and      esi,ecx
   ror      ecx,(7-3)
   xor      esi,ecx
   ror      ecx,(18-7)
   xor      esi,ecx

   add      edi,esi        ; SIG0() +W[i-7] +W[i-16] + SIG1()
   mov      [%%wBuff+(%%nr&0Fh)*sizeof(dword)],edi
%endmacro


;
; SUM1(x) = ROR32(x, 6) ^ ROR32(x,11) ^ ROR32(x,25)
; SUM0(x) = ROR32(x, 2) ^ ROR32(x,13) ^ ROR32(x,22)
;
; CH(x,y,x)  = (x & y) ^ (~x & z)
; MAJ(x,y,z) = (x & y) ^ (x & z) ^ (y & z) = (x&y)^((x^y)&z)
;
; T1 = SUM1(E) +CH(E,F,G) +K[i] +W[i] + H
; T2 = SUM0(A) +MAJ(A,B,C)
;
; D += T1
; H  = T1+T2
;
; eax = E
; edx = A
;
%macro SHA256_STEP 4.nolist
  %xdefine %%nr %1
  %xdefine %%hashBuff %2
  %xdefine %%wBuff %3
  %xdefine %%immCnt %4

   mov      esi,[%%hashBuff+((vF-%%nr)&7)*sizeof(dword)] ; init CH()
   mov      ecx,eax
   mov      edi,[%%hashBuff+((vG-%%nr)&7)*sizeof(dword)] ; init CH()
   mov      ebx,eax           ; SUM1()
   ror      eax,6             ; SUM1()
   not      ecx               ; CH()
   and      esi,ebx           ; CH()
   ror      ebx,11            ; SUM1()
   and      edi,ecx           ; CH()
   xor      eax,ebx           ; SUM1()
   ror      ebx,(25-11)       ; SUM1()
   xor      esi,edi           ; CH()
   xor      eax,ebx           ; SUM1()
   add      eax,esi           ; T1 = SUM1() +CH()
   add      eax,[%%hashBuff+((vH-%%nr)&7)*sizeof(dword)] ; T1 += h
;; add      eax,[wBuff+(nr&0Fh)*sizeof(dword)]       ; T1 += w[]
   add      eax,[%%wBuff]                                  ; T1 += w[]
   add      eax, %%immCnt       ; T1 += K[]

   mov      esi,[%%hashBuff+((vB-%%nr)&7)*sizeof(dword)] ; init MAJ()
   mov      ecx,edx
   mov      edi,[%%hashBuff+((vC-%%nr)&7)*sizeof(dword)] ; init MAJ()
   mov      ebx,edx           ; SUM0()
   and      ecx,esi           ; MAJ()
   ror      edx,2             ; SUM0()
   xor      esi,ebx           ; MAJ()
   ror      ebx,13            ; SUM0()
   and      esi,edi           ; MAJ()
   xor      edx,ebx           ; SUM0()
   xor      esi,ecx           ; MAJ()
   ror      ebx,(22-13)       ; SUM0()
   xor      edx,ebx           ; SUM0()
   add      edx,esi           ; T2 = SUM0()+MAJ()

   add      edx,eax                             ; T2+T1
   add      eax,[%%hashBuff+((vD-%%nr)&7)*sizeof(dword)] ; T1+d

   mov      [%%hashBuff+((vH-%%nr)&7)*sizeof(dword)],edx
   mov      [%%hashBuff+((vD-%%nr)&7)*sizeof(dword)],eax
%endmacro



segment .text align=IPP_ALIGN_FACTOR



;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA256(DigestSHA256 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;*****************************************************************************************

;;
;; Lib = W7
;;
;; Caller = ippsSHA256Update
;; Caller = ippsSHA256Final
;; Caller = ippsSHA256MessageDigest
;;
;; Caller = ippsSHA224Update
;; Caller = ippsSHA224Final
;; Caller = ippsSHA224MessageDigest
;;
;; Caller = ippsHMACSHA256Update
;; Caller = ippsHMACSHA256Final
;; Caller = ippsHMACSHA256MessageDigest
;;
;; Caller = ippsHMACSHA224Update
;; Caller = ippsHMACSHA224Final
;; Caller = ippsHMACSHA224MessageDigest
;;

align IPP_ALIGN_FACTOR
IPPASM UpdateSHA256,PUBLIC
  USES_GPR esi,edi,ebx,ebp

%xdefine pHash [esp + ARG_1 + 0*sizeof(dword)] ; pointer to the input/output hash
%xdefine pSrc  [esp + ARG_1 + 1*sizeof(dword)] ; input message
%xdefine srcL  [esp + ARG_1 + 2*sizeof(dword)] ; message length
%xdefine pParm [esp + ARG_1 + 3*sizeof(dword)] ; dummy parameter

%xdefine MBS_SHA256 (64)                        ; SHA256 block data size

%assign  dSize      8                           ; size of digest (dwords)
%assign  wSize      16                          ; W values queue (dwords)

%assign  hashOffset 0                           ; hash address
%assign  msgOffset  hashOffset+sizeof(dword)    ; message address offset
%assign  lenOffset  msgOffset+sizeof(dword)     ; message length offset
%assign  dOffset    lenOffset+sizeof(dword)     ; hash buffer offset
%assign  wOffset    dOffset+dSize*sizeof(dword) ; W values offset

%assign  stackSize  3*sizeof(dword)             + (dSize+wSize)*sizeof(dword)

%assign vA  0
%assign vB  1
%assign vC  2
%assign vD  3
%assign vE  4
%assign vF  5
%assign vG  6
%assign vH  7
   mov      eax, pParm           ; dummy
   mov      edi, pHash           ; hash address
   mov      esi, pSrc            ; source data address
   mov      eax, srcL            ; source data length

   sub      esp, stackSize       ; allocate local buffers
   mov      [esp+hashOffset], edi; save hash address

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha256_block_loop:
   mov      [esp+msgOffset], esi ; save message address
   mov      [esp+lenOffset], eax ; save message length

;;
;; copy input digest
;;
   mov      eax, [edi+0*sizeof(dword)]
   mov      ebx, [edi+1*sizeof(dword)]
   mov      ecx, [edi+2*sizeof(dword)]
   mov      edx, [edi+3*sizeof(dword)]
   mov      [esp+dOffset+vA*sizeof(dword)], eax
   mov      [esp+dOffset+vB*sizeof(dword)], ebx
   mov      [esp+dOffset+vC*sizeof(dword)], ecx
   mov      [esp+dOffset+vD*sizeof(dword)], edx
   mov      eax, [edi+4*sizeof(dword)]
   mov      ebx, [edi+5*sizeof(dword)]
   mov      ecx, [edi+6*sizeof(dword)]
   mov      edx, [edi+7*sizeof(dword)]
   mov      [esp+dOffset+vE*sizeof(dword)], eax
   mov      [esp+dOffset+vF*sizeof(dword)], ebx
   mov      [esp+dOffset+vG*sizeof(dword)], ecx
   mov      [esp+dOffset+vH*sizeof(dword)], edx

;;
;; initialize the first 16 qwords of W array
;; - remember about endian
;;
   xor      ecx, ecx
.loop1:
   mov      eax, [esi+ecx*sizeof(dword)]
   bswap    eax
   mov      ebx, [esi+ecx*sizeof(dword)+sizeof(dword)]
   bswap    ebx

   mov      [esp+wOffset+ecx*sizeof(dword)], eax
   mov      [esp+wOffset+ecx*sizeof(dword)+sizeof(dword)], ebx

   add      ecx, 2
   cmp      ecx, 16
   jl       .loop1

;;
;; perform 0-64 steps
;;
   mov      eax, [esp+dOffset+vE*sizeof(dword)]
   mov      edx, [esp+dOffset+vA*sizeof(dword)]


   SHA256_STEP  0, {esp+dOffset}, {esp+wOffset+ 0*sizeof(dword)}, 0428A2F98h
   UPDATE      16, {esp+wOffset}
   SHA256_STEP  1, {esp+dOffset}, {esp+wOffset+ 1*sizeof(dword)}, 071374491h
   UPDATE      17, {esp+wOffset}
   SHA256_STEP  2, {esp+dOffset}, {esp+wOffset+ 2*sizeof(dword)}, 0B5C0FBCFh
   UPDATE      18, {esp+wOffset}
   SHA256_STEP  3, {esp+dOffset}, {esp+wOffset+ 3*sizeof(dword)}, 0E9B5DBA5h
   UPDATE      19, {esp+wOffset}
   SHA256_STEP  4, {esp+dOffset}, {esp+wOffset+ 4*sizeof(dword)}, 03956C25Bh
   UPDATE      20, {esp+wOffset}
   SHA256_STEP  5, {esp+dOffset}, {esp+wOffset+ 5*sizeof(dword)}, 059F111F1h
   UPDATE      21, {esp+wOffset}
   SHA256_STEP  6, {esp+dOffset}, {esp+wOffset+ 6*sizeof(dword)}, 0923F82A4h
   UPDATE      22, {esp+wOffset}
   SHA256_STEP  7, {esp+dOffset}, {esp+wOffset+ 7*sizeof(dword)}, 0AB1C5ED5h
   UPDATE      23, {esp+wOffset}
   SHA256_STEP  8, {esp+dOffset}, {esp+wOffset+ 8*sizeof(dword)}, 0D807AA98h
   UPDATE      24, {esp+wOffset}
   SHA256_STEP  9, {esp+dOffset}, {esp+wOffset+ 9*sizeof(dword)}, 012835B01h
   UPDATE      25, {esp+wOffset}
   SHA256_STEP 10,{esp+dOffset}, {esp+wOffset+10*sizeof(dword)}, 0243185BEh
   UPDATE      26, {esp+wOffset}
   SHA256_STEP 11, {esp+dOffset}, {esp+wOffset+11*sizeof(dword)}, 0550C7DC3h
   UPDATE      27, {esp+wOffset}
   SHA256_STEP 12, {esp+dOffset}, {esp+wOffset+12*sizeof(dword)}, 072BE5D74h
   UPDATE      28, {esp+wOffset}
   SHA256_STEP 13, {esp+dOffset}, {esp+wOffset+13*sizeof(dword)}, 080DEB1FEh
   UPDATE      29, {esp+wOffset}
   SHA256_STEP 14, {esp+dOffset}, {esp+wOffset+14*sizeof(dword)}, 09BDC06A7h
   UPDATE      30, {esp+wOffset}
   SHA256_STEP 15, {esp+dOffset}, {esp+wOffset+15*sizeof(dword)}, 0C19BF174h
   UPDATE      31, {esp+wOffset}

   SHA256_STEP 16, {esp+dOffset}, {esp+wOffset+ 0*sizeof(dword)}, 0E49B69C1h
   UPDATE      32, {esp+wOffset}
   SHA256_STEP 17, {esp+dOffset}, {esp+wOffset+ 1*sizeof(dword)}, 0EFBE4786h
   UPDATE      33, {esp+wOffset}
   SHA256_STEP 18, {esp+dOffset}, {esp+wOffset+ 2*sizeof(dword)}, 00FC19DC6h
   UPDATE      34, {esp+wOffset}
   SHA256_STEP 19, {esp+dOffset}, {esp+wOffset+ 3*sizeof(dword)}, 0240CA1CCh
   UPDATE      35, {esp+wOffset}
   SHA256_STEP 20, {esp+dOffset}, {esp+wOffset+ 4*sizeof(dword)}, 02DE92C6Fh
   UPDATE      36, {esp+wOffset}
   SHA256_STEP 21, {esp+dOffset}, {esp+wOffset+ 5*sizeof(dword)}, 04A7484AAh
   UPDATE      37, {esp+wOffset}
   SHA256_STEP 22, {esp+dOffset}, {esp+wOffset+ 6*sizeof(dword)}, 05CB0A9DCh
   UPDATE      38, {esp+wOffset}
   SHA256_STEP 23, {esp+dOffset}, {esp+wOffset+ 7*sizeof(dword)}, 076F988DAh
   UPDATE      39, {esp+wOffset}
   SHA256_STEP 24, {esp+dOffset}, {esp+wOffset+ 8*sizeof(dword)}, 0983E5152h
   UPDATE      40, {esp+wOffset}
   SHA256_STEP 25, {esp+dOffset}, {esp+wOffset+ 9*sizeof(dword)}, 0A831C66Dh
   UPDATE      41, {esp+wOffset}
   SHA256_STEP 26, {esp+dOffset}, {esp+wOffset+10*sizeof(dword)}, 0B00327C8h
   UPDATE      42, {esp+wOffset}
   SHA256_STEP 27, {esp+dOffset}, {esp+wOffset+11*sizeof(dword)}, 0BF597FC7h
   UPDATE      43, {esp+wOffset}
   SHA256_STEP 28, {esp+dOffset}, {esp+wOffset+12*sizeof(dword)}, 0C6E00BF3h
   UPDATE      44, {esp+wOffset}
   SHA256_STEP 29, {esp+dOffset}, {esp+wOffset+13*sizeof(dword)}, 0D5A79147h
   UPDATE      45, {esp+wOffset}
   SHA256_STEP 30, {esp+dOffset}, {esp+wOffset+14*sizeof(dword)}, 006CA6351h
   UPDATE      46, {esp+wOffset}
   SHA256_STEP 31, {esp+dOffset}, {esp+wOffset+15*sizeof(dword)}, 014292967h
   UPDATE      47, {esp+wOffset}

   SHA256_STEP 32, {esp+dOffset}, {esp+wOffset+ 0*sizeof(dword)}, 027B70A85h
   UPDATE      48, {esp+wOffset}
   SHA256_STEP 33, {esp+dOffset}, {esp+wOffset+ 1*sizeof(dword)}, 02E1B2138h
   UPDATE      49, {esp+wOffset}
   SHA256_STEP 34, {esp+dOffset}, {esp+wOffset+ 2*sizeof(dword)}, 04D2C6DFCh
   UPDATE      50, {esp+wOffset}
   SHA256_STEP 35, {esp+dOffset}, {esp+wOffset+ 3*sizeof(dword)}, 053380D13h
   UPDATE      51, {esp+wOffset}
   SHA256_STEP 36, {esp+dOffset}, {esp+wOffset+ 4*sizeof(dword)}, 0650A7354h
   UPDATE      52, {esp+wOffset}
   SHA256_STEP 37, {esp+dOffset}, {esp+wOffset+ 5*sizeof(dword)}, 0766A0ABBh
   UPDATE      53, {esp+wOffset}
   SHA256_STEP 38, {esp+dOffset}, {esp+wOffset+ 6*sizeof(dword)}, 081C2C92Eh
   UPDATE      54, {esp+wOffset}
   SHA256_STEP 39, {esp+dOffset}, {esp+wOffset+ 7*sizeof(dword)}, 092722C85h
   UPDATE      55, {esp+wOffset}
   SHA256_STEP 40, {esp+dOffset}, {esp+wOffset+ 8*sizeof(dword)}, 0A2BFE8A1h
   UPDATE      56, {esp+wOffset}
   SHA256_STEP 41, {esp+dOffset}, {esp+wOffset+ 9*sizeof(dword)}, 0A81A664Bh
   UPDATE      57, {esp+wOffset}
   SHA256_STEP 42, {esp+dOffset}, {esp+wOffset+10*sizeof(dword)}, 0C24B8B70h
   UPDATE      58, {esp+wOffset}
   SHA256_STEP 43, {esp+dOffset}, {esp+wOffset+11*sizeof(dword)}, 0C76C51A3h
   UPDATE      59, {esp+wOffset}
   SHA256_STEP 44, {esp+dOffset}, {esp+wOffset+12*sizeof(dword)}, 0D192E819h
   UPDATE      60, {esp+wOffset}
   SHA256_STEP 45, {esp+dOffset}, {esp+wOffset+13*sizeof(dword)}, 0D6990624h
   UPDATE      61, {esp+wOffset}
   SHA256_STEP 46, {esp+dOffset}, {esp+wOffset+14*sizeof(dword)}, 0F40E3585h
   UPDATE      62, {esp+wOffset}
   SHA256_STEP 47, {esp+dOffset}, {esp+wOffset+15*sizeof(dword)}, 0106AA070h
   UPDATE      63, {esp+wOffset}

   SHA256_STEP 48, {esp+dOffset}, {esp+wOffset+ 0*sizeof(dword)}, 019A4C116h
   SHA256_STEP 49, {esp+dOffset}, {esp+wOffset+ 1*sizeof(dword)}, 01E376C08h
   SHA256_STEP 50, {esp+dOffset}, {esp+wOffset+ 2*sizeof(dword)}, 02748774Ch
   SHA256_STEP 51, {esp+dOffset}, {esp+wOffset+ 3*sizeof(dword)}, 034B0BCB5h
   SHA256_STEP 52, {esp+dOffset}, {esp+wOffset+ 4*sizeof(dword)}, 0391C0CB3h
   SHA256_STEP 53, {esp+dOffset}, {esp+wOffset+ 5*sizeof(dword)}, 04ED8AA4Ah
   SHA256_STEP 54, {esp+dOffset}, {esp+wOffset+ 6*sizeof(dword)}, 05B9CCA4Fh
   SHA256_STEP 55, {esp+dOffset}, {esp+wOffset+ 7*sizeof(dword)}, 0682E6FF3h
   SHA256_STEP 56, {esp+dOffset}, {esp+wOffset+ 8*sizeof(dword)}, 0748F82EEh
   SHA256_STEP 57, {esp+dOffset}, {esp+wOffset+ 9*sizeof(dword)}, 078A5636Fh
   SHA256_STEP 58, {esp+dOffset}, {esp+wOffset+10*sizeof(dword)}, 084C87814h
   SHA256_STEP 59, {esp+dOffset}, {esp+wOffset+11*sizeof(dword)}, 08CC70208h
   SHA256_STEP 60, {esp+dOffset}, {esp+wOffset+12*sizeof(dword)}, 090BEFFFAh
   SHA256_STEP 61, {esp+dOffset}, {esp+wOffset+13*sizeof(dword)}, 0A4506CEBh
   SHA256_STEP 62, {esp+dOffset}, {esp+wOffset+14*sizeof(dword)}, 0BEF9A3F7h
   SHA256_STEP 63, {esp+dOffset}, {esp+wOffset+15*sizeof(dword)}, 0C67178F2h

;
; update digest
;
   mov      edi,[esp+hashOffset]
   mov      esi,[esp+msgOffset]

   mov      eax,[esp+dOffset+vA*sizeof(dword)]
   mov      ebx,[esp+dOffset+vB*sizeof(dword)]
   mov      ecx,[esp+dOffset+vC*sizeof(dword)]
   mov      edx,[esp+dOffset+vD*sizeof(dword)]
   add      [edi+0*sizeof(dword)],eax
   add      [edi+1*sizeof(dword)],ebx
   add      [edi+2*sizeof(dword)],ecx
   add      [edi+3*sizeof(dword)],edx
   mov      eax,[esp+dOffset+vE*sizeof(dword)]
   mov      ebx,[esp+dOffset+vF*sizeof(dword)]
   mov      ecx,[esp+dOffset+vG*sizeof(dword)]
   mov      edx,[esp+dOffset+vH*sizeof(dword)]
   add      [edi+4*sizeof(dword)],eax
   add      [edi+5*sizeof(dword)],ebx
   add      [edi+6*sizeof(dword)],ecx
   add      [edi+7*sizeof(dword)],edx

   mov      eax,[esp+lenOffset]
   add      esi, MBS_SHA256
   sub      eax, MBS_SHA256
   jg       .sha256_block_loop

   add      esp,stackSize        ; remove local buffers
   REST_GPR
   ret
ENDFUNC UpdateSHA256

%endif    ;; (_IPP >= _IPP_M5) && (_IPP < _IPP_V8)
%endif    ;; _FEATURE_OFF_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA256_

