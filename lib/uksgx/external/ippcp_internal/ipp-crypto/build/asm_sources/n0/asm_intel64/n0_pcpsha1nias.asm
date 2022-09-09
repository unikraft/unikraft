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
;               Message block processing according to SHA-1
;
;     Content:
;        UpdateSHA1ni
;
;
%include "asmdefs.inc"
%include "ia_32e.inc"
%include "pcpvariant.inc"

%if (_ENABLE_ALG_SHA1_)
%if (_SHA_NI_ENABLING_ == _FEATURE_ON_) || (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_)
;;%if (_IPP32E >= _IPP32E_Y8 )

segment .text align=IPP_ALIGN_FACTOR


align IPP_ALIGN_FACTOR

UPPER_DWORD_MASK \
      DQ    00000000000000000h, 0ffffffff00000000h
PSHUFFLE_BYTE_FLIP_MASK \
      DB    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

align IPP_ALIGN_FACTOR
;*****************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA1ni(DigestSHA1 digest, const Ipp32u* mblk, int mlen, const void* pParam)
;*
;*****************************************************************************************

%ifndef _VXWORKS

IPPASM UpdateSHA1ni,PUBLIC
%assign LOCAL_FRAME 16*2
        USES_GPR rsi,rdi
        USES_XMM xmm6,xmm7
        COMP_ABI 4

%xdefine MBS_SHA1  (64) ; SHA-1 message block length (bytes)

%xdefine HASH_PTR  rdi  ; 1st arg
%xdefine MSG_PTR   rsi  ; 2nd arg
%xdefine MSG_LEN   rdx  ; 3rd arg

%xdefine ABCD      xmm0
%xdefine E0        xmm1 ; Need two E's b/c they ping pong
%xdefine E1        xmm2
%xdefine MSG0      xmm3
%xdefine MSG1      xmm4
%xdefine MSG2      xmm5
%xdefine MSG3      xmm6
%xdefine SHUF_MASK xmm7

;
; stack frame
;
%xdefine abcd_save rsp
%xdefine e_save    rsp+16

   movsxd   MSG_LEN, edx      ; expand mLen
   test     MSG_LEN, MSG_LEN
   jz       .quit

;; load initial hash values
   movdqu   ABCD, oword [HASH_PTR]
   pinsrd   E0, dword [HASH_PTR+16], 3
   pand     E0, oword [rel UPPER_DWORD_MASK]
   pshufd   ABCD, ABCD, 01Bh

   movdqa   SHUF_MASK, oword [rel PSHUFFLE_BYTE_FLIP_MASK]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha1_block_loop:
   movdqa   oword [abcd_save], ABCD
   movdqa   oword [e_save], E0

   ;; rounds 0-3
   movdqu      MSG0, oword [MSG_PTR +0*16]
   pshufb      MSG0, SHUF_MASK
   paddd       E0, MSG0
   movdqa      E1, ABCD
   sha1rnds4   ABCD, E0, 0
   ;movdqu      oword [rcx+16*0], ABCD

   ;; rounds 4-7
   movdqu      MSG1, oword [MSG_PTR +1*16]
   pshufb      MSG1, SHUF_MASK
   sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   sha1rnds4   ABCD, E1, 0
   sha1msg1    MSG0, MSG1
   ;movdqu      oword [rcx+16*1], ABCD

   ;; rounds 8-11
   movdqu      MSG2, oword [MSG_PTR +2*16]
   pshufb      MSG2, SHUF_MASK
   sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   sha1rnds4   ABCD, E0, 0
   sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*2], ABCD

   ;; rounds 12-15
   movdqu      MSG3, oword [MSG_PTR +3*16]
   pshufb      MSG3, SHUF_MASK
   sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   sha1msg2    MSG0, MSG3
   sha1rnds4   ABCD, E1, 0
   sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*3], ABCD

   ;; rounds 16-19
   sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   sha1msg2    MSG1, MSG0
   sha1rnds4   ABCD, E0, 0
   sha1msg1    MSG3, MSG0
   pxor        MSG2, MSG0
   ;movdqu      oword [rcx+16*4], ABCD

   ;; rounds 20-23
   sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   sha1msg2    MSG2, MSG1
   sha1rnds4   ABCD, E1, 1
   sha1msg1    MSG0, MSG1
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*5], ABCD

   ;; rounds 24-27
   sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   sha1msg2    MSG3, MSG2
   sha1rnds4   ABCD, E0, 1
   sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*6], ABCD

   ;; rounds 28-31
   sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   sha1msg2    MSG0, MSG3
   sha1rnds4   ABCD, E1, 1
   sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*7], ABCD

   ;; rounds 32-35
   sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   sha1msg2    MSG1, MSG0
   sha1rnds4   ABCD, E0, 1
   sha1msg1    MSG3, MSG0
   pxor        MSG2, MSG0
   ;movdqu      oword [rcx+16*8], ABCD

   ;; rounds 36-39
   sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   sha1msg2    MSG2, MSG1
   sha1rnds4   ABCD, E1, 1
   sha1msg1    MSG0, MSG1
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*9], ABCD

   ;; rounds 40-43
   sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   sha1msg2    MSG3, MSG2
   sha1rnds4   ABCD, E0, 2
   sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*10], ABCD

   ;; rounds 44-47
   sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   sha1msg2    MSG0, MSG3
   sha1rnds4   ABCD, E1, 2
   sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*11], ABCD

   ;; rounds 48-51
   sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   sha1msg2    MSG1, MSG0
   sha1rnds4   ABCD, E0, 2
   sha1msg1    MSG3, MSG0
   pxor  MSG2, MSG0
   ;movdqu      oword [rcx+16*12], ABCD

   ;; rounds 52-55
   sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   sha1msg2    MSG2, MSG1
   sha1rnds4   ABCD, E1, 2
   sha1msg1    MSG0, MSG1
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*13], ABCD

   ;; rounds 56-59
   sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   sha1msg2    MSG3, MSG2
   sha1rnds4   ABCD, E0, 2
   sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*14], ABCD

   ;; rounds 60-63
   sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   sha1msg2    MSG0, MSG3
   sha1rnds4   ABCD, E1, 3
   sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*15], ABCD

   ;; rounds 64-67
   sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   sha1msg2    MSG1, MSG0
   sha1rnds4   ABCD, E0, 3
   sha1msg1    MSG3, MSG0
   pxor        MSG2, MSG0
   ;movdqu      oword [rcx+16*16], ABCD

   ;; rounds 68-71
   sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   sha1msg2    MSG2, MSG1
   sha1rnds4   ABCD, E1, 3
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*17], ABCD

   ;; rounds 72-75
   sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   sha1msg2    MSG3, MSG2
   sha1rnds4   ABCD, E0, 3
   ;movdqu      oword [rcx+16*18], ABCD

   ;; rounds 76-79
   sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   sha1rnds4   ABCD, E1, 3
   ;movdqu      oword [rcx+16*19], ABCD

   ;; add current hash values with previously saved
   sha1nexte   E0, oword [e_save]
   paddd       ABCD, oword [abcd_save]

   add         MSG_PTR, MBS_SHA1
   sub         MSG_LEN, MBS_SHA1
   jg          .sha1_block_loop

   ;; write hash values back in the correct order
   pshufd      ABCD, ABCD, 01Bh
   movdqu      oword [HASH_PTR], ABCD
   pextrd      dword [HASH_PTR+16], E0, 3

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA1ni

%else ;; no sha ni support in VxWorks - therefore we temporary use db
IPPASM UpdateSHA1ni,PUBLIC
%assign LOCAL_FRAME 16*2
        USES_GPR rsi,rdi
        USES_XMM xmm6,xmm7
        COMP_ABI 4

%xdefine MBS_SHA1  (64) ; SHA-1 message block length (bytes)

%xdefine HASH_PTR  rdi  ; 1st arg
%xdefine MSG_PTR   rsi  ; 2nd arg
%xdefine MSG_LEN   rdx  ; 3rd arg

%xdefine ABCD      xmm0
%xdefine E0        xmm1 ; Need two E's b/c they ping pong
%xdefine E1        xmm2
%xdefine MSG0      xmm3
%xdefine MSG1      xmm4
%xdefine MSG2      xmm5
%xdefine MSG3      xmm6
%xdefine SHUF_MASK xmm7

;
; stack frame
;
%xdefine abcd_save rsp
%xdefine e_save    rsp+16

   movsxd   MSG_LEN, edx      ; expand mLen
   test     MSG_LEN, MSG_LEN
   jz       .quit

;; load initial hash values
   movdqu   ABCD, oword [HASH_PTR]
   pinsrd   E0, dword [HASH_PTR+16], 3
   pand     E0, oword [rel UPPER_DWORD_MASK]
   pshufd   ABCD, ABCD, 01Bh

   movdqa   SHUF_MASK, oword [rel PSHUFFLE_BYTE_FLIP_MASK]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.sha1_block_loop:
   movdqa   oword [abcd_save], ABCD
   movdqa   oword [e_save], E0

   ;; rounds 0-3
   movdqu      MSG0, oword [MSG_PTR +0*16]
   pshufb      MSG0, SHUF_MASK
   paddd       E0, MSG0
   movdqa      E1, ABCD
   db 0FH,3AH,0CCH,0C1H,00H ;;sha1rnds4   ABCD, E0, 0
   ;movdqu      oword [rcx+16*0], ABCD

   ;; rounds 4-7
   movdqu      MSG1, oword [MSG_PTR +1*16]
   pshufb      MSG1, SHUF_MASK
   db 0FH,38H,0C8H,0D4H ;;sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   db 0FH,3AH,0CCH,0C2H,00H ;;sha1rnds4   ABCD, E1, 0
   db 0FH,38H,0C9H,0DCH ;;sha1msg1    MSG0, MSG1
   ;movdqu      oword [rcx+16*1], ABCD

   ;; rounds 8-11
   movdqu      MSG2, oword [MSG_PTR +2*16]
   pshufb      MSG2, SHUF_MASK
   db 0FH,38H,0C8H,0CDH ;;sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   db 0Fh,3Ah,0CCh,0C1h,00h ;;sha1rnds4   ABCD, E0, 0
   db 0Fh,38h,0C9h,0E5h ;;sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*2], ABCD

   ;; rounds 12-15
   movdqu      MSG3, oword [MSG_PTR +3*16]
   pshufb      MSG3, SHUF_MASK
   db 0Fh,38H,0C8h,0D6h ;;sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   db 0Fh,38H,0CAh,0DEh ;;sha1msg2    MSG0, MSG3
   db 0Fh,3AH,0CCh,0C2h,00h ;;sha1rnds4   ABCD, E1, 0
   db 0Fh,38H,0C9h,0EEh ;;sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*3], ABCD

   ;; rounds 16-19
   db 0Fh,38H,0C8h,0CBh ;;sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   db 0Fh,38H,0CAh,0E3h ;;sha1msg2    MSG1, MSG0
   db 0Fh,3AH,0CCh,0C1h,00h ;;sha1rnds4   ABCD, E0, 0
   db 0Fh,38H,0C9h,0F3h ;;sha1msg1    MSG3, MSG0
   pxor        MSG2, MSG0
   ;movdqu      oword [rcx+16*4], ABCD

   ;; rounds 20-23
   db 0FH,38H,0C8h,0D4h ;;sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   db 0FH,38H,0CAh,0ECH ;;sha1msg2    MSG2, MSG1
   db 0FH,3AH,0CCH,0C2H,01H ;;sha1rnds4   ABCD, E1, 1
   db 0FH,38H,0C9h,0DCh ;;sha1msg1    MSG0, MSG1
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*5], ABCD

   ;; rounds 24-27
   db 0FH,38H,0C8h,0CDh ;;sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   db 0FH,38H,0CAh,0F5h ;;sha1msg2    MSG3, MSG2
   db 0FH,3AH,0CCh,0C1h,01h ;;sha1rnds4   ABCD, E0, 1
   db 0FH,38H,0C9h,0E5h ;;sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*6], ABCD

   ;; rounds 28-31
   db 0FH,38H,0C8H,0D6h ;;sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   db 0FH,38H,0CAH,0DEh ;;sha1msg2    MSG0, MSG3
   db 0FH,3AH,0CCH,0C2H,01h ;;sha1rnds4   ABCD, E1, 1
   db 0FH,38H,0C9H,0EEh ;;sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*7], ABCD

   ;; rounds 32-35
   db 0FH,38H,0C8H,0CBh ;;sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   db 0FH,38H,0CAH,0E3h ;;sha1msg2    MSG1, MSG0
   db 0FH,3AH,0CCH,0C1H,01h ;;sha1rnds4   ABCD, E0, 1
   db 0FH,38H,0C9H,0F3h ;;sha1msg1    MSG3, MSG0
   pxor        MSG2, MSG0
   ;movdqu      oword [rcx+16*8], ABCD

   ;; rounds 36-39
   db 0FH,38H,0C8H,0D4h ;;sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   db 0FH,38H,0CAH,0ECh ;;sha1msg2    MSG2, MSG1
   db 0FH,3AH,0CCH,0C2H,01h ;;sha1rnds4   ABCD, E1, 1
   db 0FH,38H,0C9H,0DCh ;;sha1msg1    MSG0, MSG1
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*9], ABCD

   ;; rounds 40-43
   db 0FH,38H,0C8H,0CDh ;;sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   db 0FH,38H,0CAH,0F5h ;;sha1msg2    MSG3, MSG2
   db 0FH,3AH,0CCH,0C1H,02h ;;sha1rnds4   ABCD, E0, 2
   db 0FH,38H,0C9H,0E5h ;;sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*10], ABCD

   ;; rounds 44-47
   db 0FH,38H,0C8H,0D6h ;;sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   db 0FH,38H,0CAH,0DEh ;;sha1msg2    MSG0, MSG3
   db 0FH,3AH,0CCH,0C2h,02h ;;sha1rnds4   ABCD, E1, 2
   db 0FH,38H,0C9H,0EEh ;;sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*11], ABCD

   ;; rounds 48-51
   db 0Fh,38H,0C8h,0CBh ;;sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   db 0Fh,38H,0CAH,0E3h ;;sha1msg2    MSG1, MSG0
   db 0Fh,3AH,0CCH,0C1H,02h ;;sha1rnds4   ABCD, E0, 2
   db 0Fh,38H,0C9H,0F3h ;;sha1msg1    MSG3, MSG0
   pxor  MSG2, MSG0
   ;movdqu      oword [rcx+16*12], ABCD

   ;; rounds 52-55
   db 0Fh,38H,0C8h,0D4h ;;sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   db 0Fh,38H,0CAH,0ECh ;;sha1msg2    MSG2, MSG1
   db 0Fh,3AH,0CCH,0C2H,02h ;;sha1rnds4   ABCD, E1, 2
   db 0Fh,38H,0C9H,0DCh ;;sha1msg1    MSG0, MSG1
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*13], ABCD

   ;; rounds 56-59
   db 0Fh,38H,0C8H,0CDh ;;sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   db 0Fh,38H,0CAH,0F5h ;;sha1msg2    MSG3, MSG2
   db 0Fh,3AH,0CCH,0C1H,02h ;;sha1rnds4   ABCD, E0, 2
   db 0Fh,38H,0C9H,0E5h ;;sha1msg1    MSG1, MSG2
   pxor        MSG0, MSG2
   ;movdqu      oword [rcx+16*14], ABCD

   ;; rounds 60-63
   db 0Fh,38H,0C8H,0D6h ;;sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   db 0Fh,38H,0CAH,0DEh ;;sha1msg2    MSG0, MSG3
   db 0Fh,3AH,0CCH,0C2H,03h ;;sha1rnds4   ABCD, E1, 3
   db 0Fh,38H,0C9H,0EEh ;;sha1msg1    MSG2, MSG3
   pxor        MSG1, MSG3
   ;movdqu      oword [rcx+16*15], ABCD

   ;; rounds 64-67
   db 0Fh,38H,0C8H,0CBh ;;sha1nexte   E0, MSG0
   movdqa      E1, ABCD
   db 0Fh,38H,0CAH,0E3h ;;sha1msg2    MSG1, MSG0
   db 0Fh,3AH,0CCH,0C1H,03h ;;sha1rnds4   ABCD, E0, 3
   db 0Fh,38H,0C9H,0F3h ;;sha1msg1    MSG3, MSG0
   pxor        MSG2, MSG0
   ;movdqu      oword [rcx+16*16], ABCD

   ;; rounds 68-71
   db 0Fh,38H,0C8h,0D4h ;;sha1nexte   E1, MSG1
   movdqa      E0, ABCD
   db 0Fh,38H,0CAh,0ECh ;;sha1msg2    MSG2, MSG1
   db 0Fh,3AH,0CCh,0C2h,03h ;;sha1rnds4   ABCD, E1, 3
   pxor        MSG3, MSG1
   ;movdqu      oword [rcx+16*17], ABCD

   ;; rounds 72-75
   db 0Fh,38H,0C8h,0CDh ;;sha1nexte   E0, MSG2
   movdqa      E1, ABCD
   db 0Fh,38H,0CAh,0F5h ;;sha1msg2    MSG3, MSG2
   db 0Fh,3AH,0CCh,0C1h,03h ;;sha1rnds4   ABCD, E0, 3
   ;movdqu      oword [rcx+16*18], ABCD

   ;; rounds 76-79
   db 0Fh,38H,0C8h,0D6h ;;sha1nexte   E1, MSG3
   movdqa      E0, ABCD
   db 0Fh,3AH,0CCh,0C2h,03h ;;sha1rnds4   ABCD, E1, 3
   ;movdqu      oword [rcx+16*19], ABCD

   ;; add current hash values with previously saved
   db 0Fh,38H,0C8h,4Ch,24h,10h ;;sha1nexte   E0, oword [e_save]
   paddd       ABCD, oword [abcd_save]

   add         MSG_PTR, MBS_SHA1
   sub         MSG_LEN, MBS_SHA1
   jg          .sha1_block_loop

   ;; write hash values back in the correct order
   pshufd      ABCD, ABCD, 01Bh
   movdqu      oword [HASH_PTR], ABCD
   pextrd      dword [HASH_PTR+16], E0, 3

.quit:
   REST_XMM
   REST_GPR
   ret
ENDFUNC UpdateSHA1ni

%endif ;; VxWorks

;;%endif    ;; (_IPP32E >= _IPP32E_Y8)
%endif    ;; _FEATURE_ON_ / _FEATURE_TICKTOCK_
%endif    ;; _ENABLE_ALG_SHA1_

