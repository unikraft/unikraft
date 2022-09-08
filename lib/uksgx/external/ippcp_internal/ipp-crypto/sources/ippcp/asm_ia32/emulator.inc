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
;     Purpose:  x86 Cryptography Primitive.
;
;
;

%macro my_pclmulqdq 3.nolist
  %define %%xxDst  %1
  %define %%xxSrc  %2
  %define %%xxOp   %3

%if (my_emulator == 0)
   pclmulqdq %%xxDst, %%xxSrc, %%xxOp
%else
;;;; esp => &p1
;; +4  => &p2
;; +8  => p3
;; +12 => p2 (16 bytes)
;; +28 => p1 (16 bytes)
;; +44 => eax

   %assign %%stackSize (4+4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+44], eax    ;; save eax
   mov      dword     [esp+48], ebx    ;; save ebx
   mov      dword     [esp+52], ecx    ;; save ecx
   mov      dword     [esp+56], edx    ;; save edx
   mov      dword     [esp+60], esi    ;; save esi
   mov      dword     [esp+64], edi    ;; save edi

   movdqu   oword     [esp+28], %%xxDst  ;; store mmDst
   movdqu   oword     [esp+12], %%xxSrc  ;; store mmSrc
   mov      dword     [esp+8],  %%xxOp   ;; put p3 (mmOp) into the stack
   lea      eax, [esp+28]              ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+12]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_pclmulqdq
   movdqu   %%xxDst, oword    [esp+28]   ;; load result

   mov      eax, dword     [esp+44]    ;; restore eax
   mov      ebx, dword     [esp+48]    ;; restore ebx
   mov      ecx, dword     [esp+52]    ;; restore ecx
   mov      edx, dword     [esp+56]    ;; restore edx
   mov      esi, dword     [esp+60]    ;; restore esi
   mov      edi, dword     [esp+64]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%macro my_palignr 3.nolist
  %define %%xxDst %1
  %define %%xxSrc %2
  %define %%xxOp  %3

%if (my_emulator == 0)
   palignr %%xxDst, %%xxSrc, %%xxOp
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p3
;; +12 => p2 (16 bytes)
;; +28 => p1 (16 bytes)
;; +44 => eax

   %assign %%stackSize (4+4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+44], eax    ;; save eax
   mov      dword     [esp+48], ebx    ;; save ebx
   mov      dword     [esp+52], ecx    ;; save ecx
   mov      dword     [esp+56], edx    ;; save edx
   mov      dword     [esp+60], esi    ;; save esi
   mov      dword     [esp+64], edi    ;; save edi

   movdqu   oword     [esp+28], %%xxDst  ;; store mmDst
   movdqu   oword     [esp+12], %%xxSrc  ;; store mmSrc
   mov      dword     [esp+8],  %%xxOp   ;; put p3 (mmOp) into the stack
   lea      eax, [esp+28]              ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+12]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_palignr
   movdqu   %%xxDst, oword    [esp+28]   ;; load result

   mov      eax, dword     [esp+44]    ;; restore eax
   mov      ebx, dword     [esp+48]    ;; restore ebx
   mov      ecx, dword     [esp+52]    ;; restore ecx
   mov      edx, dword     [esp+56]    ;; restore edx
   mov      esi, dword     [esp+60]    ;; restore esi
   mov      edi, dword     [esp+64]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%macro my_pshufd 3.nolist
  %define %%xxDst %1
  %define %%xxSrc %2
  %define %%xxOp  %3

%if (my_emulator == 0)
   pshufd %%xxDst, %%xxSrc, %%xxOp
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p3
;; +12 => p2 (16 bytes)
;; +28 => p1 (16 bytes)
;; +44 => eax

   %assign %%stackSize (4+4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+44], eax    ;; save eax
   mov      dword     [esp+48], ebx    ;; save ebx
   mov      dword     [esp+52], ecx    ;; save ecx
   mov      dword     [esp+56], edx    ;; save edx
   mov      dword     [esp+60], esi    ;; save esi
   mov      dword     [esp+64], edi    ;; save edi

   movdqu   oword     [esp+28], %%xxDst  ;; store mmDst
   movdqu   oword     [esp+12], %%xxSrc  ;; store mmSrc
   mov      dword     [esp+8],  %%xxOp   ;; put p3 (mmOp) into the stack
   lea      eax, [esp+28]              ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+12]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_pshufd
   movdqu   %%xxDst, oword    [esp+28]   ;; load result

   mov      eax, dword     [esp+44]    ;; restore eax
   mov      ebx, dword     [esp+48]    ;; restore ebx
   mov      ecx, dword     [esp+52]    ;; restore ecx
   mov      edx, dword     [esp+56]    ;; restore edx
   mov      esi, dword     [esp+60]    ;; restore esi
   mov      edi, dword     [esp+64]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro


%macro my_pshufb 2.nolist
  %define %%xxDst %1
  %define %%xxSrc %2

%if (my_emulator == 0)
   pshufb %%xxDst, %%xxSrc
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p2 (16 bytes)
;; +24 => p1 (16 bytes)
;; +40 => eax

   %assign %%stackSize (4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+40], eax    ;; save eax
   mov      dword     [esp+44], ebx    ;; save ebx
   mov      dword     [esp+48], ecx    ;; save ecx
   mov      dword     [esp+52], edx    ;; save edx
   mov      dword     [esp+56], esi    ;; save esi
   mov      dword     [esp+60], edi    ;; save edi

   movdqu   oword     [esp+24], %%xxDst  ;; store mmDst
   movdqu   oword     [esp+8],  %%xxSrc  ;; store mmSrc
   lea      eax, [esp+24]              ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+8]               ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_pshufb
   movdqu   %%xxDst, oword    [esp+24]   ;; load result

   mov      eax, dword     [esp+40]    ;; restore eax
   mov      ebx, dword     [esp+44]    ;; restore ebx
   mov      ecx, dword     [esp+48]    ;; restore ecx
   mov      edx, dword     [esp+52]    ;; restore edx
   mov      esi, dword     [esp+56]    ;; restore esi
   mov      edi, dword     [esp+60]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%macro my_pshufbM 2.nolist
  %define %%xxDst %1
  %define %%xxSrc %2

%if (my_emulator == 0)
   pshufb %%xxDst, %%xxSrc
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p2 (16 bytes)
;; +24 => p1 (16 bytes)
;; +40 => eax

   %assign %%stackSize (4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+40], eax    ;; save eax
   mov      dword     [esp+44], ebx    ;; save ebx
   mov      dword     [esp+48], ecx    ;; save ecx
   mov      dword     [esp+52], edx    ;; save edx
   mov      dword     [esp+56], esi    ;; save esi
   mov      dword     [esp+60], edi    ;; save edi

   movdqu   oword     [esp+24], %%xxDst  ;; store mmDst
   movdqu   %%xxDst, %%xxSrc
   movdqu   oword     [esp+8],  %%xxDst  ;; store mmSrc
   lea      eax, [esp+24]              ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+8]               ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_pshufb
   movdqu   %%xxDst, oword    [esp+24]   ;; load result

   mov      eax, dword     [esp+40]    ;; restore eax
   mov      ebx, dword     [esp+44]    ;; restore ebx
   mov      ecx, dword     [esp+48]    ;; restore ecx
   mov      edx, dword     [esp+52]    ;; restore edx
   mov      esi, dword     [esp+56]    ;; restore esi
   mov      edi, dword     [esp+60]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%macro my_aesenc 2.nolist
  %define %%xxDst %1
  %define %%xxSrc %2

%if (my_emulator == 0)
   aesenc %%xxDst, %%xxSrc
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p1 (16 bytes)
;; +24 => p2 (16 bytes)
;; +40 => eax

   %assign %%stackSize (4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+40], eax    ;; save eax
   mov      dword     [esp+44], ebx    ;; save ebx
   mov      dword     [esp+48], ecx    ;; save ecx
   mov      dword     [esp+52], edx    ;; save edx
   mov      dword     [esp+56], esi    ;; save esi
   mov      dword     [esp+60], edi    ;; save edi

   movdqu   oword     [esp+8],  %%xxDst  ;; store mmDst
   movdqu   oword     [esp+24], %%xxSrc  ;; store mmSrc
   lea      eax, [esp+8]               ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+24]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_aesenc
   movdqu   %%xxDst, oword    [esp+8]    ;; load result

   mov      eax, dword     [esp+40]    ;; restore eax
   mov      ebx, dword     [esp+44]    ;; restore ebx
   mov      ecx, dword     [esp+48]    ;; restore ecx
   mov      edx, dword     [esp+52]    ;; restore edx
   mov      esi, dword     [esp+56]    ;; restore esi
   mov      edi, dword     [esp+60]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%macro my_aesenclast 2.nolist
  %define %%xxDst %1
  %define %%xxSrc %2

%if (my_emulator == 0)
   aesenclast %%xxDst, %%xxSrc
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p1 (16 bytes)
;; +24 => p2 (16 bytes)
;; +40 => eax

   %assign %%stackSize (4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+40], eax    ;; save eax
   mov      dword     [esp+44], ebx    ;; save ebx
   mov      dword     [esp+48], ecx    ;; save ecx
   mov      dword     [esp+52], edx    ;; save edx
   mov      dword     [esp+56], esi    ;; save esi
   mov      dword     [esp+60], edi    ;; save edi

   movdqu   oword     [esp+8],  %%xxDst  ;; store mmDst
   movdqu   oword     [esp+24], %%xxSrc  ;; store mmSrc
   lea      eax, [esp+8]               ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+24]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_aesenclast
   movdqu   %%xxDst, oword    [esp+8]    ;; load result

   mov      eax, dword     [esp+40]    ;; restore eax
   mov      ebx, dword     [esp+44]    ;; restore ebx
   mov      ecx, dword     [esp+48]    ;; restore ecx
   mov      edx, dword     [esp+52]    ;; restore edx
   mov      esi, dword     [esp+56]    ;; restore esi
   mov      edi, dword     [esp+60]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro


%macro my_aesdec 2.nolist
  %define %%xxDst %1
  %define %%xxSrc %2

%if (my_emulator == 0)
   aesdec %%xxDst, %%xxSrc
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p1 (16 bytes)
;; +24 => p2 (16 bytes)
;; +40 => eax

   %assign %%stackSize (4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+40], eax    ;; save eax
   mov      dword     [esp+44], ebx    ;; save ebx
   mov      dword     [esp+48], ecx    ;; save ecx
   mov      dword     [esp+52], edx    ;; save edx
   mov      dword     [esp+56], esi    ;; save esi
   mov      dword     [esp+60], edi    ;; save edi

   movdqu   oword     [esp+8],  %%xxDst  ;; store mmDst
   movdqu   oword     [esp+24], %%xxSrc  ;; store mmSrc
   lea      eax, [esp+8]               ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+24]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_aesdec
   movdqu   %%xxDst, oword    [esp+8]    ;; load result

   mov      eax, dword     [esp+40]    ;; restore eax
   mov      ebx, dword     [esp+44]    ;; restore ebx
   mov      ecx, dword     [esp+48]    ;; restore ecx
   mov      edx, dword     [esp+52]    ;; restore edx
   mov      esi, dword     [esp+56]    ;; restore esi
   mov      edi, dword     [esp+60]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%macro my_aesdeclast 2.nolist
  %define %%xxDst %1
  %define %%xxSrc %2

%if (my_emulator == 0)
   aesdeclast %%xxDst, %%xxSrc
%else
;;
;; esp => &p1
;; +4  => &p2
;; +8  => p1 (16 bytes)
;; +24 => p2 (16 bytes)
;; +40 => eax

   %assign %%stackSize (4+4)+(16+16)+(4+4+4+4+4+4)
   sub      esp,%%stackSize

   mov      dword     [esp+40], eax    ;; save eax
   mov      dword     [esp+44], ebx    ;; save ebx
   mov      dword     [esp+48], ecx    ;; save ecx
   mov      dword     [esp+52], edx    ;; save edx
   mov      dword     [esp+56], esi    ;; save esi
   mov      dword     [esp+60], edi    ;; save edi

   movdqu   oword     [esp+8],  %%xxDst  ;; store mmDst
   movdqu   oword     [esp+24], %%xxSrc  ;; store mmSrc
   lea      eax, [esp+8]               ;; &p1 (&mmSrc)
   mov      dword     [esp], eax       ;; put &p1 into the stack
   lea      eax, [esp+24]              ;; &p2
   mov      dword     [esp+4], eax     ;; put &p2 into the stack

   call     emu_aesdeclast
   movdqu   %%xxDst, oword    [esp+8]    ;; load result

   mov      eax, dword     [esp+40]    ;; restore eax
   mov      ebx, dword     [esp+44]    ;; restore ebx
   mov      ecx, dword     [esp+48]    ;; restore ecx
   mov      edx, dword     [esp+52]    ;; restore edx
   mov      esi, dword     [esp+56]    ;; restore esi
   mov      edi, dword     [esp+60]    ;; restore edi
   add      esp, %%stackSize
%endif
%endmacro

%if (my_emulator != 0)
  extern emu_pclmulqdq
  extern emu_palignr
  extern emu_pshufd
  extern emu_pshufb
  extern emu_aesenc
  extern emu_aesenclast
  extern emu_aesdec
  extern emu_aesdeclast
%endif
