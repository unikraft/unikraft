%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSetKeyPair%+elf_symbol_type
extern n8_ippsECCPSetKeyPair%+elf_symbol_type
extern y8_ippsECCPSetKeyPair%+elf_symbol_type
extern e9_ippsECCPSetKeyPair%+elf_symbol_type
extern l9_ippsECCPSetKeyPair%+elf_symbol_type
extern n0_ippsECCPSetKeyPair%+elf_symbol_type
extern k0_ippsECCPSetKeyPair%+elf_symbol_type
extern k1_ippsECCPSetKeyPair%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSetKeyPair
.Larraddr_ippsECCPSetKeyPair:
    dq m7_ippsECCPSetKeyPair
    dq n8_ippsECCPSetKeyPair
    dq y8_ippsECCPSetKeyPair
    dq e9_ippsECCPSetKeyPair
    dq l9_ippsECCPSetKeyPair
    dq n0_ippsECCPSetKeyPair
    dq k0_ippsECCPSetKeyPair
    dq k1_ippsECCPSetKeyPair

segment .text
global ippsECCPSetKeyPair:function (ippsECCPSetKeyPair.LEndippsECCPSetKeyPair - ippsECCPSetKeyPair)
.Lin_ippsECCPSetKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSetKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSetKeyPair]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSetKeyPair:
