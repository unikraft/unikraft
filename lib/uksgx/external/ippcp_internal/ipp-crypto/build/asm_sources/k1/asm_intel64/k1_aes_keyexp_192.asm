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

%ifdef LINUX
%define KEY		rdi
%define EXP_ENC_KEYS	rsi
%define EXP_DEC_KEYS	rdx
%else
%define KEY		rcx
%define EXP_ENC_KEYS	rdx
%define EXP_DEC_KEYS	r8
%endif

%macro key_expansion_1_192_avx 1
	;; Assumes the xmm3 includes all zeros at this point.
        vpshufd xmm2, xmm2, 11111111b
        vshufps xmm3, xmm3, xmm1, 00010000b
        vpxor xmm1, xmm1, xmm3
        vshufps xmm3, xmm3, xmm1, 10001100b
        vpxor xmm1, xmm1, xmm3
	vpxor xmm1, xmm1, xmm2
	vmovdqu [EXP_ENC_KEYS + %1], xmm1
%endmacro

; Calculate w10 and w11 using calculated w9 and known w4-w5
%macro key_expansion_2_192_avx 1
		vmovdqa xmm5, xmm4
		vpslldq xmm5, xmm5, 4
		vshufps xmm6, xmm6, xmm1, 11110000b
		vpxor xmm6, xmm6, xmm5
		vpxor xmm4, xmm4, xmm6
		vpshufd xmm7, xmm4, 00001110b
		vmovdqu [EXP_ENC_KEYS + %1], xmm7
%endmacro

%macro key_dec_192_avx 1
  	vmovdqa  xmm0, [EXP_ENC_KEYS + 16 * %1]
	vaesimc	xmm1, xmm0
	vmovdqa [EXP_DEC_KEYS + 16 * (12 - %1)], xmm1
%endmacro

section .text
IPPASM aes_keyexp_192_enc, PUBLIC

%ifndef LINUX
	sub	rsp, 16*2 + 8
	vmovdqa	[rsp + 0*16], xmm6
	vmovdqa	[rsp + 1*16], xmm7
%endif

	vmovq xmm7, [KEY + 16]	; loading the AES key, 64 bits
        vmovq [EXP_ENC_KEYS + 16], xmm7  ; Storing key in memory where all key expansion
        vpshufd xmm4, xmm7, 01001111b
        vmovdqu xmm1, [KEY]	; loading the AES key, 128 bits
        vmovdqu [EXP_ENC_KEYS], xmm1  ; Storing key in memory where all key expansion

        vpxor xmm3, xmm3, xmm3
        vpxor xmm6, xmm6, xmm6

        vaeskeygenassist xmm2, xmm4, 0x1      ; Complete round key 1 and generate round key 2
        key_expansion_1_192_avx 24
		key_expansion_2_192_avx 40

        vaeskeygenassist xmm2, xmm4, 0x2     ; Generate round key 3 and part of round key 4
        key_expansion_1_192_avx 48
		key_expansion_2_192_avx 64

        vaeskeygenassist xmm2, xmm4, 0x4     ; Complete round key 4 and generate round key 5
        key_expansion_1_192_avx 72
		key_expansion_2_192_avx 88

        vaeskeygenassist xmm2, xmm4, 0x8     ; Generate round key 6 and part of round key 7
        key_expansion_1_192_avx 96
		key_expansion_2_192_avx 112

        vaeskeygenassist xmm2, xmm4, 0x10    ; Complete round key 7 and generate round key 8
        key_expansion_1_192_avx 120
		key_expansion_2_192_avx 136

        vaeskeygenassist xmm2, xmm4, 0x20    ; Generate round key 9 and part of round key 10
        key_expansion_1_192_avx 144
		key_expansion_2_192_avx 160

        vaeskeygenassist xmm2, xmm4, 0x40    ; Complete round key 10 and generate round key 11
        key_expansion_1_192_avx 168
		key_expansion_2_192_avx 184

        vaeskeygenassist xmm2, xmm4, 0x80   ; Generate round key 12
        key_expansion_1_192_avx 192

        clear_scratch_gps_asm
        clear_scratch_xmms_avx_asm

%ifndef LINUX
	vmovdqa	xmm6, [rsp + 0*16]
	vmovdqa	xmm7, [rsp + 1*16]
	add	rsp, 16*2 + 8
%endif

	ret

ENDFUNC aes_keyexp_192_enc

%endif 
