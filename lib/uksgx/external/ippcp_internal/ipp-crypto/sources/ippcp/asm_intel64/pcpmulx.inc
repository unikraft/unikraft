;===============================================================================
; Copyright 2013-2021 Intel Corporation
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
;               Emulation of Intel(R) instructions MULX, ADCX, ADOX (for debug only)
;
;
%ifndef _PCPMULX_INC_
%assign _PCPMULX_INC_  1

%ifndef _EMULATION_
%macro gsmulx 3.nolist
  %xdefine %%resH %1
  %xdefine %%resL %2
  %xdefine %%src %3

   mulx  %%resH,%%resL,%%src
%endmacro

%endif

%ifdef _EMULATION_
%macro gsmulx 3.nolist
  %xdefine %%resH %1
  %xdefine %%resL %2
  %xdefine %%src %3

   pushf                            ;; store flags

   sub   rsp, sizeof(qword)*4
   mov   [rsp-sizeof(qword)*3], rax ;; store RAX
   mov   [rsp-sizeof(qword)*2], rdx ;; store RDX
   mov   rax,rdx
   mov   rdx, %%src

   mul   rdx

   mov   [rsp-sizeof(qword)*1], rax ;; store Low product
   mov   [rsp-sizeof(qword)*0], rdx ;; store Hig product

   mov   rax, [rsp-sizeof(qword)*3] ;; re-store RAX
   mov   rdx, [rsp-sizeof(qword)*2] ;; re-store RDX
   mov   %%resL, [rsp-sizeof(qword)*1];; load Low product
   mov   %%resH, [rsp-sizeof(qword)*0];; load Hig product
   add   rsp, sizeof(qword)*4

   popf                             ;; re-store flags
%endmacro

%endif

%ifndef _EMULATION_
%macro gsadcx 2.nolist
  %xdefine %%rdst %1
  %xdefine %%rsrc %2

   adcx     %%rdst, %%rsrc
%endmacro

%endif

%ifdef _EMULATION_
%macro gsadcx 2.nolist
  %xdefine %%rdst %1
  %xdefine %%src %2

   push  %%rdst      ;; slot for result
   push  rax         ;; save rax
   pushfq            ;; flags before adc

   adc   %%rdst, %%src
   mov   [rsp+2*sizeof(qword)], %%rdst

   pushfq            ;; rsrc = flags after operation
   pop   rax
   and   rax, 1      ;; cf after operation
   and   qword [rsp], (-2)   ;; clear cf before operation
   or    [rsp], rax  ;; new psw
   popfq

   pop   rax
   pop   %%rdst
%endmacro

%endif

%ifndef _EMULATION_
%macro gsadox 2.nolist
  %xdefine %%rdst %1
  %xdefine %%rsrc %2

   adox     %%rdst, %%rsrc
%endmacro

%endif

%ifdef _EMULATION_
%macro gsadox 2.nolist
  %xdefine %%rdst %1
  %xdefine %%src %2

   push  %%rdst
   push  rax         ;; save rax

   pushfq            ;; rax = flags before adc
   mov   rax, [rsp]
   and   rax, 800h   ;; of
   xor   [rsp], rax  ;; clear of

   shr   rax, 11     ;; mov of to cf position
   push  rax         ;; new psw
   popfq

%ifidni %%src,rax
   mov   rax, [rsp+sizeof(qword)]
%endif
%ifidni %%rdst,rax
   mov   %%rdst, [rsp+2*sizeof(qword)]
%endif

   adc   %%rdst, %%src
   mov   [rsp+2*sizeof(qword)], %%rdst

   pushfq            ;; rsrc = flags after operation
   pop   rax
   and   rax, 1      ;; cf after operation

   shl   rax, 11     ;; mov cf into of position
   or    [rsp], rax  ;; new psw
   popfq

   pop   rax
   pop   %%rdst
%endmacro

%endif

%endif ;; _PCPMULX_INC_
