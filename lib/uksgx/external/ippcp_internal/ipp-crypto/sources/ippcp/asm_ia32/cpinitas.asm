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
%include "ia_emm.inc"

%assign LOCAL_ALIGN_FACTOR 32

segment .text align=LOCAL_ALIGN_FACTOR

%ifdef _IPP_DATA

;####################################################################
;#          void ownGetReg( int* buf, int valueEAX, int valueECX ); #
;####################################################################

%define buf       [esp+12]
%define valueEAX  [esp+16]
%define valueECX  [esp+20]

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpGetReg,PUBLIC

        push    ebx
        push    esi
        mov     eax, valueEAX
        mov     ecx, valueECX
        xor     ebx, ebx
        xor     edx, edx
        mov     esi, buf
        cpuid
        mov     [esi], eax
        mov     [esi + 4], ebx
        mov     [esi + 8], ecx
        mov     [esi + 12], edx
        pop     esi
        pop     ebx
        ret
ENDFUNC cpGetReg

;###################################################

; Feature information after XGETBV(ECX=0), EAX, bits 2,1 ( XMM state and YMM state are enabled by OS )
%assign XGETBV_MASK          06h
; OSXSAVE support, feature information after cpuid(1), ECX, bit 27 ( XGETBV is enabled by OS )
%assign XSAVEXGETBV_FLAG     8000000h
%assign XGETBV_AVX512_MASK   0E0h

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_is_avx_extension,PUBLIC
  USES_GPR ebx,edx,ecx
         mov eax, 1
         cpuid
         xor   eax, eax
         and   ecx, 018000000h
         cmp   ecx, 018000000h
         jne  .not_avx
         xor   ecx, ecx
         db 00fh,001h,0d0h        ; xgetbv
         mov   ecx, eax
         xor   eax, eax
         and   ecx, XGETBV_MASK
         cmp   ecx, XGETBV_MASK
         jne  .not_avx
         mov   eax, 1
.not_avx:
         REST_GPR
         ret
ENDFUNC cp_is_avx_extension

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_is_avx512_extension,PUBLIC
  USES_GPR ebx,edx,ecx
         mov eax, 1
         cpuid
         xor   eax, eax
         and   ecx, XSAVEXGETBV_FLAG
         cmp   ecx, XSAVEXGETBV_FLAG
         jne  .not_avx512
         xor   ecx, ecx
         db 00fh,001h,0d0h        ; xgetbv
         mov   ecx, eax
         xor   eax, eax
         and   ecx, XGETBV_AVX512_MASK
         cmp   ecx, XGETBV_AVX512_MASK
         jne  .not_avx512
         mov   eax, 1
.not_avx512:
         REST_GPR
         ret
ENDFUNC cp_is_avx512_extension

%ifdef LINUX32
  %ifndef OSX32

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __ashldi3,PUBLIC,WEAK
        mov    eax, [esp+4]
        mov    edx, [esp+8]
        mov    ecx, [esp+12]
        test   cl, 20H
        je    .less
        mov    edx, eax
        xor    eax, eax
        shl    edx, cl
        ret
.less:
        shld   edx, eax, cl
        shl    eax, cl
        ret
ENDFUNC __ashldi3

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __ashrdi3,PUBLIC,WEAK
        mov    eax, [esp+4]
        mov    edx, [esp+8]
        mov    ecx, [esp+12]
        test   cl, 20H
        je    .less
        mov    eax, edx
        sar    edx, 1FH
        sar    eax, cl
        ret
.less:
        shrd   eax, edx, cl
        sar    edx, cl
        ret
ENDFUNC __ashrdi3

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __divdi3,PUBLIC,WEAK
        xor    ecx, ecx
        mov    eax, dword     [8+esp]
        or     eax, eax
        jge   .Apositive
        mov    ecx, 1
        mov    edx, dword     [4+esp]
        neg    eax
        neg    edx
        sbb    eax, 0
        mov    dword     [4+esp], edx
        mov    dword     [8+esp], eax
.Apositive:
        mov    eax, dword     [16+esp]
        or     eax,eax
        jge   .ABpositive
        sub    ecx, 1
        mov    edx, dword     [12+esp]
        neg    eax
        neg    edx
        sbb    eax, 0
        mov    dword     [12+esp], edx
        mov    dword     [16+esp], eax
.ABpositive:
        mov    eax, dword     [16+esp]
        xor    edx, edx
        push   ecx
        test   eax, eax
        jne   .non_zero_hi
        mov    eax, dword     [12+esp]
        div    dword     [16+esp]
        mov    ecx, eax
        mov    eax, dword     [8+esp]
        div    dword     [16+esp]
        mov    edx, ecx
        jmp   .return
.non_zero_hi:
        mov    ecx, dword     [12+esp]
        cmp    eax, ecx
        jb    .divisor_greater
        jne   .return_zero
        mov    ecx, dword     [16+esp]
        mov    eax, dword     [8+esp]
        cmp    ecx, eax
        ja    .return_zero
.return_one:
        mov    eax, 1
        jmp   .return
.return_zero:
        add    esp, 4
        xor    eax, eax
        ret
.divisor_greater:
        test   eax, 80000000h
        jne   .return_one
.find_hi_bit:
        bsr    ecx, eax
        add    ecx, 1
.hi_bit_found:
        mov    edx, dword     [16+esp]
        push   ebx
        shrd   edx, eax, cl
        mov    ebx, edx
        mov    eax, dword     [12+esp]
        mov    edx, dword     [16+esp]
        shrd   eax, edx, cl
        shr    edx, cl
.make_div:
        div    ebx
        mov    ebx, eax
        mul    dword     [24+esp]
        mov    ecx, eax
        mov    eax, dword     [20+esp]
        mul    ebx
        add    edx, ecx
        jb    .need_dec
        cmp    dword     [16+esp], edx
        jb    .need_dec
        ja    .after_dec
        cmp    dword     [12+esp], eax
        jae   .after_dec
.need_dec:
        sub    ebx, 1
.after_dec:
        xor    edx, edx
        mov    eax, ebx
        pop    ebx
.return:
        pop    ecx
        test   ecx, ecx
        jne   .ch_sign
        ret
.ch_sign:
        neg    edx
        neg    eax
        sbb    edx, 0
        ret
ENDFUNC __divdi3

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __udivdi3,PUBLIC,WEAK
        xor    ecx,ecx
.ABpositive:
        mov    eax, dword     [16+esp]
        xor    edx, edx
        push   ecx
        test   eax, eax
        jne   .non_zero_hi
        mov    eax, dword     [12+esp]
        div    dword     [16+esp]
        mov    ecx, eax
        mov    eax, dword     [8+esp]
        div    dword     [16+esp]
        mov    edx, ecx
        jmp   .return
.non_zero_hi:
        mov    ecx, dword     [12+esp]
        cmp    eax, ecx
        jb    .divisor_greater
        jne   .return_zero
        mov    ecx, dword     [16+esp]
        mov    eax, dword     [8+esp]
        cmp    ecx, eax
        ja    .return_zero
.return_one:
        mov    eax, 1
        jmp   .return
.return_zero:
        add    esp, 4
        xor    eax, eax
        ret
.divisor_greater:
        test   eax, 80000000h
        jne   .return_one
.find_hi_bit:
        bsr    ecx, eax
        add    ecx, 1
.hi_bit_found:
        mov    edx, dword     [16+esp]
        push   ebx
        shrd   edx, eax, cl
        mov    ebx, edx
        mov    eax, dword     [12+esp]
        mov    edx, dword     [16+esp]
        shrd   eax, edx, cl
        shr    edx, cl
.make_div:
        div    ebx
        mov    ebx, eax
        mul    dword     [24+esp]
        mov    ecx, eax
        mov    eax, dword     [20+esp]
        mul    ebx
        add    edx, ecx
        jb    .need_dec
        cmp    dword     [16+esp], edx
        jb    .need_dec
        ja    .after_dec
        cmp    dword     [12+esp], eax
        jae   .after_dec
.need_dec:
        sub    ebx, 1
.after_dec:
        xor    edx, edx
        mov    eax, ebx
        pop    ebx
.return:
        pop    ecx
        test   ecx, ecx
        jne   .ch_sign
        ret
.ch_sign:
        neg    edx
        neg    eax
        sbb    edx, 0
        ret
ENDFUNC __udivdi3

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __moddi3,PUBLIC,WEAK
        sub    esp, 8
        mov    dword     [esp], 0
        mov    eax, dword     [esp+16]
        or     eax, eax
        jge   .Apositive
        inc    dword     [esp]
        mov    edx, dword     [esp+12]
        neg    eax
        neg    edx
        sbb    eax, 0
        mov    dword     [esp+16], eax
        mov    dword     [esp+12], edx

.Apositive:
        mov    eax, dword     [esp+24]
        or     eax, eax
        jge   .ABpositive
        mov    edx, dword     [esp+20]
        neg    eax
        neg    edx
        sbb    eax, 0
        mov    dword     [esp+24], eax
        mov    dword     [esp+20], edx
        jmp   .ABpositive
        lea    esi, [esi]
        lea    edi, [edi]

        sub    esp, 8
        mov    dword     [esp], 0

.ABpositive:
        mov    eax, dword     [esp+24]
        test   eax, eax
        jne   .non_zero_hi
        mov    eax, dword     [esp+16]
        mov    edx, 0
        div    dword     [esp+20]
        mov    ecx, eax
        mov    eax, dword     [12+esp]
        div    dword     [20+esp]
        mov    eax, edx
        xor    edx, edx
        jmp   .return

.non_zero_hi:
        mov    ecx, dword     [16+esp]
        cmp    eax, ecx
        jb    .divisor_greater
        jne   .return_devisor
        mov    ecx, dword     [20+esp]
        cmp    ecx, dword     [12+esp]
        ja    .return_devisor

.return_dif:
        mov    eax, dword     [12+esp]
        mov    edx, dword     [16+esp]
        sub    eax, dword     [20+esp]
        sbb    edx, dword     [24+esp]
        jmp   .return

.return_devisor:
        mov    eax, dword     [12+esp]
        mov    edx, dword     [16+esp]
        jmp   .return

.divisor_greater:
        test   eax, 80000000h
        jne   .return_dif

.find_hi_bit:
        bsr    ecx, eax
        add    ecx, 1

.hi_bit_found:
        push   ebx
        mov    edx, dword     [24+esp]
        shrd   edx, eax, cl
        mov    ebx, edx
        mov    eax, dword     [16+esp]
        mov    edx, dword     [20+esp]
        shrd   eax, edx, cl
        shr    edx, cl
        div    ebx
        mov    ebx, eax

.multiple:
        mul    dword     [28+esp]
        mov    ecx, eax
        mov    eax, dword     [24+esp]
        mul    ebx
        add    edx, ecx
        jb    .need_dec
        cmp    dword     [20+esp], edx
        jb    .need_dec
        ja    .after_dec
        cmp    dword     [16+esp], eax
        jb    .need_dec

.after_dec:
        mov    ebx, eax
        mov    eax, dword     [16+esp]
        sub    eax, ebx
        mov    ebx, edx
        mov    edx, dword     [20+esp]
        sbb    edx, ebx
        pop    ebx

.return:
        mov    dword     [4+esp], eax
        mov    eax, dword     [esp]
        test   eax, eax
        jne   .ch_sign
        mov    eax, dword     [4+esp]
        add    esp, 8
        ret

.ch_sign:
        mov    eax, dword     [4+esp]
        neg    edx
        neg    eax
        sbb    edx, 0
        add    esp, 8
        ret

.need_dec:
        dec    ebx
        mov    eax, ebx
        jmp   .multiple
ENDFUNC __moddi3

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __umoddi3,PUBLIC,WEAK
        sub    esp, 8
        mov    dword     [esp], 0

.ABpositive:
        mov    eax, dword     [esp+24]
        test   eax, eax
        jne   .non_zero_hi
        mov    eax, dword     [esp+16]
        mov    edx, 0
        div    dword     [esp+20]
        mov    ecx, eax
        mov    eax, dword     [12+esp]
        div    dword     [20+esp]
        mov    eax, edx
        xor    edx, edx
        jmp   .return

.non_zero_hi:
        mov    ecx, dword     [16+esp]
        cmp    eax, ecx
        jb    .divisor_greater
        jne   .return_devisor
        mov    ecx, dword     [20+esp]
        cmp    ecx, dword     [12+esp]
        ja    .return_devisor

.return_dif:
        mov    eax, dword     [12+esp]
        mov    edx, dword     [16+esp]
        sub    eax, dword     [20+esp]
        sbb    edx, dword     [24+esp]
        jmp   .return

.return_devisor:
        mov    eax, dword     [12+esp]
        mov    edx, dword     [16+esp]
        jmp   .return

.divisor_greater:
        test   eax, 80000000h
        jne   .return_dif

.find_hi_bit:
        bsr    ecx, eax
        add    ecx, 1

.hi_bit_found:
        push   ebx
        mov    edx, dword     [24+esp]
        shrd   edx, eax, cl
        mov    ebx, edx
        mov    eax, dword     [16+esp]
        mov    edx, dword     [20+esp]
        shrd   eax, edx, cl
        shr    edx, cl
        div    ebx
        mov    ebx, eax

.multiple:
        mul    dword     [28+esp]
        mov    ecx, eax
        mov    eax, dword     [24+esp]
        mul    ebx
        add    edx, ecx
        jb    .need_dec
        cmp    dword     [20+esp], edx
        jb    .need_dec
        ja    .after_dec
        cmp    dword     [16+esp], eax
        jb    .need_dec

.after_dec:
        mov    ebx, eax
        mov    eax, dword     [16+esp]
        sub    eax, ebx
        mov    ebx, edx
        mov    edx, dword     [20+esp]
        sbb    edx, ebx
        pop    ebx

.return:
        mov    dword     [4+esp], eax
        mov    eax, dword     [esp]
        test   eax, eax
        jne   .ch_sign
        mov    eax, dword     [4+esp]
        add    esp, 8
        ret

.ch_sign:
        mov    eax, dword     [4+esp]
        neg    edx
        neg    eax
        sbb    edx, 0
        add    esp, 8
        ret

.need_dec:
        dec    ebx
        mov    eax, ebx
        jmp   .multiple
ENDFUNC __umoddi3

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC __muldi3,PUBLIC,WEAK
        mov    eax, dword     [esp+8]
        mul    dword     [esp+12]
        mov    ecx, eax
        mov    eax, dword     [esp+4]
        mul    dword     [esp+16]
        add    ecx, eax
        mov    eax, dword     [esp+4]
        mul    dword     [esp+12]
        add    edx, ecx
        ret
ENDFUNC __muldi3

  %endif;  IFNDEF OSX32
%endif; IFDEF LINUX32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cp_get_pentium_counter,PUBLIC
         rdtsc
         ret
ENDFUNC cp_get_pentium_counter

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStartTscp,PUBLIC
         push     ebx
         xor      eax, eax
         cpuid
         pop      ebx
         rdtscp
         ret
ENDFUNC cpStartTscp

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStopTscp,PUBLIC
         rdtscp
         push     eax
         push     edx
         push     ebx
         xor      eax, eax
         cpuid
         pop      ebx
         pop      edx
         pop      eax
         ret
ENDFUNC cpStopTscp

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStartTsc,PUBLIC
         push     ebx
         xor      eax, eax
         cpuid
         pop      ebx
         rdtsc
         ret
ENDFUNC cpStartTsc

align LOCAL_ALIGN_FACTOR
DECLARE_FUNC cpStopTsc,PUBLIC
         rdtsc
         push     eax
         push     edx
         push     ebx
         xor      eax, eax
         cpuid
         pop      ebx
         pop      edx
         pop      eax
         ret
ENDFUNC cpStopTsc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;*****************************************
; int cpGetCacheSize( int* tableCache );
align LOCAL_ALIGN_FACTOR
%define table   [36+esp]
DECLARE_FUNC cpGetCacheSize,PUBLIC
        push    edi
        push    esi
        push    ebx
        push    ebp
        sub     esp, 16

        mov     edi, table
        mov     ebp, esp
        xor     esi, esi

        mov     eax, 2
        cpuid

        cmp     al, 1
        jne    .GetCacheSize_11

        test    eax, 080000000h
        jz     .GetCacheSize_00
        xor     eax, eax
.GetCacheSize_00:
        test    ebx, 080000000h
        jz     .GetCacheSize_01
        xor     ebx, ebx
.GetCacheSize_01:
        test    ecx, 080000000h
        jz     .GetCacheSize_02
        xor     ecx, ecx
.GetCacheSize_02:
        test    edx, 080000000h
        jz     .GetCacheSize_03
        xor     edx, edx

.GetCacheSize_03:
        test    eax, eax
        jz     .GetCacheSize_04
        mov     [ebp], eax
        add     ebp, 4
        add     esi, 3
.GetCacheSize_04:
        test    ebx, ebx
        jz     .GetCacheSize_05
        mov     [ebp], ebx
        add     ebp, 4
        add     esi, 4
.GetCacheSize_05:
        test    ecx, ecx
        jz     .GetCacheSize_06
        mov     [ebp], ecx
        add     ebp, 4
        add     esi, 4
.GetCacheSize_06:
        test    edx, edx
        jz     .GetCacheSize_07
        mov     [ebp], edx
        add     esi, 4

.GetCacheSize_07:
        test    esi, esi
        jz     .GetCacheSize_11
        mov     eax, -1
.GetCacheSize_08:
        xor     edx, edx
        add     edx, [edi]
        jz      .ExitGetCacheSize00
        add     edi, 8
        mov     ecx, esi
.GetCacheSize_09:
        cmp     dl, BYTE     [esp + ecx]
        je     .GetCacheSize_10
        dec     ecx
        jnz    .GetCacheSize_09
        jmp    .GetCacheSize_08

.GetCacheSize_10:
        mov     eax, [edi - 4]

.ExitGetCacheSize00:
        add     esp, 16
        pop     ebp
        pop     ebx
        pop     esi
        pop     edi
        ret

.GetCacheSize_11:
        mov     eax, -1
        jmp    .ExitGetCacheSize00
ENDFUNC cpGetCacheSize

; *****************************************

%endif ; IPP_DATA
