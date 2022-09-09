%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPValidateDH%+elf_symbol_type
extern n8_ippsDLPValidateDH%+elf_symbol_type
extern y8_ippsDLPValidateDH%+elf_symbol_type
extern e9_ippsDLPValidateDH%+elf_symbol_type
extern l9_ippsDLPValidateDH%+elf_symbol_type
extern n0_ippsDLPValidateDH%+elf_symbol_type
extern k0_ippsDLPValidateDH%+elf_symbol_type
extern k1_ippsDLPValidateDH%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPValidateDH
.Larraddr_ippsDLPValidateDH:
    dq m7_ippsDLPValidateDH
    dq n8_ippsDLPValidateDH
    dq y8_ippsDLPValidateDH
    dq e9_ippsDLPValidateDH
    dq l9_ippsDLPValidateDH
    dq n0_ippsDLPValidateDH
    dq k0_ippsDLPValidateDH
    dq k1_ippsDLPValidateDH

segment .text
global ippsDLPValidateDH:function (ippsDLPValidateDH.LEndippsDLPValidateDH - ippsDLPValidateDH)
.Lin_ippsDLPValidateDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPValidateDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPValidateDH]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPValidateDH:
