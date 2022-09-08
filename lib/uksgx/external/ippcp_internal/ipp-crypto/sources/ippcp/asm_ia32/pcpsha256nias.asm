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
;               Message block processing according to SHA-256
;
;     Content:
;        UpdateSHA256ni
;
;





%include "asmdefs.inc"
%include "ia_emm.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA256_)
%if (_SHA_NI_ENABLING_ == _FEATURE_ON_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)


segment .text align=IPP_ALIGN_FACTOR

align IPP_ALIGN_FACTOR
CODE_DATA:
PSHUFFLE_BYTE_FLIP_MASK \
      DB     3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12

align IPP_ALIGN_FACTOR
;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA256ni(DigestSHA256 digest, const Ipp8u* msg, int mlen, const Ipp32u K_256[])
;*
;*****************************************************************************************

%ifndef _VXWORKS

IPPASM UpdateSHA256ni,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pDigest [ebp + ARG_1 + 0*sizeof(dword)] ; pointer to the in/out digest
%xdefine pMsg    [ebp + ARG_1 + 1*sizeof(dword)] ; pointer to the inp message
%xdefine msgLen  [ebp + ARG_1 + 2*sizeof(dword)] ; message length
%xdefine pTbl    [ebp + ARG_1 + 3*sizeof(dword)] ; pointer to SHA256 table of constants

%xdefine MBS_SHA256 (64) ; SHA-1 message block length (bytes)

%xdefine HASH_PTR   edi  ; 1st arg
%xdefine MSG_PTR    esi  ; 2nd arg
%xdefine MSG_LEN    edx  ; 3rd arg
%xdefine K256_PTR   ebx  ; 4rd arg

%xdefine MSG        xmm0
%xdefine STATE0     xmm1
%xdefine STATE1     xmm2
%xdefine MSGTMP0    xmm3
%xdefine MSGTMP1    xmm4
%xdefine MSGTMP2    xmm5
%xdefine MSGTMP3    xmm6
%xdefine MSGTMP4    xmm7

;
; stack frame
;
%xdefine mask_save  eax
%xdefine abef_save  eax+sizeof(oword)
%xdefine cdgh_save  eax+sizeof(oword)*2
%xdefine frame_size sizeof(oword)+sizeof(oword)+sizeof(oword)

   sub      esp, (frame_size+16)
   lea      eax, [esp+16]
   and      eax, -16

   mov      MSG_LEN, msgLen   ; message length
   test     MSG_LEN, MSG_LEN
   jz       .quit

   mov      HASH_PTR, pDigest
   mov      MSG_PTR, pMsg
   mov      K256_PTR, pTbl

   ;; load input hash value, reorder these appropriately
   movdqu   STATE0, oword [HASH_PTR+0*sizeof(oword)]
   movdqu   STATE1, oword [HASH_PTR+1*sizeof(oword)]

   pshufd   STATE0,  STATE0,  0B1h  ; CDAB
   pshufd   STATE1,  STATE1,  01Bh  ; EFGH
   movdqa   MSGTMP4, STATE0
   palignr  STATE0,  STATE1,  8     ; ABEF
   pblendw  STATE1,  MSGTMP4, 0F0h  ; CDGH

   ;; copy byte_flip_mask to stack
   mov      ecx, 000010203h            ;; DB     3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12
   mov      dword [mask_save], ecx
   mov      ecx, 004050607h
   mov      dword [mask_save+1*sizeof(dword)], ecx
   mov      ecx, 008090a0bh
   mov      dword [mask_save+2*sizeof(dword)], ecx
   mov      ecx, 00c0d0e0fh
   mov      dword [mask_save+3*sizeof(dword)], ecx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha256_block_loop:
   movdqa   oword [abef_save], STATE0 ; save for addition after rounds
   movdqa   oword [cdgh_save], STATE1

   ;; rounds 0-3
   movdqu      MSG, oword [MSG_PTR + 0*sizeof(oword)]
   pshufb      MSG, [mask_save]
   movdqa      MSGTMP0, MSG
   paddd       MSG, oword [K256_PTR + 0*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1

   ;; rounds 4-7
   movdqu      MSG, oword [MSG_PTR + 1*sizeof(oword)]
   pshufb      MSG, [mask_save]
   movdqa      MSGTMP1, MSG
   paddd       MSG, oword [K256_PTR + 1*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP0, MSGTMP1

   ;; rounds 8-11
   movdqu      MSG, oword [MSG_PTR + 2*sizeof(oword)]
   pshufb      MSG, [mask_save]
   movdqa      MSGTMP2, MSG
   paddd       MSG, oword [K256_PTR + 2*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP1, MSGTMP2

   ;; rounds 12-15
   movdqu      MSG, oword [MSG_PTR + 3*sizeof(oword)]
   pshufb      MSG, [mask_save]
   movdqa      MSGTMP3, MSG
   paddd       MSG, oword [K256_PTR + 3*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP3
   palignr     MSGTMP4, MSGTMP2, 4
   paddd       MSGTMP0, MSGTMP4
   sha256msg2  MSGTMP0, MSGTMP3
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP2, MSGTMP3
   ;; rounds 16-19
   movdqa      MSG, MSGTMP0
   paddd       MSG, oword [K256_PTR + 4*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP0
   palignr     MSGTMP4, MSGTMP3, 4
   paddd       MSGTMP1, MSGTMP4
   sha256msg2  MSGTMP1, MSGTMP0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP3, MSGTMP0

   ;; rounds 20-23
   movdqa      MSG, MSGTMP1
   paddd       MSG, oword [K256_PTR + 5*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP1
   palignr     MSGTMP4, MSGTMP0, 4
   paddd       MSGTMP2, MSGTMP4
   sha256msg2  MSGTMP2, MSGTMP1
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP0, MSGTMP1

   ;; rounds 24-27
   movdqa      MSG, MSGTMP2
   paddd       MSG, oword [K256_PTR + 6*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP2
   palignr     MSGTMP4, MSGTMP1, 4
   paddd       MSGTMP3, MSGTMP4
   sha256msg2  MSGTMP3, MSGTMP2
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP1, MSGTMP2

   ;; rounds 28-31
   movdqa      MSG, MSGTMP3
   paddd       MSG, oword [K256_PTR + 7*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP3
   palignr     MSGTMP4, MSGTMP2, 4
   paddd       MSGTMP0, MSGTMP4
   sha256msg2  MSGTMP0, MSGTMP3
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP2, MSGTMP3

   ;; rounds 32-35
   movdqa      MSG, MSGTMP0
   paddd       MSG, oword [K256_PTR + 8*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP0
   palignr     MSGTMP4, MSGTMP3, 4
   paddd       MSGTMP1, MSGTMP4
   sha256msg2  MSGTMP1, MSGTMP0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP3, MSGTMP0

   ;; rounds 36-39
   movdqa      MSG, MSGTMP1
   paddd       MSG, oword [K256_PTR + 9*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP1
   palignr     MSGTMP4, MSGTMP0, 4
   paddd       MSGTMP2, MSGTMP4
   sha256msg2  MSGTMP2, MSGTMP1
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP0, MSGTMP1

   ;; rounds 40-43
   movdqa      MSG, MSGTMP2
   paddd       MSG, oword [K256_PTR + 10*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP2
   palignr     MSGTMP4, MSGTMP1, 4
   paddd       MSGTMP3, MSGTMP4
   sha256msg2  MSGTMP3, MSGTMP2
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP1, MSGTMP2

   ;; rounds 44-47
   movdqa      MSG, MSGTMP3
   paddd       MSG, oword [K256_PTR + 11*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP3
   palignr     MSGTMP4, MSGTMP2, 4
   paddd       MSGTMP0, MSGTMP4
   sha256msg2  MSGTMP0, MSGTMP3
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP2, MSGTMP3

   ;; rounds 48-51
   movdqa      MSG, MSGTMP0
   paddd       MSG, oword [K256_PTR + 12*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP0
   palignr     MSGTMP4, MSGTMP3, 4
   paddd       MSGTMP1, MSGTMP4
   sha256msg2  MSGTMP1, MSGTMP0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1
   sha256msg1  MSGTMP3, MSGTMP0

   ;; rounds 52-55
   movdqa      MSG, MSGTMP1
   paddd       MSG, oword [K256_PTR + 13*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP1
   palignr     MSGTMP4, MSGTMP0, 4
   paddd       MSGTMP2, MSGTMP4
   sha256msg2  MSGTMP2, MSGTMP1
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1

   ;; rounds 56-59
   movdqa      MSG, MSGTMP2
   paddd       MSG, oword [K256_PTR + 14*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP2
   palignr     MSGTMP4, MSGTMP1, 4
   paddd       MSGTMP3, MSGTMP4
   sha256msg2  MSGTMP3, MSGTMP2
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1

   ;; rounds 60-63
   movdqa      MSG, MSGTMP3
   paddd       MSG, oword [K256_PTR + 15*sizeof(oword)]
   sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   sha256rnds2 STATE0, STATE1

   paddd       STATE0, oword [abef_save]  ; update previously saved hash
   paddd       STATE1, oword [cdgh_save]

   add         MSG_PTR, MBS_SHA256
   sub         MSG_LEN, MBS_SHA256
   jg          .sha256_block_loop

   ; reorder hash
   pshufd      STATE0,  STATE0,  01Bh  ; FEBA
   pshufd      STATE1,  STATE1,  0B1h  ; DCHG
   movdqa      MSGTMP4, STATE0
   pblendw     STATE0,  STATE1,  0F0h  ; DCBA
   palignr     STATE1,  MSGTMP4, 8     ; HGFE

   ; and store it back
   movdqu      oword [HASH_PTR + 0*sizeof(oword)], STATE0
   movdqu      oword [HASH_PTR + 1*sizeof(oword)], STATE1

.quit:
   add   esp, (frame_size+16)
   REST_GPR
   ret
ENDFUNC UpdateSHA256ni

%else ;; no sha ni support in VxWorks - therefore we temporary use db

IPPASM UpdateSHA256ni,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pDigest [ebp + ARG_1 + 0*sizeof(dword)] ; pointer to the in/out digest
%xdefine pMsg    [ebp + ARG_1 + 1*sizeof(dword)] ; pointer to the inp message
%xdefine msgLen  [ebp + ARG_1 + 2*sizeof(dword)] ; message length
%xdefine pTbl    [ebp + ARG_1 + 3*sizeof(dword)] ; pointer to SHA256 table of constants

%xdefine MBS_SHA256 (64) ; SHA-1 message block length (bytes)

%xdefine HASH_PTR   edi  ; 1st arg
%xdefine MSG_PTR    esi  ; 2nd arg
%xdefine MSG_LEN    edx  ; 3rd arg
%xdefine K256_PTR   ebx  ; 4rd arg

%xdefine MSG        xmm0
%xdefine STATE0     xmm1
%xdefine STATE1     xmm2
%xdefine MSGTMP0    xmm3
%xdefine MSGTMP1    xmm4
%xdefine MSGTMP2    xmm5
%xdefine MSGTMP3    xmm6
%xdefine MSGTMP4    xmm7

;
; stack frame
;
%xdefine mask_save  eax
%xdefine abef_save  eax+sizeof(oword)
%xdefine cdgh_save  eax+sizeof(oword)*2
%xdefine frame_size sizeof(oword)+sizeof(oword)+sizeof(oword)

   sub      esp, (frame_size+16)
   lea      eax, [esp+16]
   and      eax, -16

   mov      MSG_LEN, msgLen   ; message length
   test     MSG_LEN, MSG_LEN
   jz       .quit

   mov      HASH_PTR, pDigest
   mov      MSG_PTR, pMsg
   mov      K256_PTR, pTbl

   ;; load input hash value, reorder these appropriately
   movdqu   STATE0, oword [HASH_PTR+0*sizeof(oword)]
   movdqu   STATE1, oword [HASH_PTR+1*sizeof(oword)]

   pshufd   STATE0,  STATE0,  0B1h  ; CDAB
   pshufd   STATE1,  STATE1,  01Bh  ; EFGH
   movdqa   MSGTMP4, STATE0
   palignr  STATE0,  STATE1,  8     ; ABEF
   pblendw  STATE1,  MSGTMP4, 0F0h  ; CDGH

   ;; copy byte_flip_mask to stack
  ;movdqa   MSGTMP4, oword [PSHUFFLE_BYTE_FLIP_MASK]
   LD_ADDR  ecx, CODE_DATA
   movdqa   MSGTMP4, oword [ecx+(PSHUFFLE_BYTE_FLIP_MASK-CODE_DATA)]
   movdqa   oword [mask_save], MSGTMP4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha256_block_loop:
   movdqa   oword [abef_save], STATE0 ; save for addition after rounds
   movdqa   oword [cdgh_save], STATE1

   ;; rounds 0-3
   movdqu      MSG, oword [MSG_PTR + 0*sizeof(oword)]
   pshufb      MSG, oword [mask_save]
   movdqa      MSGTMP0, MSG
   paddd       MSG, oword [K256_PTR + 0*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1

   ;; rounds 4-7
   movdqu      MSG, oword [MSG_PTR + 1*sizeof(oword)]
   pshufb      MSG, oword [mask_save]
   movdqa      MSGTMP1, MSG
   paddd       MSG, oword [K256_PTR + 1*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0DCH ;; sha256msg1  MSGTMP0, MSGTMP1

   ;; rounds 8-11
   movdqu      MSG, oword [MSG_PTR + 2*sizeof(oword)]
   pshufb      MSG, oword [mask_save]
   movdqa      MSGTMP2, MSG
   paddd       MSG, oword [K256_PTR + 2*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0E5H ;; sha256msg1  MSGTMP1, MSGTMP2

   ;; rounds 12-15
   movdqu      MSG, oword [MSG_PTR + 3*sizeof(oword)]
   pshufb      MSG, oword [mask_save]
   movdqa      MSGTMP3, MSG
   paddd       MSG, oword [K256_PTR + 3*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP3
   palignr     MSGTMP4, MSGTMP2, 4
   paddd       MSGTMP0, MSGTMP4
   db 0FH,038H,0CDH,0DEH ;; sha256msg2  MSGTMP0, MSGTMP3
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0EEH ;; sha256msg1  MSGTMP2, MSGTMP3
   ;; rounds 16-19
   movdqa      MSG, MSGTMP0
   paddd       MSG, oword [K256_PTR + 4*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP0
   palignr     MSGTMP4, MSGTMP3, 4
   paddd       MSGTMP1, MSGTMP4
   db 0FH,038H,0CDH,0E3H ;; sha256msg2  MSGTMP1, MSGTMP0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0F3H ;; sha256msg1  MSGTMP3, MSGTMP0

   ;; rounds 20-23
   movdqa      MSG, MSGTMP1
   paddd       MSG, oword [K256_PTR + 5*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP1
   palignr     MSGTMP4, MSGTMP0, 4
   paddd       MSGTMP2, MSGTMP4
   db 0FH,038H,0CDH,0ECH ;; sha256msg2  MSGTMP2, MSGTMP1
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0DCH ;; sha256msg1  MSGTMP0, MSGTMP1

   ;; rounds 24-27
   movdqa      MSG, MSGTMP2
   paddd       MSG, oword [K256_PTR + 6*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP2
   palignr     MSGTMP4, MSGTMP1, 4
   paddd       MSGTMP3, MSGTMP4
   db 0FH,038H,0CDH,0F5H ;; sha256msg2  MSGTMP3, MSGTMP2
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0E5H ;; sha256msg1  MSGTMP1, MSGTMP2

   ;; rounds 28-31
   movdqa      MSG, MSGTMP3
   paddd       MSG, oword [K256_PTR + 7*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP3
   palignr     MSGTMP4, MSGTMP2, 4
   paddd       MSGTMP0, MSGTMP4
   db 0FH,038H,0CDH,0DEH ;; sha256msg2  MSGTMP0, MSGTMP3
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0EEH ;; sha256msg1  MSGTMP2, MSGTMP3

   ;; rounds 32-35
   movdqa      MSG, MSGTMP0
   paddd       MSG, oword [K256_PTR + 8*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP0
   palignr     MSGTMP4, MSGTMP3, 4
   paddd       MSGTMP1, MSGTMP4
   db 0FH,038H,0CDH,0E3H ;; sha256msg2  MSGTMP1, MSGTMP0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0F3H ;; sha256msg1  MSGTMP3, MSGTMP0

   ;; rounds 36-39
   movdqa      MSG, MSGTMP1
   paddd       MSG, oword [K256_PTR + 9*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP1
   palignr     MSGTMP4, MSGTMP0, 4
   paddd       MSGTMP2, MSGTMP4
   db 0FH,038H,0CDH,0ECH ;; sha256msg2  MSGTMP2, MSGTMP1
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0DCH ;; sha256msg1  MSGTMP0, MSGTMP1

   ;; rounds 40-43
   movdqa      MSG, MSGTMP2
   paddd       MSG, oword [K256_PTR + 10*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP2
   palignr     MSGTMP4, MSGTMP1, 4
   paddd       MSGTMP3, MSGTMP4
   db 0FH,038H,0CDH,0F5H ;; sha256msg2  MSGTMP3, MSGTMP2
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0E5H ;; sha256msg1  MSGTMP1, MSGTMP2

   ;; rounds 44-47
   movdqa      MSG, MSGTMP3
   paddd       MSG, oword [K256_PTR + 11*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP3
   palignr     MSGTMP4, MSGTMP2, 4
   paddd       MSGTMP0, MSGTMP4
   db 0FH,038H,0CDH,0DEH ;; sha256msg2  MSGTMP0, MSGTMP3
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0EEH ;; sha256msg1  MSGTMP2, MSGTMP3

   ;; rounds 48-51
   movdqa      MSG, MSGTMP0
   paddd       MSG, oword [K256_PTR + 12*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP0
   palignr     MSGTMP4, MSGTMP3, 4
   paddd       MSGTMP1, MSGTMP4
   db 0FH,038H,0CDH,0E3H ;; sha256msg2  MSGTMP1, MSGTMP0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1
   db 0FH,038H,0CCH,0F3H ;; sha256msg1  MSGTMP3, MSGTMP0

   ;; rounds 52-55
   movdqa      MSG, MSGTMP1
   paddd       MSG, oword [K256_PTR + 13*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP1
   palignr     MSGTMP4, MSGTMP0, 4
   paddd       MSGTMP2, MSGTMP4
   db 0FH,038H,0CDH,0ECH ;; sha256msg2  MSGTMP2, MSGTMP1
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1

   ;; rounds 56-59
   movdqa      MSG, MSGTMP2
   paddd       MSG, oword [K256_PTR + 14*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   movdqa      MSGTMP4, MSGTMP2
   palignr     MSGTMP4, MSGTMP1, 4
   paddd       MSGTMP3, MSGTMP4
   db 0FH,038H,0CDH,0F5H ;; sha256msg2  MSGTMP3, MSGTMP2
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1

   ;; rounds 60-63
   movdqa      MSG, MSGTMP3
   paddd       MSG, oword [K256_PTR + 15*sizeof(oword)]
   db 0FH,038H,0CBH,0D1H ;; sha256rnds2 STATE1, STATE0
   pshufd      MSG, MSG, 0Eh
   db 0FH,038H,0CBH,0CAH ;; sha256rnds2 STATE0, STATE1

   paddd       STATE0, oword [abef_save]  ; update previously saved hash
   paddd       STATE1, oword [cdgh_save]

   add         MSG_PTR, MBS_SHA256
   sub         MSG_LEN, MBS_SHA256
   jg          .sha256_block_loop

   ; reorder hash
   pshufd      STATE0,  STATE0,  01Bh  ; FEBA
   pshufd      STATE1,  STATE1,  0B1h  ; DCHG
   movdqa      MSGTMP4, STATE0
   pblendw     STATE0,  STATE1,  0F0h  ; DCBA
   palignr     STATE1,  MSGTMP4, 8     ; HGFE

   ; and store it back
   movdqu      oword [HASH_PTR + 0*sizeof(oword)], STATE0
   movdqu      oword [HASH_PTR + 1*sizeof(oword)], STATE1

.quit:
   add   esp, (frame_size+16)
   REST_GPR
   ret
ENDFUNC UpdateSHA256ni

%endif ;; VxWorks

%endif    ;; _FEATURE_ON_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA256_

