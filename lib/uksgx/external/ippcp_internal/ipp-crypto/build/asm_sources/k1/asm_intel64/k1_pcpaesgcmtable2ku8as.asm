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
%include "ia_32e.inc"


%if (_IPP32E >= _IPP32E_U8)

segment .text align=IPP_ALIGN_FACTOR

;
; getAesGcmConst_table_ct provides c-e-t access to pre-computed Ipp16u AesGcmConst_table[256]
;
;  input:
;   r9: address of the AesGcmConst_table
;  rcx: index in the table
;
;  output:
;  rax
;
;  register  rcx destoyed
;  registers mmx2, mmx3, mmx6, and mmx7 destoyed
;
align IPP_ALIGN_FACTOR
INIT_IDX    dw    000h,001h,002h,003h,004h,005h,006h,007h   ;; initial search inx = {0:1:2:3:4:5:6:7}
INCR_IDX    dw    008h,008h,008h,008h,008h,008h,008h,008h   ;; index increment = {8:8:8:8:8:8:8:8}

align IPP_ALIGN_FACTOR
IPPASM getAesGcmConst_table_ct,PRIVATE
   pxor     xmm2, xmm2                 ;; accumulator xmm2 = 0

   mov      rax, rcx                   ;; broadcast inx into dword
   shl      rcx, 16
   or       rcx, rax
   movq     xmm3, rcx
   pshufd   xmm3, xmm3, 00b            ;; search index xmm3 = broadcast(idx)

   movdqa   xmm6, xmmword [rel INIT_IDX] ;; current indexes

   xor      rax, rax
align IPP_ALIGN_FACTOR
.search_loop:
   movdqa   xmm7, xmm6                             ;; copy current indexes
   paddw    xmm6, xmmword [rel INCR_IDX]             ;; advance current indexes

   pcmpeqw  xmm7, xmm3                             ;; selection mask
   pand     xmm7, xmmword [r9+rax*sizeof(word)] ;; mask data

   add      rax, 8
   cmp      rax, 256

   por      xmm2, xmm7                             ;; and accumulate
   jl       .search_loop

   movdqa   xmm3, xmm2                 ;; pack result in qword
   psrldq   xmm2, sizeof(xmmword)/2
   por      xmm2, xmm3
   movq     rax, xmm2

   and      rcx, 3                     ;; select tbl[idx] value
   shl      rcx, 4                     ;; rcx *=16 = sizeof(word)*8
   shr      rax, cl
   ret
ENDFUNC getAesGcmConst_table_ct

;
; void AesGcmMulGcm_table2K(Ipp8u* pHash, const Ipp8u* pPrecomputedData, const void* pParam)
;
align IPP_ALIGN_FACTOR
IPPASM AesGcmMulGcm_table2K,PUBLIC
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7
        COMP_ABI 3

   movdqu   xmm0, [rdi]                ; hash value
   mov      r8, rsi                    ; precomputed data pointer

   mov      r9, rdx                    ; pointer to the fixed table (AesGcmConst_table)

   movd     ebx, xmm0         ; ebx = hash.0
   mov      eax, 0f0f0f0f0h
   and      eax, ebx          ; eax = 4 x 4_bits
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h   ; ebx = 4 x 4_bits (another)
   movzx    ecx, ah
   movdqa   xmm5, oword [r8+1024+rcx]
   movzx    ecx, al
   movdqa   xmm4, oword [r8+1024+rcx]
   shr      eax, 16
   movzx    ecx, ah
   movdqa   xmm3, oword [r8+1024+rcx]
   movzx    ecx, al
   movdqa   xmm2, oword [r8+1024+rcx]

   psrldq   xmm0, 4           ; shift xmm0
   movd     eax, xmm0         ; eax = hash[1]
   and      eax, 0f0f0f0f0h   ; eax = 4 x 4_bits

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ (1-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ (1-1)*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ (1-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ (1-1)*256 + rcx]

   movd     ebx, xmm0         ; ebx = hash[1]
   shl      ebx, 4            ; another 4 x 4_bits
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [r8+1024+ 1*256 + rcx]
   movzx    ecx, al
   pxor     xmm4, oword [r8+1024+ 1*256 + rcx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [r8+1024+ 1*256 + rcx]
   movzx    ecx, al
   pxor     xmm2, oword [r8+1024+ 1*256 + rcx]
   psrldq   xmm0, 4

   movd     eax, xmm0            ; eax = hash.2
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ (2-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ (2-1)*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ (2-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ (2-1)*256 + rcx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [r8+1024+ 2*256 + rcx]
   movzx    ecx, al
   pxor     xmm4, oword [r8+1024+ 2*256 + rcx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [r8+1024+ 2*256 + rcx]
   movzx    ecx, al
   pxor     xmm2, oword [r8+1024+ 2*256 + rcx]

   psrldq   xmm0, 4
   movd     eax, xmm0         ; eax = hash.3
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ (3-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ (3-1)*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ (3-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ (3-1)*256 + rcx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [r8+1024+ 3*256 + rcx]
   movzx    ecx, al
   pxor     xmm4, oword [r8+1024+ 3*256 + rcx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [r8+1024+ 3*256 + rcx]
   movzx    ecx, al
   pxor     xmm2, oword [r8+1024+ 3*256 + rcx]

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ 3*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ 3*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ 3*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ 3*256 + rcx]

   movdqa   xmm0, xmm3
   pslldq   xmm3, 1
   pxor     xmm2, xmm3
   movdqa   xmm1, xmm2
   pslldq   xmm2, 1
   pxor     xmm5, xmm2
   psrldq   xmm0, 15

   movd     ecx, xmm0
   CALL_IPPASM     getAesGcmConst_table_ct ;;movzx    eax, word [r9 + rcx*sizeof(word)]
   shl      eax, 8
   movdqa   xmm0, xmm5
   pslldq   xmm5, 1
   pxor     xmm4, xmm5
   psrldq   xmm1, 15
   movd     ecx, xmm1
   mov      rbx, rax                ;;xor      ax, word [r9 + rcx*sizeof(word)]
   CALL_IPPASM     getAesGcmConst_table_ct ;;
   xor      rax, rbx                ;;
   shl      eax, 8
   psrldq   xmm0, 15
   movd     ecx, xmm0
   mov      rbx, rax                ;;xor      ax, word [r9 + rcx*sizeof(word)]
   CALL_IPPASM     getAesGcmConst_table_ct ;;
   xor      rax, rbx                ;;
   movd     xmm0, eax
   pxor     xmm0, xmm4

   movdqu   oword [rdi], xmm0    ; store hash value

   REST_XMM
   REST_GPR
   ret
ENDFUNC AesGcmMulGcm_table2K


;
; void AesGcmAuth_table2K(Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pPrecomputedData, const void* pParam)
;
align IPP_ALIGN_FACTOR
IPPASM AesGcmAuth_table2K,PUBLIC
        USES_GPR rsi,rdi,rbx
        USES_XMM xmm6,xmm7
        COMP_ABI 5

   mov      r9, r8                     ; pointer to the fixed table (pParam)

   movdqu   xmm0, [rdi]                ; hash value
   mov      r8, rcx                    ; precomputed data pointer

align IPP_ALIGN_FACTOR
.auth_loop:
   movdqu   xmm4, [rsi]       ; get src[]
   pxor     xmm0, xmm4        ; hash ^= src[]

   movd     ebx, xmm0         ; ebx = hash.0
   mov      eax, 0f0f0f0f0h
   and      eax, ebx          ; eax = 4 x 4_bits
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h   ; ebx = 4 x 4_bits (another)
   movzx    ecx, ah
   movdqa   xmm5, oword [r8+1024+rcx]
   movzx    ecx, al
   movdqa   xmm4, oword [r8+1024+rcx]
   shr      eax, 16
   movzx    ecx, ah
   movdqa   xmm3, oword [r8+1024+rcx]
   movzx    ecx, al
   movdqa   xmm2, oword [r8+1024+rcx]

   psrldq   xmm0, 4           ; shift xmm0
   movd     eax, xmm0         ; eax = hash[1]
   and      eax, 0f0f0f0f0h   ; eax = 4 x 4_bits

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ (1-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ (1-1)*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ (1-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ (1-1)*256 + rcx]

   movd     ebx, xmm0         ; ebx = hash[1]
   shl      ebx, 4            ; another 4 x 4_bits
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [r8+1024+ 1*256 + rcx]
   movzx    ecx, al
   pxor     xmm4, oword [r8+1024+ 1*256 + rcx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [r8+1024+ 1*256 + rcx]
   movzx    ecx, al
   pxor     xmm2, oword [r8+1024+ 1*256 + rcx]

   psrldq   xmm0, 4
   movd     eax, xmm0            ; eax = hash[2]
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ (2-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ (2-1)*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ (2-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ (2-1)*256 + rcx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [r8+1024+ 2*256 + rcx]
   movzx    ecx, al
   pxor     xmm4, oword [r8+1024+ 2*256 + rcx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [r8+1024+ 2*256 + rcx]
   movzx    ecx, al
   pxor     xmm2, oword [r8+1024+ 2*256 + rcx]

   psrldq   xmm0, 4
   movd     eax, xmm0         ; eax = hash[3]
   and      eax, 0f0f0f0f0h

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ (3-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ (3-1)*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ (3-1)*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ (3-1)*256 + rcx]

   movd     ebx, xmm0
   shl      ebx, 4
   and      ebx, 0f0f0f0f0h

   movzx    ecx, ah
   pxor     xmm5, oword [r8+1024+ 3*256 + rcx]
   movzx    ecx, al
   pxor     xmm4, oword [r8+1024+ 3*256 + rcx]
   shr      eax, 16
   movzx    ecx, ah
   pxor     xmm3, oword [r8+1024+ 3*256 + rcx]
   movzx    ecx, al
   pxor     xmm2, oword [r8+1024+ 3*256 + rcx]

   movzx    ecx, bh
   pxor     xmm5, oword [r8+ 3*256 + rcx]
   movzx    ecx, bl
   pxor     xmm4, oword [r8+ 3*256 + rcx]
   shr      ebx, 16
   movzx    ecx, bh
   pxor     xmm3, oword [r8+ 3*256 + rcx]
   movzx    ecx, bl
   pxor     xmm2, oword [r8+ 3*256 + rcx]

   movdqa   xmm0, xmm3
   pslldq   xmm3, 1
   pxor     xmm2, xmm3
   movdqa   xmm1, xmm2
   pslldq   xmm2, 1
   pxor     xmm5, xmm2
   psrldq   xmm0, 15

   movd     ecx, xmm0
   CALL_IPPASM     getAesGcmConst_table_ct ;;movzx    eax, word [r9 + rcx*sizeof(word)]
   shl      eax, 8
   movdqa   xmm0, xmm5
   pslldq   xmm5, 1
   pxor     xmm4, xmm5
   psrldq   xmm1, 15
   movd     ecx, xmm1
   mov      rbx, rax                ;;xor      ax, word [r9 + rcx*sizeof(word)]
   CALL_IPPASM     getAesGcmConst_table_ct ;;
   xor      rax, rbx                ;;
   shl      eax, 8
   psrldq   xmm0, 15
   movd     ecx, xmm0
   mov      rbx, rax                ;;xor      ax, word [r9 + rcx*sizeof(word)]
   CALL_IPPASM     getAesGcmConst_table_ct ;;
   xor      rax, rbx                ;;
   movd     xmm0, eax
   pxor     xmm0, xmm4

   add      rsi, sizeof(oword)      ; advance src address
   sub      rdx, sizeof(oword)      ; decrease counter
   jnz      .auth_loop               ; process next block

   movdqu   oword [rdi], xmm0    ; store hash value

   REST_XMM
   REST_GPR
   ret
ENDFUNC AesGcmAuth_table2K

%endif

