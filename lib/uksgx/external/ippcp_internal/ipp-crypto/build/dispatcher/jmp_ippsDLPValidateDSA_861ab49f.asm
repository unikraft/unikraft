%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPValidateDSA%+elf_symbol_type
extern n8_ippsDLPValidateDSA%+elf_symbol_type
extern y8_ippsDLPValidateDSA%+elf_symbol_type
extern e9_ippsDLPValidateDSA%+elf_symbol_type
extern l9_ippsDLPValidateDSA%+elf_symbol_type
extern n0_ippsDLPValidateDSA%+elf_symbol_type
extern k0_ippsDLPValidateDSA%+elf_symbol_type
extern k1_ippsDLPValidateDSA%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPValidateDSA
.Larraddr_ippsDLPValidateDSA:
    dq m7_ippsDLPValidateDSA
    dq n8_ippsDLPValidateDSA
    dq y8_ippsDLPValidateDSA
    dq e9_ippsDLPValidateDSA
    dq l9_ippsDLPValidateDSA
    dq n0_ippsDLPValidateDSA
    dq k0_ippsDLPValidateDSA
    dq k1_ippsDLPValidateDSA

segment .text
global ippsDLPValidateDSA:function (ippsDLPValidateDSA.LEndippsDLPValidateDSA - ippsDLPValidateDSA)
.Lin_ippsDLPValidateDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPValidateDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPValidateDSA]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPValidateDSA:
