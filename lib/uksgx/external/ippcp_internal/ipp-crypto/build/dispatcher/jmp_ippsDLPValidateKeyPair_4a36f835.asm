%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPValidateKeyPair%+elf_symbol_type
extern n8_ippsDLPValidateKeyPair%+elf_symbol_type
extern y8_ippsDLPValidateKeyPair%+elf_symbol_type
extern e9_ippsDLPValidateKeyPair%+elf_symbol_type
extern l9_ippsDLPValidateKeyPair%+elf_symbol_type
extern n0_ippsDLPValidateKeyPair%+elf_symbol_type
extern k0_ippsDLPValidateKeyPair%+elf_symbol_type
extern k1_ippsDLPValidateKeyPair%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPValidateKeyPair
.Larraddr_ippsDLPValidateKeyPair:
    dq m7_ippsDLPValidateKeyPair
    dq n8_ippsDLPValidateKeyPair
    dq y8_ippsDLPValidateKeyPair
    dq e9_ippsDLPValidateKeyPair
    dq l9_ippsDLPValidateKeyPair
    dq n0_ippsDLPValidateKeyPair
    dq k0_ippsDLPValidateKeyPair
    dq k1_ippsDLPValidateKeyPair

segment .text
global ippsDLPValidateKeyPair:function (ippsDLPValidateKeyPair.LEndippsDLPValidateKeyPair - ippsDLPValidateKeyPair)
.Lin_ippsDLPValidateKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPValidateKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPValidateKeyPair]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPValidateKeyPair:
