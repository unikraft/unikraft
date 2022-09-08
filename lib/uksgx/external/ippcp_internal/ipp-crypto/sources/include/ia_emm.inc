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

%include "asmdefs.inc"
%include "ia_common.inc"
%include "utils.inc"

; Decorates function name with appropriate CPU prefix (for the merged library).
; The macro is context-dependent and returns decorated name in the %$decorated_func_name
; context variable.
%macro CPU_PREFIX_DECORATE 1.nolist
  %ifnctx _CPU_PREFIX_DECORATE_CTX_
    %fatal "Not in the context: _CPU_PREFIX_DECORATE_CTX_"
  %endif

  ; Add CPU-specific suffix for the dispatched library
  %ifdef _OWN_MERGED_BLD
    %if (_IPP == _IPP_PX)
      %xdefine %%func_name px_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP == _IPP_W7)
      %xdefine %%func_name w7_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP == _IPP_V8)
      %xdefine %%func_name v8_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP == _IPP_S8)
      %xdefine %%func_name s8_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP == _IPP_P8)
      %xdefine %%func_name p8_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP == _IPP_G9)
      %xdefine %%func_name g9_%1
      %assign %%decoration_length 3
    %endif
    %if (_IPP == _IPP_H9)
      %xdefine %%func_name h9_%1
      %assign %%decoration_length 3
    %endif
  %else
    %xdefine %%func_name %1
    %assign %%decoration_length 0
  %endif

  %ifndef %%func_name
    %fatal "CPU_PREFIX_DECORATE: unknown decoration for: _IPP = " _IPP
  %endif
  %xdefine %$decorated_func_name %[%%func_name]
  %assign %$decoration_length %%decoration_length
%endmacro

%define NONVOLATILE_REGS_32_GPR ebp,ebx,esi,edi

; Saves non-volatile GPR registers on stack.
; Input - list of used registers.
%macro USES_GPR 1+.nolist
  %assign LOCAL_FRAME 0
  %assign GPR_FRAME 0
  %define GPR_CUR

  BEGIN_INTERSECT
  INTERSECT {%1},{%[NONVOLATILE_REGS_32_GPR]}
  ; List of non-volatile GPR registers in the order they will be pushed on stack
  %xdefine GPR_CUR %$intersection
  %assign GPR_FRAME %$cardinality * 4
  END_INTERSECT

  ; Push non-volatile GPRs on stack
  FOREACH GPR_CUR,{push}

  ; Set up offset of arguments from ESP
  %assign ARG_1 %[GPR_FRAME + 4]
%endmacro

; Restore preliminary saved by USES_GPR non-volatile GPR registers from the stack.
; The macro shall be called after function processing.
%macro REST_GPR 0.nolist
  %ifndef GPR_CUR
    %fatal "REST_GPR: no GPR_CUR defined"
  %endif
  ; Pop saved GPRs from the stack
  RFOREACH GPR_CUR,{pop}
%endmacro

%macro LD_ADDR 2.nolist
  %xdefine %%reg %1
  %xdefine %%addr %2

%ifdef IPP_PIC
  call  %%LABEL
%%LABEL:  pop      %%reg
  sub   %%reg, %%LABEL-%%addr
%else
  lea   %%reg, [%%addr]
%endif
%endmacro
