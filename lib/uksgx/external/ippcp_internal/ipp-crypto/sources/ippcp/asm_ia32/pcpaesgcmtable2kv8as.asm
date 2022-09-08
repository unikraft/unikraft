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
;               Encrypt/Decrypt byte data stream according to Rijndael128 (GCM mode)
;
;     Content:
;      AesGcmMulGcm_table2K()
;      AesGcmAuth_table2K()
;




%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP >= _IPP_V8)

segment .text align=IPP_ALIGN_FACTOR

;
; getAesGcmConst_table_ct provides c-e-t access to pre-computed Ipp16u AesGcmConst_table[256]
;
;  input:
;  edx: address of the AesGcmConst_table
;  ecx: index in the table
;
;  output:
;  eax
;
;  register  ecx destoyed
;  registers mmx2, mmx3, mmx6, and mmx7 destoyed
;
align IPP_ALIGN_FACTOR
_CONST_DATA:
_INIT_IDX   DW    000h,001h,002h,003h,004h,005h,006h,007h   ;; initial search inx = {0:1:2:3:4:5:6:7}
_INCR_IDX   DW    008h,008h,008h,008h,008h,008h,008h,008h   ;; index increment = {8:8:8:8:8:8:8:8}

%xdefine INIT_IDX  [ebx+(_INIT_IDX - _CONST_DATA)]
%xdefine INCR_IDX  [ebx+(_INCR_IDX - _CONST_DATA)]

align IPP_ALIGN_FACTOR
IPPASM getAesGcmConst_table_ct,PRIVATE
   push     ebx
   LD_ADDR  ebx, _CONST_DATA

   pxor     xmm2, xmm2                 ;; accumulator xmm2 = 0

   mov      eax, ecx                   ;; broadcast inx into dword
   shl      ecx, 16
   or       ecx, eax
   movd     xmm3, ecx
   pshufd   xmm3, xmm3, 00b            ;; search index xmm3 = broadcast(idx)

   movdqa   xmm6, xmmword INIT_IDX ;; current indexes

   xor      eax, eax
align IPP_ALIGN_FACTOR
.search_loop:
   movdqa   xmm7, xmm6                             ;; copy current indexes
   paddw    xmm6, xmmword INCR_IDX             ;; advance current indexes

   pcmpeqw  xmm7, xmm3                             ;; selection mask
   pand     xmm7, xmmword [edx+eax*sizeof(word)];; mask data

   add      eax, 8
   cmp      eax, 256

   por      xmm2, xmm7                             ;; and accumulate
   jl       .search_loop

   movdqa   xmm3, xmm2                 ;; pack result in qword
   psrldq   xmm2, sizeof(xmmword)/2
   por      xmm2, xmm3
   movdqa   xmm3, xmm2                 ;; pack result in dword
   psrldq   xmm2, sizeof(xmmword)/4
   por      xmm2, xmm3
   movd     eax, xmm2

   pop      ebx

   and      ecx, 3                     ;; select tbl[idx] value
   shl      ecx, 4                     ;; rcx *=16 = sizeof(word)*8
   shr      eax, cl
   ret
ENDFUNC getAesGcmConst_table_ct


;
; void AesGcmMulGcm_table2K(Ipp8u* pHash, const Ipp8u* pPrecomputedData, , const void* pParam))
;
align IPP_ALIGN_FACTOR
IPPASM AesGcmMulGcm_table2K,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pHash   [esp + ARG_1 + 0*sizeof(dword)]
%xdefine pMulTbl [esp + ARG_1 + 1*sizeof(dword)]
%xdefine pParam  [esp + ARG_1 + 2*sizeof(dword)]

   mov      edi, pHash
   movdqu   xmm0, [edi]                ; hash value

   mov      esi, pMulTbl
   mov      edx, pParam                ; pointer to the fixed table

   movd     ebx, xmm0         ; ebx = hash.0
   mov      eax, 0f0f0f0f0h
   and      eax, ebx          ; eax = 4 x 4_bits
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h   ; ebx = 4 x 4_bits (another)
   movzx    ecx, ah
   movdqa   xmm5, oword [esi+1024+ecx]
   movzx    ecx, al
   movdqa   xmm4, oword [esi+1024+ecx]
   shr      eax, 16
   movzx    ecx, ah
   movdqa   xmm3, oword [esi+1024+ecx]
   movzx    ecx, al
   movdqa   xmm2, oword [esi+1024+ecx]

   psrldq   xmm0, 4           ; shift xmm0
   movd     eax, xmm0         ; eax = hash[1]
   and      eax, 0f0f0f0f0h   ; eax = 4 x 4_bits

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 0*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 0*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 0*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 0*256 + ecx]

   movd     ebx, xmm0         ; ebx = hash[1]
   shl      ebx, 4            ; another 4 x 4_bits
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [esi+1024+ 1*256 + ecx]
   movzx    ecx, al
   pxor     xmm4, oword [esi+1024+ 1*256 + ecx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [esi+1024+ 1*256 + ecx]
   movzx    ecx, al
   pxor     xmm2, oword [esi+1024+ 1*256 + ecx]
   psrldq   xmm0, 4

   movd     eax, xmm0            ; eax = hash.2
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 1*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 1*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 1*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 1*256 + ecx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [esi+1024+ 2*256 + ecx]
   movzx    ecx, al
   pxor     xmm4, oword [esi+1024+ 2*256 + ecx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [esi+1024+ 2*256 + ecx]
   movzx    ecx, al
   pxor     xmm2, oword [esi+1024+ 2*256 + ecx]

   psrldq   xmm0, 4
   movd     eax, xmm0         ; eax = hash.3
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 2*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 2*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 2*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 2*256 + ecx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [esi+1024+ 3*256 + ecx]
   movzx    ecx, al
   pxor     xmm4, oword [esi+1024+ 3*256 + ecx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [esi+1024+ 3*256 + ecx]
   movzx    ecx, al
   pxor     xmm2, oword [esi+1024+ 3*256 + ecx]

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 3*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 3*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 3*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 3*256 + ecx]

   movdqa   xmm0, xmm3
   pslldq   xmm3, 1
   pxor     xmm2, xmm3
   movdqa   xmm1, xmm2
   pslldq   xmm2, 1
   pxor     xmm5, xmm2

   psrldq   xmm0, 15
   movd     ecx, xmm0
   CALL_IPPASM  getAesGcmConst_table_ct ;;movzx    eax, word [edx + ecx*sizeof(word)]
   shl      eax, 8

   movdqa   xmm0, xmm5
   pslldq   xmm5, 1
   pxor     xmm4, xmm5

   psrldq   xmm1, 15
   movd     ecx, xmm1
   mov      ebx, eax                ;;xor      ax, word [edx + ecx*sizeof(word)]
   CALL_IPPASM  getAesGcmConst_table_ct ;;
   xor      eax, ebx                ;;
   shl      eax, 8

   psrldq   xmm0, 15
   movd     ecx, xmm0
   mov      ebx, eax                ;;xor      ax, word [edx + ecx*sizeof(word)]
   CALL_IPPASM  getAesGcmConst_table_ct ;;
   xor      eax, ebx                ;;

   movd     xmm0, eax
   pxor     xmm0, xmm4

   movdqu   oword [edi], xmm0    ; store hash value

   REST_GPR
   ret
ENDFUNC AesGcmMulGcm_table2K

;
; void AesGcmAuth_table2K(Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pPrecomputedData, const void* pParam)
;
align IPP_ALIGN_FACTOR
IPPASM AesGcmAuth_table2K,PUBLIC
  USES_GPR esi,edi,ebx

%xdefine pHash   [esp + ARG_1 + 0*sizeof(dword)]
%xdefine pSrc    [esp + ARG_1 + 1*sizeof(dword)]
%xdefine len     [esp + ARG_1 + 2*sizeof(dword)]
%xdefine pMulTbl [esp + ARG_1 + 3*sizeof(dword)]
%xdefine pParam  [esp + ARG_1 + 4*sizeof(dword)]

   mov      edi, pHash
   movdqu   xmm0, [edi]                ; hash value

   mov      esi, pMulTbl
   mov      edi, pSrc

   mov      edx, pParam                ; pointer to the fixed table

align IPP_ALIGN_FACTOR
.auth_loop:
   movdqu   xmm4, [edi]       ; get src[]
   pxor     xmm0, xmm4        ; hash ^= src[]

   movd     ebx, xmm0         ; ebx = hash.0
   mov      eax, 0f0f0f0f0h
   and      eax, ebx          ; eax = 4 x 4_bits
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h   ; ebx = 4 x 4_bits (another)
   movzx    ecx, ah
   movdqa   xmm5, oword [esi+1024+ecx]
   movzx    ecx, al
   movdqa   xmm4, oword [esi+1024+ecx]
   shr      eax, 16
   movzx    ecx, ah
   movdqa   xmm3, oword [esi+1024+ecx]
   movzx    ecx, al
   movdqa   xmm2, oword [esi+1024+ecx]

   psrldq   xmm0, 4           ; shift xmm0
   movd     eax, xmm0         ; eax = hash[1]
   and      eax, 0f0f0f0f0h   ; eax = 4 x 4_bits

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 0*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 0*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 0*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 0*256 + ecx]

   movd     ebx, xmm0         ; ebx = hash[1]
   shl      ebx, 4            ; another 4 x 4_bits
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [esi+1024+ 1*256 + ecx]
   movzx    ecx, al
   pxor     xmm4, oword [esi+1024+ 1*256 + ecx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [esi+1024+ 1*256 + ecx]
   movzx    ecx, al
   pxor     xmm2, oword [esi+1024+ 1*256 + ecx]

   psrldq   xmm0, 4
   movd     eax, xmm0            ; eax = hash[2]
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 1*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 1*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 1*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 1*256 + ecx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [esi+1024+ 2*256 + ecx]
   movzx    ecx, al
   pxor     xmm4, oword [esi+1024+ 2*256 + ecx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [esi+1024+ 2*256 + ecx]
   movzx    ecx, al
   pxor     xmm2, oword [esi+1024+ 2*256 + ecx]

   psrldq   xmm0, 4
   movd     eax, xmm0         ; eax = hash[3]
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 2*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 2*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 2*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 2*256 + ecx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [esi+1024+ 3*256 + ecx]
   movzx    ecx, al
   pxor     xmm4, oword [esi+1024+ 3*256 + ecx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [esi+1024+ 3*256 + ecx]
   movzx    ecx, al
   pxor     xmm2, oword [esi+1024+ 3*256 + ecx]

   movzx    ecx, bh
   pxor     xmm5, oword [esi+ 3*256 + ecx]
   movzx    ecx, bl
   pxor     xmm4, oword [esi+ 3*256 + ecx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [esi+ 3*256 + ecx]
   movzx    ecx, bl
   pxor     xmm2, oword [esi+ 3*256 + ecx]

   movdqa   xmm0, xmm3
   pslldq   xmm3, 1
   pxor     xmm2, xmm3
   movdqa   xmm1, xmm2
   pslldq   xmm2, 1
   pxor     xmm5, xmm2
   psrldq   xmm0, 15

   movd     ecx, xmm0
   CALL_IPPASM  getAesGcmConst_table_ct ;;movzx    eax, word [edx + ecx*sizeof(word)]
   shl      eax, 8

   movdqa   xmm0, xmm5
   pslldq   xmm5, 1
   pxor     xmm4, xmm5

   psrldq   xmm1, 15
   movd     ecx, xmm1
   mov      ebx, eax                ;;xor      ax, word [edx + ecx*sizeof(word)]
   CALL_IPPASM  getAesGcmConst_table_ct ;;
   xor      eax, ebx                ;;
   shl      eax, 8

   psrldq   xmm0, 15
   movd     ecx, xmm0
   mov      ebx, eax                ;;xor      ax, word [edx + ecx*sizeof(word)]
   CALL_IPPASM  getAesGcmConst_table_ct ;;
   xor      eax, ebx                ;;

   movd     xmm0, eax
   pxor     xmm0, xmm4

   add      edi, sizeof(oword)      ; advance src address
   sub      dword len, sizeof(oword)      ; decrease counter
   jnz      .auth_loop               ; process next block

   mov      edi, pHash
   movdqu   oword [edi], xmm0    ; store hash value

   REST_GPR
   ret
ENDFUNC AesGcmAuth_table2K

%endif

