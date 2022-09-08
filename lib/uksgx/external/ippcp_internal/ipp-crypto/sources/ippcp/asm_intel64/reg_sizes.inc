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

; define d and w variants for registers

%ifndef _REG_SIZES_ASM_
%define _REG_SIZES_ASM_

%define	raxd	eax
%define raxw	ax
%define raxb	al

%define	rbxd	ebx
%define rbxw	bx
%define rbxb	bl

%define	rcxd	ecx
%define rcxw	cx
%define rcxb	cl

%define	rdxd	edx
%define rdxw	dx
%define rdxb	dl

%define	rsid	esi
%define rsiw	si
%define rsib	sil

%define	rdid	edi
%define rdiw	di
%define rdib	dil

%define	rbpd	ebp
%define rbpw	bp
%define rbpb	bpl

%define zmm0x xmm0
%define zmm1x xmm1
%define zmm2x xmm2
%define zmm3x xmm3
%define zmm4x xmm4
%define zmm5x xmm5
%define zmm6x xmm6
%define zmm7x xmm7
%define zmm8x xmm8
%define zmm9x xmm9
%define zmm10x xmm10
%define zmm11x xmm11
%define zmm12x xmm12
%define zmm13x xmm13
%define zmm14x xmm14
%define zmm15x xmm15
%define zmm16x xmm16
%define zmm17x xmm17
%define zmm18x xmm18
%define zmm19x xmm19
%define zmm20x xmm20
%define zmm21x xmm21
%define zmm22x xmm22
%define zmm23x xmm23
%define zmm24x xmm24
%define zmm25x xmm25
%define zmm26x xmm26
%define zmm27x xmm27
%define zmm28x xmm28
%define zmm29x xmm29
%define zmm30x xmm30
%define zmm31x xmm31

%define ymm0x xmm0
%define ymm1x xmm1
%define ymm2x xmm2
%define ymm3x xmm3
%define ymm4x xmm4
%define ymm5x xmm5
%define ymm6x xmm6
%define ymm7x xmm7
%define ymm8x xmm8
%define ymm9x xmm9
%define ymm10x xmm10
%define ymm11x xmm11
%define ymm12x xmm12
%define ymm13x xmm13
%define ymm14x xmm14
%define ymm15x xmm15
%define ymm16x xmm16
%define ymm17x xmm17
%define ymm18x xmm18
%define ymm19x xmm19
%define ymm20x xmm20
%define ymm21x xmm21
%define ymm22x xmm22
%define ymm23x xmm23
%define ymm24x xmm24
%define ymm25x xmm25
%define ymm26x xmm26
%define ymm27x xmm27
%define ymm28x xmm28
%define ymm29x xmm29
%define ymm30x xmm30
%define ymm31x xmm31

%define xmm0x xmm0
%define xmm1x xmm1
%define xmm2x xmm2
%define xmm3x xmm3
%define xmm4x xmm4
%define xmm5x xmm5
%define xmm6x xmm6
%define xmm7x xmm7
%define xmm8x xmm8
%define xmm9x xmm9
%define xmm10x xmm10
%define xmm11x xmm11
%define xmm12x xmm12
%define xmm13x xmm13
%define xmm14x xmm14
%define xmm15x xmm15
%define xmm16x xmm16
%define xmm17x xmm17
%define xmm18x xmm18
%define xmm19x xmm19
%define xmm20x xmm20
%define xmm21x xmm21
%define xmm22x xmm22
%define xmm23x xmm23
%define xmm24x xmm24
%define xmm25x xmm25
%define xmm26x xmm26
%define xmm27x xmm27
%define xmm28x xmm28
%define xmm29x xmm29
%define xmm30x xmm30
%define xmm31x xmm31

%define zmm0y ymm0
%define zmm1y ymm1
%define zmm2y ymm2
%define zmm3y ymm3
%define zmm4y ymm4
%define zmm5y ymm5
%define zmm6y ymm6
%define zmm7y ymm7
%define zmm8y ymm8
%define zmm9y ymm9
%define zmm10y ymm10
%define zmm11y ymm11
%define zmm12y ymm12
%define zmm13y ymm13
%define zmm14y ymm14
%define zmm15y ymm15
%define zmm16y ymm16
%define zmm17y ymm17
%define zmm18y ymm18
%define zmm19y ymm19
%define zmm20y ymm20
%define zmm21y ymm21
%define zmm22y ymm22
%define zmm23y ymm23
%define zmm24y ymm24
%define zmm25y ymm25
%define zmm26y ymm26
%define zmm27y ymm27
%define zmm28y ymm28
%define zmm29y ymm29
%define zmm30y ymm30
%define zmm31y ymm31

%define xmm0y ymm0
%define xmm1y ymm1
%define xmm2y ymm2
%define xmm3y ymm3
%define xmm4y ymm4
%define xmm5y ymm5
%define xmm6y ymm6
%define xmm7y ymm7
%define xmm8y ymm8
%define xmm9y ymm9
%define xmm10y ymm10
%define xmm11y ymm11
%define xmm12y ymm12
%define xmm13y ymm13
%define xmm14y ymm14
%define xmm15y ymm15
%define xmm16y ymm16
%define xmm17y ymm17
%define xmm18y ymm18
%define xmm19y ymm19
%define xmm20y ymm20
%define xmm21y ymm21
%define xmm22y ymm22
%define xmm23y ymm23
%define xmm24y ymm24
%define xmm25y ymm25
%define xmm26y ymm26
%define xmm27y ymm27
%define xmm28y ymm28
%define xmm29y ymm29
%define xmm30y ymm30
%define xmm31y ymm31

%define xmm0z zmm0
%define xmm1z zmm1
%define xmm2z zmm2
%define xmm3z zmm3
%define xmm4z zmm4
%define xmm5z zmm5
%define xmm6z zmm6
%define xmm7z zmm7
%define xmm8z zmm8
%define xmm9z zmm9
%define xmm10z zmm10
%define xmm11z zmm11
%define xmm12z zmm12
%define xmm13z zmm13
%define xmm14z zmm14
%define xmm15z zmm15
%define xmm16z zmm16
%define xmm17z zmm17
%define xmm18z zmm18
%define xmm19z zmm19
%define xmm20z zmm20
%define xmm21z zmm21
%define xmm22z zmm22
%define xmm23z zmm23
%define xmm24z zmm24
%define xmm25z zmm25
%define xmm26z zmm26
%define xmm27z zmm27
%define xmm28z zmm28
%define xmm29z zmm29
%define xmm30z zmm30
%define xmm31z zmm31

%define ymm0z zmm0
%define ymm1z zmm1
%define ymm2z zmm2
%define ymm3z zmm3
%define ymm4z zmm4
%define ymm5z zmm5
%define ymm6z zmm6
%define ymm7z zmm7
%define ymm8z zmm8
%define ymm9z zmm9
%define ymm10z zmm10
%define ymm11z zmm11
%define ymm12z zmm12
%define ymm13z zmm13
%define ymm14z zmm14
%define ymm15z zmm15
%define ymm16z zmm16
%define ymm17z zmm17
%define ymm18z zmm18
%define ymm19z zmm19
%define ymm20z zmm20
%define ymm21z zmm21
%define ymm22z zmm22
%define ymm23z zmm23
%define ymm24z zmm24
%define ymm25z zmm25
%define ymm26z zmm26
%define ymm27z zmm27
%define ymm28z zmm28
%define ymm29z zmm29
%define ymm30z zmm30
%define ymm31z zmm31

%define DWORD(reg) reg %+ d
%define WORD(reg)  reg %+ w
%define BYTE(reg)  reg %+ b

%define XWORD(reg) reg %+ x
%define YWORD(reg) reg %+ y
%define ZWORD(reg) reg %+ z

%endif ;; _REG_SIZES_ASM_
