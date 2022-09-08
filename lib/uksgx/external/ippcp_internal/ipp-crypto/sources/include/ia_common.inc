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

%ifndef __IA_COMMON_INC__
%define __IA_COMMON_INC__ 1

; use multi-byte nop's sequences to align loops and jmp's when threshold is reached
%use smartalign
ALIGNMODE p6,16

; Declares function, sets visibility and binding and adds __cdecl decoration when needed.
%macro DECLARE_FUNC 2-3.nolist
  %xdefine %%func_name %1
  %xdefine %%visibility %2
  %xdefine %%binding %3

  %ifctx _DECLARE_FUNC_CTX_
    %fatal "DECLARE_FUNC: already in the context, need to call ENDFUNC"
  %endif

  ; Accepted visibility values are PUBLIC and PRIVATE
  %ifnidni %%visibility, PUBLIC
    %ifnidni %%visibility, PRIVATE
      %fatal Function %%func_name visibility is not properly defined. Shall be: PRIVATE or PUBLIC.
    %endif
  %endif

  ; Accepted binding values are WEAK or STRONG (default)
  %ifnempty %%binding
    %ifnidni %%binding, WEAK
      %ifnidni %%binding, STRONG
        %%fatal Function %%func_name binding is not properly defined. Shall be: WEAK or STRONG.
      %endif
    %endif
  %endif

  ; Function decoration length
  %assign %%decoration_length 0

  ; The __cdecl calling convention name decoration (to have interoperability with C).
  ; Only public functions are decorated
  %ifidni %%visibility, PUBLIC
    %if ((IPP_BINARY_FORMAT == 2) || (IPP_BINARY_FORMAT == 3)) ; WIN32 or OSXEM64T
      %xdefine %%func_name _%[%%func_name]
      %assign %%decoration_length %%decoration_length+1
    %endif
  %endif

  ; If current macro is called from IPPASM macro, then function might be decorated by CPU-prefix
  %ifctx _IPPASM_CTX_
    %assign %%decoration_length %%decoration_length + %$decoration_length ; %$decoration_length belongs to _IPPASM_CTX_
  %endif

  %push _DECLARE_FUNC_CTX_
  ; setup context variables to use in ENDFUNC
  %xdefine %$func_name_ctx %%func_name
  %assign %$decoration_length %%decoration_length ; %$decoration_length belongs to _DECLARE_FUNC_CTX_

  %ifidn %%visibility, PUBLIC
    %if (IPP_BINARY_FORMAT < 2) ; LINUX32 or LINUX32E
      %ifnempty %%binding
        global %%func_name:function %%binding (%%func_name%+.LEnd_%+%%func_name - %%func_name)
      %else
        global %%func_name:function (%%func_name%+.LEnd_%+%%func_name - %%func_name)
      %endif
    %else
      global %%func_name
    %endif
  %endif
  %%func_name:

  ; CET enabling (macOS not supported)
  %if ((IPP_BINARY_FORMAT == 0) || (IPP_BINARY_FORMAT == 3)) ; elf32/win32
    db 0F3h, 00Fh, 01Eh, 0FBh ; endbr32
  %elif ((IPP_BINARY_FORMAT == 1) || (IPP_BINARY_FORMAT == 4)) ; elf64/win64
    db 0F3h, 00Fh, 01Eh, 0FAh ; endbr64
  %endif
%endmacro

; Calls assembler function declared by DECLARE_FUNC
; Default visibility is PRIVATE (affects decoration)
%macro CALL_FUNC 1-2.nolist PRIVATE
  %xdefine %%func_name %1
  %xdefine %%visibility %2

  ; Accepted visibility values are PUBLIC and PRIVATE
  %ifnidni %%visibility, PUBLIC
    %ifnidni %%visibility, PRIVATE
      %fatal Function %%func_name visibility is not properly defined. Shall be: PRIVATE or PUBLIC.
    %endif
  %endif

  ; __cdecl on WIN32/OSXEM64T obligates to have undersore prefix decoration.
  ; Only PUBLIC functions are decorated.
  %ifidni %%visibility, PUBLIC
    %if ((IPP_BINARY_FORMAT == 2) || (IPP_BINARY_FORMAT == 3)) ; WIN32 or OSXEM64T
      %xdefine %%func_name _%1
    %endif
  %endif

  call %%func_name
%endmacro

; Declares function decorated by appropriate CPU prefix (for the merged library)
; Default visibility (if not defined) is PUBLIC.
%macro IPPASM 1-2.nolist PUBLIC
  %xdefine %%func_name %1
  %xdefine %%visibility %2

  %ifctx _IPPASM_CTX_
    %fatal "IPPASM: already in the context, need to call ENDFUNC"
  %endif
  %push _IPPASM_CTX_

  %push _CPU_PREFIX_DECORATE_CTX_
  CPU_PREFIX_DECORATE %%func_name
  %xdefine %%func_name %$decorated_func_name
  %assign %$$decoration_length %$decoration_length
  %pop _CPU_PREFIX_DECORATE_CTX_

  DECLARE_FUNC %%func_name, %%visibility
%endmacro

; Calls assembler function declared by IPPASM
; Default visibility is PRIVATE (affects decoration)
%macro CALL_IPPASM 1-2.nolist PRIVATE
  %xdefine %%func_name %1
  %xdefine %%visibility %2

  ; Accepted visibility values are PUBLIC and PRIVATE
  %ifnidni %%visibility, PUBLIC
    %ifnidni %%visibility, PRIVATE
      %fatal Function %%func_name visibility is not properly defined. Shall be: PRIVATE or PUBLIC.
    %endif
  %endif

  %push _CPU_PREFIX_DECORATE_CTX_
  CPU_PREFIX_DECORATE %%func_name
  %xdefine %%func_name %$decorated_func_name
  %pop _CPU_PREFIX_DECORATE_CTX_

  CALL_FUNC %%func_name,%%visibility
%endmacro

; End function macro - required to be called after IPPASM or DECLARE_FUNC macro invokation.
%macro ENDFUNC 1.nolist
  %xdefine %%func_name %1
  %ifnctx _DECLARE_FUNC_CTX_
    %fatal "Not in the context: _DECLARE_FUNC_CTX_"
  %endif

  ; Cross-check of context variable with macro parameter
  %defstr %%func_name_str %%func_name
  %defstr %%func_name_ctx_str %$func_name_ctx
  %substr %%func_name_ctx_str_not_decorated %%func_name_ctx_str %[%$decoration_length+1],-1   ; remove decoration (first X symbols)
  %ifnidn %%func_name_str,%%func_name_ctx_str
    %ifnidn %%func_name_str,%%func_name_ctx_str_not_decorated
      %fatal ENDFUNC: function name [%%func_name] does match context: [%$func_name_ctx]
    %endif
  %endif

  ; Add local label to be able calculate function size
  ; Take function name from the context (real declaration name)
.LEnd_%+%$func_name_ctx:
  %pop _DECLARE_FUNC_CTX_

  %ifctx _IPPASM_CTX_
    %pop _IPPASM_CTX_
  %endif
%endmacro

%endif
