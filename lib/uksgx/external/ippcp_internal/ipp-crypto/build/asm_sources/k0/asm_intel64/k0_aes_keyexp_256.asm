;===============================================================================
; Copyright 2020-2021 Intel Corporation
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

; Routine to do AES key expansion
%include "os.inc"
%define NO_AESNI_RENAME
%include "clear_regs.inc"

%include "asmdefs.inc"
%include "ia_32e.inc"

%if (_IPP32E >= _IPP32E_K0)

; Uses the f() function of the aeskeygenassist result
%macro key_expansion_256_avx 0
	;; Assumes the xmm3 includes all zeros at this point.
        vpshufd	xmm2, xmm2, 11111111b
        vshufps	xmm3, xmm3, xmm1, 00010000b
        vpxor	xmm1, xmm1, xmm3
        vshufps	xmm3, xmm3, xmm1, 10001100b
        vpxor	xmm1, xmm1, xmm3
	vpxor	xmm1, xmm1, xmm2
%endmacro

; Uses the SubWord function of the aeskeygenassist result
%macro key_expansion_256_avx_2 0
	;; Assumes the xmm3 includes all zeros at this point.
        vpshufd	xmm2, xmm2, 10101010b
        vshufps	xmm3, xmm3, xmm4, 00010000b
        vpxor	xmm4, xmm4, xmm3
        vshufps	xmm3, xmm3, xmm4, 10001100b
        vpxor	xmm4, xmm4, xmm3
	vpxor	xmm4, xmm4, xmm2
%endmacro

%ifdef LINUX
%define KEY		rdi
%define EXP_ENC_KEYS	rsi
%define EXP_DEC_KEYS	rdx
%else
%define KEY		rcx
%define EXP_ENC_KEYS	rdx
%define EXP_DEC_KEYS	r8
%endif

section .text

IPPASM aes_keyexp_256_enc, PUBLIC

        vmovdqu	xmm1, [KEY]			; loading the AES key
	vmovdqa	[EXP_ENC_KEYS + 16*0], xmm1

        vmovdqu	xmm4, [KEY+16]			; loading the AES key
	vmovdqa	[EXP_ENC_KEYS + 16*1], xmm4

        vpxor xmm3, xmm3, xmm3			; Required for the key_expansion.

        vaeskeygenassist xmm2, xmm4, 0x1		; Generating round key 2
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*2], xmm1

        vaeskeygenassist xmm2, xmm1, 0x1		; Generating round key 3
        key_expansion_256_avx_2
	vmovdqa	[EXP_ENC_KEYS + 16*3], xmm4

        vaeskeygenassist xmm2, xmm4, 0x2		; Generating round key 4
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*4], xmm1

        vaeskeygenassist xmm2, xmm1, 0x2		; Generating round key 5
        key_expansion_256_avx_2
	vmovdqa	[EXP_ENC_KEYS + 16*5], xmm4

        vaeskeygenassist xmm2, xmm4, 0x4		; Generating round key 6
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*6], xmm1

        vaeskeygenassist xmm2, xmm1, 0x4		; Generating round key 7
        key_expansion_256_avx_2
	vmovdqa	[EXP_ENC_KEYS + 16*7], xmm4

        vaeskeygenassist xmm2, xmm4, 0x8		; Generating round key 8
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*8], xmm1

        vaeskeygenassist xmm2, xmm1, 0x8		; Generating round key 9
        key_expansion_256_avx_2
	vmovdqa	[EXP_ENC_KEYS + 16*9], xmm4

        vaeskeygenassist xmm2, xmm4, 0x10	; Generating round key 10
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*10], xmm1

        vaeskeygenassist xmm2, xmm1, 0x10	; Generating round key 11
        key_expansion_256_avx_2
	vmovdqa	[EXP_ENC_KEYS + 16*11], xmm4

        vaeskeygenassist xmm2, xmm4, 0x20	; Generating round key 12
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*12], xmm1

        vaeskeygenassist xmm2, xmm1, 0x20	; Generating round key 13
        key_expansion_256_avx_2
	vmovdqa	[EXP_ENC_KEYS + 16*13], xmm4

        vaeskeygenassist xmm2, xmm4, 0x40	; Generating round key 14
        key_expansion_256_avx
	vmovdqa	[EXP_ENC_KEYS + 16*14], xmm1

        clear_scratch_gps_asm
        clear_scratch_xmms_avx_asm

        ret

ENDFUNC aes_keyexp_256_enc

%endif 
