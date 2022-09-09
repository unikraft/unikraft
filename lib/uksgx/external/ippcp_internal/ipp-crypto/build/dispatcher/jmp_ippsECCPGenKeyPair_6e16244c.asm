%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPGenKeyPair%+elf_symbol_type
extern n8_ippsECCPGenKeyPair%+elf_symbol_type
extern y8_ippsECCPGenKeyPair%+elf_symbol_type
extern e9_ippsECCPGenKeyPair%+elf_symbol_type
extern l9_ippsECCPGenKeyPair%+elf_symbol_type
extern n0_ippsECCPGenKeyPair%+elf_symbol_type
extern k0_ippsECCPGenKeyPair%+elf_symbol_type
extern k1_ippsECCPGenKeyPair%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPGenKeyPair
.Larraddr_ippsECCPGenKeyPair:
    dq m7_ippsECCPGenKeyPair
    dq n8_ippsECCPGenKeyPair
    dq y8_ippsECCPGenKeyPair
    dq e9_ippsECCPGenKeyPair
    dq l9_ippsECCPGenKeyPair
    dq n0_ippsECCPGenKeyPair
    dq k0_ippsECCPGenKeyPair
    dq k1_ippsECCPGenKeyPair

segment .text
global ippsECCPGenKeyPair:function (ippsECCPGenKeyPair.LEndippsECCPGenKeyPair - ippsECCPGenKeyPair)
.Lin_ippsECCPGenKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPGenKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPGenKeyPair]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPGenKeyPair:
