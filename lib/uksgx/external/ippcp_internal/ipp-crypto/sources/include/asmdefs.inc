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

%ifndef __ASMDEFS_INC__
%define __ASMDEFS_INC__ 1

%assign _IPP_PX 0    ; pure C-code ia32
%assign _IPP_M5 1    ; Intel(R) Quark(TM) processor - ia32
%assign _IPP_W7 8    ; Intel(R) Streaming SIMD Extensions 2 - ia32
%assign _IPP_T7 16   ; Intel(R) Streaming SIMD Extensions 3 - ia32
%assign _IPP_V8 32   ; Supplemental Streaming SIMD Extensions 3 (SSSE3)
%assign _IPP_S8 33   ; SSSE3 + MOVBE instruction - ia32
%assign _IPP_P8 64   ; Intel(R) Streaming SIMD Extensions 4.2 - ia32
%assign _IPP_G9 128  ; Intel(R) Advanced Vector Extensions - ia32
%assign _IPP_H9 256  ; Intel(R) Advanced Vector Extensions 2 - ia32
%assign _IPP_I0 512  ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon Phi(TM) processor (formerly Knight Landing) - ia32
%assign _IPP_S0 1024 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon(R) processor (formerly Skylake) - ia32

%assign _IPP32E_PX _IPP_PX ; pure C-code x64
%assign _IPP32E_M7 32   ; Intel(R) Streaming SIMD Extensions 3 - intel64
%assign _IPP32E_U8 64   ; Supplemental Streaming SIMD Extensions 3 (SSSE3) - intel64
%assign _IPP32E_N8 65   ; SSSE3 + MOVBE instruction - intel64
%assign _IPP32E_Y8 128  ; Intel(R) Streaming SIMD Extensions 4.2 - intel64
%assign _IPP32E_E9 256  ; Intel(R) Advanced Vector Extensions - intel64
%assign _IPP32E_L9 512  ; Intel(R) Advanced Vector Extensions 2 - intel64
%assign _IPP32E_N0 1024 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon Phi(TM) processor (formerly Knight Landing) - intel64
%assign _IPP32E_K0 2048 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon(R) processor (formerly Skylake) - intel64
%assign _IPP32E_K1 4096 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon(R) processor (formerly Icelake) - intel64

%assign _IPP _IPP_PX
%assign _IPP32E _IPP32E_PX

%ifdef _M5 ; Intel(R) Quark(TM) processor - ia32
  %assign _IPP _IPP_M5
%elifdef _W7 ; Intel(R) Streaming SIMD Extensions 2 - ia32
  %assign _IPP _IPP_W7
%elifdef _T7 ; Intel(R) Streaming SIMD Extensions 3 - ia32
  %assign _IPP _IPP_T7
%elifdef _V8 ; Supplemental Streaming SIMD Extensions 3 (SSSE3)
  %assign _IPP _IPP_V8
%elifdef _S8 ; SSSE3 + MOVBE instruction - ia32
  %assign _IPP _IPP_S8
%elifdef _P8 ; Intel(R) Streaming SIMD Extensions 4.2 - ia32
  %assign _IPP _IPP_P8
%elifdef _G9 ; Intel(R) Advanced Vector Extensions - ia32
  %assign IPP_ALIGN_FACTOR 32
  %assign _IPP _IPP_G9
%elifdef _H9 ; Intel(R) Advanced Vector Extensions 2 - ia32
  %assign IPP_ALIGN_FACTOR 32
  %assign _IPP _IPP_H9
%elifdef _S0 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon(R) processor (formerly Skylake) - ia32
  %assign IPP_ALIGN_FACTOR 64
  %assign _IPP _IPP_S0
%elifdef _M7 ; Intel(R) Streaming SIMD Extensions 3 - intel64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_M7
%elifdef _U8 ; Supplemental Streaming SIMD Extensions 3 (SSSE3) - intel64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_U8
%elifdef _N8 ; SSSE3 + MOVBE instruction - intel64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_N8
%elifdef _Y8 ; Intel(R) Streaming SIMD Extensions 4.2 - intel64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_Y8
%elifdef _E9 ; Intel(R) Advanced Vector Extensions - intel64
  %assign IPP_ALIGN_FACTOR 32
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_E9
%elifdef _L9 ; Intel(R) Advanced Vector Extensions 2 - intel64
  %assign IPP_ALIGN_FACTOR 32
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_L9
%elifdef _N0 ; Intel(R) Advanced Vector Extensions 512 (formerly Knights Landing) - intel64
  %assign IPP_ALIGN_FACTOR 64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_N0
%elifdef _K0 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon(R) processor (formerly Skylake) - intel64
  %assign IPP_ALIGN_FACTOR 64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_K0
%elifdef _K1 ; Intel(R) Advanced Vector Extensions 512 - Intel(R) Xeon(R) processor (formerly Icelake) - intel64
  %assign IPP_ALIGN_FACTOR 64
  %assign _IPP _IPP_PX
  %assign _IPP32E _IPP32E_K1
%else
  %assign _IPP _IPP_PX ; pure C-code
%endif

%if (_IPP > _IPP_H9) || (_IPP32E > _IPP32E_L9)
  %assign IPP_ALIGN_FACTOR 64
%elif (_IPP > _IPP_P8) || (_IPP32E > _IPP32E_Y8)
  %assign IPP_ALIGN_FACTOR 32
%else
  %assign IPP_ALIGN_FACTOR 16
%endif

; noexec stack
%ifdef LINUX32
  %ifndef OSX32
section .note.GNU-stack noalloc noexec nowrite progbits
  %endif
%endif

; noexec stack
%ifdef LINUX32E
  %ifndef OSXEM64T
    %ifndef _ARCH_KNC
section .note.GNU-stack noalloc noexec nowrite progbits
    %endif
  %endif
%endif


%ifidn __OUTPUT_FORMAT__, elf32
  %assign IPP_BINARY_FORMAT 0
%elifidn __OUTPUT_FORMAT__, elf64
  %assign IPP_BINARY_FORMAT 1
%elifidn __OUTPUT_FORMAT__, macho64
  %assign IPP_BINARY_FORMAT 2
%elifidn __OUTPUT_FORMAT__, win32
  %assign IPP_BINARY_FORMAT 3
%elifidn __OUTPUT_FORMAT__, win64
  %assign IPP_BINARY_FORMAT 4
%else
  %fatal Unsupported output format: __OUTPUT_FORMAT__. Shall be: elf32, elf64, win32, win64, macho64
%endif

%ifdef _MERGED_BLD
  %assign _OWN_MERGED_BLD 1
%endif ; _MERGED_BLD

; data compilation definitions: merged builds shall compile data only as
; part of one single object build to avoid multiple definition warnings at link time
%ifndef _MERGED_BLD
  %assign _IPP_DATA 1
%else
  %if (_IPP == _IPP_G9) || (_IPP32E == _IPP32E_E9)
    %assign _IPP_DATA 1
  %endif
%endif ; _MERGED_BLD

; Definitions of sizeof(type)
%iassign ZWORD_size 64 ; zmm-word
%iassign YWORD_size 32 ; ymm-word
%iassign OWORD_size 16 ; octo-word
%iassign TWORD_size 10 ; ten-bytes word
%iassign QWORD_size 8  ; quad-word
%iassign DWORD_size 4  ; double-word
%iassign WORD_size 2
%iassign BYTE_size 1

%idefine YMMWORD YWORD
%idefine XMMWORD OWORD
%iassign YMMWORD_size YWORD_size
%iassign XMMWORD_size OWORD_size

%idefine sizeof(_x_) _x_%+_size

%endif
;;;;;;;;;;;;;;;;;;;;;;;;;; End of file "asmdefs.inc" ;;;;;;;;;;;;;;;;;;;;;;;;;;
