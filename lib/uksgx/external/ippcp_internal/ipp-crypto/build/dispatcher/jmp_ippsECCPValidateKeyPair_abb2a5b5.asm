%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPValidateKeyPair%+elf_symbol_type
extern n8_ippsECCPValidateKeyPair%+elf_symbol_type
extern y8_ippsECCPValidateKeyPair%+elf_symbol_type
extern e9_ippsECCPValidateKeyPair%+elf_symbol_type
extern l9_ippsECCPValidateKeyPair%+elf_symbol_type
extern n0_ippsECCPValidateKeyPair%+elf_symbol_type
extern k0_ippsECCPValidateKeyPair%+elf_symbol_type
extern k1_ippsECCPValidateKeyPair%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPValidateKeyPair
.Larraddr_ippsECCPValidateKeyPair:
    dq m7_ippsECCPValidateKeyPair
    dq n8_ippsECCPValidateKeyPair
    dq y8_ippsECCPValidateKeyPair
    dq e9_ippsECCPValidateKeyPair
    dq l9_ippsECCPValidateKeyPair
    dq n0_ippsECCPValidateKeyPair
    dq k0_ippsECCPValidateKeyPair
    dq k1_ippsECCPValidateKeyPair

segment .text
global ippsECCPValidateKeyPair:function (ippsECCPValidateKeyPair.LEndippsECCPValidateKeyPair - ippsECCPValidateKeyPair)
.Lin_ippsECCPValidateKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPValidateKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPValidateKeyPair]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPValidateKeyPair:
