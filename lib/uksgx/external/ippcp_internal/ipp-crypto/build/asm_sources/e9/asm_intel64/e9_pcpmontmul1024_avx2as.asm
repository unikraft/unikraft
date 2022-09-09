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
;        cpMontMul1024_avx2()
;

%include "asmdefs.inc"
%include "ia_32e.inc"
;include pcpbnumulschool.inc
;include pcpvariant.inc

%if (_IPP32E >= _IPP32E_L9)

segment .text align=IPP_ALIGN_FACTOR

%assign DIGIT_BITS 27
%assign DIGIT_MASK (1 << DIGIT_BITS) -1

;*************************************************************
;* void cpMontMul1024_avx2(Ipp64u* pR,
;*                   const Ipp64u* pA,
;*                   const Ipp64u* pB,
;*                   const Ipp64u* pModulo, int mSize,
;*                         Ipp64u m0)
;*************************************************************
align IPP_ALIGN_FACTOR
IPPASM cpMontMul1024_avx2,PUBLIC
%assign LOCAL_FRAME sizeof(ymmword)
        USES_GPR rsi,rdi,rbx,rbp,r12,r13,r14
        USES_XMM_AVX ymm6,ymm7,ymm8,ymm9,ymm10,ymm11,ymm12,ymm13,ymm14
        COMP_ABI 6

      mov      rbp,rdx     ; pointer to B operand
      movsxd   r8, r8d     ; redLen value counter

;; clear results and buffers
      vzeroall

;; expands A and M operands
      vmovdqu  ymmword [rsi+r8*sizeof(qword)], ymm0
      vmovdqu  ymmword [rcx+r8*sizeof(qword)], ymm0

      xor      r10, r10                      ; ac0 = 0

      vmovdqu  ymmword [rsp], ymm0       ; {r3:r2:r1;R0} = 0

align IPP_ALIGN_FACTOR
;;
;; process b[] by quadruples (b[j], b[j+1], b[j+2] and b[j+3]) per pass
;;
.loop4_B:
      mov      rbx, qword [rbp]                 ; rbx = b[j]
      vpbroadcastq ymm0, qword [rbp]

      mov      r10, rbx                            ; ac0 = pa[0]*b[j]+pr[0]
      imul     r10, qword [rsi]
      add      r10, qword [rsp]
      mov      rdx, r10                            ; y0 = (ac0*m0) & DIGIT_MASK
      imul     edx, r9d
      and      edx, DIGIT_MASK
      mov      r11, rbx                            ; ac1 = pa[1]*b[j]+pr[1]
      imul     r11, qword [rsi+sizeof(qword)]
      add      r11, qword [rsp+sizeof(qword)]
      mov      r12, rbx                            ; ac2 = pa[2]*b[j]+pr[2]
      imul     r12, qword [rsi+sizeof(qword)*2]
      add      r12, qword [rsp+sizeof(qword)*2]
      mov      r13, rbx                            ; ac3 = pa[3]*b[j]+pr[3]
      imul     r13, qword [rsi+sizeof(qword)*3]
      add      r13, qword [rsp+sizeof(qword)*3]
      vmovd    xmm11, edx
      vpbroadcastq ymm11, xmm11

      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*2]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*3]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*4]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*5]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*6]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*7]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*8]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*9]
      vpaddq   ymm9,  ymm9, ymm14

      mov      rax, rdx                            ; ac0 += pn[0]*y0
      imul     rax, qword [rcx]
      add      r10, rax
      shr      r10, DIGIT_BITS
      mov      rax, rdx                            ; ac1 += pn[1]*y0
      imul     rax, qword [rcx+sizeof(qword)]
      add      r11, rax
      add      r11, r10
      mov      rax, rdx                            ; ac2 += pn[2]*y0
      imul     rax, qword [rcx+sizeof(qword)*2]
      add      r12, rax
      mov      rax, rdx                            ; ac3 += pn[3]*y0
      imul     rax, qword [rcx+sizeof(qword)*3]
      add      r13, rax

      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*2]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*3]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*4]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*5]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*6]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*7]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*8]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*9]
      vpaddq   ymm9,  ymm9, ymm14
;; ------------------------------------------------------------

      mov      rbx, qword [rbp+sizeof(qword)]   ; rbx = b[j+1]
      vpbroadcastq ymm0, qword [rbp+sizeof(qword)]
      mov      rax, rbx                            ; ac1 += pa[0]*b[j+1]
      imul     rax, qword [rsi]
      add      r11, rax
      mov      rdx, r11                            ; y1 = (ac1*m0) & DIGIT_MASK
      imul     edx, r9d
      and      edx, DIGIT_MASK
      mov      rax, rbx                            ; ac2 += pa[1]*b[j+1]
      imul     rax, qword [rsi+sizeof(qword)]
      add      r12, rax
      mov      rax, rbx                            ; ac3 += pa[2]*b[j+1]
      imul     rax, qword [rsi+sizeof(qword)*2]
      add      r13, rax
      vmovd    xmm11, edx
      vpbroadcastq ymm11, xmm11

      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)-sizeof(qword)]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*2-sizeof(qword)]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*3-sizeof(qword)]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*4-sizeof(qword)]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*5-sizeof(qword)]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*6-sizeof(qword)]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*7-sizeof(qword)]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*8-sizeof(qword)]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*9-sizeof(qword)]
      vpaddq   ymm9,  ymm9, ymm14

      mov      rax, rdx                            ; ac1 += pn[0]*y1
      imul     rax, qword [rcx]
      add      r11, rax
      shr      r11, DIGIT_BITS
      mov      rax, rdx                            ; ac2 += pn[1]*y1
      imul     rax, qword [rcx+sizeof(qword)]
      add      r12, rax
      add      r12, r11
      mov      rax, rdx                            ; ac3 += pn[2]*y1
      imul     rax, qword [rcx+sizeof(qword)*2]
      add      r13, rax

      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)-sizeof(qword)]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*2-sizeof(qword)]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*3-sizeof(qword)]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*4-sizeof(qword)]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*5-sizeof(qword)]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*6-sizeof(qword)]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*7-sizeof(qword)]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*8-sizeof(qword)]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*9-sizeof(qword)]
      vpaddq   ymm9,  ymm9, ymm14
      sub      r8, 2
      jz       .exit_loop_B
;; ------------------------------------------------------------

      mov      rbx, qword [rbp+sizeof(qword)*2] ; rbx = b[j+2]
      vpbroadcastq ymm0, qword [rbp+sizeof(qword)*2]
      mov      rax, rbx                            ; ac2 += pa[0]*b[j+2]
      imul     rax, qword [rsi]
      add      r12, rax
      mov      rdx, r12                            ; y2 = (ac2*m0) & DIGIT_MASK
      imul     edx, r9d
      and      edx, DIGIT_MASK
      mov      rax, rbx                            ; ac2 += pa[0]*b[j+2]
      imul     rax, qword [rsi+sizeof(qword)]
      add      r13, rax
      vmovd    xmm11, edx
      vpbroadcastq ymm11, xmm11

      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)-sizeof(qword)*2]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*2-sizeof(qword)*2]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*3-sizeof(qword)*2]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*4-sizeof(qword)*2]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*5-sizeof(qword)*2]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*6-sizeof(qword)*2]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*7-sizeof(qword)*2]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*8-sizeof(qword)*2]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*9-sizeof(qword)*2]
      vpaddq   ymm9,  ymm9, ymm14

      mov      rax, rdx                            ; ac2 += pn[0]*y2
      imul     rax, qword [rcx]
      add      r12, rax
      shr      r12, DIGIT_BITS
      mov      rax, rdx                            ; ac3 += pn[1]*y2
      imul     rax, qword [rcx+sizeof(qword)]
      add      r13, rax
      add      r13, r12

      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)-sizeof(qword)*2]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*2-sizeof(qword)*2]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*3-sizeof(qword)*2]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*4-sizeof(qword)*2]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*5-sizeof(qword)*2]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*6-sizeof(qword)*2]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*7-sizeof(qword)*2]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*8-sizeof(qword)*2]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*9-sizeof(qword)*2]
      vpaddq   ymm9,  ymm9, ymm14
;; ------------------------------------------------------------

      mov      rbx, qword [rbp+sizeof(qword)*3] ; rbx = b[j+3]
      vpbroadcastq ymm0, qword [rbp+sizeof(qword)*3]
      imul     rbx, qword [rsi]                 ; ac3 += pa[0]*b[j+3]
      add      r13, rbx
      mov      rdx, r13                            ; y3 = (ac3*m0) & DIGIT_MASK
      imul     edx, r9d
      and      edx, DIGIT_MASK
      vmovd    xmm11, edx
      vpbroadcastq ymm11, xmm11

      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)-sizeof(qword)*3]
      vpaddq   ymm1,  ymm1, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*2-sizeof(qword)*3]
      vpaddq   ymm2,  ymm2, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*3-sizeof(qword)*3]
      vpaddq   ymm3,  ymm3, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*4-sizeof(qword)*3]
      vpaddq   ymm4,  ymm4, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*5-sizeof(qword)*3]
      vpaddq   ymm5,  ymm5, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*6-sizeof(qword)*3]
      vpaddq   ymm6,  ymm6, ymm14
      vpmuludq ymm12, ymm0, ymmword [rsi+sizeof(ymmword)*7-sizeof(qword)*3]
      vpaddq   ymm7,  ymm7, ymm12
      vpmuludq ymm13, ymm0, ymmword [rsi+sizeof(ymmword)*8-sizeof(qword)*3]
      vpaddq   ymm8,  ymm8, ymm13
      vpmuludq ymm14, ymm0, ymmword [rsi+sizeof(ymmword)*9-sizeof(qword)*3]
      vpaddq   ymm9,  ymm9, ymm14
      vpmuludq ymm10, ymm0, ymmword [rsi+sizeof(ymmword)*10-sizeof(qword)*3]

      imul     rdx, qword [rcx]                 ; ac3 += pn[0]*y3
      add      r13, rdx
      shr      r13, DIGIT_BITS
      vmovq    xmm14, r13

      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)-sizeof(qword)*3]
      vpaddq   ymm1,  ymm1, ymm12
      vpaddq   ymm1,  ymm1, ymm14
      vmovdqu  ymmword [rsp], ymm1
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*2-sizeof(qword)*3]
      vpaddq   ymm1,  ymm2, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*3-sizeof(qword)*3]
      vpaddq   ymm2,  ymm3, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*4-sizeof(qword)*3]
      vpaddq   ymm3,  ymm4, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*5-sizeof(qword)*3]
      vpaddq   ymm4,  ymm5, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*6-sizeof(qword)*3]
      vpaddq   ymm5,  ymm6, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*7-sizeof(qword)*3]
      vpaddq   ymm6,  ymm7, ymm12
      vpmuludq ymm13, ymm11, ymmword [rcx+sizeof(ymmword)*8-sizeof(qword)*3]
      vpaddq   ymm7,  ymm8, ymm13
      vpmuludq ymm14, ymm11, ymmword [rcx+sizeof(ymmword)*9-sizeof(qword)*3]
      vpaddq   ymm8,  ymm9, ymm14
      vpmuludq ymm12, ymm11, ymmword [rcx+sizeof(ymmword)*10-sizeof(qword)*3]
      vpaddq   ymm9,  ymm10, ymm12
;; ------------------------------------------------------------

      add      rbp, sizeof(qword)*4
      sub      r8, 2
      jnz      .loop4_B

.exit_loop_B:
      mov      qword [rdi], r12
      mov      qword [rdi+sizeof(qword)], r13
      vmovdqu  ymmword [rdi+sizeof(qword)*2], ymm1
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)], ymm2
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*2], ymm3
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*3], ymm4
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*4], ymm5
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*5], ymm6
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*6], ymm7
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*7], ymm8
      vmovdqu  ymmword [rdi+sizeof(qword)*2+sizeof(ymmword)*8], ymm9

      ;;
      ;; normalize
      ;;
      mov      r8, dword 38
      xor      rax, rax
.norm_loop:
      add      rax, qword [rdi]
      add      rdi, sizeof(qword)
      mov      rdx, dword DIGIT_MASK
      and      rdx, rax
      shr      rax, DIGIT_BITS
      mov      qword [rdi-sizeof(qword)], rdx
      sub      r8, 1
      jg       .norm_loop
      mov      qword [rdi], rax

   REST_XMM_AVX
   REST_GPR
   ret
ENDFUNC cpMontMul1024_avx2


%endif       ;  _IPP32E_L9

