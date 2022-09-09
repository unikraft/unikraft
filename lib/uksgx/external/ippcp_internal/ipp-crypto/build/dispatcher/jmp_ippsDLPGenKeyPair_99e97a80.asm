%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPGenKeyPair%+elf_symbol_type
extern n8_ippsDLPGenKeyPair%+elf_symbol_type
extern y8_ippsDLPGenKeyPair%+elf_symbol_type
extern e9_ippsDLPGenKeyPair%+elf_symbol_type
extern l9_ippsDLPGenKeyPair%+elf_symbol_type
extern n0_ippsDLPGenKeyPair%+elf_symbol_type
extern k0_ippsDLPGenKeyPair%+elf_symbol_type
extern k1_ippsDLPGenKeyPair%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPGenKeyPair
.Larraddr_ippsDLPGenKeyPair:
    dq m7_ippsDLPGenKeyPair
    dq n8_ippsDLPGenKeyPair
    dq y8_ippsDLPGenKeyPair
    dq e9_ippsDLPGenKeyPair
    dq l9_ippsDLPGenKeyPair
    dq n0_ippsDLPGenKeyPair
    dq k0_ippsDLPGenKeyPair
    dq k1_ippsDLPGenKeyPair

segment .text
global ippsDLPGenKeyPair:function (ippsDLPGenKeyPair.LEndippsDLPGenKeyPair - ippsDLPGenKeyPair)
.Lin_ippsDLPGenKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPGenKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPGenKeyPair]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPGenKeyPair:
