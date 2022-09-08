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


%include "asmdefs.inc"
%include "ia_common.inc"
%include "utils.inc"

%ifndef __IA_32E_INC__
%define __IA_32E_INC__ 1

%ifndef LINUX32E
 %ifndef WIN32E
    %fatal <Platform is not defined { LINUX32E or WIN32E }> \
            LINUX32E or WIN32E - Linux ABI (parameter passing in rdi, rsi, rdx, rcx, r8, r9...)
 %endif
%endif

; Force RIP-relative addressing
; default rel

%ifdef LINUX32E
  %ifdef STACK_ABI
    %assign IPP_ABI 2
  %else
    %assign IPP_ABI 3
  %endif
%endif

%ifdef WIN32E
  %ifdef STACK_ABI
    %assign IPP_ABI 1
  %else
    %assign IPP_ABI 0
  %endif
%endif

; Decorates function name with appropriate CPU prefix (for the merged library).
; The macro is context-dependent and returns decorated name in the %$decorated_func_name
; context variable along with the decoration length in the %$decoration_length context variable.
%macro CPU_PREFIX_DECORATE 1.nolist
  %ifnctx _CPU_PREFIX_DECORATE_CTX_
    %fatal "Not in the context: _CPU_PREFIX_DECORATE_CTX_"
  %endif

  ; Add CPU-specific suffix for the dispatched library
  %ifdef _OWN_MERGED_BLD
    %if (_IPP32E == _IPP32E_PX)
      %xdefine %%func_name mx_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_M7)
      %xdefine %%func_name m7_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_U8)
      %xdefine %%func_name u8_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_N8)
      %xdefine %%func_name n8_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_Y8)
      %xdefine %%func_name y8_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_E9)
      %xdefine %%func_name e9_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_L9)
      %xdefine %%func_name l9_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_N0)
      %xdefine %%func_name n0_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_K0)
      %xdefine %%func_name k0_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP32E == _IPP32E_K1)
      %xdefine %%func_name k1_%1
      %assign %%decoration_length 3
    %endif
  %else
      %xdefine %%func_name %1
      %assign %%decoration_length 0
  %endif

  %ifndef %%func_name
    %fatal "CPU_PREFIX_DECORATE: unknown decoration for : _IPP32E = " _IPP32E
  %endif
  %xdefine %$decorated_func_name %[%%func_name]
  %assign %$decoration_length %%decoration_length
%endmacro

; Lists of non-volatile registers that needs to be preserved in a function call
%define NONVOLATILE_REGS_LIN64_GPR rbx,rbp,r12,r13,r14,r15
%define NONVOLATILE_REGS_WIN64_GPR rbx,rbp,r12,r13,r14,r15,rsi,rdi
%define NONVOLATILE_REGS_WIN64_XMM xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
%define NONVOLATILE_REGS_WIN64_YMM ymm6,ymm7,ymm8,ymm9,ymm10,ymm11,ymm12,ymm13,ymm14,ymm15

; Saves non-volatile GPR registers on stack.
; Input - list of used registers.
%macro USES_GPR 1+.nolist
  %assign GPR_FRAME 0
  %define GPR_CUR

  %if (IPP_ABI < 2) ; Win64
    %define %%nonvolatile_regs_list %[NONVOLATILE_REGS_WIN64_GPR]
  %else ; Lin64
    %define %%nonvolatile_regs_list %[NONVOLATILE_REGS_LIN64_GPR]
  %endif
  BEGIN_INTERSECT
  INTERSECT {%1},{%%nonvolatile_regs_list}
  ; List of non-volatile GPR registers in the order they will be pushed on stack
  %xdefine GPR_CUR %[%$intersection]
  %assign GPR_FRAME %$cardinality * 8
  END_INTERSECT

  ; Push non-volatile GPRs on stack
  FOREACH GPR_CUR,{push}
%endmacro

; Restore preliminary saved by USES_GPR non-volatile GPR registers from the stack.
; The macro shall be called after function processing.
%macro REST_GPR 0.nolist
  %ifndef GPR_CUR
    %fatal "REST_GPR: no GPR_CUR defined"
  %endif

  ; Pop saved registers from the stack
  RFOREACH GPR_CUR,{pop}
%endmacro

; Saves XMM register on stack (SSE version).
; An offset from RSP, where the register will be saved, shall be provided in the calling context.
%macro PUSH_XMM_REG 1.nolist
  movdqa [rsp + %$rsp_offset], %1
  %assign %$rsp_offset %$rsp_offset + 16
%endmacro

; Saves non-volatile XMM registers on stack and allocates stack size for
; local variables if needed.
; Input - list of used registers (can be empty).
%macro USES_XMM 0-*.nolist
  ; LOCAL_FRAME - stack size required for all local variables of the procedure. Shall be defined before USES_XMM macro call if
  ; local variables are used in the procedure.
  %ifndef LOCAL_FRAME
    %assign LOCAL_FRAME 0
  %endif
  ; to align size for local variables size on 16-bytes
  %assign LOCAL_FRAME (LOCAL_FRAME + 15) & (-16)
  %assign S_FRAME 0
  %define XMM_CUR

  ; Convert parameters to the list variable if there are arguments
  %if (%0 > 0)
    %xdefine %%param_list %1
    %rotate 1
    %rep %0-1
      %xdefine %%param_list %[%%param_list],%1
    %rotate 1
    %endrep
  %endif

  %if (IPP_ABI < 2) ; Win64
    %assign %%T_FRAME 0
    BEGIN_INTERSECT
    INTERSECT {%%param_list},{%[NONVOLATILE_REGS_WIN64_XMM]}
    %define XMM_CUR %[%$intersection]
    %assign %%T_FRAME %$cardinality * 16
    END_INTERSECT

    ; Adjust offset depending on function frame
    %if ((%%T_FRAME > 0) || (LOCAL_FRAME > 0))
      %assign S_FRAME %%T_FRAME + LOCAL_FRAME
      %if (((S_FRAME + GPR_FRAME ) & 8) == 0)
        %assign S_FRAME S_FRAME + 8
      %endif
    %endif

    ; Allocate space on stack and push XMM registers
    %if (S_FRAME > 0)
      sub rsp, S_FRAME
      %push %??
      %assign %$rsp_offset LOCAL_FRAME
      FOREACH %[XMM_CUR],{PUSH_XMM_REG}
      %pop %??
    %endif
  %else
    ; Linux x86_64 ABI does not count <X,Y,Z>MM registers as non-volatile, so they do not need to be
    ; preserved, so just allocate stack space for local variables and duplicated register parameters if needed.
    %if (IPP_ABI == 2) ; LINUX32S
      %assign S_FRAME LOCAL_FRAME + 48   ; 48 = 6 * 8 - stack frame for 6 register inputs
      %if (((S_FRAME + GPR_FRAME) & 8 ) == 0)
        %assign S_FRAME S_FRAME + 8
      %endif
      %assign INP_FRAME S_FRAME - 48 ; for Linux32s-key stack-frame for 6 registers inputs
    %else ; LINUX32E
      %if (LOCAL_FRAME > 0)
        %assign S_FRAME LOCAL_FRAME
        %if (((S_FRAME + GPR_FRAME) & 8 ) == 0)
          %assign S_FRAME S_FRAME + 8
        %endif
      %endif
    %endif
    %if (S_FRAME > 0)
      sub rsp, S_FRAME
    %endif
  %endif
%endmacro

; Pop input list of XMM registers from the stack.
; The offset from RSP, where the registers will be taken from, shall be provided in the calling context.
%macro POP_XMM_REG 1.nolist
  movdqa %1, [rsp + %$rsp_offset]
  %assign %$rsp_offset %$rsp_offset + 16
%endmacro

; Restore preliminary saved by USES_XMM non-volatile XMM registers from the stack.
; The macro shall be called after function processing.
%macro REST_XMM 0.nolist
  %if (IPP_ABI < 2)
    %if (S_FRAME > 0)
      %push %??
      %assign %$rsp_offset LOCAL_FRAME
      FOREACH %[XMM_CUR],{POP_XMM_REG}
      %pop %??
    %endif
  %endif
  %if (S_FRAME > 0)
    add rsp, S_FRAME
  %endif
  %if (_IPP32E >= _IPP32E_E9)
    %if (_IPP32E != _IPP32E_N0)
      vzeroupper
    %endif
  %endif
%endmacro

; Saves XMM or YMM register on stack (AVX version).
; An offset from RSP, where the registers will be saved, shall be provided in the calling context.
%macro PUSH_XMM_AVX_REG 1.nolist
  ; Process registers depending on type
  %defstr %%reg_str %1 ; convert register token to the string
  %substr %%reg_type %%reg_str 1, 3 ; take first 3 elems of the string
  %ifidni %%reg_type, 'xmm'
    vmovdqa oword [rsp + %$rsp_offset], %1
    %assign %$rsp_offset %$rsp_offset + 16
  %elifidni %%reg_type, 'ymm'
    vmovdqu ymmword [rsp + %$rsp_offset], %1
    %assign %$rsp_offset %$rsp_offset + 32
  %else
    %fatal PUSH_XMM_AVX_REG: inconsistent usage - only XMM/YMM registers supported, found: %%reg_type
  %endif
%endmacro

; Saves non-volatile XMM/YMM registers on stack and allocates stack size for
; local variables if needed (AVX version).
; Input - list of used registers (can be empty).
%macro USES_XMM_AVX 0-*.nolist
  ; LOCAL_FRAME - stack size required for all local variables of the procedure. Shall be defined before USES_XMM macro call if
  ; local variables are used in the procedure.
  %ifndef LOCAL_FRAME
    %assign LOCAL_FRAME 0
  %endif
  ; to align size for local variables size on 16-bytes
  %assign LOCAL_FRAME (LOCAL_FRAME + 15) & (-16)
  %assign S_FRAME 0
  %define XMM_CUR
  %define YMM_CUR

  ; Convert parameters to the list variable if there are arguments
  %if (%0 > 0)
    %xdefine %%param_list %1
    %rotate 1
    %rep %0-1
      %xdefine %%param_list %[%%param_list],%1
    %rotate 1
    %endrep
  %endif

  %if (IPP_ABI < 2) ; Win64
    %assign %%T_FRAME 0
    ; Process XMM registers
    BEGIN_INTERSECT
    INTERSECT {%%param_list},{%[NONVOLATILE_REGS_WIN64_XMM]}
    %define XMM_CUR %[%$intersection]
    %assign %%T_FRAME %$cardinality * 16
    END_INTERSECT

    ; Process YMM registers
    BEGIN_INTERSECT
    INTERSECT {%%param_list},{%[NONVOLATILE_REGS_WIN64_YMM]}
    %define YMM_CUR %[%$intersection]
    %assign %%T_FRAME %%T_FRAME + %$cardinality * 32
    END_INTERSECT

    ; Adjust offset depending on function frame
    %if ((%%T_FRAME > 0) || (LOCAL_FRAME > 0))
      %assign S_FRAME %%T_FRAME + LOCAL_FRAME
      %if (((S_FRAME + GPR_FRAME ) & 8) == 0)
        %assign S_FRAME S_FRAME + 8
      %endif
    %endif

    ; Allocate space on stack and push registers
    %if (S_FRAME > 0)
      sub rsp, S_FRAME
      CONCATENATE {%[XMM_CUR]},{%[YMM_CUR]},%%XMM_YMM_CUR
      %push %??
      %assign %$rsp_offset LOCAL_FRAME
      FOREACH %[%%XMM_YMM_CUR],{PUSH_XMM_AVX_REG}
      %pop %??
    %endif
  %else
    ; Linux x86_64 ABI does not count <X,Y,Z>MM registers as non-volatile, so they do not need to be
    ; preserved, so just allocate stack space for local variables and duplicated register parameters if needed.
    %if (IPP_ABI == 2) ; LINUX32S
      %assign S_FRAME LOCAL_FRAME + 48   ; 48 = 6 * 8 - stack frame for 6 register inputs
      %if (((S_FRAME + GPR_FRAME) & 8 ) == 0)
        %assign S_FRAME S_FRAME + 8
      %endif
      %assign INP_FRAME S_FRAME - 48 ; for Linux32s-key stack-frame for 6 registers inputs
    %else ; LINUX32E
      %if (LOCAL_FRAME > 0)
        %assign S_FRAME LOCAL_FRAME
        %if (((S_FRAME + GPR_FRAME) & 8 ) == 0)
          %assign S_FRAME S_FRAME + 8
        %endif
      %endif
    %endif
    %if (S_FRAME > 0)
      sub rsp, S_FRAME
    %endif
  %endif
%endmacro

; Pop XMM or YMM register from the stack.
; The offset from RSP, where the registers will be taken from, shall be provided in the calling context.
%macro POP_XMM_AVX_REG 1.nolist
  ; Process registers depending on type
  %defstr %%reg_str %1 ; convert register token to the string
  %substr %%reg_type %%reg_str 1, 3 ; take first 3 elems of the string
  %ifidni %%reg_type, 'xmm'
    vmovdqa %1, oword [rsp + %$rsp_offset]
    %assign %$rsp_offset %$rsp_offset + 16
  %elifidni %%reg_type, 'ymm'
    vmovdqu %1, ymmword [rsp + %$rsp_offset]
    %assign %$rsp_offset %$rsp_offset + 32
  %else
    %fatal POP_XMM_AVX_REG: inconsistent usage - only XMM/YMM registers supported, found: %%reg_type
  %endif
%endmacro

; Restore preliminary saved by USES_XMM_AVX non-volatile XMM/YMM registers from the stack.
; The macro shall be called after function processing.
%macro REST_XMM_AVX 0.nolist
  %if (IPP_ABI < 2)
    %if (S_FRAME > 0)
      CONCATENATE {%[XMM_CUR]},{%[YMM_CUR]},%%XMM_YMM_CUR
      %push %??
      %assign %$rsp_offset LOCAL_FRAME
      FOREACH %[%%XMM_YMM_CUR],{POP_XMM_AVX_REG}
      %pop %??
    %endif
  %endif
  %if (S_FRAME > 0)
    add rsp, S_FRAME
  %endif
  %if (_IPP32E != _IPP32E_N0)
    vzeroupper
  %endif
%endmacro

; Helper macro to align different ABIs parameters for uniform usage
%macro COMP_ABI 1.nolist
  %if (IPP_ABI == 0)                              ;; if defined WIN32E
    %if (%1 > 0)
      mov  rdi, rcx                               ;; ARG_1
    %endif
    %if (%1 > 1)
      mov  rsi, rdx                               ;; ARG_2
    %endif
    %if (%1 > 2)
      mov  rdx, r8                                ;; ARG_3
    %endif
    %if (%1 > 3)
      mov  rcx, r9                                ;; ARG_4
    %endif
    %if (%1 > 4)
      mov  r8, [rsp + S_FRAME + GPR_FRAME + 40]   ;; ARG_5
    %endif
    %if (%1 > 5)
      mov  r9, [rsp + S_FRAME + GPR_FRAME + 48]   ;; ARG_6
    %endif
    %if (%1 > 6)
      %assign FIRST_P S_FRAME + GPR_FRAME + 56    ;; ARG_7
      %assign ARG_7   S_FRAME + GPR_FRAME + 56
    %endif
  %endif
  %if (IPP_ABI == 1)                              ;; if defined WIN32S
    %assign FIRST_P S_FRAME + GPR_FRAME + 8
    %if (%1 > 0)
      mov   [rsp + FIRST_P],rcx
      %assign ARG_1 FIRST_P
    %endif
    %if (%1 > 1)
      mov   [rsp + FIRST_P + 8],rdx
      %assign ARG_2 ARG_1 + 8
    %endif
    %if (%1 > 2)
      mov   [rsp + FIRST_P + 16],r8
      %assign ARG_3 ARG_2 + 8
    %endif
    %if (%1 > 3)
      mov   [rsp + FIRST_P + 24],r9
      %assign ARG_4 ARG_3 + 8
    %endif
    %if (%1 > 4)
      %assign ARG_5 ARG_4 + 8
    %endif
    %if (%1 > 5)
      %assign ARG_6 ARG_5 + 8
    %endif
    %if (%1 > 6)
      %assign ARG_7 ARG_6 + 8                     ;; ARG_7
    %endif
  %endif
  %if (IPP_ABI == 2)                              ;; if defined LINUX32S
    %assign FIRST_P INP_FRAME
    %if (%1 > 0)
      mov   [rsp + FIRST_P],rdi
      %assign ARG_1 FIRST_P
    %endif
    %if (%1 > 1)
      mov   [rsp + FIRST_P + 8],rsi
      %assign ARG_2 ARG_1 + 8
    %endif
    %if (%1 > 2)
      mov   [rsp + FIRST_P + 16],rdx
      %assign ARG_3 ARG_2 + 8
    %endif
    %if (%1 > 3)
      mov   [rsp + FIRST_P + 24],rcx
      %assign ARG_4 ARG_3 + 8
    %endif
    %if (%1 > 4)
      mov   [rsp + FIRST_P + 32],r8
      %assign ARG_5 ARG_4 + 8
    %endif
    %if (%1 > 5)
      mov   [rsp + FIRST_P + 40],r9
      %assign ARG_6 ARG_5 + 8
    %endif
    %if (%1 > 6)
      %assign ARG_7 S_FRAME+GPR_FRAME+8
    %endif
  %endif
  %if (IPP_ABI == 3)
    %if (%1 > 6) ;; ARG_1 = rdi ARG_2 = rsi ARG_3 = rdx ARG_4 = rcx ARG_5 = r8 ARG_6 = r9
      %assign FIRST_P S_FRAME + GPR_FRAME + 8       ;; ARG_7
      %assign ARG_7   S_FRAME + GPR_FRAME + 8
    %endif
  %endif
  %if (%1 > 7)
    %assign ARG_8   ARG_7 + 8                       ;; ARG_8
  %endif
  %if (%1 > 8)
    %assign ARG_9   ARG_8 + 8                       ;; ARG_9
  %endif
  %if (%1 > 9)
    %assign ARG_10  ARG_9 + 8                       ;; ARG_10
  %endif
  %if (%1 > 10)
    %assign ARG_11  ARG_10 + 8                      ;; ARG_11
  %endif
  %if (%1 > 11)
    %assign ARG_12  ARG_11 + 8                      ;; ARG_12
  %endif
  %if (%1 > 12)
    %assign ARG_13  ARG_12 + 8                      ;; ARG_13
  %endif
  %if (%1 > 13)
    %assign ARG_14  ARG_13 + 8                      ;; ARG_14
  %endif
  %if (%1 > 14)
    %assign ARG_15  ARG_14 + 8                      ;; ARG_15
  %endif
  %if (%1 > 15)
    %assign ARG_16  ARG_15 + 8                      ;; ARG_16
  %endif
  %if (%1 > 16)
    %assign ARG_17  ARG_16 + 8                      ;; ARG_17
  %endif
  %if (%1 > 17)
    %assign ARG_18  ARG_17 + 8                      ;; ARG_18
  %endif
%endmacro

%macro LD_ADDR 2.nolist
  %xdefine %%reg %1
  %xdefine %%addr %2

%ifdef xxIPP_PIC
   call     %%LABEL
%%LABEL:  pop   %%reg
   sub     %%reg, %%LABEL-%%addr
%else
   lea      %%reg, [%%addr]
%endif
%endmacro

%endif
