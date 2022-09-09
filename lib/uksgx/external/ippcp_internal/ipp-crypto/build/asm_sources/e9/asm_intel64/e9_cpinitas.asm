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
%include "ia_32e.inc"

%assign LOCAL_ALIGN_FACTOR 32

%ifdef _IPP_DATA

segment .text align=LOCAL_ALIGN_FACTOR

;####################################################################
;#          void cpGetReg( int* buf, int valueEAX, int valueECX ); #
;####################################################################

%ifdef WIN32E
  %define buf       rcx
  %define valueEAX  edx
  %define valueECX  r8d
%else
  %define buf       rdi
  %define valueEAX  esi
  %define valueECX  edx
%endif

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpGetReg,PUBLIC
        push rbx
        movsxd  r9, valueEAX
        movsxd  r10, valueECX
        mov     r11, buf

        mov     rax, r9
        mov     rcx, r10
        xor     ebx, ebx
        xor     edx, edx
        cpuid
        mov     [r11], eax
        mov     [r11 + 4], ebx
        mov     [r11 + 8], ecx
        mov     [r11 + 12], edx
        pop rbx
        ret
ENDFUNC cpGetReg

;###################################################

; OSXSAVE support, feature information after cpuid(1), ECX, bit 27 ( XGETBV is enabled by OS )
%assign XSAVEXGETBV_FLAG   8000000h

; Feature information after XGETBV(ECX=0), EAX, bits 2,1 ( XMM state and YMM state are enabled by OS )
%assign XGETBV_MASK        06h

%assign XGETBV_AVX512_MASK 0E0h

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_is_avx_extension,PUBLIC
         push  rbx
         mov   eax, 1
         cpuid
         xor   eax, eax
         and   ecx, 018000000h
         cmp   ecx, 018000000h
         jne   .not_avx
         xor   ecx, ecx
         db 00fh,001h,0d0h        ; xgetbv
         mov   ecx, eax
         xor   eax, eax
         and   ecx, XGETBV_MASK
         cmp   ecx, XGETBV_MASK
         jne   .not_avx
         mov   eax, 1
.not_avx:
         pop   rbx
         ret
ENDFUNC cp_is_avx_extension

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_is_avx512_extension,PUBLIC
         push  rbx
         mov   eax, 1
         cpuid
         xor   eax, eax
         and   ecx, XSAVEXGETBV_FLAG
         cmp   ecx, XSAVEXGETBV_FLAG
         jne   .not_avx512
         xor   ecx, ecx
         db 00fh,001h,0d0h        ; xgetbv
         mov   ecx, eax
         xor   eax, eax
         and   ecx, XGETBV_AVX512_MASK
         cmp   ecx, XGETBV_AVX512_MASK
         jne   .not_avx512
         mov   eax, 1
.not_avx512:
         pop   rbx
         ret
ENDFUNC cp_is_avx512_extension

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_issue_avx512_instruction,PUBLIC
         db    062h,0f1h,07dh,048h,0efh,0c0h ; vpxord  zmm0, zmm0, zmm0
         xor   eax, eax
         ret
ENDFUNC cp_issue_avx512_instruction

%ifdef OSXEM64T
  extern _ippcpInit
%else
  extern ippcpInit
%endif

;####################################################################
;#          void ippSafeInit( );                                    #
;####################################################################

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC ippcpSafeInit,PUBLIC
        push rcx
        push rdx
%ifdef LINUX32E
        push rdi
        push rsi
%endif
        push r8
        push r9
%ifdef LINUX32E
  %ifdef OSXEM64T
    call _ippcpInit
  %else
    %ifdef IPP_PIC
      call ippcpInit wrt ..plt
    %else
      call ippcpInit
    %endif
  %endif
%else
  call ippcpInit
%endif
        pop  r9
        pop  r8
%ifdef LINUX32E
        pop  rsi
        pop  rdi
%endif
        pop  rdx
        pop  rcx
        ret
ENDFUNC ippcpSafeInit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_get_pentium_counter,PUBLIC
         rdtsc
         sal    rdx,32
         or     rax,rdx
         ret
ENDFUNC cp_get_pentium_counter

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStartTscp,PUBLIC
         push     rbx
         xor      rax, rax
         cpuid
         pop      rbx
         rdtscp
         sal      rdx,32
         or       rax,rdx
         ret
ENDFUNC cpStartTscp

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStopTscp,PUBLIC
         rdtscp
         sal      rdx,32
         or       rax,rdx
         push     rax
         push     rbx
         xor      rax, rax
         cpuid
         pop      rbx
         pop      rax
         ret
ENDFUNC cpStopTscp

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStartTsc,PUBLIC
         push     rbx
         xor      rax, rax
         cpuid
         pop      rbx
         rdtsc
         sal      rdx,32
         or       rax,rdx
         ret
ENDFUNC cpStartTsc

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStopTsc,PUBLIC
         rdtsc
         sal      rdx,32
         or       rax,rdx
         push     rax
         push     rbx
         xor      rax, rax
         cpuid
         pop      rbx
         pop      rax
         ret
ENDFUNC cpStopTsc


;*****************************************
; int cpGetCacheSize( int* tableCache );
align LOCAL_ALIGN_FACTOR
%define table rdi
DECLARE_FUNC cpGetCacheSize,PUBLIC
%assign LOCAL_FRAME 16
        USES_GPR rsi, rdi, rbx, rbp
        USES_XMM
        COMP_ABI 1

        mov     rbp, rsp
        xor     esi, esi

        mov     eax, 2
        cpuid

        cmp     al, 1
        jne     .GetCacheSize_11

        test    eax, 080000000h
        jz      .GetCacheSize_00
        xor     eax, eax
.GetCacheSize_00:
        test    ebx, 080000000h
        jz      .GetCacheSize_01
        xor     ebx, ebx
.GetCacheSize_01:
        test    ecx, 080000000h
        jz      .GetCacheSize_02
        xor     ecx, ecx
.GetCacheSize_02:
        test    edx, 080000000h
        jz      .GetCacheSize_03
        xor     edx, edx

.GetCacheSize_03:
        test    eax, eax
        jz      .GetCacheSize_04
        mov     [rbp], eax
        add     rbp, 4
        add     esi, 3
.GetCacheSize_04:
        test    ebx, ebx
        jz      .GetCacheSize_05
        mov     [rbp], ebx
        add     rbp, 4
        add     esi, 4
.GetCacheSize_05:
        test    ecx, ecx
        jz      .GetCacheSize_06
        mov     [rbp], ecx
        add     rbp, 4
        add     esi, 4
.GetCacheSize_06:
        test    edx, edx
        jz      .GetCacheSize_07
        mov     [rbp], edx
        add     esi, 4

.GetCacheSize_07:
        test    esi, esi
        jz      .GetCacheSize_11
        mov     eax, -1
.GetCacheSize_08:
        xor     edx, edx
        add     edx, [table]
        jz      .ExitGetCacheSize00
        add     table, 8
        mov     ecx, esi
.GetCacheSize_09:
        cmp     dl, BYTE [rsp + rcx]
        je      .GetCacheSize_10
        dec     ecx
        jnz     .GetCacheSize_09
        jmp     .GetCacheSize_08

.GetCacheSize_10:
        mov     eax, [table - 4]

.ExitGetCacheSize00:
        REST_XMM
        REST_GPR
        ret

.GetCacheSize_11:
        mov     eax, -1
        jmp     .ExitGetCacheSize00
ENDFUNC cpGetCacheSize

;****************************

%endif ; _IPP_DATA
