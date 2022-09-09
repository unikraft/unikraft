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
;               Big Number Operations
;
;     Content:
;        cpMontSqr1024_avx2()
;

%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_L9)

segment .text align=IPP_ALIGN_FACTOR


%assign DIGIT_BITS  27
%assign DIGIT_MASK  (1 << DIGIT_BITS) -1

;*************************************************************
;* void cpMontSqr1024_avx2(Ipp64u* pR,
;*                   const Ipp64u* pA,
;*                   const Ipp64u* pModulus, int mSize,
;*                         Ipp64u k0,
;*                         Ipp64u* pBuffer)
;*************************************************************

align IPP_ALIGN_FACTOR
IPPASM cpMontSqr1024_avx2,PUBLIC
%assign LOCAL_FRAME sizeof(qword)*7
        USES_GPR rsi,rdi,rbx,rbp,r12,r13,r14
        USES_XMM_AVX ymm6,ymm7,ymm8,ymm9,ymm10,ymm11,ymm12,ymm13,ymm14
        COMP_ABI 6

      movsxd   rcx, ecx             ; redLen value counter
      vpxor    ymm11, ymm11, ymm11

;; expands A and M operands
      vmovdqu  ymmword [rsi+rcx*sizeof(qword)], ymm11
      vmovdqu  ymmword [rdx+rcx*sizeof(qword)], ymm11

;
; stack struct
;
%assign pResult 0                     ; pointer to result
%assign pA      pResult+sizeof(qword) ; pointer to A operand
%assign pM      pA+sizeof(qword)      ; pointer to modulus
%assign redLen  pM+sizeof(qword)      ; length
%assign m0      redLen+sizeof(qword)  ; m0 value
%assign pA2     m0+sizeof(qword)      ; pointer to buffer (contains doubled input (A*2) and temporary result (A^2) )
%assign pAxA    pA2+sizeof(qword)     ; pointer to temporary result (A^2)

      mov      qword [rsp+pResult], rdi      ; store pointer to result
      mov      qword [rsp+pA], rsi           ; store pointer to input A
      mov      qword [rsp+pM], rdx           ; store pointer to modulus
      mov      qword [rsp+redLen], rcx       ; store redLen
      mov      qword [rsp+m0], r8            ; store m0 value

      mov      rcx, dword 40

      mov      rdi, r9
      mov      qword [rsp+pAxA], rdi         ; pointer to temporary result (low A^2)
      lea      rbx, [rdi+rcx*sizeof(qword)]     ; pointer to temporary result (high A^2)
      lea      r9, [rbx+rcx*sizeof(qword)]      ; pointer to doubled input (A*2)
      mov      qword [rsp+pA2], r9

      mov      rax, rsi
      mov      rcx, dword sizeof(ymmword)/sizeof(qword)

;; doubling input
      vmovdqu  ymm0, ymmword [rsi]
      vmovdqu  ymm1, ymmword [rsi+sizeof(ymmword)]
      vmovdqu  ymm2, ymmword [rsi+sizeof(ymmword)*2]
      vmovdqu  ymm3, ymmword [rsi+sizeof(ymmword)*3]
      vmovdqu  ymm4, ymmword [rsi+sizeof(ymmword)*4]
      vmovdqu  ymm5, ymmword [rsi+sizeof(ymmword)*5]
      vmovdqu  ymm6, ymmword [rsi+sizeof(ymmword)*6]
      vmovdqu  ymm7, ymmword [rsi+sizeof(ymmword)*7]
      vmovdqu  ymm8, ymmword [rsi+sizeof(ymmword)*8]
      vmovdqu  ymm9, ymmword [rsi+sizeof(ymmword)*9]

      vmovdqu  ymmword [r9], ymm0
      vpbroadcastq ymm10, qword [rax]                           ; ymm10 = {a0:a0:a0:a0}
      vpaddq   ymm1, ymm1, ymm1
      vmovdqu  ymmword [r9+sizeof(ymmword)], ymm1
      vpaddq   ymm2, ymm2, ymm2
      vmovdqu  ymmword [r9+sizeof(ymmword)*2], ymm2
      vpaddq   ymm3, ymm3, ymm3
      vmovdqu  ymmword [r9+sizeof(ymmword)*3], ymm3
      vpaddq   ymm4, ymm4, ymm4
      vmovdqu  ymmword [r9+sizeof(ymmword)*4], ymm4
      vpaddq   ymm5, ymm5, ymm5
      vmovdqu  ymmword [r9+sizeof(ymmword)*5], ymm5
      vpaddq   ymm6, ymm6, ymm6
      vmovdqu  ymmword [r9+sizeof(ymmword)*6], ymm6
      vpaddq   ymm7, ymm7, ymm7
      vmovdqu  ymmword [r9+sizeof(ymmword)*7], ymm7
      vpaddq   ymm8, ymm8, ymm8
      vmovdqu  ymmword [r9+sizeof(ymmword)*8], ymm8
      vpaddq   ymm9, ymm9, ymm9
      vmovdqu  ymmword [r9+sizeof(ymmword)*9], ymm9

;;
;; squaring
;;
   vpmuludq    ymm0, ymm10, ymmword [rsi]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*4]           ; ymm14 = {a4:a4:a4:a4}
   vmovdqu     ymmword [rbx], ymm11
   vpmuludq    ymm1, ymm10, ymmword [r9+sizeof(ymmword)]
   vmovdqu     ymmword [rbx+sizeof(ymmword)], ymm11
   vpmuludq    ymm2, ymm10, ymmword [r9+sizeof(ymmword)*2]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*2], ymm11
   vpmuludq    ymm3, ymm10, ymmword [r9+sizeof(ymmword)*3]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*3], ymm11
   vpmuludq    ymm4, ymm10, ymmword [r9+sizeof(ymmword)*4]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*4], ymm11
   vpmuludq    ymm5, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*5], ymm11
   vpmuludq    ymm6, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*6], ymm11
   vpmuludq    ymm7, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*7], ymm11
   vpmuludq    ymm8, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*8], ymm11
   vpmuludq    ymm9, ymm10, ymmword [r9+sizeof(ymmword)*9]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*9], ymm11

   jmp      .sqr1024_ep

align IPP_ALIGN_FACTOR
.sqr1024_loop4:
   vpmuludq    ymm0, ymm10, ymmword [rsi]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*4]           ; ymm14 = {a4:a4:a4:a4}
   vpaddq      ymm0, ymm0, ymmword [rdi]

   vpmuludq    ymm1, ymm10, ymmword [r9+sizeof(ymmword)]
   vpaddq      ymm1, ymm1, ymmword [rdi+sizeof(ymmword)]

   vpmuludq    ymm2, ymm10, ymmword [r9+sizeof(ymmword)*2]
   vpaddq      ymm2, ymm2, ymmword [rdi+sizeof(ymmword)*2]

   vpmuludq    ymm3, ymm10, ymmword [r9+sizeof(ymmword)*3]
   vpaddq      ymm3, ymm3, ymmword [rdi+sizeof(ymmword)*3]

   vpmuludq    ymm4, ymm10, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm4, ymm4, ymmword [rdi+sizeof(ymmword)*4]

   vpmuludq    ymm5, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm5, ymm5, ymmword [rdi+sizeof(ymmword)*5]

   vpmuludq    ymm6, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm6, ymm6, ymmword [rdi+sizeof(ymmword)*6]

   vpmuludq    ymm7, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm7, ymm7, ymmword [rdi+sizeof(ymmword)*7]

   vpmuludq    ymm8, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm8, ymm8, ymmword [rdi+sizeof(ymmword)*8]

   vpmuludq    ymm9, ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm9, ymm9, ymmword [rdi+sizeof(ymmword)*9]

.sqr1024_ep:
   vmovdqu     ymmword [rdi], ymm0
   vmovdqu     ymmword [rdi+sizeof(ymmword)], ymm1

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*8]           ; ymm10 = {a8:a8:a8:a8}
   vpaddq      ymm2, ymm2, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*2]
   vpaddq      ymm3, ymm3, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*3]
   vpaddq      ymm4, ymm4, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm5, ymm5, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm6, ymm6, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm7, ymm7, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm8, ymm8, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm9, ymm9, ymm12
   vpmuludq    ymm0,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm0, ymm0,  ymmword [rbx]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*2], ymm2
   vmovdqu     ymmword [rdi+sizeof(ymmword)*3], ymm3

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*2]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*12]          ; ymm14 = {a12:a12:a12:a12}
   vpaddq      ymm4, ymm4, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*3]
   vpaddq      ymm5, ymm5, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm6, ymm6, ymm13
   vpmuludq    ymm11, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm7, ymm7, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm8, ymm8, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm9, ymm9, ymm13
   vpmuludq    ymm11, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm0, ymm0, ymm11
   vpmuludq    ymm1,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm1, ymm1,  ymmword [rbx+sizeof(ymmword)]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*4], ymm4
   vmovdqu     ymmword [rdi+sizeof(ymmword)*5], ymm5

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)*3]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*16]          ; ymm10 = {a16:a16:a16:a16}
   vpaddq      ymm6, ymm6, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm7, ymm7, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm8, ymm8, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm9, ymm9, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm0, ymm0, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm1, ymm1, ymm13
   vpmuludq    ymm2,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm2, ymm2,  ymmword [rbx+sizeof(ymmword)*2]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*6], ymm6
   vmovdqu     ymmword [rdi+sizeof(ymmword)*7], ymm7

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*4]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*20]          ; ymm14 = {a20:a20:a20:a20}
   vpaddq      ymm8, ymm8, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm9, ymm9, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm0, ymm0, ymm13
   vpmuludq    ymm11, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm1, ymm1, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm2, ymm2, ymm12
   vpmuludq    ymm3,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm3, ymm3,  ymmword [rbx+sizeof(ymmword)*3]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*8], ymm8
   vmovdqu     ymmword [rdi+sizeof(ymmword)*9], ymm9

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)*5]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*24]          ; ymm10 = {a24:a24:a24:a24}
   vpaddq      ymm0, ymm0, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm1, ymm1, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm2, ymm2, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm3, ymm3, ymm11
   vpmuludq    ymm4,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm4, ymm4,  ymmword [rbx+sizeof(ymmword)*4]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*10], ymm0
   vmovdqu     ymmword [rdi+sizeof(ymmword)*11], ymm1

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*6]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*28]          ; ymm14 = {a28:a28:a28:a28}
   vpaddq      ymm2, ymm2, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm3, ymm3, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm4, ymm4, ymm13
   vpmuludq    ymm5,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm5, ymm5,  ymmword [rbx+sizeof(ymmword)*5]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*12], ymm2
   vmovdqu     ymmword [rdi+sizeof(ymmword)*13], ymm3

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)*7]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*32]          ; ymm10 = {a32:a32:a32:a32}
   vpaddq      ymm4, ymm4, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm5, ymm5, ymm12
   vpmuludq    ymm6,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm6, ymm6,  ymmword [rbx+sizeof(ymmword)*6]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*14], ymm4
   vmovdqu     ymmword [rdi+sizeof(ymmword)*15], ymm5

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*8]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*36]          ; ymm14 = {a36:a36:a36:a36}
   vpaddq      ymm6, ymm6, ymm11
   vpmuludq    ymm7,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm7, ymm7,  ymmword [rbx+sizeof(ymmword)*7]

   vpmuludq    ymm8,  ymm14, ymmword [rsi+sizeof(ymmword)*9]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)]             ; ymm10 = {a[1/2/3]:a[1/2/3]:a[1/2/3]:a[1/2/3]}
   vpaddq      ymm8, ymm8,  ymmword [rbx+sizeof(ymmword)*8]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*16], ymm6
   vmovdqu     ymmword [rdi+sizeof(ymmword)*17], ymm7
   vmovdqu     ymmword [rdi+sizeof(ymmword)*18], ymm8

   add         rdi, sizeof(qword)
   add         rbx, sizeof(qword)
   add         rax, sizeof(qword)
   sub         rcx, 1
   jnz         .sqr1024_loop4

;;
;; reduction
;;
   mov      rdi, qword [rsp+pAxA]                  ; restore pointer to temporary result (low A^2)
   mov      rcx, qword [rsp+pM]                    ; restore pointer to modulus
   mov      r8, qword [rsp+m0]                     ; restore m0 value
   mov      r9,  dword 38                                   ; modulus length

   mov      r10, qword [rdi]                       ; load low part of temporary result
   mov      r11, qword [rdi+sizeof(qword)]         ;
   mov      r12, qword [rdi+sizeof(qword)*2]       ;
   mov      r13, qword [rdi+sizeof(qword)*3]       ;

   mov      rdx, r10                                  ; y0 = (ac0*k0) & DIGIT_MASK
   imul     edx, r8d
   and      edx, DIGIT_MASK
   vmovd    xmm10, edx

   vmovdqu  ymm1, ymmword [rdi+sizeof(ymmword)*1]  ; load other data
   vmovdqu  ymm2, ymmword [rdi+sizeof(ymmword)*2]  ;
   vmovdqu  ymm3, ymmword [rdi+sizeof(ymmword)*3]  ;
   vmovdqu  ymm4, ymmword [rdi+sizeof(ymmword)*4]  ;
   vmovdqu  ymm5, ymmword [rdi+sizeof(ymmword)*5]  ;
   vmovdqu  ymm6, ymmword [rdi+sizeof(ymmword)*6]  ;

   mov      rax, rdx                                  ; ac0 += pn[0]*y0
   imul     rax, qword [rcx]
   add      r10, rax

   vpbroadcastq ymm10, xmm10

   vmovdqu  ymm7, ymmword [rdi+sizeof(ymmword)*7]  ; load other data
   vmovdqu  ymm8, ymmword [rdi+sizeof(ymmword)*8]  ;
   vmovdqu  ymm9, ymmword [rdi+sizeof(ymmword)*9]  ;

   mov      rax, rdx                                  ; ac1 += pn[1]*y0
   imul     rax, qword [rcx+sizeof(qword)]
   add      r11, rax

   mov      rax, rdx                                  ; ac2 += pn[2]*y0
   imul     rax, qword [rcx+sizeof(qword)*2]
   add      r12, rax

   shr      r10, DIGIT_BITS                           ; updtae ac1

   imul     rdx, qword [rcx+sizeof(qword)*3]       ; ac3 += pn[3]*y0
   add      r13, rdx
   add      r11, r10                                  ; updtae ac1

   mov      rdx, r11                                  ; y1 = (ac1*k0) & DIGIT_MASK
   imul     edx, r8d
   and      rdx, DIGIT_MASK

align IPP_ALIGN_FACTOR
.reduction_loop:
   vmovd    xmm11, edx
   vpbroadcastq ymm11, xmm11

   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)]
   mov      rax, rdx                            ; ac1 += pn[0]*y1
   imul     rax, qword [rcx]
   vpaddq   ymm1,  ymm1, ymm14
   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*2]
   vpaddq   ymm2,  ymm2, ymm14
   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*3]
   vpaddq   ymm3,  ymm3, ymm14

   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*4]
   add      r11, rax
   shr      r11, DIGIT_BITS                     ; update ac2
   vpaddq   ymm4,  ymm4, ymm14

   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*5]
   mov      rax, rdx                            ; ac2 += pn[1]*y1
   imul     rax, qword [rcx+sizeof(qword)]
   vpaddq   ymm5,  ymm5, ymm14
   add      r12, rax

   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*6]
   imul     rdx, qword [rcx+sizeof(qword)*2] ; ac3 += pn[2]*y1
   add      r12, r11
   vpaddq   ymm6,  ymm6, ymm14
   add      r13, rdx

   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*7]
   mov      rdx, r12                            ; y2 = (ac2*m0) & DIGIT_MASK
   imul     edx, r8d
   vpaddq   ymm7,  ymm7, ymm14
   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*8]
   vpaddq   ymm8,  ymm8, ymm14
   and      rdx, DIGIT_MASK

   vpmuludq ymm14, ymm10, ymmword [rcx+sizeof(ymmword)*9]
   vpaddq   ymm9,  ymm9, ymm14
;; ------------------------------------------------------------

   vmovd    xmm12, edx
   vpbroadcastq ymm12, xmm12

   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)-sizeof(qword)]
   mov      rax, rdx                            ; ac2 += pn[0]*y2
   imul     rax, qword [rcx]
   vpaddq   ymm1,  ymm1, ymm14

   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*2-sizeof(qword)]
   vpaddq   ymm2,  ymm2, ymm14

   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*3-sizeof(qword)]
   vpaddq   ymm3,  ymm3, ymm14
   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*4-sizeof(qword)]
   vpaddq   ymm4,  ymm4, ymm14
   add      rax, r12
   shr      rax, DIGIT_BITS                     ; update ac3


   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*5-sizeof(qword)]
   imul     rdx, qword [rcx+sizeof(qword)]   ; ac3 += pn[1]*y2
   vpaddq   ymm5,  ymm5, ymm14
   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*6-sizeof(qword)]
   vpaddq   ymm6,  ymm6, ymm14
   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*7-sizeof(qword)]
   vpaddq   ymm7,  ymm7, ymm14
   add      rdx, r13
   add      rdx, rax                            ; update ac3

   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*8-sizeof(qword)]
   vpaddq   ymm8,  ymm8, ymm14
   vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*9-sizeof(qword)]
   vpaddq   ymm9,  ymm9, ymm14

   sub      r9, 2
   jz       .exit_reduction_loop
;; ------------------------------------------------------------

   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)-sizeof(qword)*2]
   mov      r13, rdx                            ; y3 = (ac3*m0) & DIGIT_MASK
   imul     edx, r8d
   vpaddq   ymm1,  ymm1, ymm14
   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*2-sizeof(qword)*2]
   vpaddq   ymm2,  ymm2, ymm14
   and      rdx, DIGIT_MASK

   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*3-sizeof(qword)*2]
   vmovd    xmm13, edx
   vpaddq   ymm3,  ymm3, ymm14
   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*4-sizeof(qword)*2]
   vpbroadcastq ymm13, xmm13
   vpaddq   ymm4,  ymm4, ymm14

   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*5-sizeof(qword)*2]
   imul     rdx, qword [rcx]                 ; ac3 += pn[0]*y3
   vpaddq   ymm5,  ymm5, ymm14
   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*6-sizeof(qword)*2]
   vpaddq   ymm6,  ymm6, ymm14
   add      r13, rdx

   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*7-sizeof(qword)*2]
   vpaddq   ymm7,  ymm7, ymm14
   shr      r13, DIGIT_BITS
   vmovq    xmm0, r13

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)-sizeof(qword)*3]
   vpaddq   ymm1,  ymm1, ymm0
   vpaddq   ymm1,  ymm1, ymm14
   vmovdqu  ymmword [rdi], ymm1

   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*8-sizeof(qword)*2]
   vpaddq   ymm8,  ymm8, ymm14

   vpmuludq ymm14, ymm12, ymmword [rcx+sizeof(ymmword)*9-sizeof(qword)*2]
   vpaddq   ymm9,  ymm9, ymm14
;; ------------------------------------------------------------

   vmovq    rdx, xmm1                           ; y0 = (ac0*k0) & DIGIT_MASK
   imul     edx, r8d
   and      edx, DIGIT_MASK

   vmovq    r10, xmm1                           ; update lowest part of temporary result
   mov      r11, qword [rdi+sizeof(qword)]   ;
   mov      r12, qword [rdi+sizeof(qword)*2] ;
   mov      r13, qword [rdi+sizeof(qword)*3] ;

   vmovd    xmm10, edx
   vpbroadcastq ymm10, xmm10

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*2-sizeof(qword)*3]
   mov      rax, rdx                            ; ac0 += pn[0]*y0
   imul     rax, qword [rcx]
   vpaddq   ymm1,  ymm2, ymm14
   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*3-sizeof(qword)*3]
   add      r10, rax
   vpaddq   ymm2,  ymm3, ymm14
   shr      r10, DIGIT_BITS                     ; updtae ac1

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*4-sizeof(qword)*3]
   mov      rax, rdx                            ; ac1 += pn[1]*y0
   imul     rax, qword [rcx+sizeof(qword)]
   vpaddq   ymm3,  ymm4, ymm14
   add      r11, rax

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*5-sizeof(qword)*3]
   mov      rax, rdx                            ; ac2 += pn[2]*y0
   imul     rax, qword [rcx+sizeof(qword)*2]
   vpaddq   ymm4,  ymm5, ymm14
   add      r12, rax

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*6-sizeof(qword)*3]
   imul     rdx, qword [rcx+sizeof(qword)*3] ; ac3 += pn[3]*y0
   vpaddq   ymm5,  ymm6, ymm14
   add      r13, rdx
   add      r11, r10

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*7-sizeof(qword)*3]
   mov      rdx, r11                            ; y1 = (ac1*k0) & DIGIT_MASK
   imul     edx, r8d
   vpaddq   ymm6,  ymm7, ymm14
   and      rdx, DIGIT_MASK

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*8-sizeof(qword)*3]
   vpaddq   ymm7,  ymm8, ymm14

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*9-sizeof(qword)*3]
   vpaddq   ymm8,  ymm9, ymm14

   vpmuludq ymm14, ymm13, ymmword [rcx+sizeof(ymmword)*10-sizeof(qword)*3]
   vpaddq   ymm9,  ymm14, ymmword [rdi+sizeof(ymmword)*10]

   add      rdi, sizeof(qword)*4
   sub      r9, 2
   jnz      .reduction_loop

.exit_reduction_loop:
   mov      rdi, qword [rsp+pResult]      ; restore pointer to result
   mov      qword [rdi], r12
   mov      qword [rdi+sizeof(qword)], r13
   vmovdqu  ymmword [rdi+sizeof(ymmword)-sizeof(qword)*2], ymm1
   vmovdqu  ymmword [rdi+sizeof(ymmword)*2-sizeof(qword)*2], ymm2
   vmovdqu  ymmword [rdi+sizeof(ymmword)*3-sizeof(qword)*2], ymm3
   vmovdqu  ymmword [rdi+sizeof(ymmword)*4-sizeof(qword)*2], ymm4
   vmovdqu  ymmword [rdi+sizeof(ymmword)*5-sizeof(qword)*2], ymm5
   vmovdqu  ymmword [rdi+sizeof(ymmword)*6-sizeof(qword)*2], ymm6
   vmovdqu  ymmword [rdi+sizeof(ymmword)*7-sizeof(qword)*2], ymm7
   vmovdqu  ymmword [rdi+sizeof(ymmword)*8-sizeof(qword)*2], ymm8
   vmovdqu  ymmword [rdi+sizeof(ymmword)*9-sizeof(qword)*2], ymm9

;;
;; normalization
;;
      mov      r9, dword 38
      xor      rax, rax
.norm_loop:
      add      rax, qword [rdi]
      add      rdi, sizeof(qword)
      mov      rdx, dword DIGIT_MASK
      and      rdx, rax
      shr      rax, DIGIT_BITS
      mov      qword [rdi-sizeof(qword)], rdx
      sub      r9, 1
      jg       .norm_loop
      mov      qword [rdi], rax

   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC cpMontSqr1024_avx2

;*************************************************************
;* void cpSqr1024_avx2(Ipp64u* pR,
;*               const Ipp64u* pA, int nsA,
;*                     Ipp64u* pBuffer)
;*************************************************************

align IPP_ALIGN_FACTOR
IPPASM cpSqr1024_avx2,PUBLIC
%assign LOCAL_FRAME sizeof(qword)*5
        USES_GPR rsi,rdi,rbx
        USES_XMM_AVX ymm6,ymm7,ymm8,ymm9,ymm10,ymm11,ymm12,ymm13,ymm14
        COMP_ABI 4

      movsxd   rdx, edx             ; redLen value counter
      vpxor    ymm11, ymm11, ymm11

;; expands A operand
      vmovdqu  ymmword [rsi+rdx*sizeof(qword)], ymm11

;
; stack struct
;
%assign pResult  0                        ; pointer to result
%assign pA  pResult+sizeof(qword)    ; pointer to A operand
%assign redLen  pA+sizeof(qword)         ; length
%assign pA2  redLen+sizeof(qword)     ; pointer to buffer (contains doubled input (A*2) and temporary result (A^2) )
%assign pAxA  pA2+sizeof(qword)        ; pointer to temporary result (A^2)

      mov      qword [rsp+pResult], rdi      ; store pointer to result
      mov      qword [rsp+pA], rsi           ; store pointer to input A
      mov      qword [rsp+redLen], rdx       ; store redLen

      mov      rdx, dword 40

      mov      rdi, rcx                         ; pointer to buffer
      mov      qword [rsp+pAxA], rdi         ; pointer to temporary result (low A^2)
      lea      rbx, [rdi+rdx*sizeof(qword)]     ; pointer to temporary result (high A^2)
      lea      r9, [rbx+rdx*sizeof(qword)]      ; pointer to doubled input (A*2)
      mov      qword [rsp+pA2], r9

      mov      rax, rsi
      mov      rcx, dword sizeof(ymmword)/sizeof(qword)

;; doubling input
      vmovdqu  ymm0, ymmword [rsi]
      vmovdqu  ymm1, ymmword [rsi+sizeof(ymmword)]
      vmovdqu  ymm2, ymmword [rsi+sizeof(ymmword)*2]
      vmovdqu  ymm3, ymmword [rsi+sizeof(ymmword)*3]
      vmovdqu  ymm4, ymmword [rsi+sizeof(ymmword)*4]
      vmovdqu  ymm5, ymmword [rsi+sizeof(ymmword)*5]
      vmovdqu  ymm6, ymmword [rsi+sizeof(ymmword)*6]
      vmovdqu  ymm7, ymmword [rsi+sizeof(ymmword)*7]
      vmovdqu  ymm8, ymmword [rsi+sizeof(ymmword)*8]
      vmovdqu  ymm9, ymmword [rsi+sizeof(ymmword)*9]

      vmovdqu  ymmword [r9], ymm0
      vpbroadcastq ymm10, qword [rax]                           ; ymm10 = {a0:a0:a0:a0}
      vpaddq   ymm1, ymm1, ymm1
      vmovdqu  ymmword [r9+sizeof(ymmword)], ymm1
      vpaddq   ymm2, ymm2, ymm2
      vmovdqu  ymmword [r9+sizeof(ymmword)*2], ymm2
      vpaddq   ymm3, ymm3, ymm3
      vmovdqu  ymmword [r9+sizeof(ymmword)*3], ymm3
      vpaddq   ymm4, ymm4, ymm4
      vmovdqu  ymmword [r9+sizeof(ymmword)*4], ymm4
      vpaddq   ymm5, ymm5, ymm5
      vmovdqu  ymmword [r9+sizeof(ymmword)*5], ymm5
      vpaddq   ymm6, ymm6, ymm6
      vmovdqu  ymmword [r9+sizeof(ymmword)*6], ymm6
      vpaddq   ymm7, ymm7, ymm7
      vmovdqu  ymmword [r9+sizeof(ymmword)*7], ymm7
      vpaddq   ymm8, ymm8, ymm8
      vmovdqu  ymmword [r9+sizeof(ymmword)*8], ymm8
      vpaddq   ymm9, ymm9, ymm9
      vmovdqu  ymmword [r9+sizeof(ymmword)*9], ymm9

;;
;; squaring
;;
   vpmuludq    ymm0, ymm10, ymmword [rsi]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*4]           ; ymm14 = {a4:a4:a4:a4}
   vmovdqu     ymmword [rbx], ymm11
   vpmuludq    ymm1, ymm10, ymmword [r9+sizeof(ymmword)]
   vmovdqu     ymmword [rbx+sizeof(ymmword)], ymm11
   vpmuludq    ymm2, ymm10, ymmword [r9+sizeof(ymmword)*2]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*2], ymm11
   vpmuludq    ymm3, ymm10, ymmword [r9+sizeof(ymmword)*3]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*3], ymm11
   vpmuludq    ymm4, ymm10, ymmword [r9+sizeof(ymmword)*4]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*4], ymm11
   vpmuludq    ymm5, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*5], ymm11
   vpmuludq    ymm6, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*6], ymm11
   vpmuludq    ymm7, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*7], ymm11
   vpmuludq    ymm8, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*8], ymm11
   vpmuludq    ymm9, ymm10, ymmword [r9+sizeof(ymmword)*9]
   vmovdqu     ymmword [rbx+sizeof(ymmword)*9], ymm11

   jmp      .sqr1024_ep

align IPP_ALIGN_FACTOR
.sqr1024_loop4:
   vpmuludq    ymm0, ymm10, ymmword [rsi]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*4]           ; ymm14 = {a4:a4:a4:a4}
   vpaddq      ymm0, ymm0, ymmword [rdi]

   vpmuludq    ymm1, ymm10, ymmword [r9+sizeof(ymmword)]
   vpaddq      ymm1, ymm1, ymmword [rdi+sizeof(ymmword)]

   vpmuludq    ymm2, ymm10, ymmword [r9+sizeof(ymmword)*2]
   vpaddq      ymm2, ymm2, ymmword [rdi+sizeof(ymmword)*2]

   vpmuludq    ymm3, ymm10, ymmword [r9+sizeof(ymmword)*3]
   vpaddq      ymm3, ymm3, ymmword [rdi+sizeof(ymmword)*3]

   vpmuludq    ymm4, ymm10, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm4, ymm4, ymmword [rdi+sizeof(ymmword)*4]

   vpmuludq    ymm5, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm5, ymm5, ymmword [rdi+sizeof(ymmword)*5]

   vpmuludq    ymm6, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm6, ymm6, ymmword [rdi+sizeof(ymmword)*6]

   vpmuludq    ymm7, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm7, ymm7, ymmword [rdi+sizeof(ymmword)*7]

   vpmuludq    ymm8, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm8, ymm8, ymmword [rdi+sizeof(ymmword)*8]

   vpmuludq    ymm9, ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm9, ymm9, ymmword [rdi+sizeof(ymmword)*9]

.sqr1024_ep:
   vmovdqu     ymmword [rdi], ymm0
   vmovdqu     ymmword [rdi+sizeof(ymmword)], ymm1

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*8]           ; ymm10 = {a8:a8:a8:a8}
   vpaddq      ymm2, ymm2, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*2]
   vpaddq      ymm3, ymm3, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*3]
   vpaddq      ymm4, ymm4, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm5, ymm5, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm6, ymm6, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm7, ymm7, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm8, ymm8, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm9, ymm9, ymm12
   vpmuludq    ymm0,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm0, ymm0,  ymmword [rbx]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*2], ymm2
   vmovdqu     ymmword [rdi+sizeof(ymmword)*3], ymm3

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*2]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*12]          ; ymm14 = {a12:a12:a12:a12}
   vpaddq      ymm4, ymm4, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*3]
   vpaddq      ymm5, ymm5, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm6, ymm6, ymm13
   vpmuludq    ymm11, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm7, ymm7, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm8, ymm8, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm9, ymm9, ymm13
   vpmuludq    ymm11, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm0, ymm0, ymm11
   vpmuludq    ymm1,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm1, ymm1,  ymmword [rbx+sizeof(ymmword)]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*4], ymm4
   vmovdqu     ymmword [rdi+sizeof(ymmword)*5], ymm5

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)*3]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*16]          ; ymm10 = {a16:a16:a16:a16}
   vpaddq      ymm6, ymm6, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*4]
   vpaddq      ymm7, ymm7, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm8, ymm8, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm9, ymm9, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm0, ymm0, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm1, ymm1, ymm13
   vpmuludq    ymm2,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm2, ymm2,  ymmword [rbx+sizeof(ymmword)*2]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*6], ymm6
   vmovdqu     ymmword [rdi+sizeof(ymmword)*7], ymm7

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*4]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*20]          ; ymm14 = {a20:a20:a20:a20}
   vpaddq      ymm8, ymm8, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*5]
   vpaddq      ymm9, ymm9, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm0, ymm0, ymm13
   vpmuludq    ymm11, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm1, ymm1, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm2, ymm2, ymm12
   vpmuludq    ymm3,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm3, ymm3,  ymmword [rbx+sizeof(ymmword)*3]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*8], ymm8
   vmovdqu     ymmword [rdi+sizeof(ymmword)*9], ymm9

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)*5]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*24]          ; ymm10 = {a24:a24:a24:a24}
   vpaddq      ymm0, ymm0, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*6]
   vpaddq      ymm1, ymm1, ymm12
   vpmuludq    ymm13, ymm14, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm2, ymm2, ymm13
   vpmuludq    ymm11, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm3, ymm3, ymm11
   vpmuludq    ymm4,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm4, ymm4,  ymmword [rbx+sizeof(ymmword)*4]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*10], ymm0
   vmovdqu     ymmword [rdi+sizeof(ymmword)*11], ymm1

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*6]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*28]          ; ymm14 = {a28:a28:a28:a28}
   vpaddq      ymm2, ymm2, ymm11
   vpmuludq    ymm12, ymm10, ymmword [r9+sizeof(ymmword)*7]
   vpaddq      ymm3, ymm3, ymm12
   vpmuludq    ymm13, ymm10, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm4, ymm4, ymm13
   vpmuludq    ymm5,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm5, ymm5,  ymmword [rbx+sizeof(ymmword)*5]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*12], ymm2
   vmovdqu     ymmword [rdi+sizeof(ymmword)*13], ymm3

   vpmuludq    ymm11, ymm14, ymmword [rsi+sizeof(ymmword)*7]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)*32]          ; ymm10 = {a32:a32:a32:a32}
   vpaddq      ymm4, ymm4, ymm11
   vpmuludq    ymm12, ymm14, ymmword [r9+sizeof(ymmword)*8]
   vpaddq      ymm5, ymm5, ymm12
   vpmuludq    ymm6,  ymm14, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm6, ymm6,  ymmword [rbx+sizeof(ymmword)*6]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*14], ymm4
   vmovdqu     ymmword [rdi+sizeof(ymmword)*15], ymm5

   vpmuludq    ymm11, ymm10, ymmword [rsi+sizeof(ymmword)*8]
   vpbroadcastq ymm14, qword [rax+sizeof(qword)*36]          ; ymm14 = {a36:a36:a36:a36}
   vpaddq      ymm6, ymm6, ymm11
   vpmuludq    ymm7,  ymm10, ymmword [r9+sizeof(ymmword)*9]
   vpaddq      ymm7, ymm7,  ymmword [rbx+sizeof(ymmword)*7]

   vpmuludq    ymm8,  ymm14, ymmword [rsi+sizeof(ymmword)*9]
   vpbroadcastq ymm10, qword [rax+sizeof(qword)]             ; ymm10 = {a[1/2/3]:a[1/2/3]:a[1/2/3]:a[1/2/3]}
   vpaddq      ymm8, ymm8,  ymmword [rbx+sizeof(ymmword)*8]

   vmovdqu     ymmword [rdi+sizeof(ymmword)*16], ymm6
   vmovdqu     ymmword [rdi+sizeof(ymmword)*17], ymm7
   vmovdqu     ymmword [rdi+sizeof(ymmword)*18], ymm8

   add         rdi, sizeof(qword)
   add         rbx, sizeof(qword)
   add         rax, sizeof(qword)
   sub         rcx, 1
   jnz         .sqr1024_loop4


;;
;; normalization
;;
      mov      rsi, qword [rsp+pAxA]
      mov      rdi, qword [rsp+pResult]
      mov      r9, dword 38*2
      xor      rax, rax
.norm_loop:
      add      rax, qword [rsi]
      add      rsi, sizeof(qword)
      mov      rdx, dword DIGIT_MASK
      and      rdx, rax
      shr      rax, DIGIT_BITS
      mov      qword [rdi], rdx
      add      rdi, sizeof(qword)
      sub      r9, 1
      jg       .norm_loop
      mov      qword [rdi], rax

   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC cpSqr1024_avx2

%endif       ;  _IPP32E_L9

