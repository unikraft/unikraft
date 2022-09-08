;===============================================================================
; Copyright 2012-2021 Intel Corporation
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

;;
;; Just for unify GPRs usage
;;

%ifndef _IA_32_REGS_INC_
%define _IA_32_REGS_INC_

%define r0  rax     ;; 64-bits GPRs
%define r1  rbx
%define r2  rcx
%define r3  rdx
%define r4  rdi
%define r5  rsi
%define r6  rbp
%define r7  rsp

%define r0d  eax     ;; 32-bits GPRs
%define r1d  ebx
%define r2d  ecx
%define r3d  edx
%define r4d  edi
%define r5d  esi
%define r6d  ebp
%define r7d  esp

%define raxd  eax
%define rbxd  ebx
%define rcxd  ecx
%define rdxd  edx
%define rdid  edi
%define rsid  esi
%define rbpd  ebp

%define r0w  ax     ;; 16-bits GPRs
%define r1w  bx
%define r2w  cx
%define r3w  dx
%define r4w  di
%define r5w  si
%define r6w  bp
%define r7w  sp

%define raxw  ax
%define rbxw  bx
%define rcxw  cx
%define rdxw  dx
%define rdiw  di
%define rsiw  si
%define rbpw  bp

%define r0b  al     ;;  8-bits GPRs
%define r1b  bl
%define r2b  cl
%define r3b  dl
%define r4b  dil
%define r5b  sil
%define r6b  bpl
%define r7b  spl

%define raxb  al
%define rbxb  bl
%define rcxb  cl
%define rdxb  dl
%define rdib  dil
%define rsib  sil
%define rbpb  bpl

%define raxbl  al
%define rbxbl  bl
%define rcxbl  cl
%define rdxbl  dl
%define raxbh  ah
%define rbxbh  bh
%define rcxbh  ch
%define rdxbh  dh

;;
;; Register Parameters (depend on used OS)
;;
%ifdef WIN32E
  %define rpar1 rcx
  %define rpar2 rdx
  %define rpar3 r8
  %define rpar4 r9
  %define rpar5 [rsp + ARG_5]
  %define rpar6 [rsp + ARG_6]
%endif

%ifdef LINUX32E
  %define rpar1 rdi
  %define rpar2 rsi
  %define rpar3 rdx
  %define rpar4 rcx
  %define rpar5 r8
  %define rpar6 r9
%endif

;; use GPR implementation everywhere possible
%assign GPR_version 1

%endif
