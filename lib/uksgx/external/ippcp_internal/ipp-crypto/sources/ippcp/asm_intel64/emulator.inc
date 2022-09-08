;===============================================================================
; Copyright 2009-2021 Intel Corporation
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
;     Purpose:  EM64T Cryptography Primitive.
;
;
;

%ifndef _EMULATOR_INC_
%define _EMULATOR_INC_

%macro my_pclmulqdq 3.nolist
  %xdefine %%xxDst %1
  %xdefine %%xxSrc %2
  %xdefine %%xxOp  %3

  %if (my_emulator == 0)
    pclmulqdq   %%xxDst, %%xxSrc, %%xxOp
  %else
;;
;; rsp
;; registers <rax,rbx,rcx,rdx,rdi,rsi,rbp,r8,..,r15>
;; +00  => xxDst
;; +16  => xxSrc

    pushf
    push     rax
    push     rbx
    push     rcx
    push     rdx
    push     rdi
    push     rsi
    push     rbp
    push     r8
    push     r9
    push     r10
    push     r11
    push     r12
    push     r13
    push     r14
    push     r15

    %assign %%stackSize (sizeof(oword)*2)
    sub      rsp,%%stackSize

    movdqu   oword [rsp+00], %%xxDst  ;; save Dst
    movdqu   oword [rsp+16], %%xxSrc  ;; save Src

    lea      rcx, [rsp+00]
    lea      rdx, [rsp+16]
    mov      r8, %%xxOp

    sub      rsp, (sizeof(qword)*3)
    call     emu_pclmulqdq
    add      rsp, (sizeof(qword)*3)

    movdqu   %%xxDst, oword [rsp+00]  ;; return Dst
   ;movdqu   xxSrc, oword [rsp+16]  ;; return Src
    add      esp, %%stackSize

    pop      r15
    pop      r14
    pop      r13
    pop      r12
    pop      r11
    pop      r10
    pop      r9
    pop      r8
    pop      rbp
    pop      rsi
    pop      rdi
    pop      rdx
    pop      rcx
    pop      rbx
    pop      rax
    popf
  %endif
%endmacro

%macro my_aesenc 2.nolist
  %xdefine %%xxDst %1
  %xdefine %%xxSrc %2

  %if (my_emulator == 0)
    aesenc %%xxDst, %%xxSrc
  %else
    pushf
    push     rax
    push     rbx
    push     rcx
    push     rdx
    push     rdi
    push     rsi
    push     rbp
    push     r8
    push     r9
    push     r10
    push     r11
    push     r12
    push     r13
    push     r14
    push     r15

    %assign %%stackSize (sizeof(oword)*2)
    sub      rsp,%%stackSize

    movdqu   oword [rsp+00], %%xxDst  ;; save Dst
    movdqu   oword [rsp+16], %%xxSrc  ;; save Src

    lea      rcx, [rsp+00]
    lea      rdx, [rsp+16]

    sub      rsp, (sizeof(qword)*2)
    call     emu_aesenc
    add      rsp, (sizeof(qword)*2)

    movdqu   %%xxDst, oword [rsp+00]  ;; return Dst
    add      esp, %%stackSize

    pop      r15
    pop      r14
    pop      r13
    pop      r12
    pop      r11
    pop      r10
    pop      r9
    pop      r8
    pop      rbp
    pop      rsi
    pop      rdi
    pop      rdx
    pop      rcx
    pop      rbx
    pop      rax
    popf
  %endif
%endmacro

%macro my_aesenclast 2.nolist
  %xdefine %%xxDst %1
  %xdefine %%xxSrc %2

  %if (my_emulator == 0)
    aesenclast %%xxDst, %%xxSrc
  %else
    pushf
    push     rax
    push     rbx
    push     rcx
    push     rdx
    push     rdi
    push     rsi
    push     rbp
    push     r8
    push     r9
    push     r10
    push     r11
    push     r12
    push     r13
    push     r14
    push     r15

    %assign %%stackSize (sizeof(oword)*2)
    sub      rsp,%%stackSize

    movdqu   oword [rsp+00], %%xxDst  ;; save Dst
    movdqu   oword [rsp+16], %%xxSrc  ;; save Src

    lea      rcx, [rsp+00]
    lea      rdx, [rsp+16]

    sub      rsp, (sizeof(qword)*2)
    call     emu_aesenclast
    add      rsp, (sizeof(qword)*2)

    movdqu   %%xxDst, oword [rsp+00]  ;; return Dst
    add      esp, %%stackSize

    pop      r15
    pop      r14
    pop      r13
    pop      r12
    pop      r11
    pop      r10
    pop      r9
    pop      r8
    pop      rbp
    pop      rsi
    pop      rdi
    pop      rdx
    pop      rcx
    pop      rbx
    pop      rax
    popf
  %endif
%endmacro

%macro my_aesdec 2.nolist
  %xdefine %%xxDst %1
  %xdefine %%xxSrc %2

  %if (my_emulator == 0)
    aesdec %%xxDst, %%xxSrc
  %else
    pushf
    push     rax
    push     rbx
    push     rcx
    push     rdx
    push     rdi
    push     rsi
    push     rbp
    push     r8
    push     r9
    push     r10
    push     r11
    push     r12
    push     r13
    push     r14
    push     r15

    %assign %%stackSize (sizeof(oword)*2)
    sub      rsp,%%stackSize

    movdqu   oword [rsp+00], %%xxDst  ;; save Dst
    movdqu   oword [rsp+16], %%xxSrc  ;; save Src

    lea      rcx, [rsp+00]
    lea      rdx, [rsp+16]

    sub      rsp, (sizeof(qword)*2)
    call     emu_aesdec
    add      rsp, (sizeof(qword)*2)

    movdqu   %%xxDst, oword [rsp+00]  ;; return Dst
    add      esp, %%stackSize

    pop      r15
    pop      r14
    pop      r13
    pop      r12
    pop      r11
    pop      r10
    pop      r9
    pop      r8
    pop      rbp
    pop      rsi
    pop      rdi
    pop      rdx
    pop      rcx
    pop      rbx
    pop      rax
    popf
  %endif
%endmacro

%macro my_aesdeclast 2.nolist
  %xdefine %%xxDst %1
  %xdefine %%xxSrc %2

  %if (my_emulator == 0)
    aesenclast %%xxDst, %%xxSrc
  %else
    pushf
    push     rax
    push     rbx
    push     rcx
    push     rdx
    push     rdi
    push     rsi
    push     rbp
    push     r8
    push     r9
    push     r10
    push     r11
    push     r12
    push     r13
    push     r14
    push     r15

    %assign %%stackSize (sizeof(oword)*2)
    sub      rsp,%%stackSize

    movdqu   oword [rsp+00], %%xxDst  ;; save Dst
    movdqu   oword [rsp+16], %%xxSrc  ;; save Src

    lea      rcx, [rsp+00]
    lea      rdx, [rsp+16]

    sub      rsp, (sizeof(qword)*2)
    call     emu_aesdeclast
    add      rsp, (sizeof(qword)*2)

    movdqu   %%xxDst, oword [rsp+00]  ;; return Dst
    add      esp, %%stackSize

    pop      r15
    pop      r14
    pop      r13
    pop      r12
    pop      r11
    pop      r10
    pop      r9
    pop      r8
    pop      rbp
    pop      rsi
    pop      rdi
    pop      rdx
    pop      rcx
    pop      rbx
    pop      rax
    popf
  %endif
%endmacro

%if (my_emulator != 0)
  extern emu_pclmulqdq
  extern emu_aesenc
  extern emu_aesenclast
  extern emu_aesdec
  extern emu_aesdeclast
%endif

%endif
