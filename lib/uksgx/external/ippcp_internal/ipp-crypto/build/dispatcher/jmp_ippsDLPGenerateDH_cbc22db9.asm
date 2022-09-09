%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPGenerateDH%+elf_symbol_type
extern n8_ippsDLPGenerateDH%+elf_symbol_type
extern y8_ippsDLPGenerateDH%+elf_symbol_type
extern e9_ippsDLPGenerateDH%+elf_symbol_type
extern l9_ippsDLPGenerateDH%+elf_symbol_type
extern n0_ippsDLPGenerateDH%+elf_symbol_type
extern k0_ippsDLPGenerateDH%+elf_symbol_type
extern k1_ippsDLPGenerateDH%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPGenerateDH
.Larraddr_ippsDLPGenerateDH:
    dq m7_ippsDLPGenerateDH
    dq n8_ippsDLPGenerateDH
    dq y8_ippsDLPGenerateDH
    dq e9_ippsDLPGenerateDH
    dq l9_ippsDLPGenerateDH
    dq n0_ippsDLPGenerateDH
    dq k0_ippsDLPGenerateDH
    dq k1_ippsDLPGenerateDH

segment .text
global ippsDLPGenerateDH:function (ippsDLPGenerateDH.LEndippsDLPGenerateDH - ippsDLPGenerateDH)
.Lin_ippsDLPGenerateDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPGenerateDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPGenerateDH]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPGenerateDH:
