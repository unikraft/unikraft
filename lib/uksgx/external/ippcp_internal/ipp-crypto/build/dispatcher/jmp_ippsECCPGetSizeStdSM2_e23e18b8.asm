%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern n8_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern y8_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern e9_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern l9_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern n0_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern k0_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern k1_ippsECCPGetSizeStdSM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPGetSizeStdSM2
.Larraddr_ippsECCPGetSizeStdSM2:
    dq m7_ippsECCPGetSizeStdSM2
    dq n8_ippsECCPGetSizeStdSM2
    dq y8_ippsECCPGetSizeStdSM2
    dq e9_ippsECCPGetSizeStdSM2
    dq l9_ippsECCPGetSizeStdSM2
    dq n0_ippsECCPGetSizeStdSM2
    dq k0_ippsECCPGetSizeStdSM2
    dq k1_ippsECCPGetSizeStdSM2

segment .text
global ippsECCPGetSizeStdSM2:function (ippsECCPGetSizeStdSM2.LEndippsECCPGetSizeStdSM2 - ippsECCPGetSizeStdSM2)
.Lin_ippsECCPGetSizeStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPGetSizeStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPGetSizeStdSM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPGetSizeStdSM2:
