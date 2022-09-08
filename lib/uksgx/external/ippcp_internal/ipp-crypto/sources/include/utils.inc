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

%ifndef __UTILS_INC__
%define __UTILS_INC__ 1

; Apply a functor provided as a last parameter to each element of the list, provided as sequence of first parameters.
; A list is processed in direct order. Note: an input list can be empty.
%macro FOREACH 2-*.nolist
  %rotate -1
  %xdefine %%functor %1
  %rep %0-1
  %rotate 1
    %ifnempty %1
      %%functor %1
    %endif
  %endrep
%endmacro

; Apply a functor provided as a last parameter to each element of the list, provided as sequence of first parameters.
; A list is processed in reverse order. Note: an input list can be empty.
%macro RFOREACH 2-*.nolist
  %rotate -1
  %xdefine %%functor %1
  %rep %0-1
  %rotate -1
    %ifnempty %1
      %%functor %1
    %endif
  %endrep
%endmacro

; Shall be called before INTERSECT macro to open corresponding context.
%macro BEGIN_INTERSECT 0.nolist
  %push _INTERSECT_CTX_
  %xdefine %$intersection
  %assign %$cardinality 0
%endmacro

; Shall be called after INTERSECT macro to close corresponding context.
%macro END_INTERSECT 0.nolist
  %pop _INTERSECT_CTX_
%endmacro

; The macro searches intersection between two lists.
; Input: two comma-separated lists, enclosed in curly braces.
; Output:
; - Intersection will be located in the %$instersection context macro (can be empty).
; - Count of intersection elements list will be stored in the %$cardinality context variable.
%macro INTERSECT 2.nolist
  %ifnctx _INTERSECT_CTX_
    %fatal "Not in the context: _INTERSECT_CTX_"
  %endif

  %xdefine %%list1 %1
  %xdefine %%list2 %2

  FOREACH %%list1,{?INTERSECT_BODY {%%list2},}
%endmacro

; Helper macro to concatenate two lists.
; The result will be stored in the 3rd parameter that must be a macro identifier.
%macro CONCATENATE 3.nolist
  %ifnid %3
    %fatal "CONCATENATE: 3rd parameter must be a macro identifier."
  %endif
  %define %3 %[%1]
  %ifnempty %3
    %ifnempty %2
      %define %3 %[%3],%[%2]
    %endif
  %else
    %define %3 %[%2]
  %endif
%endmacro

; Helper macro that searches the specified element in the input list.
; Input:
; - Last parameter - target element
; - First parameters refer to the list where the search is processed.
; Output:
; - The macro is context dependent and upon the element is found, the context macro %$elem_exists will be defined.
%macro ?FIND 2-*.nolist
  %ifnctx _FIND_CTX_
    %fatal "Not in the context: _FIND_CTX_"
  %endif
  %rotate -1
  %xdefine %%elem_to_check %1
  %undef %$elem_exists

  %rep %0-1
    %rotate -1
    %ifidni %%elem_to_check, %1
      %define %$elem_exists %1
      %exitrep
    %endif
  %endrep
%endmacro

; Macro that finds and collects intersection elements. To be used as INTERSECT macro functor.
%macro ?INTERSECT_BODY 2.nolist
  %xdefine %%list %1
  %xdefine %%elem %2

  %push _FIND_CTX_
  ?FIND %%list,%%elem
  %ifdef %$elem_exists
    %ifempty %$$intersection
      %define %$$intersection %2
    %else
      %define %$$intersection %[%$$intersection],%%elem
    %endif
    %assign %$$cardinality %$$cardinality + 1
  %endif
  %pop _FIND_CTX_
%endmacro

%endif
